#include "AbilitySystem/Abilities/JY_GameplayAbility.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "Combat/JY_CombatQuerySubsystem.h"
#include "System/JY_GameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility)

UJY_GameplayAbility::UJY_GameplayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/* 어빌리티 안에 상태(태스크·델리게이트 핸들)를 두므로 액터마다 인스턴스 1개 */
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	/* 죽으면 모든 어빌리티 차단 — Status.Death.Dying도 부모 태그 일치로 걸린다(FGameplayTagContainer::HasAny) */
	ActivationBlockedTags.AddTag(JYGameplayTags::Status_Death);
}

UJY_AbilitySystemComponent* UJY_GameplayAbility::GetJYAbilitySystemComponentFromActorInfo() const
{
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	return CastChecked<UJY_AbilitySystemComponent>(AbilitySystemComponent);
}

// [OwnClient]
/* 바닥 충돌 대신 발 높이 평면 — 커서가 벽·적 위에 있어도 방향이 틀어지지 않는다 */
FVector UJY_GameplayAbility::ComputeCursorDirection() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const FVector ForwardVector = AvatarActor->GetActorForwardVector();
	const FVector ForwardDirection = ForwardVector.GetSafeNormal2D();

	/* 커서가 화면 밖이면 광선을 못 만든다 */
	const APlayerController* PlayerController = CurrentActorInfo->PlayerController.Get();
	FVector RayOrigin;
	FVector RayDirection;
	if (PlayerController->DeprojectMousePositionToWorld(OUT RayOrigin, OUT RayDirection) == false)
		return ForwardDirection;

	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	const float FootHeight = AvatarLocation.Z - AvatarActor->GetSimpleCollisionHalfHeight();
	const FVector FootPoint(AvatarLocation.X, AvatarLocation.Y, FootHeight);
	const FPlane FootPlane(FootPoint, FVector::UpVector);
	const FVector CursorPoint = FMath::RayPlaneIntersection(RayOrigin, RayDirection, FootPlane);
	const FVector ToCursor = CursorPoint - AvatarLocation;
	const FVector CursorDirection = ToCursor.GetSafeNormal2D();

	/* 커서가 발밑이면 방향이 없다 */
	if (CursorDirection.IsNearlyZero() == true)
		return ForwardDirection;

	return CursorDirection;
}

// [Server] [OwnClient]
/* 루트모션 중엔 CMC가 몸을 돌리지 않는다(bAllowPhysicsRotationDuringAnimRootMotion=false, CharacterMovementComponent.cpp:3044) → 동작 시작 때 직접 돌린다 */
void UJY_GameplayAbility::FaceDirection(const FVector& Direction) const
{
	const FRotator DirectionRotation = Direction.Rotation();
	const FRotator YawRotation(0.0f, DirectionRotation.Yaw, 0.0f);
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AvatarActor->SetActorRotation(YawRotation);
}

// [Server] [OwnClient]
/* 서버 확인을 기다리면 왕복 지연만큼 늦어 조종 클라는 같은 판정을 미리 돌려 화면만 흔든다. 밀기 방향 = 판정 원점 → 대상 */
void UJY_GameplayAbility::ApplyHitInShape(const FJY_HitShape& Shape, const FTransform& Origin, float Damage, float KnockbackSpeed, FGameplayTag HitCueTag, TSubclassOf<UCameraShakeBase> HitCameraShake, const FVector& ObjectPushDirection) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const UWorld* World = GetWorld();
	const UJY_CombatQuerySubsystem* CombatQuery = World->GetSubsystem<UJY_CombatQuerySubsystem>();
	TArray<FJY_CombatTarget> Targets;
	CombatQuery->FindTargets(Shape, Origin, AvatarActor, OUT Targets);
	if (Targets.Num() == 0)
		return;

	if (CurrentActorInfo->IsNetAuthority() == true)
	{
		const FVector OriginLocation = Origin.GetLocation();
		for (const FJY_CombatTarget& Target : Targets)
		{
			const FVector ToTarget = Target.Location - OriginLocation;
			const FVector KnockbackDirection = ToTarget.GetSafeNormal2D();

			FJY_HitPayload HitPayload;
			HitPayload.Damage = Damage;
			HitPayload.KnockbackImpulse = FVector2f(KnockbackDirection.X, KnockbackDirection.Y) * KnockbackSpeed;
			HitPayload.HitCueTag = HitCueTag;
			HitPayload.Instigator = AvatarActor;
			HitPayload.ObjectPushDirection = ObjectPushDirection;
			CombatQuery->ApplyHit(Target, HitPayload);
		}
	}

	if (CurrentActorInfo->IsLocallyControlled() == true && HitCameraShake != nullptr)
	{
		const APlayerController* PlayerController = CurrentActorInfo->PlayerController.Get();
		APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
		CameraManager->StartCameraShake(HitCameraShake);
	}
}

// [Server] [OwnClient]
/* 몽타주가 끝나면 어빌리티도 끝내고, 어빌리티가 끝나면 몽타주도 블렌드 아웃 (bStopWhenAbilityEnds) */
void UJY_GameplayAbility::PlayMontageUntilEnd(UAnimMontage* MontageToPlay, float MontagePlayRate, FName StartSection, float RootMotionScale)
{
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontageToPlay, MontagePlayRate, StartSection, true, RootMotionScale);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UJY_GameplayAbility::OnMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UJY_GameplayAbility::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
