#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "JY_DistanceCurveModifier.generated.h"

/*
 * 루트 모션으로 거리 커브를 굽는다 (엔진 DistanceCurveModifier와 같은 값 규칙: 멈춤점 전은 남은 거리 음수, 뒤는 지난 거리 양수).
 * 멈춤점만 다르다 — 엔진은 임계 속도 아래 구간 중 "가장 느린" 순간을 잡아, 멈춘 뒤에도 루트가 조금씩 기어가는 애니는 끝을 멈춤점으로 삼는다
 */
UCLASS()
class UJY_DistanceCurveModifier : public UAnimationModifier
{
	GENERATED_BODY()

public:
	virtual void OnApply_Implementation(UAnimSequence* Animation) override;
	virtual void OnRevert_Implementation(UAnimSequence* Animation) override;

public:
	/* 설정값 — ApplyAnimationModifier 도구의 PropertyValues로 바꾼다 */

	/* 커브를 굽는 샘플 수 (초당) */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = "1"))
	int32 SampleRate = 30;

	/* 만들 커브 이름 */
	UPROPERTY(EditAnywhere, Category = "Settings")
	FName CurveName = TEXT("Distance");

	/* 처음으로 이 속도(cm/s) 아래로 떨어지는 순간을 멈춤점으로 본다 */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = "0.0"))
	float StopSpeedThreshold = 5.0f;

	/* 이 시간(초) 이후에서만 멈춤점을 찾는다 — 출발 직후처럼 원래 느린 앞부분을 건너뛴다 */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = "0.0"))
	float SearchStartTime = 0.0f;
};
