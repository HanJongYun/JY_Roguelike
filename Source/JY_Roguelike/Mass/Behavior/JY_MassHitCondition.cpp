#include "Mass/Behavior/JY_MassHitCondition.h"

#include "Mass/JY_MassTypes.h"
#include "MassStateTreeDependency.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassHitCondition)

bool FJY_MassHitCondition::Link(FStateTreeLinker& Linker) //override
{
	Linker.LinkExternalData(BehaviorHandle);
	return true;
}

void FJY_MassHitCondition::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const //override
{
	Builder.AddReadOnly(BehaviorHandle);
}

/* 움찔하는 중에 또 맞으면 끝나는 시각만 밀린다 — 상태에 다시 들어가지 않게 상태도 본다 */
// [Server]
bool FJY_MassHitCondition::TestCondition(FStateTreeExecutionContext& Context) const //override
{
	const FJY_EnemyBehaviorFragment& Behavior = Context.GetExternalData(BehaviorHandle);
	const UWorld* World = Context.GetWorld();
	return World->GetTimeSeconds() < Behavior.HitEndSeconds && Behavior.State != EJY_EnemyState::Hit;
}
