#include "System/JY_GameMode.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "System/JY_GameplayTags.h"
#include "System/JY_LogChannels.h"
#include "System/JY_WorldSettings.h"
#include "Interfaces/JY_InteractableTarget.h"
#include "Interfaces/JY_InterfaceUtils.h"
#include "Character/JY_Character.h"
#include "Player/JY_PlayerController.h"
#include "Player/JY_PlayerState.h"
#include "Progress/JY_ProgressSaveGame.h"
#include "Progress/JY_MetaUpgradeData.h"
#include "Room/JY_RoomFlowSubsystem.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameMode)

/* C++ 기본값. 실제 에셋(메시·입력)은 BP_JY_GameMode에서 BP 클래스로 덮어쓴다 */
AJY_GameMode::AJY_GameMode()
{
	PlayerStateClass = AJY_PlayerState::StaticClass();
	PlayerControllerClass = AJY_PlayerController::StaticClass();
	DefaultPawnClass = AJY_Character::StaticClass();

	/* 맵 이동(로비↔던전) 중 화면이 멈추지 않게 */
	bUseSeamlessTravel = true;
}

/* PostLogin 대신 여기 — 일반 접속과 심리스 이동 모두 거친다 (GameModeBase.cpp:607) */
/* 맵마다 PlayerState가 새로 생겨 매번 다시 읽는다. Super 전에 읽어 폰 생성 때 최대 체력이 이미 정해져 있게 */
// [Server]
void AJY_GameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) //override
{
	LoadProgress(NewPlayer->GetPlayerState<AJY_PlayerState>());

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	/* 심리스 이동도 도착 맵에서 PC를 새로 만들어 여기가 다시 불린다 → 맵 도착마다 배너 1번 */
	const AJY_WorldSettings* WorldSettings = CastChecked<AJY_WorldSettings>(GetWorldSettings());
	/* 에디터 시뮬레이트(SIE)는 관전용 기본 PlayerController를 만든다 — 그때는 배너 없음 */
	AJY_PlayerController* JYPlayerController = Cast<AJY_PlayerController>(NewPlayer);
	if (JYPlayerController != nullptr && WorldSettings->GetAreaNameText().IsEmpty() == false)
	{
		JYPlayerController->Client_ShowNotification(EJY_NotificationType::Banner, WorldSettings->GetAreaNameText());
	}
}

/* 던전은 첫 방이 모든 클라에 뜬 뒤에 캐릭터를 만든다 — 그 전엔 설 바닥이 없다. 모아서 만드는 쪽은 UJY_RoomFlowSubsystem */
// [Server]
bool AJY_GameMode::PlayerCanRestart_Implementation(APlayerController* Player) //override
{
	if (Super::PlayerCanRestart_Implementation(Player) == false)
		return false;

	const UJY_RoomFlowSubsystem* RoomFlow = GetWorld()->GetSubsystem<UJY_RoomFlowSubsystem>();
	return RoomFlow->CanStartPlayers();
}

/* 엔진 기본은 월드 전체를 돌아 꺼지는 중인 이전 방 PlayerStart도 고른다 — 지금 방 것만 본다 (GameModeBase.cpp:1093) */
// [Server]
AActor* AJY_GameMode::ChoosePlayerStart_Implementation(AController* Player) //override
{
	const UJY_RoomFlowSubsystem* RoomFlow = GetWorld()->GetSubsystem<UJY_RoomFlowSubsystem>();
	const ULevel* RoomLevel = RoomFlow->GetCurrentRoomLevel();
	if (RoomLevel == nullptr)
		return Super::ChoosePlayerStart_Implementation(Player);

	const UClass* PawnClass = GetDefaultPawnClassForController(Player);
	const APawn* PawnToFit = PawnClass != nullptr ? PawnClass->GetDefaultObject<APawn>() : nullptr;

	APlayerStart* OccupiedStart = nullptr;
	for (AActor* Actor : RoomLevel->Actors)
	{
		APlayerStart* PlayerStart = Cast<APlayerStart>(Actor);
		if (PlayerStart == nullptr)
			continue;

		if (GetWorld()->EncroachingBlockingGeometry(PawnToFit, PlayerStart->GetActorLocation(), PlayerStart->GetActorRotation()) == false)
			return PlayerStart;

		OccupiedStart = PlayerStart;
	}

	ensureMsgf(OccupiedStart != nullptr, TEXT("%s에 PlayerStart가 없음"), *RoomLevel->GetOuter()->GetName());
	return OccupiedStart;
}

