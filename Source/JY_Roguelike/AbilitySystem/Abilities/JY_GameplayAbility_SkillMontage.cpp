#include "AbilitySystem/Abilities/JY_GameplayAbility_SkillMontage.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Tasks/JY_AbilityTask_WaitDirection.h"
#include "System/JY_GameplayTags.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_SkillMontage)

// [Server] [OwnClient]
/* 조종 클라가 커서 방향을 보내고 서버는 도착하면 시작 */
void UJY_GameplayAbility_SkillMontage::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) //override
{
	UJY_AbilityTask_WaitDirection* DirectionTask = UJY_AbilityTask_WaitDirection::WaitDirection(this, false);
	DirectionTask->OnDirection.AddDynamic(this, &ThisClass::OnDirectionReceived);
	DirectionTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, JYGameplayTags::Event_Skill_Hit, nullptr, false);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnHitEvent);
	HitTask->ReadyForActivation();

	if (ActorInfo->IsLocallyControlled() == false)
		return;

	StartMoveCancelListener();

	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();
	FScopedPredictionWindow ScopedPrediction(JYASC, ActivationPredictionKey);

	const FVector Direction = ComputeCursorDirection();
	DirectionTask->SendDirection(Direction, FGameplayTag());
}

// [Server] [OwnClient]
void UJY_GameplayAbility_SkillMontage::OnDirectionReceived(const FVector& Direction, FGameplayTag ApplicationTag)
{
	if (Direction.IsNearlyZero() == true)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	/* 비용(쿨타임)을 내기 전에 검사 */
	if (ensureMsgf(Montage != nullptr, TEXT("%s에 Montage가 없음"), *GetNameSafe(this)) == false)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo) == false)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	FaceDirection(Direction);
	HitEventIndex = 0;
	OnSkillStarted();

	PlayMontageUntilEnd(Montage, PlayRate, StartSectionName);
}

void UJY_GameplayAbility_SkillMontage::OnSkillStarted()
{
}

FJY_HitShape UJY_GameplayAbility_SkillMontage::MakeHitShapeForEvent(int32 EventIndex, FTransform& InOutOrigin) const
{
	if (ensureMsgf(HitShapes.Num() > 0, TEXT("%s에 HitShapes가 없음"), *GetNameSafe(this)) == false)
		return FJY_HitShape();

	const int32 ShapeIndex = FMath::Min(EventIndex, HitShapes.Num() - 1);
	return HitShapes[ShapeIndex];
}

// [Server] [OwnClient]
void UJY_GameplayAbility_SkillMontage::OnHitEvent(FGameplayEventData Payload)
{
	FTransform HitOrigin = GetAvatarHitOrigin();
	const FJY_HitShape EventShape = MakeHitShapeForEvent(HitEventIndex, OUT HitOrigin);
	++HitEventIndex;

	const FJY_SkillStats Stats = GetSkillStats();
	const FJY_HitShape ScaledShape = ScaleHitShape(EventShape, Stats.RadiusScale);
	const float ScaledDamage = Damage * Stats.DamageScale;
	ApplyHitInShape(ScaledShape, HitOrigin, ScaledDamage, KnockbackSpeed, HitCueTag, HitCameraShake);
}
