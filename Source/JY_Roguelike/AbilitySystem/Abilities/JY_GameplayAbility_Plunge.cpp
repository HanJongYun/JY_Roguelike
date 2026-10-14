#include "AbilitySystem/Abilities/JY_GameplayAbility_Plunge.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "System/JY_GameplayTags.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_Plunge)

// [Server] [OwnClient]
/* 공중 여부는 조종 클라가 정해 첫 방향 TargetData의 태그로 보낸다 — 서버가 자기 낙하 상태로 정하면 지연만큼 어긋난다 (기본 공격과 같음) */
void UJY_GameplayAbility_Plunge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) //override
{
	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();
	FAbilityTargetDataSetDelegate& DirectionDelegate = JYASC->AbilityTargetDataSetDelegate(Handle, ActivationPredictionKey);
	DirectionDelegateHandle = DirectionDelegate.AddUObject(this, &ThisClass::OnDirectionReceived);

	UAbilityTask_WaitGameplayEvent* DiveStartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, JYGameplayTags::Event_Skill_DiveStart, nullptr, false);
	DiveStartTask->EventReceived.AddDynamic(this, &ThisClass::OnDiveStartEvent);
	DiveStartTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, JYGameplayTags::Event_Skill_Hit, nullptr, false);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnHitEvent);
	HitTask->ReadyForActivation();

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	ACharacter* Character = CastChecked<ACharacter>(AvatarActor);
	Character->LandedDelegate.AddDynamic(this, &ThisClass::OnLanded);

	if (ActorInfo->IsLocallyControlled() == false)
		return;

	StartMoveCancelListener();

	FScopedPredictionWindow ScopedPrediction(JYASC, ActivationPredictionKey);

	const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	FGameplayTag StartTag;
	if (MovementComponent->IsFalling() == true)
	{
		StartTag = JYGameplayTags::Ability_Start_Air;
	}

	const FVector Direction = ComputeCursorDirection();
	const FGameplayAbilityTargetDataHandle DirectionDataHandle = MakeDirectionTargetData(Direction);
	SendTargetDataToServer(DirectionDataHandle, StartTag);
	OnDirectionReceived(DirectionDataHandle, StartTag);
}

// [Server] [OwnClient]
void UJY_GameplayAbility_Plunge::OnDirectionReceived(const FGameplayAbilityTargetDataHandle& DirectionDataHandle, FGameplayTag ApplicationTag)
{
	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	const FPredictionKey ActivationPredictionKey = CurrentActivationInfo.GetActivationPredictionKey();

	/* 첫 방향만 받는다 */
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

	bAirPlunge = ApplicationTag.MatchesTagExact(JYGameplayTags::Ability_Start_Air);
	UAnimMontage* PlungeMontage = bAirPlunge == true ? AirMontage.Get() : GroundMontage.Get();

	/* 비용(쿨타임)을 내기 전에 검사 */
	if (ensureMsgf(PlungeMontage != nullptr, TEXT("%s에 내려찍기 몽타주가 없음 (공중 %d)"), *GetNameSafe(this), bAirPlunge) == false)
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

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	PlungeStartHeight = AvatarLocation.Z;

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, PlungeMontage, PlayRate, NAME_None, true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
}

// [Server] [OwnClient]
/*
 * 능력이 주는 이동은 CMC 루트모션 소스가 예측·보정하는 표준 경로 (양쪽 LaunchCharacter는 시점 차이로 보정이 생김).
 * 길이 -1 = 착지할 때 손으로 끝냄(RootMotionSource.h:261), Override라 애니 루트모션보다 우선(CharacterMovementComponent.cpp:4509~4531)
 */
void UJY_GameplayAbility_Plunge::OnDiveStartEvent(FGameplayEventData Payload)
{
	if (bAirPlunge == false || bLanded == true)
		return;

	DiveTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(this, NAME_None, FVector::DownVector, DiveSpeed, -1.0f, false, nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.0f, false);
	DiveTask->ReadyForActivation();
}

// [Server] [OwnClient] 각자 자기 CMC 착지
/*
 * 착지 섹션으로 넘기는 건 조종 클라만 — 엔진이 서버로 같은 섹션 이동을 보낸다(AbilitySystemComponent_Abilities.cpp:3566~3571).
 * 서버가 직접 넘기면 클라 RPC와 겹쳐 두 번 넘어갈 수 있다 (기본 공격 다음 타와 같은 원칙)
 */
void UJY_GameplayAbility_Plunge::OnLanded(const FHitResult& Hit)
{
	if (bAirPlunge == false || bLanded == true)
		return;

	bLanded = true;

	if (DiveTask != nullptr)
	{
		DiveTask->EndTask();
		DiveTask = nullptr;
	}

	if (CurrentActorInfo->IsLocallyControlled() == true)
	{
		MontageJumpToSection(LandSectionName);
	}
}

// [Server] [OwnClient]
/* 낙하 높이는 타격 시점 Z로 계산 — 서버는 섹션 이동 RPC가 자기 착지보다 먼저 올 수 있다(캐릭터 이동 RPC와 ASC RPC는 순서 보장 없음) */
void UJY_GameplayAbility_Plunge::OnHitEvent(FGameplayEventData Payload)
{
	const FJY_SkillStats Stats = GetSkillStats();
	const FTransform HitOrigin = GetAvatarHitOrigin();

	float FallRadiusScale = 1.0f;
	if (bAirPlunge == true)
	{
		const FVector HitLocation = HitOrigin.GetLocation();
		const float FallHeight = PlungeStartHeight - HitLocation.Z;
		const float FallAlpha = FMath::Clamp(FallHeight / FallHeightForMaxRadius, 0.0f, 1.0f);
		FallRadiusScale = FMath::Lerp(1.0f, MaxFallRadiusScale, FallAlpha);
	}

	const FJY_HitShape ScaledShape = ScaleHitShape(HitShape, Stats.RadiusScale * FallRadiusScale);
	const float ScaledDamage = Damage * Stats.DamageScale;
	ApplyHitInShape(ScaledShape, HitOrigin, ScaledDamage, KnockbackSpeed, HitCueTag, HitCameraShake);
}

void UJY_GameplayAbility_Plunge::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UJY_GameplayAbility_Plunge::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UJY_GameplayAbility_Plunge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) //override
{
	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	const FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();

	/* 방향 구독·저장본 정리 (대시 EndAbility와 같음) */
	FAbilityTargetDataSetDelegate& DirectionDelegate = JYASC->AbilityTargetDataSetDelegate(Handle, ActivationPredictionKey);
	DirectionDelegate.Remove(DirectionDelegateHandle);
	JYASC->ConsumeClientReplicatedTargetData(Handle, ActivationPredictionKey);

	/* 캐릭터가 파괴되면서 끝날 때(PIE 종료·리스폰)는 약한 참조가 이미 비어 있다 */
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	ACharacter* Character = Cast<ACharacter>(AvatarActor);
	if (Character != nullptr)
	{
		Character->LandedDelegate.RemoveDynamic(this, &ThisClass::OnLanded);
	}

	/* 하강 태스크는 어빌리티가 끝나면 엔진이 정리(소스 제거 AbilityTask_ApplyRootMotionConstantForce.cpp:146~151) */
	DiveTask = nullptr;
	bAirPlunge = false;
	bLanded = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
