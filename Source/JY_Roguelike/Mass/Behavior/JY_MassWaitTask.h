#pragma once

#include "CoreMinimal.h"
#include "MassStateTreeTypes.h"
#include "JY_MassWaitTask.generated.h"

struct FJY_EnemyBehaviorFragment;
struct FMassMoveTargetFragment;
struct FTransformFragment;

/* StateTree는 Task마다 적별 데이터 타입을 요구한다 — 적마다 따로 들 값이 없어 비어 있다 (StateTreeCompiler.cpp:2108) */
USTRUCT()
struct FJY_MassWaitTaskInstanceData
{
	GENERATED_BODY()
};

/* 차례 대기 — 들어올 때 지금 자리에 선다 */
USTRUCT(meta = (DisplayName = "JY Mass Wait"))
struct FJY_MassWaitTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FJY_MassWaitTaskInstanceData;

public:
	FJY_MassWaitTask();

protected:
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<FJY_EnemyBehaviorFragment> BehaviorHandle;
	TStateTreeExternalDataHandle<FMassMoveTargetFragment> MoveTargetHandle;
	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
};
