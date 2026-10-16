#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_CameraOcclusionFadeComponent.generated.h"

class UPrimitiveComponent;

/* 카메라와 내 캐릭터 사이를 가리는 벽(FadeActorTag 액터)을 내 화면에서 흐리게 함 */
UCLASS()
class UJY_CameraOcclusionFadeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_CameraOcclusionFadeComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/* 카메라→캐릭터 구간을 훑어 지금 가리는 물체 목록을 새로 만듦 */
	void UpdateOccluders();

	/* 흐림 값을 목표 쪽으로 옮기고, 원래대로 돌아온 물체는 목록에서 뺌 */
	void UpdateFadeAmounts(float DeltaTime);

private:
	/* 설정값 — BP_JY_PlayerController의 CameraOcclusionFade 컴포넌트 기본값에서 수정 */

	/* 흐려질 액터에 붙이는 태그 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|CameraFade")
	FName FadeActorTag = TEXT("CameraFade");

	/* 재질 CameraFade 파라미터의 Primitive Data Index */
	UPROPERTY(EditDefaultsOnly, Category = "JY|CameraFade", Meta = (ClampMin = "0"))
	int32 FadeDataIndex = 0;

	/* 가리는 물체를 다시 찾는 간격 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|CameraFade", Meta = (ClampMin = "0.02"))
	float OccluderCheckInterval = 0.1f;

	/* 훑는 구의 반지름 (cm) — 캐릭터 몸 폭 정도 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|CameraFade", Meta = (ClampMin = "0"))
	float ProbeRadius = 50.0f;

	/* 1초에 바뀌는 흐림 양 (0~1) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|CameraFade", Meta = (ClampMin = "0.1"))
	float FadeSpeed = 5.0f;

private:
	/* 물체별 지금 흐림 값 (0 = 원래, 1 = 최대) */
	TMap<TWeakObjectPtr<UPrimitiveComponent>, float> FadeAmounts;

	/* 지금 가리고 있는 물체 */
	TSet<TWeakObjectPtr<UPrimitiveComponent>> Occluders;

	/* 다음 검사까지 남은 시간 (초) */
	float TimeUntilOccluderCheck = 0.0f;
};
