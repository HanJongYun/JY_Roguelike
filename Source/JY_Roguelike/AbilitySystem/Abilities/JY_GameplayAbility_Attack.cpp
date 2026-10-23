#include "AbilitySystem/Abilities/JY_GameplayAbility_Attack.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "System/JY_GameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_Attack)

namespace JY_GameplayAbility_Attack
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

UJY_GameplayAbility_Attack::UJY_GameplayAbility_Attack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationBlockedTags.AddTag(JYGameplayTags::State_Dashing);
}

// [Server] [OwnClient]
/* 서버도 같은 검사를 하므로 서버 몽타주가 발동 RPC 지연만큼 늦게 시작해도 클라와 같은 위치에서 판정한다 */
bool UJY_GameplayAbility_Attack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const //override
{
	if (Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) == false)
		return false;

	return IsBlockingAttackActive(ActorInfo) == false;
}

bool UJY_GameplayAbility_Attack::IsBlockingOtherAttacks() const
{
	return false;
}

// [Server] [OwnClient]
/* 막기 태그(BlockAbilitiesWithTag)는 실행 내내 고정이라 "끊기 구간 안에서만 허용"을 표현할 수 없다 → 실행 중 공격에 직접 묻는다 */
bool UJY_GameplayAbility_Attack::IsBlockingAttackActive(const FGameplayAbilityActorInfo* ActorInfo)
{
	const UAbilitySystemComponent* AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();
	for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (AbilitySpec.IsActive() == false)
			continue;

		const UJY_GameplayAbility_Attack* ActiveAttack = Cast<UJY_GameplayAbility_Attack>(AbilitySpec.GetPrimaryInstance());
		if (ActiveAttack == nullptr)
			continue;

		if (ActiveAttack->IsBlockingOtherAttacks() == true)
			return true;
	}
	return false;
}

// [OwnClient]
/* 공중 여부는 클라가 정해 보낸다 — 서버가 자기 낙하 상태로 정하면 지연만큼 어긋난다 */
FGameplayTag UJY_GameplayAbility_Attack::MakeStartTag() const
{
	const ACharacter* Character = CastChecked<ACharacter>(GetAvatarActorFromActorInfo());
	const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	if (MovementComponent->IsFalling() == true)
		return JYGameplayTags::Ability_Start_Air;

	return FGameplayTag();
}

FTransform UJY_GameplayAbility_Attack::GetAvatarHitOrigin() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const FRotator AvatarRotation = AvatarActor->GetActorRotation();
	const FRotator YawRotation(0.0f, AvatarRotation.Yaw, 0.0f);
	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	return FTransform(YawRotation, AvatarLocation);
}

// [Server] [OwnClient]
/* 타격 순간에만 두 시점을 계산한다 — 매 프레임 위치를 기억해 둘 필요가 없다 */
FVector UJY_GameplayAbility_Attack::GetMontageSwingDirection(FName BoneName, float SampleSeconds) const
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
	if (JY_GameplayAbility_Attack::GetBoneLocationInMesh(Montage, Position, BoneName, OUT BoneLocation) == false)
		return FVector::ZeroVector;

	if (JY_GameplayAbility_Attack::GetBoneLocationInMesh(Montage, PreviousPosition, BoneName, OUT PreviousBoneLocation) == false)
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
