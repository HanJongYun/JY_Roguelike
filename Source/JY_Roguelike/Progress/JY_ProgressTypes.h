#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "JY_ProgressTypes.generated.h"

/* 메타 강화 하나의 현재 단계 */
USTRUCT(BlueprintType)
struct FJY_MetaRank
{
	GENERATED_BODY()

	/* 강화 종류 (Meta.*) — 세이브에 태그 이름으로 저장된다. 이름을 바꾸면 DefaultGameplayTags.ini에 GameplayTagRedirects 1줄을 남겨야 옛 세이브의 단계가 살아남는다 */
	UPROPERTY(BlueprintReadOnly, SaveGame, meta = (Categories = "Meta"))
	FGameplayTag UpgradeTag;

	/* 산 단계 수 (0 = 안 삼) */
	UPROPERTY(BlueprintReadOnly, SaveGame)
	int32 Rank = 0;
};

/*
 * 런이 끝나도 남는 플레이어 진행 — 세이브 파일과 PlayerState 복제에 같은 구조를 쓴다
 * 넣지 않는 것: 런 값(골드·부활 횟수 — 런이 끝나면 버려져 런 상태에 둔다), 설정(해상도·음량 — 클라 로컬 저장, Lyra ULyraSettingsLocal)
 */
USTRUCT(BlueprintType)
struct FJY_PlayerProgress
{
	GENERATED_BODY()

	/* 영혼석 (메타 강화 화폐) */
	UPROPERTY(BlueprintReadOnly, SaveGame)
	int32 SoulStones = 0;

	/* 던전 도전 횟수 (증가는 던전 조각에서 — 지금은 기록판 표시만) */
	UPROPERTY(BlueprintReadOnly, SaveGame)
	int32 RunCount = 0;

	/* 던전 클리어 횟수 (증가는 던전 조각에서) */
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
