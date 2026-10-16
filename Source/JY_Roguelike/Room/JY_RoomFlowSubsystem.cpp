#include "Room/JY_RoomFlowSubsystem.h"
#include "Room/JY_RegionData.h"
#include "Room/JY_RoomMarker.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "System/JY_WorldSettings.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_RoomFlowSubsystem)

namespace JY_RoomFlow
{
	ULevelStreaming* FindStreamingLevel(const UWorld* World, const TSoftObjectPtr<UWorld>& Room)
	{
		const FName PackageName(*Room.GetLongPackageName());
		return UGameplayStatics::GetStreamingLevel(World, PackageName);
	}

	/* 허브·타이틀은 비어 있음 — 방 진행 없음 */
	const UJY_RegionData* FindRegionData(const UWorld* World)
	{
		const AJY_WorldSettings* WorldSettings = Cast<AJY_WorldSettings>(World->GetWorldSettings());
		if (WorldSettings == nullptr)
			return nullptr;

		return WorldSettings->GetRegionData();
	}
}

/* 에디터 월드에선 돌지 않게 — 게임·PIE만 */
bool UJY_RoomFlowSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const //override
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

/*
 * 방 진행은 서버가 정한다. 클라는 LoadStreamLevel이 보내는 알림(LevelStreaming.cpp:368)으로 같은 방을 따라 켠다.
 * 첫 방도 다음 방과 같은 길로 켠다 — 지역 맵에 Initially Loaded로 켜 두면 심리스 이동 도착 땐 안 올라온다
 * (도착 처리는 로드 요청을 받지 않는 Visibility 플러시만 함, World.cpp:9014 · EngineTypes.h:1407).
 * 플레이어 캐릭터는 첫 방에 들어간 뒤 만든다(AJY_GameMode::PlayerCanRestart).
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

	if (ensureMsgf(RegionData->GetRoomSlots().Num() > 0, TEXT("%s의 RoomSlots가 비어 있음"), *RegionData->GetName()) == false)
		return;

	LoadRoom(0);
}

/* 심리스 이동은 BeginPlay 전에 플레이어를 받는다(World.cpp:9094 PostSeamlessTravel → 9098 BeginPlay) — RegionData를 아직 못 읽었을 수 있어 맵 설정을 직접 본다 */
// [Server]
bool UJY_RoomFlowSubsystem::IsRegionMap() const
{
	return JY_RoomFlow::FindRegionData(GetWorld()) != nullptr;
}

/* 엔진 AGameMode가 경기 시작 전 캐릭터를 막는 것(GameMode.cpp:548)과 같은 관문 — 첫 방이 모든 클라에 뜨기 전엔 설 바닥이 없다 */
// [Server]
bool UJY_RoomFlowSubsystem::CanStartPlayers() const
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
void UJY_RoomFlowSubsystem::EnterRoom(const ULevel* RoomLevel)
{
	bCurrentRoomCleared = true;
	for (const AActor* Actor : RoomLevel->Actors)
	{
		if (Cast<AJY_RoomEnemySpawnPoint>(Actor) != nullptr)
		{
			bCurrentRoomCleared = false;
			break;
		}
	}

	OnRoomClearChanged.Broadcast(RoomLevel, bCurrentRoomCleared);
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
	/* 지역 맵이 아니거나(허브 등) 첫 방에 들어가기 전 */
	if (CurrentRoom.IsNull() == true)
		return;

	/* 방을 깨기 전엔 못 넘어간다 — 출구 문은 잠겨 있으면 통과 감지를 꺼 두지만, 규칙은 여기서 정한다 */
	if (bCurrentRoomCleared == false)
		return;

	/* 다음 방을 켜는 중 — 문 판정에 몇 프레임 계속 겹쳐 있거나 여러 명이 동시에 지나감 */
	if (PendingRoom.IsNull() == false)
		return;

	const int32 NextSlotIndex = CurrentSlotIndex + 1;
	if (NextSlotIndex >= RegionData->GetRoomSlots().Num())
	{
		ReturnToHub();
		return;
	}

	LoadRoom(NextSlotIndex);
}

