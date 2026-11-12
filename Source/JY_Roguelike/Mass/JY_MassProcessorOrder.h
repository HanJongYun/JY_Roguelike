#pragma once

#include "CoreMinimal.h"

struct FMassProcessorExecutionOrder;

/*
 * 우리 Processor 실행 순서를 정하는 유일한 곳 — 엔진 그룹 순서는 그대로 두고 우리 것을 그 사이에 끼운다
 * Behavior → Tasks → (Steering) → Avoidance → ApplyForces → Movement → PositionCorrection → (장애물 그리드)
 */
namespace JY_MassProcessorOrder
{
	/* 엔진이 옮긴 위치를 고치는 단계 */
	extern const FName PositionCorrection;

	/* 상태를 정하는 단계에 넣는다 */
	void PlaceInBehavior(FMassProcessorExecutionOrder& ExecutionOrder);

	/* 갈 곳(MoveTarget)을 정하는 단계에 넣는다 */
	void PlaceInTasks(FMassProcessorExecutionOrder& ExecutionOrder);

	/* 엔진 이동 뒤 위치를 고치는 단계에 넣는다 */
	void PlaceInPositionCorrection(FMassProcessorExecutionOrder& ExecutionOrder);
}
