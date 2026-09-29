#include "System/JY_GameMode.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "System/JY_GameplayTags.h"
#include "System/JY_LogChannels.h"
#include "Interaction/JY_InteractableActor.h"
#include "Character/JY_Character.h"
#include "Player/JY_PlayerController.h"
#include "Player/JY_PlayerState.h"
#include "Progress/JY_ProgressSaveGame.h"
#include "Progress/JY_MetaUpgradeData.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameMode)

/* C++ 기본값. 실제 에셋(메시·입력)은 BP_JY_GameMode에서 BP 클래스로 덮어쓴다 */
AJY_GameMode::AJY_GameMode()
{
	PlayerStateClass = AJY_PlayerState::StaticClass();
	PlayerControllerClass = AJY_PlayerController::StaticClass();
	DefaultPawnClass = AJY_Character::StaticClass();
}

/*
 * 세이브는 서버만 만진다 — 클라가 파일을 고쳐도 영향 없음. GameMode는 서버에만 있어 별도 서브시스템이 필요 없다.
 * 1인 게임 전제 — 슬롯 1개를 접속한 플레이어가 쓴다. 협동을 넣으면 플레이어별 슬롯(계정 ID)으로 나눈다.
 * 맵이 바뀌면 GameMode·PlayerState가 새로 생기므로 들어올 때마다 파일에서 다시 읽는다.
 * PostLogin 대신 여기: 일반 접속(PostLogin)과 Seamless 이동(HandleSeamlessTravelPlayer) 모두 이 함수를 거친다(GameModeBase.cpp:607).
 * 효과는 PlayerState의 ASC에 걸려 순서와 무관하지만, 먼저 읽으면 폰이 생길 때 최대 체력이 이미 정해져 있다.
 */
// [Server]
void AJY_GameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) //override
{
	LoadProgress(NewPlayer->GetPlayerState<AJY_PlayerState>());

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

// [Server]
void AJY_GameMode::LoadProgress(AJY_PlayerState* PlayerState)
{
	if (PlayerState == nullptr)
		return;

	/* 첫 실행엔 파일이 없다 — 없는 파일을 읽으면 엔진이 경고를 남겨 먼저 확인 */
	const bool bSaveExists = UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0);
	const UJY_ProgressSaveGame* SaveGame = bSaveExists == true ? Cast<UJY_ProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0)) : nullptr;

	/*
	 * 파일은 있는데 못 읽음(클래스 이름 변경·파일 손상) → 빈 진행으로 시작하되, 다음 저장이 덮어쓰기 전에 원본 바이트를 "<슬롯>_Broken"에 남긴다.
	 * 이게 없으면 첫 구매·영혼석 획득 때 몇 시간 치 진행이 조용히 사라진다.
	 */
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

/*
 * 판매 대상 검증: 제단(Service.MetaAltar)이 맞고, 요청한 플레이어 폰이 그 사용 범위 안에 있어야 한다.
 * 개조 클라가 던전에서·멀리서 구매 요청을 보내도 여기서 거절된다. 상점·축복 NPC는 같은 방식으로 자기 서비스 태그를 쓴다.
 */
// [Server]
bool AJY_GameMode::IsValidMetaSeller(const APlayerController* Requester, const AJY_InteractableActor* Seller) const
{
	if (Requester == nullptr || Seller == nullptr)
		return false;

	if (Seller->GetServiceTag() != JYGameplayTags::Service_MetaAltar)
		return false;

	return Seller->IsWithinInteractRange(Requester->GetPawn());
}

/* 연타해도 요청이 서버에서 하나씩 처리되고, 매번 현재 값으로 다시 검증 → 영혼석이 음수가 되거나 최대를 넘지 않는다 */
// [Server]
void AJY_GameMode::TryBuyMetaUpgrade(APlayerController* Buyer, FGameplayTag UpgradeTag, AJY_InteractableActor* Seller)
{
	if (IsValidMetaSeller(Buyer, Seller) == false)
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

	NewProgress.SoulStones -= Cost;
	NewProgress.SetMetaRank(UpgradeTag, CurrentRank + 1);
	CommitProgress(PlayerState, NewProgress, EJY_ProgressCommitScope::MetaRanksChanged);
}

// [Server]
void AJY_GameMode::RefundAllMetaUpgrades(APlayerController* Requester, AJY_InteractableActor* Seller)
{
	if (IsValidMetaSeller(Requester, Seller) == false)
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

/*
 * 값이 바뀔 때마다 바로 저장 — 맵 이동·종료 시점 저장에 기대지 않는다 (작은 파일 1개라 비용 무시).
 * 메타 효과는 단계가 바뀔 때만 다시 건다 — 영혼석만 바뀔 때 다시 걸면 최대 체력 복제만 늘어난다.
 */
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
