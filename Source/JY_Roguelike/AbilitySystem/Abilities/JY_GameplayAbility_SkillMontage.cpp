#include "AbilitySystem/Abilities/JY_GameplayAbility_SkillMontage.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "System/JY_GameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_SkillMontage)

// [Server] [OwnClient]
/* 조종 클라가 커서 방향을 TargetData로 보내고 서버는 도착하면 시작 */
void UJY_GameplayAbility_SkillMontage::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) //override
{
	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();
	FAbilityTargetDataSetDelegate& DirectionDelegate = JYASC->AbilityTargetDataSetDelegate(Handle, ActivationPredictionKey);
	DirectionDelegateHandle = DirectionDelegate.AddUObject(this, &ThisClass::OnDirectionReceived);

	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, JYGameplayTags::Event_Skill_Hit, nullptr, false);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnHitEvent);
	HitTask->ReadyForActivation();

	if (ActorInfo->IsLocallyControlled() == false)
		return;

	StartMoveCancelListener();

	FScopedPredictionWindow ScopedPrediction(JYASC, ActivationPredictionKey);

	const FVector Direction = ComputeCursorDirection();
	const FGameplayAbilityTargetDataHandle DirectionDataHandle = MakeDirectionTargetData(Direction);
	SendTargetDataToServer(DirectionDataHandle, FGameplayTag());
	OnDirectionReceived(DirectionDataHandle, FGameplayTag());
}

// [Server] [OwnClient]
void UJY_GameplayAbility_SkillMontage::OnDirectionReceived(const FGameplayAbilityTargetDataHandle& DirectionDataHandle, FGameplayTag ApplicationTag)
{
	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	const FPredictionKey ActivationPredictionKey = CurrentActivationInfo.GetActivationPredictionKey();

	/* 첫 방향만 받는다 — 같은 키로 두 번 오면(변조 클라) 비용이 두 번 나간다 */
	FAbilityTargetDataSetDelegate& DirectionDelegate = JYASC->AbilityTargetDataSetDelegate(CurrentSpecHandle, ActivationPredictionKey);
	DirectionDelegate.Remove(DirectionDelegateHandle);

	/* 아래 Consume이 ASC 저장본을 지우므로 복사 */
	const FGameplayAbilityTargetDataHandle LocalDataHandle = DirectionDataHandle;
	JYASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, ActivationPredictionKey);

	const FVector Direction = GetDirectionFromTargetData(LocalDataHandle);
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

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, PlayRate, StartSectionName, true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
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

void UJY_GameplayAbility_SkillMontage::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UJY_GameplayAbility_SkillMontage::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UJY_GameplayAbility_SkillMontage::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) //override
{
	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();

	/* 방향 구독·저장본 정리 (대시 EndAbility와 같음) */
	FAbilityTargetDataSetDelegate& DirectionDelegate = JYASC->AbilityTargetDataSetDelegate(Handle, ActivationPredictionKey);
	DirectionDelegate.Remove(DirectionDelegateHandle);
	JYASC->ConsumeClientReplicatedTargetData(Handle, ActivationPredictionKey);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
