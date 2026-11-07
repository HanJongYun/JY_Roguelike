#include "Room/JY_RoomFlowSubsystem.h"
#include "Room/JY_RegionData.h"
#include "Room/JY_RoomData.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "System/JY_WorldSettings.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_RoomFlowSubsystem)

namespace JY_RoomFlow
{
	/* 시작 지점에서 이만큼 아래까지 바닥을 찾는다 — 더 떠 있으면 일부러 띄운 것으로 보고 그대로 */
	static constexpr float StandSnapDistance = 100.0f;

	ULevelStreaming* FindStreamingLevel(const UWorld* World, const TSoftObjectPtr<UWorld>& Room)
	{
		const FName PackageName(*Room.GetLongPackageName());
		return UGameplayStatics::GetStreamingLevel(World, PackageName);
	}

	/* 로비·타이틀은 비어 있음 — 방 진행 없음 */
	const UJY_RegionData* FindRegionData(const UWorld* World)
	{
		const AJY_WorldSettings* WorldSettings = Cast<AJY_WorldSettings>(World->GetWorldSettings());
		if (WorldSettings == nullptr)
			return nullptr;

		return WorldSettings->GetRegionData();
	}

	/* 방마다 PlayerStart 하나 — 플레이어가 한 명이라 하나면 된다 */
	APlayerStart* FindPlayerStart(const ULevel* Level)
	{
		for (AActor* Actor : Level->Actors)
		{
			APlayerStart* PlayerStart = Cast<APlayerStart>(Actor);
			if (PlayerStart != nullptr)
				return PlayerStart;
		}

		const UObject* LevelOuter = Level->GetOuter();
		ensureMsgf(false, TEXT("%s에 PlayerStart가 없음"), *LevelOuter->GetName());
		return nullptr;
	}
}

bool UJY_RoomFlowSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const //override
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

/*
 * 클라는 LoadStreamLevel이 보내는 알림으로 같은 방을 따라 켠다.
 * 첫 방도 같은 길로 켠다 — Initially Loaded로 두면 심리스 이동 도착 땐 안 올라온다 (World.cpp:9014)
 */
// [Server]
void UJY_RoomFlowSubsystem::OnWorldBeginPlay(UWorld& InWorld) //override
{
	Super::OnWorldBeginPlay(InWorld);

	if (InWorld.GetNetMode() == NM_Client)
		return;

	RegionData = JY_RoomFlow::FindRegionData(&InWorld);
	if (RegionData == nullptr)
		return;

	PlanRooms();
	LoadRoom(0);
}

/*
 * 순서를 처음에 다 정해 두면 다음 방을 불러오기 전에도 그 방 정보(종류)를 알 수 있다.
 * 종류는 정확히 같아야 뽑는다 — 1번 방·보스도 Combat의 자식이라 MatchesTag면 일반 전투 칸에 섞인다
 */
// [Server]
void UJY_RoomFlowSubsystem::PlanRooms()
{
	const TArray<FGameplayTag>& SlotTypes = RegionData->GetSlotTypes();
	checkf(SlotTypes.Num() > 0, TEXT("%s의 SlotTypes가 비어 있음"), *RegionData->GetName());

	/* 빈 칸은 종류를 못 읽고, 같은 맵이 두 번 있으면 한 런에 같은 방이 두 번 나온다 — 둘 다 배치 실수 */
	TSet<TSoftObjectPtr<UWorld>> RoomMaps;
	for (const UJY_RoomData* Room : RegionData->GetRooms())
	{
		checkf(Room != nullptr, TEXT("%s의 Rooms에 빈 칸이 있음"), *RegionData->GetName());

		const TSoftObjectPtr<UWorld>& RoomMap = Room->GetMap();
		bool bAlreadyInRooms = false;
		RoomMaps.Add(RoomMap, &bAlreadyInRooms);
		ensureMsgf(bAlreadyInRooms == false, TEXT("%s의 Rooms에 같은 맵이 두 번 있음 (%s)"), *RegionData->GetName(), *Room->GetName());
	}

	/* 한 런에 같은 방은 한 번만 — 뽑은 방은 여기서 뺀다 */
	TArray<const UJY_RoomData*> UnusedRooms(RegionData->GetRooms());

	PlannedRooms.Reset();
	for (const FGameplayTag& SlotType : SlotTypes)
	{
		TArray<const UJY_RoomData*> Candidates;
		for (const UJY_RoomData* Room : UnusedRooms)
		{
			if (Room->GetRoomType().MatchesTagExact(SlotType) == true)
			{
				Candidates.Add(Room);
			}
		}
		checkf(Candidates.Num() > 0, TEXT("%s: %s 칸에 넣을 안 나온 방이 없음"), *RegionData->GetName(), *SlotType.ToString());

		const int32 PickIndex = FMath::RandRange(0, Candidates.Num() - 1);
		const UJY_RoomData* PickedRoom = Candidates[PickIndex];
		PlannedRooms.Add(PickedRoom);
		UnusedRooms.RemoveSingleSwap(PickedRoom);
	}
}

