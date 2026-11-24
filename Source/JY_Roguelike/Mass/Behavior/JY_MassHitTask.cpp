#include "Mass/Behavior/JY_MassHitTask.h"

#include "Mass/JY_MassStateTreeWake.h"
#include "Mass/JY_MassTypes.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassSignalSubsystem.h"
#include "MassStateTreeDependency.h"
#include "MassStateTreeExecutionContext.h"
#include "StateTreeLinker.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassHitTask)

namespace JY_MassHitTask
{
	/* 예약 신호가 끝나는 시각보다 아주 조금 일찍 오면 다시 기다리게 되므로 조금 늦춰 깨운다 */
	constexpr float WakeDelayPaddingSeconds = 0.01f;
}

bool FJY_MassHitTask::Link(FStateTreeLinker& Linker) //override
{
	Linker.LinkExternalData(BehaviorHandle);
	Linker.LinkExternalData(KnockbackHandle);
	Linker.LinkExternalData(MoveTargetHandle);
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(DesiredMovementHandle);
	Linker.LinkExternalData(SignalSubsystemHandle);
	return true;
}

void FJY_MassHitTask::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const //override
{
	Builder.AddReadWrite(BehaviorHandle);
	Builder.AddReadOnly(KnockbackHandle);
	Builder.AddReadWrite(MoveTargetHandle);
	Builder.AddReadOnly(TransformHandle);
	Builder.AddReadWrite(DesiredMovementHandle);
	Builder.AddReadWrite(SignalSubsystemHandle);
}

/* 서 있는 적과 같은 값으로 세운다 (FJY_MassWaitTask) — 밀리는 위치는 UJY_MassApplyMovementProcessor가 넉백 속도로 따로 옮긴다 */
// [Server]
EStateTreeRunStatus FJY_MassHitTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const //override
{
	FJY_EnemyBehaviorFragment& Behavior = Context.GetExternalData(BehaviorHandle);
	Behavior.State = EJY_EnemyState::Hit;

	const FTransformFragment& TransformFragment = Context.GetExternalData(TransformHandle);
	FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
	const UWorld* World = Context.GetWorld();
	MoveTarget.CreateNewAction(EMassMovementAction::Stand, *World);
	MoveTarget.Center = TransformFragment.GetTransform().GetLocation();
	MoveTarget.DistanceToGoal = 0.0f;
	MoveTarget.DesiredSpeed.Set(0.0f);
	MoveTarget.IntentAtGoal = EMassMovementAction::Stand;

	/* 달리던 속도가 남으면 맞고도 미끄러진다 */
	FMassDesiredMovementFragment& DesiredMovement = Context.GetExternalData(DesiredMovementHandle);
	DesiredMovement.DesiredVelocity = FVector::ZeroVector;

	return WaitForHitEnd(Context);
}

/* Mass StateTree는 깨워졌을 때만 Tick이 불린다 — 예약한 신호나, 또 맞아 피해 Processor가 깨웠을 때나, 밀림이 멈춰 이동 Processor가 깨웠을 때 */
// [Server]
EStateTreeRunStatus FJY_MassHitTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const //override
{
	return WaitForHitEnd(Context);
}

// [Server]
EStateTreeRunStatus FJY_MassHitTask::WaitForHitEnd(FStateTreeExecutionContext& Context) const
{
	/* 밀리는 중이면 멈춘 프레임에 이동 Processor가 깨운다 — 그때 남은 움찔 시간을 다시 본다 */
	const FJY_EnemyKnockbackFragment& Knockback = Context.GetExternalData(KnockbackHandle);
	if (Knockback.bActive == true)
		return EStateTreeRunStatus::Running;

	const FJY_EnemyBehaviorFragment& Behavior = Context.GetExternalData(BehaviorHandle);
	const UWorld* World = Context.GetWorld();
	const double RemainingSeconds = Behavior.HitEndSeconds - World->GetTimeSeconds();
	if (RemainingSeconds <= 0.0)
		return EStateTreeRunStatus::Succeeded;

	/* 또 맞으면 끝나는 시각이 밀려 있다 — 남은 시간으로 다시 예약한다 (FJY_MassSpawningTask와 같은 방식) */
	FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	UMassSignalSubsystem& SignalSubsystem = Context.GetExternalData(SignalSubsystemHandle);
	const float WakeDelaySeconds = static_cast<float>(RemainingSeconds) + JY_MassHitTask::WakeDelayPaddingSeconds;
	JY_MassStateTreeWake::WakeEntityAfterDelay(MassStateTreeContext.GetMassEntityExecutionContext(), SignalSubsystem, MassStateTreeContext.GetEntity(), WakeDelaySeconds);

	return EStateTreeRunStatus::Running;
}
