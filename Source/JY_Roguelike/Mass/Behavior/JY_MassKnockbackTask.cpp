#include "Mass/Behavior/JY_MassKnockbackTask.h"

#include "Mass/JY_MassTypes.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassStateTreeDependency.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassKnockbackTask)

bool FJY_MassKnockbackTask::Link(FStateTreeLinker& Linker) //override
{
	Linker.LinkExternalData(BehaviorHandle);
	Linker.LinkExternalData(KnockbackHandle);
	Linker.LinkExternalData(MoveTargetHandle);
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(DesiredMovementHandle);
	return true;
}

void FJY_MassKnockbackTask::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const //override
{
	Builder.AddReadWrite(BehaviorHandle);
	Builder.AddReadOnly(KnockbackHandle);
	Builder.AddReadWrite(MoveTargetHandle);
	Builder.AddReadOnly(TransformHandle);
	Builder.AddReadWrite(DesiredMovementHandle);
}

/* 날아가는 위치는 UJY_MassApplyMovementProcessor가 따로 계산한다 — 여기서는 엔진 조향·회피가 이 적을 건드리지 않게만 한다 */
// [Server]
EStateTreeRunStatus FJY_MassKnockbackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const //override
{
	FJY_EnemyBehaviorFragment& Behavior = Context.GetExternalData(BehaviorHandle);
	Behavior.State = EJY_EnemyState::Knockback;

	/* 동작이 Animate면 엔진 회피는 건너뛰고(MassAvoidanceProcessors.cpp:393) 엔진 조향은 원하는 속도를 0으로 둔다 (MassSteeringProcessors.cpp:364) */
	const FTransformFragment& TransformFragment = Context.GetExternalData(TransformHandle);
	FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
	const UWorld* World = Context.GetWorld();
	MoveTarget.CreateNewAction(EMassMovementAction::Animate, *World);
	MoveTarget.Center = TransformFragment.GetTransform().GetLocation();
	MoveTarget.DistanceToGoal = 0.0f;
	MoveTarget.DesiredSpeed.Set(0.0f);
	MoveTarget.IntentAtGoal = EMassMovementAction::Stand;

	/* 달리던 속도가 남으면 착지한 프레임에 그 속도로 한 번 움직인다 */
	FMassDesiredMovementFragment& DesiredMovement = Context.GetExternalData(DesiredMovementHandle);
	DesiredMovement.DesiredVelocity = FVector::ZeroVector;

	return EStateTreeRunStatus::Running;
}

/* Mass StateTree는 깨워졌을 때만 Tick이 불린다 — 착지한 프레임에 이동 Processor가 깨운다 */
// [Server]
EStateTreeRunStatus FJY_MassKnockbackTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const //override
{
	const FJY_EnemyKnockbackFragment& Knockback = Context.GetExternalData(KnockbackHandle);
	if (Knockback.bActive == true)
		return EStateTreeRunStatus::Running;

	return EStateTreeRunStatus::Succeeded;
}