/* 심리스 이동은 BeginPlay 전에 플레이어를 받아(World.cpp:9094) RegionData가 아직 없을 수 있다 — 맵 설정을 직접 본다 */
// [Server]
bool UJY_RoomFlowSubsystem::IsRegionMap() const
{
	return JY_RoomFlow::FindRegionData(GetWorld()) != nullptr;
}

/* 첫 방이 플레이어 클라에 뜨기 전엔 설 바닥이 없다 */
// [Server]
bool UJY_RoomFlowSubsystem::CanStartPlayer() const
{
	if (IsRegionMap() == false)
		return true;

	return CurrentRoom.IsNull() == false;
}

// [Server]
const ULevel* UJY_RoomFlowSubsystem::GetCurrentRoomLevel() const
{
	if (CurrentRoom.IsNull() == true)
		return nullptr;

	const ULevelStreaming* StreamingLevel = JY_RoomFlow::FindStreamingLevel(GetWorld(), CurrentRoom);
	return StreamingLevel->GetLoadedLevel();
}

// [Server]
APlayerStart* UJY_RoomFlowSubsystem::GetCurrentRoomPlayerStart() const
{
	const ULevel* RoomLevel = GetCurrentRoomLevel();
	if (RoomLevel == nullptr)
		return nullptr;

	return JY_RoomFlow::FindPlayerStart(RoomLevel);
}

// [Server]
FVector UJY_RoomFlowSubsystem::GetCurrentRoomStartLocation() const
{
	check(GetCurrentRoomLevel() != nullptr);

	const APlayerStart* PlayerStart = GetCurrentRoomPlayerStart();
	if (PlayerStart == nullptr)
		return FVector::ZeroVector;

	return PlayerStart->GetActorLocation();
}

// [Server]
void UJY_RoomFlowSubsystem::EnterRoom(const ULevel* RoomLevel, const UJY_RoomData& RoomData)
{
	bCurrentRoomCleared = RoomData.IsCombatRoom() == false;

	OnRoomClearChanged.Broadcast(RoomLevel, bCurrentRoomCleared);
	OnRoomEntered.Broadcast(RoomLevel, &RoomData);
}

// [Server]
void UJY_RoomFlowSubsystem::ClearCurrentRoom()
{
	if (CurrentRoom.IsNull() == true || bCurrentRoomCleared == true)
		return;

	bCurrentRoomCleared = true;

	OnRoomClearChanged.Broadcast(GetCurrentRoomLevel(), true);
}

// [Server]
bool UJY_RoomFlowSubsystem::IsRoomCleared(const ULevel* RoomLevel) const
{
	return bCurrentRoomCleared == true && RoomLevel != nullptr && RoomLevel == GetCurrentRoomLevel();
}

// [Server]
void UJY_RoomFlowSubsystem::RequestNextRoom()
{
	/* 지역 맵이 아니거나(로비 등) 첫 방에 들어가기 전 */
	if (CurrentRoom.IsNull() == true)
		return;

	/* 방을 깨기 전엔 못 넘어간다 — 규칙은 문이 아닌 여기서 정한다 */
	if (bCurrentRoomCleared == false)
		return;

	/* 다음 방을 켜는 중 — 문 판정에 몇 프레임 계속 겹쳐 있음 */
	if (PendingRoom.IsNull() == false)
		return;

	const int32 NextSlotIndex = CurrentSlotIndex + 1;
	if (NextSlotIndex >= PlannedRooms.Num())
	{
		ReturnToHub();
		return;
	}

	LoadRoom(NextSlotIndex);
}

