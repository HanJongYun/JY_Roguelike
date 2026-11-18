#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "JY_StatusEffectData.generated.h"

class UGameplayEffect;
class UNiagaraDataChannelAsset;

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

	/* Mass 적에게 걸리는 기간 (초) — 0이면 Mass 적에게는 안 걸린다 */
	UPROPERTY(EditDefaultsOnly, Category = "Mass", Meta = (ClampMin = "0"))
	float MassDurationSeconds = 0.0f;

	/* Mass 적의 틱 간격 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Mass", Meta = (ClampMin = "0"))
	float MassTickIntervalSeconds = 0.0f;

	/* Mass 적의 한 틱 피해 */
	UPROPERTY(EditDefaultsOnly, Category = "Mass", Meta = (ClampMin = "0"))
	float MassTickDamage = 0.0f;

	/* Mass 적 몸 이펙트용 데이터 채널 (NDC_JY_MassStatus_*) — 클라가 걸린 적 위치를 여기에 쓴다 */
	UPROPERTY(EditDefaultsOnly, Category = "Mass")
	TSoftObjectPtr<UNiagaraDataChannelAsset> MassBodyEffectChannel;
};

/* 모든 상태이상 수치 (DA_JY_StatusEffectData) */
UCLASS(BlueprintType, Const)
class UJY_StatusEffectData : public UDataAsset
{
	GENERATED_BODY()

public:
	/* 태그로 상태이상 칸 찾기. 없으면 nullptr */
	const FJY_StatusEffectEntry* FindStatusEffect(const FGameplayTag& StatusTag) const;

	/* 태그로 상태이상 번호(목록 순서) 찾기. 없으면 INDEX_NONE — Mass 적은 이 번호로 상태를 구분한다 */
	int32 FindStatusEffectIndex(const FGameplayTag& StatusTag) const;

public:
	/* 설정값 — DA_JY_StatusEffectData */

	/* 상태이상 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "Status", Meta = (TitleProperty = "StatusTag"))
	TArray<FJY_StatusEffectEntry> StatusEffects;
};
