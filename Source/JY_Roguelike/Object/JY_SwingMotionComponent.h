#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_SwingMotionComponent.generated.h"

/* BP 부품 하나를 서버 시각 기준으로 피치(Y축) 진자 흔들림 (도끼) */
UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_SwingMotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_SwingMotionComponent();

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/* 설정값 — BP 컴포넌트 디테일에서 수정 (박자 어긋내기만 배치한 물체마다) */

	/* 흔들 부품의 Component Tags */
	UPROPERTY(EditAnywhere, Category = "JY|Swing")
	FName PartTag;

	/* 한 번 왕복 시간 (초) */
	UPROPERTY(EditAnywhere, Category = "JY|Swing", Meta = (ClampMin = "0.5"))
	float PeriodSeconds = 2.4f;

	/* 한쪽 최대 각도 */
	UPROPERTY(EditAnywhere, Category = "JY|Swing", Meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float AngleDegrees = 60.0f;

	/* 이 물체의 박자 어긋냄 (초) */
	UPROPERTY(EditAnywhere, Category = "JY|Swing", Meta = (ClampMin = "0.0"))
	float PhaseOffsetSeconds = 0.0f;
};
