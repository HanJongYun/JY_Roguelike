#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "JY_OwnedBlessingComponent.generated.h"

class UJY_BlessingDefinition;

/* 캐릭터가 가진 축복 — 타격 순간 그 행동의 속성·상태이상·피해 증가를 알려 준다 (서버) */
UCLASS(ClassGroup = (JY))
class UJY_OwnedBlessingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_OwnedBlessingComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	/* [Server] 축복 얻기 */
	void AddBlessing(const UJY_BlessingDefinition* Blessing);

	/* [Server] 이 행동에 붙은 축복. 없으면 nullptr */
	const UJY_BlessingDefinition* FindActionBlessing(const FGameplayTag& ActionTag) const;

	/* [Server] 이 행동의 피해에 맞는 증가분 합 (만분율) */
	int32 GetIncreaseRate(const FGameplayTag& ActionTag) const;

private:
	/* 가진 축복 (서버만) */
	UPROPERTY()
	TArray<TObjectPtr<const UJY_BlessingDefinition>> OwnedBlessings;
};
