#pragma once

#include "CoreMinimal.h"
#include "MassStateTreeTypes.h"
#include "JY_MassDeadCondition.generated.h"

struct FJY_EnemyBehaviorFragment;
struct FJY_EnemyHealthFragment;

/* StateTree는 조건마다 적별 데이터 타입을 요구한다 — 적마다 따로 들 값이 없어 비어 있다 (StateTreeCompiler.cpp:2108) */
USTRUCT()
struct FJY_MassDeadConditionInstanceData
{
	GENERATED_BODY()
};

/* 체력이 다해 죽기 시작해야 하나 — 이미 죽는 중이면 아니다 */
USTRUCT(meta = (DisplayName = "JY Mass Dead"))
struct FJY_MassDeadCondition : public FMassStateTreeConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FJY_MassDeadConditionInstanceData;

protected:
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<FJY_EnemyBehaviorFragment> BehaviorHandle;
	TStateTreeExternalDataHandle<FJY_EnemyHealthFragment> HealthHandle;
};