/* 시작 지점이 바닥보다 조금 떠 있으면 생기자마자 떨어지는 모션이 나온다 — 바닥에 붙여 만든다 */
// [Server]
APawn* AJY_GameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) //override
{
	const UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
	const ACharacter* CharacterToFit = PawnClass != nullptr ? Cast<ACharacter>(PawnClass->GetDefaultObject()) : nullptr;
	if (CharacterToFit == nullptr)
		return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);

	const UJY_RoomFlowSubsystem* RoomFlow = GetWorld()->GetSubsystem<UJY_RoomFlowSubsystem>();
	FTransform StandTransform = SpawnTransform;
	StandTransform.SetLocation(RoomFlow->FindStandLocation(CharacterToFit, SpawnTransform.GetLocation()));
	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, StandTransform);
}

/* 엔진이 접속 때 미리 저장한 시작 지점은 방이 없을 때라 월드 원점이 된다 — 스폰 순간 ChoosePlayerStart로만 정한다 (GameModeBase.cpp:1165) */
// [Server]
bool AJY_GameMode::ShouldSpawnAtStartSpot(AController* Player) //override
{
	const UJY_RoomFlowSubsystem* RoomFlow = GetWorld()->GetSubsystem<UJY_RoomFlowSubsystem>();
	if (RoomFlow->IsRegionMap() == true)
		return false;

	return Super::ShouldSpawnAtStartSpot(Player);
}

// [Server]
bool AJY_GameMode::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage) //override
{
	const UJY_RoomFlowSubsystem* RoomFlow = GetWorld()->GetSubsystem<UJY_RoomFlowSubsystem>();
	if (RoomFlow->IsRegionMap() == true)
		return true;

	return Super::UpdatePlayerStartSpot(Player, Portal, OutErrorMessage);
}

// [Server]
void AJY_GameMode::LoadProgress(AJY_PlayerState* PlayerState)
{
	if (PlayerState == nullptr)
		return;

	/* 첫 실행엔 파일이 없다 — 없는 파일을 읽으면 엔진이 경고를 남겨 먼저 확인 */
	const bool bSaveExists = UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0);
	const UJY_ProgressSaveGame* SaveGame = bSaveExists == true ? Cast<UJY_ProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0)) : nullptr;

	/* 파일은 있는데 못 읽음 → 다음 저장이 덮어쓰기 전에 원본을 "_Broken"에 남긴다 (없으면 진행이 조용히 사라짐) */
	if (bSaveExists == true && SaveGame == nullptr)
	{
		TArray<uint8> BrokenSaveData;
		if (UGameplayStatics::LoadDataFromSlot(OUT BrokenSaveData, SaveSlotName, 0) == true)
		{
			UGameplayStatics::SaveDataToSlot(BrokenSaveData, SaveSlotName + TEXT("_Broken"), 0);
		}

		UE_LOG(LogJY, Warning, TEXT("세이브 %s 를 읽지 못해 빈 진행으로 시작 — 원본은 %s_Broken 에 보관"), *SaveSlotName, *SaveSlotName);
	}

	PlayerState->SetProgress(SaveGame != nullptr ? SaveGame->Progress : FJY_PlayerProgress());

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	PlayerState->RefreshMetaEffects(GameData != nullptr ? GameData->MetaUpgradeData.Get() : nullptr);
}

/* 화면은 클라에서만 열려 서버가 모른다 — 개조 클라의 원격 구매 요청을 요청마다 거절 */
// [Server]
bool AJY_GameMode::IsUsableMetaAltar(const APlayerController* Requester, AActor* Altar) const
{
	const IJY_InteractableTarget* AltarTarget = JY_FindInterface<IJY_InteractableTarget>(Altar);
	if (Requester == nullptr || AltarTarget == nullptr)
		return false;

	if (AltarTarget->GetServiceTag() != JYGameplayTags::Service_MetaAltar)
		return false;

	return AltarTarget->IsWithinInteractRange(Requester->GetPawn());
}

