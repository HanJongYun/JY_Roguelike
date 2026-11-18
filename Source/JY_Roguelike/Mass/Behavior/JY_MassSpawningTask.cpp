#include "Mass/Behavior/JY_MassSpawningTask.h"

#include "Mass/JY_MassStateTreeWake.h"
#include "Mass/JY_MassTypes.h"
#include "MassSignalSubsystem.h"
#include "MassStateTreeDependency.h"
#include "MassStateTreeExecutionContext.h"
#include "StateTreeLinker.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassSpawningTask)

namespace JY_MassSpawningTask
{
	/* 예약 신호가 끝나는 시각보다 아주 조금 일찍 오면 다시 기다리게 되므로 조금 늦춰 깨운다 */
	constexpr float WakeDelayPaddingSeconds = 0.01f;
}

bool FJY_MassSpawningTask::Link(FStateTreeLinker& Linker) //override
{
	Linker.LinkExternalData(BehaviorHandle);
	Linker.LinkExternalData(SignalSubsystemHandle);
	return true;
}

/* 엔진이 이 목록으로 트리 전용 Processor의 쿼리를 만든다 (MassStateTreeSubsystem.cpp:100) */
void FJY_MassSpawningTask::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const //override
{
	Builder.AddReadWrite(BehaviorHandle);
	Builder.AddReadWrite(SignalSubsystemHandle);
}

// [Server]
EStateTreeRunStatus FJY_MassSpawningTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const //override
{
	FJY_EnemyBehaviorFragment& Behavior = Context.GetExternalData(BehaviorHandle);
	Behavior.State = EJY_EnemyState::Spawning;

	return WaitForSpawningEnd(Context);
}

/* Mass StateTree는 깨워졌을 때만 Tick이 불린다 — 예약한 신호로 깨어났을 때 */
// [Server]
EStateTreeRunStatus FJY_MassSpawningTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const //override
{
	return WaitForSpawningEnd(Context);
}

// [Server]
EStateTreeRunStatus FJY_MassSpawningTask::WaitForSpawningEnd(FStateTreeExecutionContext& Context) const
{
	const FJY_EnemyBehaviorFragment& Behavior = Context.GetExternalData(BehaviorHandle);
	const UWorld* World = Context.GetWorld();
	const double RemainingSeconds = Behavior.SpawningEndSeconds - World->GetTimeSeconds();
	if (RemainingSeconds <= 0.0)
		return EStateTreeRunStatus::Succeeded;

	/* 다른 신호로 먼저 깨어났을 수도 있어 남은 시간으로 다시 예약한다 — 엔진 서기 Task와 같은 방식 (MassNavMeshStandTask.cpp:60) */
	FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	UMassSignalSubsystem& SignalSubsystem = Context.GetExternalData(SignalSubsystemHandle);
	const float WakeDelaySeconds = static_cast<float>(RemainingSeconds) + JY_MassSpawningTask::WakeDelayPaddingSeconds;
	JY_MassStateTreeWake::WakeEntityAfterDelay(MassStateTreeContext.GetMassEntityExecutionContext(), SignalSubsystem, MassStateTreeContext.GetEntity(), WakeDelaySeconds);

	return EStateTreeRunStatus::Running;
}
