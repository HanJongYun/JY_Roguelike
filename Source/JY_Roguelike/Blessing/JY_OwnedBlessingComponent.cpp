#include "Blessing/JY_OwnedBlessingComponent.h"
#include "Blessing/JY_BlessingDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_OwnedBlessingComponent)

UJY_OwnedBlessingComponent::UJY_OwnedBlessingComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/* 얻을 때와 타격 때만 동작 */
	PrimaryComponentTick.bCanEverTick = false;
}

// [Server]
void UJY_OwnedBlessingComponent::AddBlessing(const UJY_BlessingDefinition* Blessing)
{
	OwnedBlessings.AddUnique(Blessing);
}

// [Server]
const UJY_BlessingDefinition* UJY_OwnedBlessingComponent::FindActionBlessing(const FGameplayTag& ActionTag) const
{
	for (const UJY_BlessingDefinition* Blessing : OwnedBlessings)
	{
		if (Blessing->ActionTag == ActionTag)
			return Blessing;
	}

	return nullptr;
}

// [Server]
int32 UJY_OwnedBlessingComponent::GetIncreaseRate(const FGameplayTag& ActionTag) const
{
	int32 IncreaseRate = 0;
	for (const UJY_BlessingDefinition* Blessing : OwnedBlessings)
	{
		if (Blessing->ActionTag == ActionTag)
		{
			IncreaseRate += Blessing->ActionIncreaseRate;
		}
	}

	return IncreaseRate;
}
