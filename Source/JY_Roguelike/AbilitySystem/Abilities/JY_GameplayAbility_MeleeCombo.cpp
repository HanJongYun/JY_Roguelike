#include "AbilitySystem/Abilities/JY_GameplayAbility_MeleeCombo.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Tasks/JY_AbilityTask_WaitDirection.h"
#include "AbilitySystem/Tasks/JY_AbilityTask_WaitLanded.h"
#include "AbilitySystem/Tasks/JY_AbilityTask_WaitMoveInput.h"
#include "Combat/JY_MeleeComboData.h"
#include "System/JY_GameplayTags.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_MeleeCombo)

UJY_GameplayAbility_MeleeCombo::UJY_GameplayAbility_MeleeCombo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AttackTags;
	AttackTags.AddTag(JYGameplayTags::Ability_Type_Attack);
	SetAssetTags(AttackTags);

	/* 스킬의 끊기 구간 안에서 누르면 스킬을 끊고 공격 (구간 밖이면 CanActivateAbility가 막음) */
	CancelAbilitiesWithTag.AddTag(JYGameplayTags::Ability_Type_Skill);
}

// [Server] [OwnClient]
/* 조종 클라가 커서 방향을 보내고 서버는 도착하면 시작. 다음 타 방향도 같은 실행 키로 오므로 구독을 유지한다 */
void UJY_GameplayAbility_MeleeCombo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) //override
{
	DirectionTask = UJY_AbilityTask_WaitDirection::WaitDirection(this, true);
	DirectionTask->OnDirection.AddDynamic(this, &ThisClass::OnDirectionReceived);
	DirectionTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* StepStartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, JYGameplayTags::Event_Melee_StepStart, nullptr, false);
	StepStartTask->EventReceived.AddDynamic(this, &ThisClass::OnStepStartEvent);
	StepStartTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, JYGameplayTags::Event_Melee_Hit, nullptr, false);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnHitEvent);
	HitTask->ReadyForActivation();

	UJY_AbilityTask_WaitLanded* LandedTask = UJY_AbilityTask_WaitLanded::WaitLanded(this);
	LandedTask->OnLanded.AddDynamic(this, &ThisClass::OnLanded);
	LandedTask->ReadyForActivation();

	if (ActorInfo->IsLocallyControlled() == false)
		return;

	UJY_AbilityTask_WaitMoveInput* MoveInputTask = UJY_AbilityTask_WaitMoveInput::WaitMoveInput(this);
	MoveInputTask->OnMoveInput.AddDynamic(this, &ThisClass::OnMoveInput);
	MoveInputTask->ReadyForActivation();

	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();
	FScopedPredictionWindow ScopedPrediction(JYASC, ActivationPredictionKey);

	const FVector Direction = ComputeCursorDirection();
	const FGameplayTag ComboTag = MakeStartTag();
	DirectionTask->SendDirection(Direction, ComboTag);
}

// [Server] [OwnClient]
void UJY_GameplayAbility_MeleeCombo::OnDirectionReceived(const FVector& Direction, FGameplayTag ApplicationTag)
{
	if (Direction.IsNearlyZero() == true)
	{
		/* 첫 방향이 비면 시작할 수 없다. 다음 타 방향이 비면 방향만 유지 */
		if (ActiveCombo == nullptr)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
		return;
	}

	/* 몸은 다음 타가 실제로 시작될 때(StepStart) 돌린다 */
	if (ActiveCombo != nullptr)
	{
		NextStepDirection = Direction;
		return;
	}

	const bool bAirCombo = ApplicationTag.MatchesTagExact(JYGameplayTags::Ability_Start_Air);
	ActiveCombo = bAirCombo == true ? AirCombo : GroundCombo;

	/* 비용을 내기 전에 검사 */
	const bool bComboValid = ActiveCombo != nullptr && ActiveCombo->Montage != nullptr && ActiveCombo->Steps.Num() > 0;
	if (ensureMsgf(bComboValid == true, TEXT("%s 콤보 데이터(몽타주·타 목록)가 없음"), *GetNameSafe(this)) == false)
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

	/* 몽타주는 실행 동안 1개 — 다음 타는 섹션 연결로만 바뀌어 태스크 교체가 없다 */
	const FJY_MeleeComboStep& FirstStep = ActiveCombo->Steps[0];
	PlayMontageUntilEnd(ActiveCombo->Montage, ActiveCombo->PlayRate, FirstStep.AttackSection);
}