// [Server]
void UJY_RoomFlowSubsystem::LoadRoom(int32 SlotIndex)
{
	PendingRoom = PickNextRoom(SlotIndex);
	PendingSlotIndex = SlotIndex;

	/* 로딩과 화면 어두워짐을 동시에 — 둘 다 끝나야 옮긴다. 첫 방은 캐릭터가 없고 로딩 화면이 덮고 있어 어둡게 할 게 없다 */
	const bool bHasCurrentRoom = CurrentRoom.IsNull() == false;
	const float FadeSeconds = bHasCurrentRoom == true ? RegionData->GetTransitionFadeSeconds() : 0.0f;
	if (bHasCurrentRoom == true)
	{
		FadePlayerScreens(true);
	}
	FadeOutEndSeconds = GetWorld()->GetTimeSeconds() + FadeSeconds;

	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	LatentInfo.ExecutionFunction = GET_FUNCTION_NAME_CHECKED(ThisClass, HandleNextRoomLoaded);
	LatentInfo.UUID = ++NextLatentUUID;
	LatentInfo.Linkage = 0;
	UGameplayStatics::LoadStreamLevelBySoftObjectPtr(this, PendingRoom, true, false, LatentInfo);
}

TSoftObjectPtr<UWorld> UJY_RoomFlowSubsystem::PickNextRoom(int32 SlotIndex) const
{
	const FJY_RoomSlot& RoomSlot = RegionData->GetRoomSlots()[SlotIndex];

	TArray<TSoftObjectPtr<UWorld>> Candidates = RoomSlot.CandidateRooms;
	auto IsVisitedRoom = [this](const TSoftObjectPtr<UWorld>& Room) { return VisitedRooms.Contains(Room); };
	Candidates.RemoveAll(IsVisitedRoom);
	ensureMsgf(Candidates.Num() > 0, TEXT("%s의 %d번 칸에 이번 런에 안 나온 방이 없음"), *RegionData->GetName(), SlotIndex);

	const int32 PickIndex = FMath::RandRange(0, Candidates.Num() - 1);
	return Candidates[PickIndex];
}

/* LoadStreamLevel의 bMakeVisibleAfterLoad=true라 서버에 보이기까지 끝난 뒤 불린다 */
// [Server]
void UJY_RoomFlowSubsystem::HandleNextRoomLoaded()
{
	TryEnterNextRoom();
}

/*
 * 클라가 방을 아직 못 띄웠는데 옮기면 바닥이 없어 떨어진다 — 모두 띄우고 화면도 다 어두워질 때까지 0.1초마다 다시 확인.
 * 클라가 띄웠다는 알림(ServerUpdateLevelVisibility)은 SealedEvent라 덮어 받을 수 없고(PlayerController.h:1532) 서버 쪽 이벤트도 없다.
 */
// [Server]
void UJY_RoomFlowSubsystem::TryEnterNextRoom()
{
	const ULevelStreaming* NextStreamingLevel = JY_RoomFlow::FindStreamingLevel(GetWorld(), PendingRoom);
	const ULevel* NextLevel = NextStreamingLevel->GetLoadedLevel();

	const bool bFadeOutFinished = GetWorld()->GetTimeSeconds() >= FadeOutEndSeconds;
	if (bFadeOutFinished == false || HaveAllClientsInitializedLevel(NextLevel) == false)
	{
		GetWorld()->GetTimerManager().SetTimer(EnterRetryTimerHandle, this, &ThisClass::TryEnterNextRoom, 0.1f, false);
		return;
	}

	MovePlayersToStart(NextLevel);

	const bool bFirstRoom = CurrentRoom.IsNull() == true;
	if (bFirstRoom == false)
	{
		/* 끄는 건 콜백이 필요 없다 — Linkage 기본값(INDEX_NONE)이면 완료 때 아무것도 안 부름(LatentActionManager.cpp:360) */
		FLatentActionInfo UnloadLatentInfo;
		UnloadLatentInfo.CallbackTarget = this;
		UnloadLatentInfo.UUID = ++NextLatentUUID;
		UGameplayStatics::UnloadStreamLevelBySoftObjectPtr(this, CurrentRoom, UnloadLatentInfo, false);
	}

	CurrentRoom = PendingRoom;
	CurrentSlotIndex = PendingSlotIndex;
	PendingRoom.Reset();
	PendingSlotIndex = INDEX_NONE;
	VisitedRooms.Add(CurrentRoom);

	EnterRoom(NextLevel);

	if (bFirstRoom == true)
	{
		StartWaitingPlayers();
		return;
	}

	FadePlayerScreens(false);
}

