#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "JY_DashGameplayEffects.generated.h"

/* 대시 비용 — 남은 횟수 -1. 대시 어빌리티의 비용 GE (0이면 CheckCost가 발동을 막는다) */
UCLASS()
class UJY_GameplayEffect_DashCost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UJY_GameplayEffect_DashCost();
};

/* 대시 충전 — 남은 횟수 +1. 대시를 쓸 때마다 충전 시간 뒤 서버가 적용 */
UCLASS()
class UJY_GameplayEffect_DashRecharge : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UJY_GameplayEffect_DashRecharge();
};
