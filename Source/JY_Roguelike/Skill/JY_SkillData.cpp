#include "Skill/JY_SkillData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_SkillData)

const FJY_SkillEntry* UJY_SkillData::FindSkill(const FGameplayTag& SkillTag) const
{
	auto IsSameSkill = [&SkillTag](const FJY_SkillEntry& Entry)
	{
		return Entry.SkillTag == SkillTag;
	};
	return Skills.FindByPredicate(IsSameSkill);
}
