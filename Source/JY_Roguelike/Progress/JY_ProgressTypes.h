#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "JY_ProgressTypes.generated.h"

/* 메타 강화 하나의 현재 단계 */
USTRUCT(BlueprintType)
struct FJY_MetaRank
{
	GENERATED_BODY()

	/* 강화 종류 (Meta.*) — 세이브에 이름으로 저장돼, 이름을 바꾸면 GameplayTagRedirects를 남겨야 옛 세이브가 산다 */
	UPROPERTY(BlueprintReadOnly, SaveGame, meta = (Categories = "Meta"))
	FGameplayTag UpgradeTag;

	/* 산 단계 수 (0 = 안 삼) */
	UPROPERTY(BlueprintReadOnly, SaveGame)
	int32 Rank = 0;
};

/* 런이 끝나도 남는 플레이어 진행 (세이브 파일·PlayerState 복제 공용). 런 값·설정은 넣지 않는다 */
USTRUCT(BlueprintType)
struct FJY_PlayerProgress
{
	GENERATED_BODY()

	/* 영혼석 (메타 강화 화폐) */
	UPROPERTY(BlueprintReadOnly, SaveGame)
	int32 SoulStones = 0;

	/* 던전 도전 횟수 (아직 증가 처리 없음) */
	UPROPERTY(BlueprintReadOnly, SaveGame)
	int32 RunCount = 0;

	/* 던전 클리어 횟수 (아직 증가 처리 없음) */
	UPROPERTY(BlueprintReadOnly, SaveGame)
	int32 ClearCount = 0;

	/* 산 메타 강화 목록 (단계 0은 넣지 않음) */
	UPROPERTY(BlueprintReadOnly, SaveGame)
	TArray<FJY_MetaRank> MetaRanks;

	/* 강화 단계 조회 (없으면 0) */
	int32 GetMetaRank(const FGameplayTag& UpgradeTag) const
	{
		auto HasSameTag = [&UpgradeTag](const FJY_MetaRank& Entry) { return Entry.UpgradeTag == UpgradeTag; };
		const FJY_MetaRank* Found = MetaRanks.FindByPredicate(HasSameTag);
		return Found != nullptr ? Found->Rank : 0;
	}

	/* 강화 단계 설정 (0이면 목록에서 뺌) */
	void SetMetaRank(const FGameplayTag& UpgradeTag, int32 NewRank)
	{
		auto HasSameTag = [&UpgradeTag](const FJY_MetaRank& Entry) { return Entry.UpgradeTag == UpgradeTag; };
		const int32 Index = MetaRanks.IndexOfByPredicate(HasSameTag);
		if (NewRank <= 0)
		{
			if (Index != INDEX_NONE)
			{
				MetaRanks.RemoveAt(Index);
			}
			return;
		}

		if (Index == INDEX_NONE)
		{
			MetaRanks.Add({ UpgradeTag, NewRank });
			return;
		}

		MetaRanks[Index].Rank = NewRank;
	}
};
