#include "Mass/JY_MassProcessorOrder.h"

#include "MassCommonTypes.h"
#include "MassNavigationProcessors.h"
#include "MassProcessor.h"

/* 같은 그룹 안의 순서는 보장되지 않는다 — 순서가 필요한 것끼리는 "JY.PositionCorrection.X"처럼 하위 그룹으로 나눈다 (MassProcessorDependencySolver.cpp:535) */
const FName JY_MassProcessorOrder::PositionCorrection = TEXT("JY.PositionCorrection");

void JY_MassProcessorOrder::PlaceInBehavior(FMassProcessorExecutionOrder& ExecutionOrder)
{
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Behavior;

	/* Behavior → Tasks는 엔진 StateTree Processor만 선언한다 (MassStateTreeProcessors.cpp:264) — 그게 등록 안 돼도 순서가 남게 우리도 건다 */
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Tasks);
}

void JY_MassProcessorOrder::PlaceInTasks(FMassProcessorExecutionOrder& ExecutionOrder)
{
	/* 엔진 Steering이 Tasks 뒤로 걸려 있어 따로 걸지 않는다 (MassSteeringProcessors.cpp:62) */
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks;
}

void JY_MassProcessorOrder::PlaceInPositionCorrection(FMassProcessorExecutionOrder& ExecutionOrder)
{
	ExecutionOrder.ExecuteInGroup = PositionCorrection;

	/* 그리드는 Movement 뒤만 걸려 있다 (MassNavigationProcessors.cpp:164) — 고친 위치로 칸을 갱신하게 그 앞에 둔다 */
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
	ExecutionOrder.ExecuteBefore.Add(UMassNavigationObstacleGridProcessor::StaticClass()->GetFName());
}
