#include "AbilitySystem/Abilities/JY_GameplayAbility_Plunge.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "System/JY_GameplayTags.h"
#include "Animation/JY_AnimNotify_GameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_Plunge)

// [Server] [OwnClient]
/* 공중 여부는 조종 클라가 정해 첫 방향 TargetData의 태그로 보낸다 — 서버가 자기 낙하 상태로 정하면 지연만큼 어긋난다 */
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

	/* 비용(쿨타임)을 내기 전에 검사 */
	if (ensureMsgf(PlungeMontage != nullptr, TEXT("%s에 내려찍기 몽타주가 없음"), *GetNameSafe(this)) == false)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	/* 칼이 땅을 치는 시각은 몽타주 노티파이에서 — 애니를 바꾸면 노티파이만 옮기면 됨 */
	const bool bSlamPoseFound = UJY_AnimNotify_GameplayEvent::FindEventTime(PlungeMontage, JYGameplayTags::Event_Skill_SlamPose, OUT SlamPoseTime);
	if (ensureMsgf(bSlamPoseFound == true, TEXT("%s에 Event.Skill.SlamPose 노티파이가 없음"), *GetNameSafe(PlungeMontage)) == false)
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
	PlungeDirection = Direction;

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	PlungeStartHeight = AvatarLocation.Z;

	/*
	 * 땅 — 예측한 착지 시각에 칼이 닿게 처음부터 속도를 맞춤 (바닥이 안 보이면 LeapDuration)
	 * 서버·조종 클라가 같은 지형으로 같은 값을 얻어 RPC가 필요 없다
	 */
	float MontagePlayRate = PlayRate;
	if (bAirPlunge == false)
	{
		float LandingTime = LeapDuration;
		PredictLeapLandingTime(Direction, OUT LandingTime);
		MontagePlayRate = ComputeSlamPlayRate(SlamPoseTime, LandingTime);
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, PlungeMontage, MontagePlayRate, NAME_None, true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();

	if (bAirPlunge == true)
		return;

	/*
	 * 땅 — 엔진 점프 루트모션 (bFinishOnLanded: 착지하면 태스크가 스스로 끝남)
	 * 착지 처리를 태스크에 맡긴다 — 이동 재생 중 직접 지우면 재생용 소스를 지운다(AbilityTask_ApplyRootMotionJumpForce.h TriggerLanded)
	 */
	const FRotator LeapRotation = Direction.Rotation();
	LeapTask = UAbilityTask_ApplyRootMotionJumpForce::ApplyRootMotionJumpForce(this, NAME_None, LeapRotation, LeapDistance, LeapHeight, LeapDuration, 0.0f, true, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.0f, nullptr, nullptr);
	LeapTask->ReadyForActivation();
}

// [Server] [OwnClient]
/*
 * 루트모션 소스로 이동 — 예측·보정되는 표준 경로 (LaunchCharacter는 시점 차이로 보정이 생김). 길이 -1 = 착지 때 직접 끝냄
 * 애니 루트모션이 우선이라(CharacterMovementComponent.cpp:4512) 몽타주 Start·Loop에 DisableRootMotion을 둔다
 */
void UJY_GameplayAbility_Plunge::OnDiveStartEvent(FGameplayEventData Payload)
{
	if (bAirPlunge == false || bLanded == true)
		return;

	/* 앞+아래 대각선 — 방향과 세기는 두 속도를 합친 벡터에서 */
	const FVector ForwardVelocity = PlungeDirection * DiveForwardSpeed;
	const FVector DownVelocity = FVector::DownVector * DiveSpeed;
	const FVector DiveVelocity = ForwardVelocity + DownVelocity;
	const FVector DiveDirection = DiveVelocity.GetSafeNormal();
	const float DiveStrength = DiveVelocity.Size();

	DiveTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(this, NAME_None, DiveDirection, DiveStrength, -1.0f, false, nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.0f, false);
	DiveTask->ReadyForActivation();

	if (CurrentActorInfo->IsLocallyControlled() == true)
	{
		MatchSlamToLanding(DiveVelocity);
	}
}