/* 연타해도 요청이 서버에서 하나씩 처리되고, 매번 현재 값으로 다시 검증 → 영혼석이 음수가 되거나 최대를 넘지 않는다 */
// [Server]
void AJY_GameMode::TryBuyMetaUpgrade(APlayerController* Buyer, FGameplayTag UpgradeTag, AActor* Altar)
{
	if (IsUsableMetaAltar(Buyer, Altar) == false)
		return;

	AJY_PlayerState* PlayerState = Buyer != nullptr ? Buyer->GetPlayerState<AJY_PlayerState>() : nullptr;
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const UJY_MetaUpgradeData* MetaData = GameData != nullptr ? GameData->MetaUpgradeData.Get() : nullptr;
	if (PlayerState == nullptr || MetaData == nullptr)
		return;

	const FJY_MetaUpgradeDefinition* Definition = MetaData->FindUpgrade(UpgradeTag);
	if (Definition == nullptr)
		return;

	FJY_PlayerProgress NewProgress = PlayerState->GetProgress();
	const int32 CurrentRank = NewProgress.GetMetaRank(UpgradeTag);
	if (CurrentRank < 0 || CurrentRank >= Definition->GetMaxRank())
		return;

	const int32 Cost = Definition->RankCosts[CurrentRank];
	if (NewProgress.SoulStones < Cost)
		return;

	const int32 NewRank = CurrentRank + 1;
	NewProgress.SoulStones -= Cost;
	NewProgress.SetMetaRank(UpgradeTag, NewRank);
	CommitProgress(PlayerState, NewProgress, EJY_ProgressCommitScope::MetaRanksChanged);
}

// [Server]
void AJY_GameMode::RefundAllMetaUpgrades(APlayerController* Requester, AActor* Altar)
{
	if (IsUsableMetaAltar(Requester, Altar) == false)
		return;

	AJY_PlayerState* PlayerState = Requester != nullptr ? Requester->GetPlayerState<AJY_PlayerState>() : nullptr;
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const UJY_MetaUpgradeData* MetaData = GameData != nullptr ? GameData->MetaUpgradeData.Get() : nullptr;
	if (PlayerState == nullptr || MetaData == nullptr)
		return;

	FJY_PlayerProgress NewProgress = PlayerState->GetProgress();
	if (NewProgress.MetaRanks.Num() == 0)
		return;

	/* 합계는 int64로 — 영혼석이 int32 끝에 가까워도 넘치지 않게 */
	int64 RefundedSoulStones = NewProgress.SoulStones;
	for (const FJY_MetaRank& MetaRank : NewProgress.MetaRanks)
	{
		const FJY_MetaUpgradeDefinition* Definition = MetaData->FindUpgrade(MetaRank.UpgradeTag);
		if (Definition == nullptr)
			continue;

		/* 표가 바뀌어 단계가 최대를 넘더라도 표에 있는 가격까지만 돌려준다 */
		const int32 PaidRanks = FMath::Min(MetaRank.Rank, Definition->GetMaxRank());
		for (int32 RankIndex = 0; RankIndex < PaidRanks; ++RankIndex)
		{
			RefundedSoulStones += Definition->RankCosts[RankIndex];
		}
	}

	NewProgress.SoulStones = static_cast<int32>(FMath::Min<int64>(RefundedSoulStones, MAX_int32));
	NewProgress.MetaRanks.Reset();
	CommitProgress(PlayerState, NewProgress, EJY_ProgressCommitScope::MetaRanksChanged);
}

// [Server]
void AJY_GameMode::AddSoulStones(APlayerController* Receiver, int32 Amount)
{
	AJY_PlayerState* PlayerState = Receiver != nullptr ? Receiver->GetPlayerState<AJY_PlayerState>() : nullptr;
	if (PlayerState == nullptr || Amount <= 0)
		return;

	FJY_PlayerProgress NewProgress = PlayerState->GetProgress();
	NewProgress.SoulStones = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(NewProgress.SoulStones) + Amount, MAX_int32));
	CommitProgress(PlayerState, NewProgress, EJY_ProgressCommitScope::ValuesOnly);
}

/* 맵 이동·종료 시점 저장에 기대지 않고 바로 저장. 효과는 단계가 바뀔 때만 다시 건다 (불필요한 복제 방지) */
// [Server]
void AJY_GameMode::CommitProgress(AJY_PlayerState* PlayerState, const FJY_PlayerProgress& NewProgress, EJY_ProgressCommitScope Scope)
{
	PlayerState->SetProgress(NewProgress);

	UJY_ProgressSaveGame* SaveGame = Cast<UJY_ProgressSaveGame>(UGameplayStatics::CreateSaveGameObject(UJY_ProgressSaveGame::StaticClass()));
	SaveGame->Progress = NewProgress;
	UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0);

	if (Scope != EJY_ProgressCommitScope::MetaRanksChanged)
		return;

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	PlayerState->RefreshMetaEffects(GameData != nullptr ? GameData->MetaUpgradeData.Get() : nullptr);
}
