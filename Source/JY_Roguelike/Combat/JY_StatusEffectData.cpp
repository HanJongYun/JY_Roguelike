#include "Combat/JY_StatusEffectData.h"

#include "Mass/JY_MassTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_StatusEffectData)

const FJY_StatusEffectEntry* UJY_StatusEffectData::FindStatusEffect(const FGameplayTag& StatusTag) const
{
	auto IsSameStatus = [&StatusTag](const FJY_StatusEffectEntry& Entry)
	{
		return Entry.StatusTag == StatusTag;
	};
	return StatusEffects.FindByPredicate(IsSameStatus);
}

/* Mass 적의 복제 값(StatusFlags)이 uint8 비트라 앞의 8종까지만 번호를 준다 */
int32 UJY_StatusEffectData::FindStatusEffectIndex(const FGameplayTag& StatusTag) const
{
	auto IsSameStatus = [&StatusTag](const FJY_StatusEffectEntry& Entry)
	{
		return Entry.StatusTag == StatusTag;
	};
	const int32 StatusIndex = StatusEffects.IndexOfByPredicate(IsSameStatus);
	if (StatusIndex == INDEX_NONE)
		return INDEX_NONE;

	if (ensureMsgf(StatusIndex < JY_Mass::MaxStatusCount, TEXT("DA_JY_StatusEffectData의 %s가 %d번째 — Mass 적은 앞의 %d종만 걸린다"), *StatusTag.ToString(), StatusIndex + 1, JY_Mass::MaxStatusCount) == false)
		return INDEX_NONE;

	return StatusIndex;
}
