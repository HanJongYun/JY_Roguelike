#pragma once

#include "CoreMinimal.h"
#include "MassStateTreeTypes.h"
#include "JY_MassDyingTask.generated.h"

struct FJY_EnemyBehaviorFragment;
struct FJY_EnemyKnockbackFragment;
struct FMassDesiredMovementFragment;
struct FMassMoveTargetFragment;
struct FTransformFragment;

/* StateTree는 Task마다 적별 데이터 타입을 요구한다 — 적마다 따로 들 값이 없어 비어 있다 (StateTreeCompiler.cpp:2108) */
USTRUCT()
struct FJY_MassDyingTaskInstanceData
{
	GENERATED_BODY()
};

/* 죽는 중 — 들어올 때 그 자리 바닥에 멈춘다. 끝나지 않는다(죽는 모션이 끝나면 UJY_MassDamageProcessor가 적을 지운다) */
USTRUCT(meta = (DisplayName = "JY Mass Dying"))
struct FJY_MassDyingTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FJY_MassDyingTaskInstanceData;

	FJY_MassDyingTask();

protected:
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<FJY_EnemyBehaviorFragment> BehaviorHandle;
	TStateTreeExternalDataHandle<FJY_EnemyKnockbackFragment> KnockbackHandle;
	TStateTreeExternalDataHandle<FMassMoveTargetFragment> MoveTargetHandle;
	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
	TStateTreeExternalDataHandle<FMassDesiredMovementFragment> DesiredMovementHandle;
};
