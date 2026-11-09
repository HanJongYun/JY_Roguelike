#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "JY_StatusEffectData.generated.h"

class UGameplayEffect;

/* 상태이상 1종 수치 */
USTRUCT(BlueprintType)
struct FJY_StatusEffectEntry
{
	GENERATED_BODY()

	/* 상태이상 식별 (Status.Effect.Burning 등) — 거는 GE가 붙이는 태그와 같다 */
	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "Status.Effect"))
	FGameplayTag StatusTag;

	/* ASC 대상에 거는 기간형 GE (GE_JY_Burn 등) */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	/* 한 틱 피해 — EffectClass의 SetByCaller.Damage */
	UPROPERTY(EditDefaultsOnly, Meta = (ClampMin = "0"))
	float TickDamage = 0.0f;
};

/* 모든 상태이상 수치 (DA_JY_StatusEffectData) */
UCLASS(BlueprintType, Const)
class UJY_StatusEffectData : public UDataAsset
{
	GENERATED_BODY()

public:
	/* 태그로 상태이상 칸 찾기. 없으면 nullptr */
	const FJY_StatusEffectEntry* FindStatusEffect(const FGameplayTag& StatusTag) const;

public:
	/* 설정값 — DA_JY_StatusEffectData */

	/* 상태이상 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "Status", Meta = (TitleProperty = "StatusTag"))
	TArray<FJY_StatusEffectEntry> StatusEffects;
};