// [OwnClient]
/*
 * 재생 속도 변경은 조종 클라만 — ASC가 서버로 보내 시뮬에 복제된다(AbilitySystemComponent_Abilities.cpp:3613)
 * 바닥이 안 보이면 원래 속도 유지 (Loop 자세로 기다림)
 */
void UJY_GameplayAbility_Plunge::MatchSlamToLanding(const FVector& DiveVelocity)
{
	const ACharacter* Character = CastChecked<ACharacter>(GetAvatarActorFromActorInfo());
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const FVector AvatarLocation = Character->GetActorLocation();
	const FVector FeetLocation = AvatarLocation - FVector::UpVector * Capsule->GetScaledCapsuleHalfHeight();

	const float DiveSpeedTotal = DiveVelocity.Size();
	const FVector DiveDirection = DiveVelocity.GetSafeNormal();
	const FVector TraceEnd = FeetLocation + DiveDirection * DiveSpeedTotal * FallTraceSeconds;

	FHitResult GroundHit;
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JYPlungeGround), false, Character);
	const bool bGroundFound = GetWorld()->LineTraceSingleByObjectType(OUT GroundHit, FeetLocation, TraceEnd, FCollisionObjectQueryParams(ECC_WorldStatic), QueryParams);
	if (bGroundFound == false)
		return;

	const float FallTime = GroundHit.Distance / DiveSpeedTotal;
	const UAnimInstance* AnimInstance = CurrentActorInfo->GetAnimInstance();
	const float MontagePosition = AnimInstance->Montage_GetPosition(PlungeMontage);
	const float SlamTimeLeft = SlamPoseTime - MontagePosition;
	if (SlamTimeLeft <= 0.0f || FallTime <= UE_KINDA_SMALL_NUMBER)
		return;

	const float SlamPlayRate = ComputeSlamPlayRate(SlamTimeLeft, FallTime);
	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	JYASC->CurrentMontageSetPlayRate(SlamPlayRate);
}

// [Server] [OwnClient]
/*
 * 엔진 점프 루트모션 경로를 그대로 재현 — LeapDuration 뒤엔 끝 속도로 직선 낙하(FRootMotionSource_JumpForce::GetPathOffset)
 * 오르는 동안은 착지할 수 없으므로 정점부터 토막으로 나눠 레이를 쏜다
 */
bool UJY_GameplayAbility_Plunge::PredictLeapLandingTime(const FVector& Direction, float& OutLandingTime) const
{
	/* 하강 구간 토막 수 — 많을수록 곡선에 가깝지만 레이가 늘어남 */
	constexpr int32 DescentSegmentCount = 8;

	const ACharacter* Character = CastChecked<ACharacter>(GetAvatarActorFromActorInfo());
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const FVector AvatarLocation = Character->GetActorLocation();
	const FVector FeetLocation = AvatarLocation - FVector::UpVector * Capsule->GetScaledCapsuleHalfHeight();
	const FVector Forward = Direction.GetSafeNormal2D();

	auto GetLeapFeetLocation = [this, &FeetLocation, &Forward](float MoveFraction)
	{
		const float Phi = 2.0f * MoveFraction - 1.0f;
		const float HeightOffset = LeapHeight * (1.0f - Phi * Phi);
		return FeetLocation + Forward * LeapDistance * MoveFraction + FVector::UpVector * HeightOffset;
	};

	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JYPlungeLeapGround), false, Character);
	const FCollisionObjectQueryParams ObjectQueryParams(ECC_WorldStatic);
	FHitResult GroundHit;

	/* 정점 → 도약 끝 */
	const float ApexTime = LeapDuration * 0.5f;
	const float SegmentDuration = ApexTime / DescentSegmentCount;
	for (int32 SegmentIndex = 0; SegmentIndex < DescentSegmentCount; ++SegmentIndex)
	{
		const float SegmentStartTime = ApexTime + SegmentDuration * SegmentIndex;
		const float SegmentEndTime = SegmentStartTime + SegmentDuration;
		const FVector SegmentStart = GetLeapFeetLocation(SegmentStartTime / LeapDuration);
		const FVector SegmentEnd = GetLeapFeetLocation(SegmentEndTime / LeapDuration);
		if (GetWorld()->LineTraceSingleByObjectType(OUT GroundHit, SegmentStart, SegmentEnd, ObjectQueryParams, QueryParams) == true)
		{
			OutLandingTime = SegmentStartTime + SegmentDuration * GroundHit.Time;
			return true;
		}
	}

	/* 도약 끝 → 끝 속도로 직선 낙하. 끝 속도 = 포물선 끝 기울기 (앞 LeapDistance, 아래 4 × LeapHeight, 둘 다 LeapDuration당) */
	const FVector EndVelocity = (Forward * LeapDistance - FVector::UpVector * 4.0f * LeapHeight) / LeapDuration;
	const FVector FallStart = GetLeapFeetLocation(1.0f);
	const FVector FallEnd = FallStart + EndVelocity * FallTraceSeconds;
	if (GetWorld()->LineTraceSingleByObjectType(OUT GroundHit, FallStart, FallEnd, ObjectQueryParams, QueryParams) == false)
		return false;

	OutLandingTime = LeapDuration + FallTraceSeconds * GroundHit.Time;
	return true;
}

