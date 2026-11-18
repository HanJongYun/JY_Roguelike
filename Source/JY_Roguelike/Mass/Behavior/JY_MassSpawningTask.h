#pragma once

#include "CoreMinimal.h"
#include "MassStateTreeTypes.h"
#include "JY_MassSpawningTask.generated.h"

class UMassSignalSubsystem;
struct FJY_EnemyBehaviorFragment;

/* StateTree는 Task마다 적별 데이터 타입을 요구한다 — 적마다 따로 들 값이 없어 비어 있다 (StateTreeCompiler.cpp:2108) */
USTRUCT()
struct FJY_MassSpawningTaskInstanceData
{
	GENERATED_BODY()
};

/* 등장 예고 — 웨이브가 정한 시각(FJY_EnemyBehaviorFragment::SpawningEndSeconds)까지 기다렸다가 끝난다 */
USTRUCT(meta = (DisplayName = "JY Mass Spawning"))
struct FJY_MassSpawningTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FJY_MassSpawningTaskInstanceData;

protected:
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	/* 예고가 끝났으면 Succeeded, 아니면 끝나는 시각에 깨우도록 예약하고 Running */
	EStateTreeRunStatus WaitForSpawningEnd(FStateTreeExecutionContext& Context) const;

private:
	TStateTreeExternalDataHandle<FJY_EnemyBehaviorFragment> BehaviorHandle;
	TStateTreeExternalDataHandle<UMassSignalSubsystem> SignalSubsystemHandle;
};
