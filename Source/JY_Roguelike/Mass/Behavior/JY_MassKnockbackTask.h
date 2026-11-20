#pragma once

#include "CoreMinimal.h"
#include "MassStateTreeTypes.h"
#include "JY_MassKnockbackTask.generated.h"

struct FJY_EnemyBehaviorFragment;
struct FJY_EnemyKnockbackFragment;
struct FMassDesiredMovementFragment;
struct FMassMoveTargetFragment;
struct FTransformFragment;

/* StateTree는 Task마다 적별 데이터 타입을 요구한다 — 적마다 따로 들 값이 없어 비어 있다 (StateTreeCompiler.cpp:2108) */
USTRUCT()
struct FJY_MassKnockbackTaskInstanceData
{
	GENERATED_BODY()
};

/* 넉백 — 들어올 때 쫓기를 멈추고, 착지(FJY_EnemyKnockbackFragment::bActive가 꺼짐)하면 끝난다 */
USTRUCT(meta = (DisplayName = "JY Mass Knockback"))
struct FJY_MassKnockbackTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FJY_MassKnockbackTaskInstanceData;

protected:
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	TStateTreeExternalDataHandle<FJY_EnemyBehaviorFragment> BehaviorHandle;
	TStateTreeExternalDataHandle<FJY_EnemyKnockbackFragment> KnockbackHandle;
	TStateTreeExternalDataHandle<FMassMoveTargetFragment> MoveTargetHandle;
	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
	TStateTreeExternalDataHandle<FMassDesiredMovementFragment> DesiredMovementHandle;
};
