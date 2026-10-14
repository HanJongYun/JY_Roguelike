#include "Skill/JY_SkillDefinition.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility_Skill.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_SkillDefinition)

const FJY_SkillLevelData& UJY_SkillDefinition::GetLevelData(int32 Level) const
{
	static const FJY_SkillLevelData DefaultLevelData;
	if (ensureMsgf(Levels.Num() > 0, TEXT("%s 레벨 데이터가 없음"), *GetName()) == false)
		return DefaultLevelData;

	const int32 LevelIndex = FMath::Clamp(Level - 1, 0, Levels.Num() - 1);
	return Levels[LevelIndex];
}
