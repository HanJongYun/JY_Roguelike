#pragma once

#include "CoreMinimal.h"
#include "MassStateTreeTypes.h"
#include "JY_MassHitTask.generated.h"

class UMassSignalSubsystem;
struct FJY_EnemyBehaviorFragment;
struct FJY_EnemyKnockbackFragment;
struct FMassDesiredMovementFragment;
struct FMassMoveTargetFragment;
struct FTransformFragment;

/* StateTree는 Task마다 적별 데이터 타입을 요구한다 — 적마다 따로 들 값이 없어 비어 있다 (StateTreeCompiler.cpp:2108) */
USTRUCT()
struct FJY_MassHitTaskInstanceData
{
	GENERATED_BODY()
};

/* 피격 움찔 — 들어올 때 쫓기를 멈추고, 움찔이 끝나는 시각(FJY_EnemyBehaviorFragment::HitEndSeconds)이 지나고 밀림도 멈추면 끝난다 */
USTRUCT(meta = (DisplayName = "JY Mass Hit"))
struct FJY_MassHitTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FJY_MassHitTaskInstanceData;

protected:
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	/* 움찔도 밀림도 끝났으면 Succeeded, 아니면 Running — 움찔만 남았으면 끝나는 시각에 깨우도록 예약한다 */
	EStateTreeRunStatus WaitForHitEnd(FStateTreeExecutionContext& Context) const;

private:
	TStateTreeExternalDataHandle<FJY_EnemyBehaviorFragment> BehaviorHandle;
	TStateTreeExternalDataHandle<FJY_EnemyKnockbackFragment> KnockbackHandle;
	TStateTreeExternalDataHandle<FMassMoveTargetFragment> MoveTargetHandle;
	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
	TStateTreeExternalDataHandle<FMassDesiredMovementFragment> DesiredMovementHandle;
	TStateTreeExternalDataHandle<UMassSignalSubsystem> SignalSubsystemHandle;
};
