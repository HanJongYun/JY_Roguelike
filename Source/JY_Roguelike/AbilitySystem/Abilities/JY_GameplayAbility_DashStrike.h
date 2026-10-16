#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility_SkillMontage.h"
#include "JY_GameplayAbility_DashStrike.generated.h"

/* 돌진 공격 (발도 돌진·찌르기) — 루트모션으로 돌진하며 지나간 경로를 상자로 벤다 */
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