/* 아주 높은 곳이면 동작이 너무 느려지지 않게 MinSlamPlayRate에서 멈춘다 */
float UJY_GameplayAbility_Plunge::ComputeSlamPlayRate(float SlamTimeLeft, float FallTime) const
{
	return FMath::Max(SlamTimeLeft / FallTime, MinSlamPlayRate);
}

// [Server] [OwnClient] 각자 자기 CMC 착지
/*
 * 착지 섹션 이동은 조종 클라만 — 엔진이 서버로 보내므로(AbilitySystemComponent_Abilities.cpp:3566) 서버도 넘기면 두 번 넘어간다
 * 도약 태스크는 스스로 끝나므로 하강 태스크만 끝낸다
 */
void UJY_GameplayAbility_Plunge::OnLanded(const FHitResult& Hit)
{
	if (bLanded == true)
		return;

	bLanded = true;

	if (DiveTask != nullptr)
	{
		DiveTask->EndTask();
		DiveTask = nullptr;
	}

	if (CurrentActorInfo->IsLocallyControlled() == true)
	{
		/* 착지 후 동작은 원래 속도로 */
		UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
		JYASC->CurrentMontageSetPlayRate(PlayRate);
		MontageJumpToSection(LandSectionName);
	}
}

// [Server] [OwnClient]
/*
 * 낙하 높이는 타격 시점 Z로 — 서버는 섹션 이동 RPC가 자기 착지보다 먼저 올 수 있다
 * 시작 높이 기준이라 도약 높이 자체는 치지 않는다
 */
void UJY_GameplayAbility_Plunge::OnHitEvent(FGameplayEventData Payload)
{
	const FJY_SkillStats Stats = GetSkillStats();
	const FTransform HitOrigin = GetAvatarHitOrigin();

	const FVector HitLocation = HitOrigin.GetLocation();
	const float FallHeight = PlungeStartHeight - HitLocation.Z;
	const float FallAlpha = FMath::Clamp(FallHeight / FallHeightForMaxRadius, 0.0f, 1.0f);
	const float FallRadiusScale = FMath::Lerp(1.0f, MaxFallRadiusScale, FallAlpha);

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

	/* 하강·도약 태스크는 어빌리티가 끝나면 엔진이 정리(소스 제거 AbilityTask_ApplyRootMotionConstantForce.cpp:146~151) */
	DiveTask = nullptr;
	LeapTask = nullptr;
	PlungeDirection = FVector::ZeroVector;
	SlamPoseTime = 0.0f;
	bAirPlunge = false;
	bLanded = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
