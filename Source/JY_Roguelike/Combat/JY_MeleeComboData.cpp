#include "Combat/JY_MeleeComboData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MeleeComboData)

int32 UJY_MeleeComboData::FindStepIndex(FName SectionName) const
{
	auto IsStepSection = [SectionName](const FJY_MeleeComboStep& Step)
	{
		return Step.AttackSection == SectionName;
	};
	return Steps.IndexOfByPredicate(IsStepSection);
}
