#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/JY_CombatTypes.h"
#include "JY_MeleeComboData.generated.h"

class UAnimMontage;
class UCameraShakeBase;

/* 콤보 한 타 */
USTRUCT(BlueprintType)
struct FJY_MeleeComboStep
{
	GENERATED_BODY()

	/* 다음 타 입력을 받는 섹션 (시작~분기 시점). 이 섹션 끝에서 다음 타로 넘어감 */
	UPROPERTY(EditDefaultsOnly)
	FName AttackSection;

	/* 타격 이벤트마다 쓰는 판정 모양 */
	UPROPERTY(EditDefaultsOnly)
	FJY_HitShape HitShape;

	/* 타격 이벤트 1회 피해 */
	UPROPERTY(EditDefaultsOnly)
	float Damage = 10.0f;

	/* 맞은 적을 바깥으로 미는 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly)
	float KnockbackSpeed = 0.0f;

	/* 맞혔을 때 내 화면 흔들림 (비우면 없음) */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UCameraShakeBase> HitCameraShake;
};

/* 콤보 한 벌 (지상 4타 · 공중 2타 — 에셋 2개) */
UCLASS(BlueprintType, Const)
class UJY_MeleeComboData : public UDataAsset
{
	GENERATED_BODY()

public:
	/* 이 섹션이 몇 번째 타의 입력 섹션인지 (Recover 등이면 INDEX_NONE) */
	int32 FindStepIndex(FName SectionName) const;

public:
	/* 설정값 — DA_JY_Combo_Ground / DA_JY_Combo_Air */

	/* 콤보 몽타주 — 노티파이 Event.Melee.StepStart(각 Attack 섹션 시작 직후), Event.Melee.Hit(타격) */
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly)
	float PlayRate = 1.0f;

	/* 타 순서대로. 개수 = 콤보 길이 */
	UPROPERTY(EditDefaultsOnly, Meta = (TitleProperty = "AttackSection"))
	TArray<FJY_MeleeComboStep> Steps;

	/* 피격 연출 큐 */
	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "GameplayCue"))
	FGameplayTag HitCueTag;
};
