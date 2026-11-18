#include "Mass/Behavior/JY_MassChaseTask.h"

#include "Mass/JY_MassTypes.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassStateTreeDependency.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassChaseTask)

FJY_MassChaseTask::FJY_MassChaseTask()
{
	/* 들어올 때 한 번만 일한다 */
	bShouldCallTick = false;
}

bool FJY_MassChaseTask::Link(FStateTreeLinker& Linker) //override
{
	Linker.LinkExternalData(BehaviorHandle);
	Linker.LinkExternalData(MoveTargetHandle);
	Linker.LinkExternalData(MovementParametersHandle);
	return true;
}

void FJY_MassChaseTask::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const //override
{
	Builder.AddReadWrite(BehaviorHandle);
	Builder.AddReadWrite(MoveTargetHandle);
	Builder.AddReadOnly(MovementParametersHandle);
}

// [Server]
EStateTreeRunStatus FJY_MassChaseTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const //override
{
	FJY_EnemyBehaviorFragment& Behavior = Context.GetExternalData(BehaviorHandle);
	Behavior.State = EJY_EnemyState::Chase;

	FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
	const UWorld* World = Context.GetWorld();
	MoveTarget.CreateNewAction(EMassMovementAction::Move, *World);

	/* 엔진 Steering은 이 속도로 가고, 0이면 안 움직인다 (MassNavMeshPathFollowTask.cpp:134) */
	const FMassMovementParameters& MovementParameters = Context.GetExternalData(MovementParametersHandle);
	MoveTarget.DesiredSpeed.Set(MovementParameters.DefaultDesiredSpeed);

	/* 남은 거리가 끝까지의 경로 거리라, 플레이어에 가까워질 때만 엔진이 감속한다 (MassSteeringProcessors.cpp:147) */
	MoveTarget.IntentAtGoal = EMassMovementAction::Stand;

	return EStateTreeRunStatus::Running;
}
