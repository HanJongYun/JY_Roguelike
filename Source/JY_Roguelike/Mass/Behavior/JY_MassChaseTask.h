#pragma once

#include "CoreMinimal.h"
#include "MassStateTreeTypes.h"
#include "JY_MassChaseTask.generated.h"

struct FJY_EnemyBehaviorFragment;
struct FMassMoveTargetFragment;
struct FMassMovementParameters;

/* StateTree는 Task마다 적별 데이터 타입을 요구한다 — 적마다 따로 들 값이 없어 비어 있다 (StateTreeCompiler.cpp:2108) */
USTRUCT()
struct FJY_MassChaseTaskInstanceData
{
	GENERATED_BODY()
};

/* 쫓기 — 들어올 때 이동을 켠다. 갈 곳은 UJY_MassFlowFieldProcessor가 매 프레임 정한다 */
USTRUCT(meta = (DisplayName = "JY Mass Chase"))
struct FJY_MassChaseTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FJY_MassChaseTaskInstanceData;

public:
	FJY_MassChaseTask();

protected:
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<FJY_EnemyBehaviorFragment> BehaviorHandle;
	TStateTreeExternalDataHandle<FMassMoveTargetFragment> MoveTargetHandle;
	TStateTreeExternalDataHandle<FMassMovementParameters> MovementParametersHandle;
};
