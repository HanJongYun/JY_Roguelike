#include "AbilitySystem/Abilities/JY_GameplayAbility_Dash.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Tasks/JY_AbilityTask_WaitDirection.h"
#include "System/JY_GameplayTags.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_Dash)

UJY_GameplayAbility_Dash::UJY_GameplayAbility_Dash(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	/* 발동 태그 대신 구르기 시작 때 붙인다 — 발동 태그면 방향을 안 보내는 변조 클라가 무적만 유지할 수 있다 */
	RollingTags.AddTag(JYGameplayTags::State_Dashing);
	RollingTags.AddTag(JYGameplayTags::Gameplay_DamageImmunity);

	/* 대시는 공격을 끊는다 (하데스식) */
	CancelAbilitiesWithTag.AddTag(JYGameplayTags::Ability_Type_Attack);
}

// [Server] [OwnClient]
/* 이동 RPC와 어빌리티 RPC는 순서 보장이 없어 서버 가속도를 믿을 수 없다 → 조종 클라가 정한 방향을 보낸다 */
void UJY_GameplayAbility_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) //override
{
	UJY_AbilityTask_WaitDirection* DirectionTask = UJY_AbilityTask_WaitDirection::WaitDirection(this, false);
	DirectionTask->OnDirection.AddDynamic(this, &ThisClass::OnDashDirectionReady);
	DirectionTask->ReadyForActivation();

	if (ActorInfo->IsLocallyControlled() == false)
		return;

	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();
	FScopedPredictionWindow ScopedPrediction(JYASC, ActivationPredictionKey);

	const FVector DashDirection = ComputeDashDirection();
	DirectionTask->SendDirection(DashDirection, FGameplayTag());
}

// [OwnClient]
/* ProcessAbilityInput은 이번 프레임 입력을 다 모은 뒤라 대기 중인 이동 입력이 이번 프레임 WASD다 (월드 방향 — Move가 카메라 기준으로 넣음) */
FVector UJY_GameplayAbility_Dash::ComputeDashDirection() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const ACharacter* Character = CastChecked<ACharacter>(AvatarActor);

	const FVector PendingInput = Character->GetPendingMovementInputVector();
	const FVector InputDirection = PendingInput.GetSafeNormal2D();
	if (InputDirection.IsNearlyZero() == false)
		return InputDirection;

	const FVector ForwardVector = Character->GetActorForwardVector();
	return ForwardVector.GetSafeNormal2D();
}

// [Server] [OwnClient]
void UJY_GameplayAbility_Dash::OnDashDirectionReady(const FVector& DashDirection, FGameplayTag ApplicationTag)
{
	if (DashDirection.IsNearlyZero() == true)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	/* 비용을 내기 전에 검사 — 커밋 뒤에 실패하면 충전 예약 없이 횟수만 줄어든다 */
	const bool bDashDataValid = DashMontage != nullptr && RechargeEffectClass != nullptr;
	if (ensureMsgf(bDashDataValid == true, TEXT("%s에 DashMontage 또는 RechargeEffectClass가 없음"), *GetNameSafe(this)) == false)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	/* 비용(횟수 -1). 0이면 실패 */
	if (CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo) == false)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	JYASC->AddLooseGameplayTags(RollingTags);
	bRollingTagsAdded = true;

	/* 루트모션 중엔 CMC가 몸을 돌리지 않아 처음 돌린 방향으로 끝까지 간다 */
	FaceDirection(DashDirection);

	if (CurrentActorInfo->IsNetAuthority() == true)
	{
		ScheduleRecharge();
	}

	PlayMontageUntilEnd(DashMontage, PlayRate, NAME_None, RootMotionScale);
}

// [Server]
/*
 * 주기 GE 대신 쓸 때마다 타이머 — 주기 GE는 쓴 직후 충전까지 시간이 들쭉날쭉하다
 * 타이머 전에 플레이어가 나가면 ASC가 사라질 수 있어 약한 참조
 */
void UJY_GameplayAbility_Dash::ScheduleRecharge()
{
	UAbilitySystemComponent* AbilitySystemComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	UJY_AbilitySystemComponent* JYASC = CastChecked<UJY_AbilitySystemComponent>(AbilitySystemComponent);
	const TWeakObjectPtr<UJY_AbilitySystemComponent> WeakJYASC = JYASC;
	const TSubclassOf<UGameplayEffect> RechargeClass = RechargeEffectClass;

	auto ApplyRecharge = [WeakJYASC, RechargeClass]()
	{
		UJY_AbilitySystemComponent* RechargeASC = WeakJYASC.Get();
		if (RechargeASC == nullptr)
			return;

		const UGameplayEffect* RechargeEffect = RechargeClass->GetDefaultObject<UGameplayEffect>();
		const FGameplayEffectContextHandle EffectContext = RechargeASC->MakeEffectContext();
		RechargeASC->ApplyGameplayEffectToSelf(RechargeEffect, 1.0f, EffectContext);
	};

	const FTimerDelegate RechargeDelegate = FTimerDelegate::CreateLambda(ApplyRecharge);
	UWorld* World = GetWorld();
	FTimerManager& TimerManager = World->GetTimerManager();
	FTimerHandle RechargeTimerHandle;
	TimerManager.SetTimer(RechargeTimerHandle, RechargeDelegate, RechargeSeconds, false);
}

void UJY_GameplayAbility_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) //override
{
	if (bRollingTagsAdded == true)
	{
		UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
		JYASC->RemoveLooseGameplayTags(RollingTags);
		bRollingTagsAdded = false;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
