#include "Mass/Behavior/JY_MassWaitTask.h"

#include "Mass/JY_MassTypes.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassStateTreeDependency.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassWaitTask)

FJY_MassWaitTask::FJY_MassWaitTask()
{
	/* 들어올 때 한 번만 일한다 */
	bShouldCallTick = false;
}

bool FJY_MassWaitTask::Link(FStateTreeLinker& Linker) //override
{
	Linker.LinkExternalData(BehaviorHandle);
	Linker.LinkExternalData(MoveTargetHandle);
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(DesiredMovementHandle);
	return true;
}

void FJY_MassWaitTask::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const //override
{
	Builder.AddReadWrite(BehaviorHandle);
	Builder.AddReadWrite(MoveTargetHandle);
	Builder.AddReadOnly(TransformHandle);
	Builder.AddReadWrite(DesiredMovementHandle);
}

/* 엔진 서기 Task와 같은 값 — 갈 곳을 지금 자리로, 동작을 서기로 (MassNavMeshStandTask.cpp:42) */
// [Server]
EStateTreeRunStatus FJY_MassWaitTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const //override
{
	FJY_EnemyBehaviorFragment& Behavior = Context.GetExternalData(BehaviorHandle);
	Behavior.State = EJY_EnemyState::Wait;

	const FTransformFragment& TransformFragment = Context.GetExternalData(TransformHandle);
	FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
	const UWorld* World = Context.GetWorld();
	MoveTarget.CreateNewAction(EMassMovementAction::Stand, *World);
	MoveTarget.Center = TransformFragment.GetTransform().GetLocation();
	MoveTarget.DistanceToGoal = 0.0f;
	MoveTarget.DesiredSpeed.Set(0.0f);
	MoveTarget.IntentAtGoal = EMassMovementAction::Stand;

	/* 엔진 서기 조향은 갈 곳 근처에서 힘을 주지 않아 달리던 속도가 그대로 남는다 (MassSteeringProcessors.cpp:316·349) — 여기서 0으로 만들어야 미끄러지지 않는다 */
	FMassDesiredMovementFragment& DesiredMovement = Context.GetExternalData(DesiredMovementHandle);
	DesiredMovement.DesiredVelocity = FVector::ZeroVector;

	return EStateTreeRunStatus::Running;
}
