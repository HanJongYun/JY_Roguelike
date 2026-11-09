#include "Combat/JY_StatusEffectData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_StatusEffectData)

const FJY_StatusEffectEntry* UJY_StatusEffectData::FindStatusEffect(const FGameplayTag& StatusTag) const
{
	auto IsSameStatus = [&StatusTag](const FJY_StatusEffectEntry& Entry)
	{
		return Entry.StatusTag == StatusTag;
	};
	return StatusEffects.FindByPredicate(IsSameStatus);
}
