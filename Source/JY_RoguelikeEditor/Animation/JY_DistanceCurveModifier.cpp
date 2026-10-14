#include "Animation/JY_DistanceCurveModifier.h"
#include "AnimationBlueprintLibrary.h"
#include "Animation/AnimSequence.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_DistanceCurveModifier)

/* 엔진 UDistanceCurveModifier::OnApply_Implementation(DistanceCurveModifier.cpp:14)과 같은 굽기, 멈춤점 고르는 규칙만 "처음 아래로" */
void UJY_DistanceCurveModifier::OnApply_Implementation(UAnimSequence* Animation)
{
	/* 루트 모션을 꺼 둔 애니에 적용한 기획 실수 */
	if (ensureMsgf(Animation != nullptr && Animation->HasRootMotion() == true, TEXT("JY_DistanceCurveModifier: animation needs root motion enabled.")) == false)
		return;

	const bool bRemoveNameFromSkeleton = false;
	UAnimationBlueprintLibrary::RemoveCurve(Animation, CurveName, bRemoveNameFromSkeleton);
	UAnimationBlueprintLibrary::AddCurve(Animation, CurveName, ERawCurveTrackTypes::RCT_Float, false);

	const float AnimLength = Animation->GetPlayLength();

	/* 멈춤점: 1/120초 간격으로 수평 루트 속도를 보고 처음 임계 아래가 되는 순간. 없으면 끝 */
	float StopTime = AnimLength;
	const float SearchInterval = 1.0f / 120.0f;
	const int32 SearchSteps = FMath::FloorToInt(AnimLength / SearchInterval);
	for (int32 Step = 0; Step < SearchSteps; ++Step)
	{
		const float Time = Step * SearchInterval;
		if (Time < SearchStartTime)
			continue;

		const bool bAllowLooping = false;
		const FAnimExtractContext Context(static_cast<double>(Time), true, FDeltaTimeRecord(SearchInterval), bAllowLooping);
		const FVector Translation = Animation->ExtractRootMotion(Context).GetTranslation();
		const float Speed = Translation.Size2D() / SearchInterval;
		if (Speed < StopSpeedThreshold)
		{
			StopTime = Time;
			break;
		}
	}

	const float SampleInterval = 1.0f / SampleRate;
	const int32 SampleSteps = FMath::CeilToInt(AnimLength / SampleInterval);
	for (int32 Step = 0; Step <= SampleSteps; ++Step)
	{
		const float Time = FMath::Min(Step * SampleInterval, AnimLength);
		const float ValueSign = Time < StopTime ? -1.0f : 1.0f;
		const FVector Translation = Animation->ExtractRootMotionFromRange(StopTime, Time, FAnimExtractContext()).GetTranslation();
		UAnimationBlueprintLibrary::AddFloatCurveKey(Animation, CurveName, Time, ValueSign * Translation.Size2D());
	}
}

void UJY_DistanceCurveModifier::OnRevert_Implementation(UAnimSequence* Animation)
{
	const bool bRemoveNameFromSkeleton = false;
	UAnimationBlueprintLibrary::RemoveCurve(Animation, CurveName, bRemoveNameFromSkeleton);
}
