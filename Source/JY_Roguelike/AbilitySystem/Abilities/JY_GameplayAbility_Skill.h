#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility.h"
#include "Combat/JY_CombatTypes.h"
#include "Skill/JY_SkillTypes.h"
#include "JY_GameplayAbility_Skill.generated.h"

class UJY_SkillDefinition;
class UCameraShakeBase;

/* 스킬 GA 공통 — 쿨타임(레벨·카드 반영), 최종 수치, 스킬 정의 찾기. 데이터는 GA_JY_Skill_* */
UCLASS(Abstract)
class UJY_GameplayAbility_Skill : public UJY_GameplayAbility
{
	GENERATED_BODY()

public:
	UJY_GameplayAbility_Skill(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

public:
	/* [Server] [OwnClient] 끊기 구간 밖에서 실행 중인 스킬이 있는지 — 있으면 다른 스킬·기본 공격이 시작되지 않는다 */
	static bool IsSkillInProgress(const FGameplayAbilityActorInfo* ActorInfo);

protected:
	/* 스펙의 SourceObject = 이 스킬 정의 */
	UJY_SkillDefinition* GetSkillDefinition() const;

	/* [Server] [OwnClient] 로드아웃에서 최종 수치 */
	FJY_SkillStats GetSkillStats() const;

	/* 판정 모양에 범위 배율 (반지름·폭만 — 상자 길이는 그대로) */
	static FJY_HitShape ScaleHitShape(const FJY_HitShape& Shape, float RadiusScale);

	/* 아바타 위치 + 수평 방향 */
	FTransform GetAvatarHitOrigin() const;

	/* [OwnClient] 이동 입력 대기 시작 — 끊기 구간 안에서 들어오면 스킬을 끝냄 */
	void StartMoveCancelListener();

	/* [Server] [OwnClient] 이 스킬이 튼 몽타주의 재생 위치가 끊기 구간(JY Cancel Window) 안인지 */
	bool IsInCancelWindow() const;

private:
	UFUNCTION()
	void OnMoveInput();

protected:
	/* 설정값 — GA_JY_Skill_* 의 Class Defaults */

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