// [Server]
void UJY_RoomFlowSubsystem::LoadRoom(int32 SlotIndex)
{
	const UJY_RoomData* RoomData = PlannedRooms[SlotIndex];
	PendingRoom = RoomData->GetMap();
	PendingSlotIndex = SlotIndex;

	/* 로딩과 화면 어두워짐을 동시에 — 둘 다 끝나야 옮긴다. 첫 방은 로딩 화면이 덮고 있어 생략 */
	const bool bHasCurrentRoom = CurrentRoom.IsNull() == false;
	const float FadeSeconds = bHasCurrentRoom == true ? RegionData->GetTransitionFadeSeconds() : 0.0f;
	if (bHasCurrentRoom == true)
	{
		FadePlayerScreen(true);
	}
	FadeOutEndSeconds = GetWorld()->GetTimeSeconds() + FadeSeconds;

	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	LatentInfo.ExecutionFunction = GET_FUNCTION_NAME_CHECKED(ThisClass, HandleNextRoomLoaded);
	LatentInfo.UUID = ++NextLatentUUID;
	LatentInfo.Linkage = 0;
	UGameplayStatics::LoadStreamLevelBySoftObjectPtr(this, PendingRoom, true, false, LatentInfo);
}

/* LoadStreamLevel의 bMakeVisibleAfterLoad=true라 서버에 보이기까지 끝난 뒤 불린다 */
// [Server]
void UJY_RoomFlowSubsystem::HandleNextRoomLoaded()
{
	TryEnterNextRoom();
}

/*
 * 클라가 방을 못 띄웠는데 옮기면 바닥이 없어 떨어진다 — 띄우고 화면도 어두워질 때까지 0.1초마다 확인.
 * 클라 알림(ServerUpdateLevelVisibility)은 SealedEvent라 받을 수 없다 (PlayerController.h:1532)
 */
// [Server]
void UJY_RoomFlowSubsystem::TryEnterNextRoom()
{
	const ULevelStreaming* NextStreamingLevel = JY_RoomFlow::FindStreamingLevel(GetWorld(), PendingRoom);
	const ULevel* NextLevel = NextStreamingLevel->GetLoadedLevel();

	const bool bFadeOutFinished = GetWorld()->GetTimeSeconds() >= FadeOutEndSeconds;
	if (bFadeOutFinished == false || HasPlayerInitializedLevel(NextLevel) == false)
	{
		GetWorld()->GetTimerManager().SetTimer(EnterRetryTimerHandle, this, &ThisClass::TryEnterNextRoom, 0.1f, false);
		return;
	}

	MovePlayerToStart(NextLevel);

	const bool bFirstRoom = CurrentRoom.IsNull() == true;
	if (bFirstRoom == false)
	{
		/* 끄는 건 콜백이 필요 없다 — Linkage 기본값이면 완료 때 아무것도 안 부름 (LatentActionManager.cpp:360) */
		FLatentActionInfo UnloadLatentInfo;
		UnloadLatentInfo.CallbackTarget = this;
		UnloadLatentInfo.UUID = ++NextLatentUUID;
		UGameplayStatics::UnloadStreamLevelBySoftObjectPtr(this, CurrentRoom, UnloadLatentInfo, false);
	}

	CurrentRoom = PendingRoom;
	CurrentSlotIndex = PendingSlotIndex;
	PendingRoom.Reset();
	PendingSlotIndex = INDEX_NONE;

	EnterRoom(NextLevel, *PlannedRooms[CurrentSlotIndex]);

	if (bFirstRoom == true)
	{
		StartWaitingPlayer();
		return;
	}

	FadePlayerScreen(false);
}

/* 데디 서버의 PlayerController는 접속한 플레이어 것뿐이라 첫 번째가 그 플레이어다 */
// [Server]
APlayerController* UJY_RoomFlowSubsystem::GetPlayerController() const
{
	return GetWorld()->GetFirstPlayerController();
}

