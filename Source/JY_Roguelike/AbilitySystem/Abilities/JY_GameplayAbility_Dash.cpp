#include "AbilitySystem/Abilities/JY_GameplayAbility_Dash.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Effects/JY_DashGameplayEffects.h"
#include "System/JY_GameplayTags.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_Dash)

UJY_GameplayAbility_Dash::UJY_GameplayAbility_Dash(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	CostGameplayEffectClass = UJY_GameplayEffect_DashCost::StaticClass();

	/*
	 * 발동 태그(ActivationOwnedTags) 대신 실제로 구르기 시작할 때 붙인다 — 서버는 방향이 올 때까지 발동 상태로 기다리므로,
	 * 발동 태그면 방향을 안 보내는 변조 클라가 무적만 계속 유지할 수 있다. 서버도 어빌리티를 실행하므로 서버 데미지 검사(UJY_HealthSet)에 걸린다
	 */
	RollingTags.AddTag(JYGameplayTags::State_Dashing);
	RollingTags.AddTag(JYGameplayTags::Gameplay_DamageImmunity);

	/* 대시는 공격을 끊는다 (하데스식) */
	CancelAbilitiesWithTag.AddTag(JYGameplayTags::Ability_Type_Attack);
}

// [Server] [OwnClient]
/*
 * 서버는 클라 입력 방향을 모른다 — 이동 RPC(캐릭터)와 어빌리티 RPC(PlayerState의 ASC)는 순서가 보장되지 않아 서버 가속도가 아직 옛 값일 수 있다.
 * 그래서 조종 클라가 방향을 TargetData로 보내고 서버는 도착을 기다린다 (LyraGameplayAbility_RangedWeapon과 같은 방식).
 * 발동 RPC와 TargetData RPC는 같은 ASC로 가는 Reliable이라 순서대로 도착한다.
 */
void UJY_GameplayAbility_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) //override
{
	UAbilitySystemComponent* AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();
	const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();

	FAbilityTargetDataSetDelegate& DirectionReadyDelegate = AbilitySystemComponent->AbilityTargetDataSetDelegate(Handle, ActivationPredictionKey);
	DirectionReadyDelegateHandle = DirectionReadyDelegate.AddUObject(this, &ThisClass::OnDashDirectionReady);

	if (ActorInfo->IsLocallyControlled() == false)
		return;

	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent, ActivationPredictionKey);

	const FVector DashDirection = ComputeDashDirection();
	const FGameplayAbilityTargetDataHandle DirectionDataHandle = MakeDirectionTargetData(DashDirection);
	OnDashDirectionReady(DirectionDataHandle, FGameplayTag());
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
void UJY_GameplayAbility_Dash::OnDashDirectionReady(const FGameplayAbilityTargetDataHandle& DirectionDataHandle, FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* AbilitySystemComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	const FPredictionKey ActivationPredictionKey = CurrentActivationInfo.GetActivationPredictionKey();

	/* 첫 방향만 받는다 — 같은 실행 키로 방향이 두 번 오면(변조 클라) 비용·태그가 두 번 적용돼 태그가 남는다 */
	FAbilityTargetDataSetDelegate& DirectionReadyDelegate = AbilitySystemComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, ActivationPredictionKey);
	DirectionReadyDelegate.Remove(DirectionReadyDelegateHandle);

	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent);

	/* 아래 Consume이 ASC 저장본을 지우므로 복사해 둔다 */
	const FGameplayAbilityTargetDataHandle LocalDataHandle = DirectionDataHandle;

	SendTargetDataToServer(LocalDataHandle, ApplicationTag);

	AbilitySystemComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, ActivationPredictionKey);

	/* 서버에선 네트워크로 온 값이라 비어 있을 수 있다 */
	const FVector DashDirection = GetDirectionFromTargetData(LocalDataHandle);
	if (DashDirection.IsNearlyZero() == true)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	/* 비용을 내기 전에 검사 — 커밋 뒤에 실패하면 충전 예약 없이 횟수만 줄어든다 */
	if (ensureMsgf(DashMontage != nullptr, TEXT("%s에 DashMontage가 없음"), *GetNameSafe(this)) == false)
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

	AbilitySystemComponent->AddLooseGameplayTags(RollingTags);
	bRollingTagsAdded = true;

	/* 몽타주 루트모션 중엔 CMC가 몸을 돌리지 않는다(bAllowPhysicsRotationDuringAnimRootMotion=false, CharacterMovementComponent.cpp:3044) → 처음 돌린 방향으로 끝까지 */
	FaceDirection(DashDirection);

	if (CurrentActorInfo->IsNetAuthority() == true)
	{
		ScheduleRecharge();
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, DashMontage, 1.0f, NAME_None, true, RootMotionScale);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnDashMontageFinished);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnDashMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnDashMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnDashMontageInterrupted);
	MontageTask->ReadyForActivation();
}

// [Server]
/*
 * 쓸 때마다 따로 충전 — 두 번 연속 쓰면 1초 뒤 1회, 그 직후 1회.
 * 항상 걸린 주기 GE 대신 타이머: 주기 GE는 "쓴 직후 충전까지 시간"이 0~1초로 들쭉날쭉하다.
 * 타이머가 끝나기 전에 플레이어가 나가면 ASC가 사라질 수 있어 약한 참조로 잡는다.
 */
void UJY_GameplayAbility_Dash::ScheduleRecharge()
{
	UAbilitySystemComponent* AbilitySystemComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	UJY_AbilitySystemComponent* JYASC = CastChecked<UJY_AbilitySystemComponent>(AbilitySystemComponent);
	const TWeakObjectPtr<UJY_AbilitySystemComponent> WeakJYASC = JYASC;

	auto ApplyRecharge = [WeakJYASC]()
	{
		UJY_AbilitySystemComponent* RechargeASC = WeakJYASC.Get();
		if (RechargeASC == nullptr)
			return;

		const UGameplayEffect* RechargeEffect = GetDefault<UJY_GameplayEffect_DashRecharge>();
		const FGameplayEffectContextHandle EffectContext = RechargeASC->MakeEffectContext();
		RechargeASC->ApplyGameplayEffectToSelf(RechargeEffect, 1.0f, EffectContext);
	};

	const FTimerDelegate RechargeDelegate = FTimerDelegate::CreateLambda(ApplyRecharge);
	UWorld* World = GetWorld();
	FTimerManager& TimerManager = World->GetTimerManager();
	FTimerHandle RechargeTimerHandle;
	TimerManager.SetTimer(RechargeTimerHandle, RechargeDelegate, RechargeSeconds, false);
}

void UJY_GameplayAbility_Dash::OnDashMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UJY_GameplayAbility_Dash::OnDashMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UJY_GameplayAbility_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) //override
{
	UAbilitySystemComponent* AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();
	const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();

	/* 방향 대기 구독·저장본 정리 (LyraGameplayAbility_RangedWeapon::EndAbility와 같음) */
	FAbilityTargetDataSetDelegate& DirectionReadyDelegate = AbilitySystemComponent->AbilityTargetDataSetDelegate(Handle, ActivationPredictionKey);
	DirectionReadyDelegate.Remove(DirectionReadyDelegateHandle);
	AbilitySystemComponent->ConsumeClientReplicatedTargetData(Handle, ActivationPredictionKey);

	if (bRollingTagsAdded == true)
	{
		AbilitySystemComponent->RemoveLooseGameplayTags(RollingTags);
		bRollingTagsAdded = false;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
