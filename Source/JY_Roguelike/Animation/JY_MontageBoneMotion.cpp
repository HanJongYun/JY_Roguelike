#include "Animation/JY_MontageBoneMotion.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"

namespace JY_MontageBoneMotion
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

	// [Server] [OwnClient]
	/* 필요한 순간에만 두 시점을 계산한다 — 매 프레임 위치를 기억해 둘 필요가 없다 */
	FVector GetMontageBoneMoveDirection(const UAnimInstance* AnimInstance, const USkeletalMeshComponent* MeshComponent, FName BoneName, float SampleSeconds)
	{
		if (AnimInstance == nullptr)
			return FVector::ZeroVector;

		const UAnimMontage* Montage = AnimInstance->GetCurrentActiveMontage();
		if (Montage == nullptr)
			return FVector::ZeroVector;

		const float Position = AnimInstance->Montage_GetPosition(Montage);
		const float PreviousPosition = FMath::Max(Position - SampleSeconds, 0.0f);
		FVector BoneLocation;
		FVector PreviousBoneLocation;
		if (GetBoneLocationInMesh(Montage, Position, BoneName, OUT BoneLocation) == false)
			return FVector::ZeroVector;

		if (GetBoneLocationInMesh(Montage, PreviousPosition, BoneName, OUT PreviousBoneLocation) == false)
			return FVector::ZeroVector;

		/* 애니는 메시 공간 — 메시 회전(캐릭터 앞 = 메시 +Y)까지 거쳐 월드로 */
		const FVector WorldDelta = MeshComponent->GetComponentTransform().TransformVectorNoScale(BoneLocation - PreviousBoneLocation);

		/* [임시 JY_SWINGDBG] */
		const AActor* DebugOwner = MeshComponent->GetOwner();
		const FVector DebugActorForward = DebugOwner->GetActorForwardVector();
		const FVector DebugActorRight = DebugOwner->GetActorRightVector();
		UE_LOG(LogTemp, Warning, TEXT("[JY_SWINGDBG][%s] Montage=%s Pos=%.3f Prev=%.3f Bone=%s Prev=%s MeshDelta=%s Forward=%.2f Right=%.2f Up=%.2f"),
			DebugOwner->HasAuthority() == true ? TEXT("Server") : TEXT("OwnClient"), *GetNameSafe(Montage), Position, PreviousPosition, *BoneLocation.ToString(), *PreviousBoneLocation.ToString(), *(BoneLocation - PreviousBoneLocation).ToString(),
			FVector::DotProduct(WorldDelta, DebugActorForward), FVector::DotProduct(WorldDelta, DebugActorRight), WorldDelta.Z);

		return WorldDelta.GetSafeNormal2D();
	}
}
