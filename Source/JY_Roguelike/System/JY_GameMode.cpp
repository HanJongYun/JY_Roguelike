#include "System/JY_GameMode.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
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
 * 맵이 바뀌면 GameMode·PlayerState가 새로 생기므로 들어올 때마다 파일에서 다시 읽는다.
 * PostLogin 대신 여기: 일반 접속(PostLogin)과 Seamless 이동(HandleSeamlessTravelPlayer) 모두 이 함수를 거친다(GameModeBase.cpp:607).
 * 효과는 PlayerState의 ASC에 걸려 순서와 무관하지만, 먼저 읽으면 폰이 생길 때 최대 체력이 이미 정해져 있다.
 */
// [Server]
void AJY_GameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) //override
{
	LoadProgress(NewPlayer->GetPlayerState<AJY_PlayerState>());

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	AJY_PlayerController* JYPlayerController = Cast<AJY_PlayerController>(NewPlayer);
	if (JYPlayerController != nullptr && WelcomeBannerText.IsEmpty() == false)
	{
		JYPlayerController->Client_ShowNotification(EJY_NotificationType::Banner, WelcomeBannerText);
	}
}

// [Server]
void AJY_GameMode::LoadProgress(AJY_PlayerState* PlayerState)
{
	if (PlayerState == nullptr)
		return;

	/* 첫 실행엔 파일이 없다 — 없는 파일을 읽으면 엔진이 경고를 남겨 먼저 확인 */
	const UJY_ProgressSaveGame* SaveGame = UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0) == true ? Cast<UJY_ProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0)) : nullptr;
	PlayerState->SetProgress(SaveGame != nullptr ? SaveGame->Progress : FJY_PlayerProgress());

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	PlayerState->RefreshMetaEffects(GameData != nullptr ? GameData->MetaUpgradeData.Get() : nullptr);
}

/* 연타해도 요청이 서버에서 하나씩 처리되고, 매번 현재 값으로 다시 검증 → 영혼석이 음수가 되거나 최대를 넘지 않는다 */
// [Server]
void AJY_GameMode::TryBuyMetaUpgrade(APlayerController* Buyer, FGameplayTag UpgradeTag)
{
	/* 제단 화면은 클라에서만 열려 서버가 모른다 → "이 맵에서 허용?"을 서버 규칙으로 막는다 */
	if (bAllowMetaUpgrades == false)
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
	CommitProgress(PlayerState, NewProgress, true);

	AJY_PlayerController* JYPlayerController = Cast<AJY_PlayerController>(Buyer);
	if (JYPlayerController != nullptr)
	{
		JYPlayerController->Client_ShowNotification(EJY_NotificationType::Toast, FText::Format(MetaData->GetPurchaseToastFormat(), Definition->DisplayName, CurrentRank + 1));
	}
}

// [Server]
void AJY_GameMode::RefundAllMetaUpgrades(APlayerController* Requester)
{
	if (bAllowMetaUpgrades == false)
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
	CommitProgress(PlayerState, NewProgress, true);
}

// [Server]
void AJY_GameMode::AddSoulStones(APlayerController* Receiver, int32 Amount)
{
	AJY_PlayerState* PlayerState = Receiver != nullptr ? Receiver->GetPlayerState<AJY_PlayerState>() : nullptr;
	if (PlayerState == nullptr || Amount <= 0)
		return;

	FJY_PlayerProgress NewProgress = PlayerState->GetProgress();
	NewProgress.SoulStones = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(NewProgress.SoulStones) + Amount, MAX_int32));
	CommitProgress(PlayerState, NewProgress, false);
}

/*
 * 값이 바뀔 때마다 바로 저장 — 맵 이동·종료 시점 저장에 기대지 않는다 (작은 파일 1개라 비용 무시).
 * 메타 효과는 단계가 바뀔 때만 다시 건다 — 영혼석만 바뀔 때 다시 걸면 최대 체력 복제만 늘어난다.
 */
// [Server]
void AJY_GameMode::CommitProgress(AJY_PlayerState* PlayerState, const FJY_PlayerProgress& NewProgress, bool bMetaRanksChanged)
{
	PlayerState->SetProgress(NewProgress);

	UJY_ProgressSaveGame* SaveGame = Cast<UJY_ProgressSaveGame>(UGameplayStatics::CreateSaveGameObject(UJY_ProgressSaveGame::StaticClass()));
	SaveGame->Progress = NewProgress;
	UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0);

	if (bMetaRanksChanged == false)
		return;

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	PlayerState->RefreshMetaEffects(GameData != nullptr ? GameData->MetaUpgradeData.Get() : nullptr);
}
