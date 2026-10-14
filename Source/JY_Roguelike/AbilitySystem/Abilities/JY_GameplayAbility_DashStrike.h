#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility_SkillMontage.h"
#include "JY_GameplayAbility_DashStrike.generated.h"

/* 돌진 공격 (발도 돌진·찌르기) — 몽타주 루트모션으로 돌진, 타격은 시작 위치~현재 위치 경로 상자. 무적이 필요하면 몽타주 노티파이 상태 JY Grant Tags */
UCLASS(Abstract)
class UJY_GameplayAbility_DashStrike : public UJY_GameplayAbility_SkillMontage
{
	GENERATED_BODY()

protected:
	virtual FJY_HitShape MakeHitShapeForEvent(int32 EventIndex, FTransform& InOutOrigin) const override;
	virtual void OnSkillStarted() override;

protected:
	/* 설정값 — GA_JY_Skill_IaiDash · GA_JY_Skill_Thrust 의 Class Defaults */

	/* 경로 상자 폭 (cm, 범위 배율 전) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (ClampMin = "0"))
	float PathWidth = 200.0f;

private:
	/* 돌진 시작 위치 */
	FVector StartLocation = FVector::ZeroVector;
};
