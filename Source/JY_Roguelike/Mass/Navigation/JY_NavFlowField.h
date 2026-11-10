#pragma once

#include "CoreMinimal.h"
#include "AI/Navigation/NavigationTypes.h"

class ARecastNavMesh;

/* 다각형 하나에서 목표 쪽으로 가는 다음 걸음 */
struct FJY_NavFlowStep
{
	/* 넘어갈 다음 다각형 (목표 다각형이면 INVALID_NAVNODEREF) */
	NavNodeRef NextPoly = INVALID_NAVNODEREF;

	/* 다음 다각형과 맞닿은 경계선(문) 양 끝 */
	FVector PortalLeft = FVector::ZeroVector;
	FVector PortalRight = FVector::ZeroVector;

	/* 이 다각형에 들어오는 지점에서 목표까지의 거리 */
	float DistanceToGoal = 0.0f;

	FVector GetPortalCenter() const { return (PortalLeft + PortalRight) * 0.5f; }

	/* From→To 직선이 문을 지나는 점 — 문 밖이면 가까운 끝에서 EdgeInset(문 길이 절반까지)만큼 안쪽 */
	FVector GetPortalCrossing(const FVector& From, const FVector& To, float EdgeInset) const;
};

/* 목표 다각형에서 NavMesh를 따라 퍼지며, 다각형마다 "목표로 가려면 다음은 여기" 표를 만든다 */
struct FJY_NavFlowField
{
public:
	/* 목표 다각형부터 이어진 다각형 전부의 다음 걸음을 다시 계산한다 */
	void Rebuild(const ARecastNavMesh& NavMesh, NavNodeRef InGoalPoly, const FVector& GoalLocation);

	void Reset();

	/* 이 다각형의 다음 걸음 (목표에 이어지지 않은 다각형이면 nullptr) */
	const FJY_NavFlowStep* FindStep(NavNodeRef PolyRef) const { return StepByPoly.Find(PolyRef); }

	NavNodeRef GetGoalPoly() const { return GoalPoly; }

	const TMap<NavNodeRef, FJY_NavFlowStep>& GetSteps() const { return StepByPoly; }

private:
	NavNodeRef GoalPoly = INVALID_NAVNODEREF;

	TMap<NavNodeRef, FJY_NavFlowStep> StepByPoly;
};
