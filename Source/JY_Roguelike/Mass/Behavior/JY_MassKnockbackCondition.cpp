#include "Mass/Behavior/JY_MassKnockbackCondition.h"

#include "Mass/JY_MassTypes.h"
#include "MassStateTreeDependency.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassKnockbackCondition)

bool FJY_MassKnockbackCondition::Link(FStateTreeLinker& Linker) //override
{
	Linker.LinkExternalData(BehaviorHandle);
	Linker.LinkExternalData(KnockbackHandle);
	return true;
}

void FJY_MassKnockbackCondition::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const //override
{
	Builder.AddReadOnly(BehaviorHandle);
	Builder.AddReadOnly(KnockbackHandle);
}

/* 날아가는 동안 다른 신호(거리 안팎 바뀜)로 깨어나도 넉백 상태에 다시 들어가지 않게 상태도 본다 */
// [Server]
bool FJY_MassKnockbackCondition::TestCondition(FStateTreeExecutionContext& Context) const //override
{
	const FJY_EnemyKnockbackFragment& Knockback = Context.GetExternalData(KnockbackHandle);
	const FJY_EnemyBehaviorFragment& Behavior = Context.GetExternalData(BehaviorHandle);
	return Knockback.bActive == true && Behavior.State != EJY_EnemyState::Knockback;
}
