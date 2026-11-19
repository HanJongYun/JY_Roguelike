#include "Mass/JY_MassProcessorOrder.h"

#include "MassCommonTypes.h"
#include "MassNavigationProcessors.h"
#include "MassProcessor.h"
#include "MassRepresentationTypes.h"

/* 같은 그룹 안의 순서는 보장되지 않는다 — 순서가 필요한 것끼리는 "JY.Movement.X"처럼 하위 그룹으로 나눈다 (MassProcessorDependencySolver.cpp:535) */
namespace JY_MassProcessorOrder
{
	/* 엔진 회피 뒤에 힘을 더하는 단계 */
	const FName AfterAvoidanceGroup = TEXT("JY.AfterAvoidance");

	/* 원하는 속도로 옮기고 NavMesh 위에 붙이는 단계 — 엔진 이동 대신 */
	const FName MovementGroup = TEXT("JY.Movement");
}

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

void JY_MassProcessorOrder::PlaceAfterAvoidance(FMassProcessorExecutionOrder& ExecutionOrder)
{
	ExecutionOrder.ExecuteInGroup = AfterAvoidanceGroup;

	/* 엔진 회피는 힘을 최대 가속도로 잘라 다시 써넣는다 (MassAvoidanceProcessors.cpp:1078) — 그 뒤에 더해야 잘리지 않는다 */
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Avoidance);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::ApplyForces);
}

void JY_MassProcessorOrder::PlaceInMovement(FMassProcessorExecutionOrder& ExecutionOrder)
{
	ExecutionOrder.ExecuteInGroup = MovementGroup;

	/* 원하는 속도는 엔진 ApplyForce가 만든다 (MassMovementProcessors.cpp:25) — 그리드는 엔진 Movement 뒤만 걸려 있어(MassNavigationProcessors.cpp:164) 우리 위치로 칸을 갱신하게 직접 앞에 둔다 */
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::ApplyForces);
	ExecutionOrder.ExecuteBefore.Add(UMassNavigationObstacleGridProcessor::StaticClass()->GetFName());
}

void JY_MassProcessorOrder::PlaceBeforeRepresentation(FMassProcessorExecutionOrder& ExecutionOrder)
{
	/* 엔진 그리기는 LODCollector → LOD → Representation 순서로 위치를 읽는다 (MassRepresentationProcessor.cpp:56) */
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::LODCollector);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Representation);

	/* 그리드도 옮긴 뒤의 위치를 적어야 한다 — 길막이 캡슐·공격 판정이 그리드를 읽는다 */
	ExecutionOrder.ExecuteBefore.Add(UMassNavigationObstacleGridProcessor::StaticClass()->GetFName());
}
