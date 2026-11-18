#pragma once

#include "CoreMinimal.h"
#include "MassStateTreeTypes.h"
#include "JY_MassInsideRangeCondition.generated.h"

struct FJY_EnemyBehaviorFragment;

/* StateTree는 조건마다 적별 데이터 타입을 요구한다 — 적마다 따로 들 값이 없어 비어 있다 (StateTreeCompiler.cpp:2108) */
USTRUCT()
struct FJY_MassInsideRangeConditionInstanceData
{
	GENERATED_BODY()
};

/* 플레이어가 이 적의 거리 안에 있나 — 값은 UJY_MassFlowFieldProcessor가 매 프레임 적어 둔 것을 읽기만 한다 */
USTRUCT(meta = (DisplayName = "JY Mass Inside Range"))
struct FJY_MassInsideRangeCondition : public FMassStateTreeConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FJY_MassInsideRangeConditionInstanceData;

protected:
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

public:
	/* 켜면 "거리 밖인가"를 본다 */
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bInvert = false;

private:
	TStateTreeExternalDataHandle<FJY_EnemyBehaviorFragment> BehaviorHandle;
};
