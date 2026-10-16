#pragma once

#include "CoreMinimal.h"
#include "Object/JY_Action.h"
#include "JY_TravelAction.generated.h"

/* 맵 이동 — 서버가 모두를 다른 맵으로 옮김 (포털) */
UCLASS(meta = (DisplayName = "Travel"))
class UJY_TravelAction : public UJY_Action
{
	GENERATED_BODY()

public:
	virtual void OnServerEnter(AJY_Object* Object) override;

private:
	/* 설정값 */

	/* 이동할 맵 */
	UPROPERTY(EditAnywhere, Category = "JY|Travel")
	TSoftObjectPtr<UWorld> DestinationMap;
};
