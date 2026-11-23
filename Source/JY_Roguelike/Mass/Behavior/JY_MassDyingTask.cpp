#include "Mass/Behavior/JY_MassDyingTask.h"

#include "Mass/JY_MassTypes.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassStateTreeDependency.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassDyingTask)

FJY_MassDyingTask::FJY_MassDyingTask()
{
	/* 들어올 때 한 번만 일한다 */
	bShouldCallTick = false;
}

bool FJY_MassDyingTask::Link(FStateTreeLinker& Linker) //override
{
	Linker.LinkExternalData(BehaviorHandle);
	Linker.LinkExternalData(KnockbackHandle);
	Linker.LinkExternalData(MoveTargetHandle);
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(DesiredMovementHandle);
	return true;
}

void FJY_MassDyingTask::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const //override
{
	Builder.AddReadWrite(BehaviorHandle);
	Builder.AddReadWrite(KnockbackHandle);
	Builder.AddReadWrite(MoveTargetHandle);
	Builder.AddReadWrite(TransformHandle);
	Builder.AddReadWrite(DesiredMovementHandle);
}

// [Server]
EStateTreeRunStatus FJY_MassDyingTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const //override
{
	FJY_EnemyBehaviorFragment& Behavior = Context.GetExternalData(BehaviorHandle);
	Behavior.State = EJY_EnemyState::Dying;

	/* 날아가다 죽었으면 넉백을 끄고 바닥에 내려놓는다 — 위치의 Z는 바닥 높이 + 뜬 높이다 */
	FJY_EnemyKnockbackFragment& Knockback = Context.GetExternalData(KnockbackHandle);
	FTransformFragment& TransformFragment = Context.GetExternalData(TransformHandle);
	FTransform& Transform = TransformFragment.GetMutableTransform();
	const FVector GroundOffset(0.0f, 0.0f, -Knockback.Height);
	Transform.AddToTranslation(GroundOffset);
	Knockback.Height = 0.0f;
	Knockback.Velocity = FVector::ZeroVector;
	Knockback.bActive = false;

	/* 동작이 Animate면 엔진 회피는 건너뛰고(MassAvoidanceProcessors.cpp:393) 엔진 조향은 원하는 속도를 0으로 둔다 (MassSteeringProcessors.cpp:364) */
	FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
	const UWorld* World = Context.GetWorld();
	MoveTarget.CreateNewAction(EMassMovementAction::Animate, *World);
	MoveTarget.Center = Transform.GetLocation();
	MoveTarget.DistanceToGoal = 0.0f;
	MoveTarget.DesiredSpeed.Set(0.0f);
	MoveTarget.IntentAtGoal = EMassMovementAction::Stand;

	FMassDesiredMovementFragment& DesiredMovement = Context.GetExternalData(DesiredMovementHandle);
	DesiredMovement.DesiredVelocity = FVector::ZeroVector;

	return EStateTreeRunStatus::Running;
}