// [OwnClient] 실행 중 다시 누름 — 엔진이 실행 중 인스턴스에 전달(AbilitySystemComponent_Abilities.cpp:2914~2934)
/*
 * 섹션 연결만 바꾸면 분기 시점에 엔진이 넘어가고 서버에도 RPC로 전달된다(AbilitySystemComponent_Abilities.cpp:3576)
 * 방향 TargetData와 연결 RPC는 같은 ASC의 Reliable RPC라 서버는 방향을 먼저 받는다
 */
void UJY_GameplayAbility_MeleeCombo::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) //override
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	const int32 StepIndex = GetCurrentStepIndex();
	const int32 LastStepIndex = ActiveCombo->Steps.Num() - 1;
	if (StepIndex == INDEX_NONE || StepIndex == LastStepIndex || bNextStepQueued == true)
		return;

	bNextStepQueued = true;

	/* 실행 중이라 열린 예측 창이 없다 — 새 키 */
	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	FScopedPredictionWindow ScopedPrediction(JYASC, true);

	const FVector Direction = ComputeCursorDirection();
	DirectionTask->SendDirection(Direction, FGameplayTag());

	const FName CurrentSection = ActiveCombo->Steps[StepIndex].AttackSection;
	const FName NextSection = ActiveCombo->Steps[StepIndex + 1].AttackSection;
	MontageSetNextSectionName(CurrentSection, NextSection);
}

int32 UJY_GameplayAbility_MeleeCombo::GetCurrentStepIndex() const
{
	const UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	const FName SectionName = JYASC->GetCurrentMontageSectionName();
	return ActiveCombo->FindStepIndex(SectionName);
}

// [Server] [OwnClient] 각 Attack 섹션 시작 0.02초 뒤 노티파이
/* 섹션 시작 정각의 노티파이는 섹션 이동 직후 실행되는지가 추출 구간 경계에 달려 있어 살짝 뒤에 둔다 */
void UJY_GameplayAbility_MeleeCombo::OnStepStartEvent(FGameplayEventData Payload)
{
	/* 다음 타는 섹션 연결로만 순서대로 시작하므로 시작 노티파이 수 = 타 번호 */
	++ActiveStepIndex;
	bNextStepQueued = false;

	if (NextStepDirection.IsNearlyZero() == true)
		return;

	FaceDirection(NextStepDirection);
	NextStepDirection = FVector::ZeroVector;
}

// [Server] [OwnClient]
/* 몇 번째 타인지 재생 위치로 구하지 않는다 — 노티파이는 위치가 앞으로 간 뒤 처리돼 이미 다음 섹션일 수 있다 */
void UJY_GameplayAbility_MeleeCombo::OnHitEvent(FGameplayEventData Payload)
{
	if (ensureMsgf(ActiveCombo->Steps.IsValidIndex(ActiveStepIndex) == true, TEXT("%s 타격 전에 타 시작 노티파이(Event.Melee.StepStart)가 없음"), *GetNameSafe(ActiveCombo)) == false)
		return;

	const FJY_MeleeComboStep& Step = ActiveCombo->Steps[ActiveStepIndex];
	const FTransform HitOrigin = GetAvatarHitOrigin();

	const FVector ObjectPushDirection = GetMontageSwingDirection(SwingBoneName, SwingSampleSeconds);
	ApplyHitInShape(Step.HitShape, HitOrigin, Step.Damage, Step.KnockbackSpeed, ActiveCombo->HitCueTag, Step.HitCameraShake, ObjectPushDirection);
}

// [OwnClient]
/* 공격 동작(Attack 섹션) 중 입력은 무시 — 분기 시점까지 동작 유지. 끝내면 서버도 끝나고 몽타주는 블렌드 아웃(bStopWhenAbilityEnds) */
void UJY_GameplayAbility_MeleeCombo::OnMoveInput()
{
	if (GetCurrentStepIndex() != INDEX_NONE)
		return;

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

// [Server] [OwnClient]
/* 서버·클라가 각자 자기 CMC 착지로 끝낸다 */
void UJY_GameplayAbility_MeleeCombo::OnLanded(const FHitResult& Hit)
{
	if (ActiveCombo != AirCombo)
		return;

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UJY_GameplayAbility_MeleeCombo::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) //override
{
	DirectionTask = nullptr;
	ActiveCombo = nullptr;
	NextStepDirection = FVector::ZeroVector;
	bNextStepQueued = false;
	ActiveStepIndex = INDEX_NONE;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
