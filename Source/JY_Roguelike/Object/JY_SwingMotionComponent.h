#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_SwingMotionComponent.generated.h"

/*
 * 진자 흔들림 — BP 부품 하나를 피치(Y축)로 앞뒤로 흔든다 (도끼).
 * 각도를 서버 시각의 사인으로 계산해 복제하지 않는다 — 서버·클라가 각자 같은 각도를 그린다
 */
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
