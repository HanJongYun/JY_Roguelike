#include "Mass/Behavior/JY_MassInsideRangeCondition.h"

#include "Mass/JY_MassTypes.h"
#include "MassStateTreeDependency.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassInsideRangeCondition)

bool FJY_MassInsideRangeCondition::Link(FStateTreeLinker& Linker) //override
{
	Linker.LinkExternalData(BehaviorHandle);
	return true;
}

void FJY_MassInsideRangeCondition::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const //override
{
	Builder.AddReadOnly(BehaviorHandle);
}

// [Server]
bool FJY_MassInsideRangeCondition::TestCondition(FStateTreeExecutionContext& Context) const //override
{
	const FJY_EnemyBehaviorFragment& Behavior = Context.GetExternalData(BehaviorHandle);
	return Behavior.bInsideRange != bInvert;
}
