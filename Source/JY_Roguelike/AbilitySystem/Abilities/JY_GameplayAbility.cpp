#include "AbilitySystem/Abilities/JY_GameplayAbility.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "Combat/JY_CombatQuerySubsystem.h"
#include "System/JY_GameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility)

namespace JY_GameplayAbility
{
	/*
	 * 몽타주 위치의 뼈를 애니 에셋에서 직접 계산한다 — 실제 뼈를 쓰지 않아 데디 서버가 포즈를 갱신하는지와 무관.
	 * 루트까지 포함해 루트모션 전진도 더한다 — 내딛으며 벤 칼은 앞으로도 움직이므로 물체도 그만큼 앞으로 날아간다
	 */
	bool GetBoneLocationInMesh(const UAnimMontage* Montage, float MontagePosition, FName BoneName, FVector& OutLocation)
	{
		if (Montage->SlotAnimTracks.Num() == 0)
			return false;

		const FAnimSegment* Segment = Montage->SlotAnimTracks[0].AnimTrack.GetSegmentAtTime(MontagePosition);
		if (Segment == nullptr)
			return false;

		const UAnimSequence* Sequence = Cast<UAnimSequence>(Segment->GetAnimReference());
		if (Sequence == nullptr)
			return false;

		const FReferenceSkeleton& ReferenceSkeleton = Sequence->GetSkeleton()->GetReferenceSkeleton();
		int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
		if (BoneIndex == INDEX_NONE)
			return false;

		const FAnimExtractContext ExtractContext(static_cast<double>(Segment->ConvertTrackPosToAnimPos(MontagePosition)));
		FTransform BoneToMesh = FTransform::Identity;
		for (; BoneIndex != INDEX_NONE; BoneIndex = ReferenceSkeleton.GetParentIndex(BoneIndex))
		{
			FTransform LocalTransform;
			Sequence->GetBoneTransform(OUT LocalTransform, FSkeletonPoseBoneIndex(BoneIndex), ExtractContext, false);
			BoneToMesh = BoneToMesh * LocalTransform;
		}

		OutLocation = BoneToMesh.GetLocation();
		return true;
	}
}

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
/* 타격 순간에만 두 시점을 계산한다 — 매 프레임 위치를 기억해 둘 필요가 없다 */
FVector UJY_GameplayAbility::GetMontageSwingDirection(FName BoneName, float SampleSeconds) const
{
	const UAnimInstance* AnimInstance = CurrentActorInfo->GetAnimInstance();
	if (AnimInstance == nullptr)
		return FVector::ZeroVector;

	const UAnimMontage* Montage = AnimInstance->GetCurrentActiveMontage();
	if (Montage == nullptr)
		return FVector::ZeroVector;

	const float Position = AnimInstance->Montage_GetPosition(Montage);
	const float PreviousPosition = FMath::Max(Position - SampleSeconds, 0.0f);
	FVector BoneLocation;
	FVector PreviousBoneLocation;
	if (JY_GameplayAbility::GetBoneLocationInMesh(Montage, Position, BoneName, OUT BoneLocation) == false)
		return FVector::ZeroVector;

	if (JY_GameplayAbility::GetBoneLocationInMesh(Montage, PreviousPosition, BoneName, OUT PreviousBoneLocation) == false)
		return FVector::ZeroVector;

	/* 애니는 메시 공간 — 메시 회전(캐릭터 앞 = 메시 +Y)까지 거쳐 월드로 */
	const USkeletalMeshComponent* MeshComponent = CurrentActorInfo->SkeletalMeshComponent.Get();
	const FVector WorldDelta = MeshComponent->GetComponentTransform().TransformVectorNoScale(BoneLocation - PreviousBoneLocation);

	/* [임시 JY_SWINGDBG] */
	const FVector DebugActorForward = GetAvatarActorFromActorInfo()->GetActorForwardVector();
	const FVector DebugActorRight = GetAvatarActorFromActorInfo()->GetActorRightVector();
	UE_LOG(LogTemp, Warning, TEXT("[JY_SWINGDBG][%s] Montage=%s Pos=%.3f Prev=%.3f Bone=%s Prev=%s MeshDelta=%s Forward=%.2f Right=%.2f Up=%.2f"),
		CurrentActorInfo->IsNetAuthority() == true ? TEXT("Server") : TEXT("OwnClient"), *GetNameSafe(Montage), Position, PreviousPosition, *BoneLocation.ToString(), *PreviousBoneLocation.ToString(), *(BoneLocation - PreviousBoneLocation).ToString(),
		FVector::DotProduct(WorldDelta, DebugActorForward), FVector::DotProduct(WorldDelta, DebugActorRight), WorldDelta.Z);

	return WorldDelta.GetSafeNormal2D();
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
