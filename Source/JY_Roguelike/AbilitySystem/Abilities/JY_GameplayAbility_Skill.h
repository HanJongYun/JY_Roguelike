#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility_Attack.h"
#include "Combat/JY_CombatTypes.h"
#include "Skill/JY_SkillTypes.h"
#include "JY_GameplayAbility_Skill.generated.h"

class UJY_SkillDefinition;
class UCameraShakeBase;

/* 스킬 GA 공통 — 쿨타임(레벨·카드 반영), 최종 수치, 스킬 정의 찾기, 끊기 구간 */
UCLASS(Abstract)
class UJY_GameplayAbility_Skill : public UJY_GameplayAbility_Attack
{
	GENERATED_BODY()

public:
	UJY_GameplayAbility_Skill(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	virtual bool IsBlockingOtherAttacks() const override;

protected:
	/* 스펙의 SourceObject = 이 스킬 정의 */
	UJY_SkillDefinition* GetSkillDefinition() const;

	/* [Server] [OwnClient] 로드아웃에서 최종 수치 */
	FJY_SkillStats GetSkillStats() const;

	/* 판정 모양에 범위 배율 (반지름·폭만 — 상자 길이는 그대로) */
	static FJY_HitShape ScaleHitShape(const FJY_HitShape& Shape, float RadiusScale);

	/* [OwnClient] 이동 입력 대기 시작 — 끊기 구간 안에서 들어오면 스킬을 끝냄 */
	void StartMoveCancelListener();

	/* [Server] [OwnClient] 이 스킬이 튼 몽타주의 재생 위치가 끊기 구간(JY Cancel Window) 안인지 */
	bool IsInCancelWindow() const;

private:
	UFUNCTION()
	void OnMoveInput();

protected:
	/* 설정값 — GA_JY_Skill_* 의 Class Defaults */

	/* 타격 1회 피해 (레벨·카드 배율 전) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	float Damage = 30.0f;

	/* 맞은 적을 미는 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	float KnockbackSpeed = 400.0f;

	/* 쿨타임 중 붙는 태그 (Cooldown.Skill.Plunge 등) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (Categories = "Cooldown"))
	FGameplayTagContainer CooldownTags;

	/* 피격 연출 큐 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill", Meta = (Categories = "GameplayCue"))
	FGameplayTag HitCueTag;

	/* 맞혔을 때 내 화면 흔들림 (비우면 없음) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Skill")
	TSubclassOf<UCameraShakeBase> HitCameraShake;
};