/* 클라가 레벨을 보이게 만들면 서버에 알리고(ServerUpdateLevelVisibility) 연결에 기록된다 */
// [Server]
bool UJY_RoomFlowSubsystem::HasPlayerInitializedLevel(const ULevel* Level) const
{
	/* 아직 접속 전이면 기다릴 클라가 없다 — 접속하면 PlayerCanRestart가 첫 방을 기다린다 */
	const APlayerController* PlayerController = GetPlayerController();
	if (PlayerController == nullptr)
		return true;

	/* 스탠드얼론 실행은 연결이 없다 */
	const UNetConnection* Connection = PlayerController->GetNetConnection();
	if (Connection == nullptr)
		return true;

	return Connection->ClientHasInitializedLevel(Level);
}

/* PlayerStart 위치는 캡슐 가운데 높이라 그대로 세운다 */
// [Server]
void UJY_RoomFlowSubsystem::MovePlayerToStart(const ULevel* Level) const
{
	const APlayerController* PlayerController = GetPlayerController();
	if (PlayerController == nullptr)
		return;

	APawn* Pawn = PlayerController->GetPawn();
	if (Pawn == nullptr)
		return;

	const APlayerStart* PlayerStart = JY_RoomFlow::FindPlayerStart(Level);
	if (PlayerStart == nullptr)
		return;

	const ACharacter* Character = Cast<ACharacter>(Pawn);
	const FVector StandLocation = Character != nullptr ? FindStandLocation(Character, PlayerStart->GetActorLocation()) : PlayerStart->GetActorLocation();
	Pawn->TeleportTo(StandLocation, PlayerStart->GetActorRotation());
}

/* 순간이동한 뒤 바닥이 2.4cm(MAX_FLOOR_DIST)보다 멀면 이동 컴포넌트가 떨어지는 중으로 바꾼다 (CharacterMovementComponent.cpp:6552) */
/* 시작 지점을 바닥 높이에 맞춰 두지 않아도 서서 시작하게, 캡슐을 내려 바닥에 닿는 곳을 쓴다 */
// [Server]
FVector UJY_RoomFlowSubsystem::FindStandLocation(const ACharacter* Character, const FVector& StartLocation) const
{
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
	const FVector TraceEnd = StartLocation - FVector::UpVector * JY_RoomFlow::StandSnapDistance;
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JY_StandLocation), false, Character);

	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(OUT Hit, StartLocation, TraceEnd, FQuat::Identity, Capsule->GetCollisionObjectType(), CapsuleShape, QueryParams);
	if (bHit == false || Hit.bStartPenetrating == true)
		return StartLocation;

	/* 이동 컴포넌트가 걷는 중으로 보는 바닥 거리 안쪽으로 */
	return Hit.Location + FVector::UpVector * UCharacterMovementComponent::MIN_FLOOR_DIST;
}

/* 첫 방 전까지 막아 두었던 캐릭터 만들기를 이제 한다 (서는 곳은 AJY_GameMode::ChoosePlayerStart) */
// [Server]
void UJY_RoomFlowSubsystem::StartWaitingPlayer() const
{
	APlayerController* PlayerController = GetPlayerController();
	if (PlayerController == nullptr || PlayerController->GetPawn() != nullptr)
		return;

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (GameMode->PlayerCanRestart(PlayerController) == false)
		return;

	GameMode->RestartPlayer(PlayerController);
}

/* 어두워진 채 유지했다가 옮긴 뒤 걷는다 */
// [Server]
void UJY_RoomFlowSubsystem::FadePlayerScreen(bool bFadeOut) const
{
	APlayerController* PlayerController = GetPlayerController();
	if (PlayerController == nullptr)
		return;

	const FVector2D FadeAlpha = bFadeOut == true ? FVector2D(0.0f, 1.0f) : FVector2D(1.0f, 0.0f);
	const float FadeSeconds = RegionData->GetTransitionFadeSeconds();
	PlayerController->ClientSetCameraFade(true, FColor::Black, FadeAlpha, FadeSeconds, false, bFadeOut);
}

/* 이동 중에 또 불려도 엔진이 두 번째 ServerTravel을 무시한다(World.cpp:9540) */
// [Server]
void UJY_RoomFlowSubsystem::ReturnToHub() const
{
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	if (GameData == nullptr)
		return;

	GetWorld()->ServerTravel(GameData->HubMap.GetLongPackageName());
}