/* 클라가 레벨을 보이게 만들면 서버에 알리고(ServerUpdateLevelVisibility) 연결에 기록된다 */
// [Server]
bool UJY_RoomFlowSubsystem::HaveAllClientsInitializedLevel(const ULevel* Level) const
{
	/* 스탠드얼론 실행은 넷 드라이버가 없다 — 기다릴 클라도 없음 */
	const UNetDriver* NetDriver = GetWorld()->GetNetDriver();
	if (NetDriver == nullptr)
		return true;

	for (const UNetConnection* Connection : NetDriver->ClientConnections)
	{
		if (Connection->ClientHasInitializedLevel(Level) == false)
			return false;
	}

	return true;
}

/*
 * 캐릭터 이동은 서버가 정하면 조종 클라에 위치 보정으로 내려간다.
 * PlayerStart 위치는 캡슐 가운데 높이(엔진 기본 배치)라 그대로 세운다. 여러 명이면 방의 PlayerStart를 돌아가며 쓴다.
 */
// [Server]
void UJY_RoomFlowSubsystem::MovePlayersToStart(const ULevel* Level) const
{
	TArray<const APlayerStart*> PlayerStarts;
	for (const AActor* Actor : Level->Actors)
	{
		const APlayerStart* PlayerStart = Cast<APlayerStart>(Actor);
		if (PlayerStart != nullptr)
		{
			PlayerStarts.Add(PlayerStart);
		}
	}

	if (ensureMsgf(PlayerStarts.Num() > 0, TEXT("%s에 PlayerStart가 없음"), *Level->GetOuter()->GetName()) == false)
		return;

	int32 StartIndex = 0;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		const APlayerController* PlayerController = Iterator->Get();
		APawn* Pawn = PlayerController->GetPawn();
		if (Pawn == nullptr)
			continue;

		const APlayerStart* PlayerStart = PlayerStarts[StartIndex % PlayerStarts.Num()];
		Pawn->TeleportTo(PlayerStart->GetActorLocation(), PlayerStart->GetActorRotation());
		++StartIndex;
	}
}

/* 엔진 AGameMode::HandleMatchHasStarted(GameMode.cpp:207)와 같은 방식 — 막아 두었던 캐릭터 만들기를 이제 한다. 서는 곳은 AJY_GameMode::ChoosePlayerStart */
// [Server]
void UJY_RoomFlowSubsystem::StartWaitingPlayers() const
{
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (PlayerController->GetPawn() != nullptr)
			continue;

		if (GameMode->PlayerCanRestart(PlayerController) == false)
			continue;

		GameMode->RestartPlayer(PlayerController);
	}
}

/* 엔진 카메라 페이드(APlayerController::ClientSetCameraFade, 클라 RPC) — 어두워진 채 유지했다가 옮긴 뒤 걷는다 */
// [Server]
void UJY_RoomFlowSubsystem::FadePlayerScreens(bool bFadeOut) const
{
	const FVector2D FadeAlpha = bFadeOut == true ? FVector2D(0.0f, 1.0f) : FVector2D(1.0f, 0.0f);
	const float FadeSeconds = RegionData->GetTransitionFadeSeconds();

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		PlayerController->ClientSetCameraFade(true, FColor::Black, FadeAlpha, FadeSeconds, false, bFadeOut);
	}
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
