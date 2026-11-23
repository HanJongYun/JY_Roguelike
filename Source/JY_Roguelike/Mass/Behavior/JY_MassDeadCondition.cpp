#include "Mass/Behavior/JY_MassDeadCondition.h"

#include "Mass/JY_MassTypes.h"
#include "MassStateTreeDependency.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassDeadCondition)

bool FJY_MassDeadCondition::Link(FStateTreeLinker& Linker) //override
{
	Linker.LinkExternalData(BehaviorHandle);
	Linker.LinkExternalData(HealthHandle);
	return true;
}

void FJY_MassDeadCondition::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const //override
{
	Builder.AddReadOnly(BehaviorHandle);
	Builder.AddReadOnly(HealthHandle);
}

/* 죽는 중에 다른 신호로 깨어나도 죽는 상태에 다시 들어가지 않게 상태도 본다 */
// [Server]
bool FJY_MassDeadCondition::TestCondition(FStateTreeExecutionContext& Context) const //override
{
	const FJY_EnemyHealthFragment& Health = Context.GetExternalData(HealthHandle);
	const FJY_EnemyBehaviorFragment& Behavior = Context.GetExternalData(BehaviorHandle);
	return Health.Current <= 0.0f && Behavior.State != EJY_EnemyState::Dying;
}
