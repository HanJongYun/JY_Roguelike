#include "Mass/Navigation/JY_NavFlowField.h"

#include "NavMesh/RecastNavMesh.h"

namespace JY_NavFlowField
{
	/* 우선순위 큐 한 칸 — 목표까지 거리가 짧은 다각형부터 꺼낸다 */
	struct FOpenPoly
	{
		NavNodeRef PolyRef = INVALID_NAVNODEREF;
		float DistanceToGoal = 0.0f;
	};

	struct FCloserToGoal
	{
		bool operator()(const FOpenPoly& A, const FOpenPoly& B) const { return A.DistanceToGoal < B.DistanceToGoal; }
	};
}

/* 목표에서 거꾸로 퍼지는 Dijkstra — 다각형마다 "들어온 문"을 이동 지점으로 보고 문 사이 거리를 비용으로 쓴다 */
// [Server]
void FJY_NavFlowField::Rebuild(const ARecastNavMesh& NavMesh, NavNodeRef InGoalPoly, const FVector& GoalLocation)
{
	Reset();
	GoalPoly = InGoalPoly;
	if (GoalPoly == INVALID_NAVNODEREF)
		return;

	/* 다각형에 들어오는 지점 — 목표 다각형은 목표 위치, 나머지는 목표 쪽 문 가운데 */
	TMap<NavNodeRef, FVector> EntryLocationByPoly;
	EntryLocationByPoly.Add(GoalPoly, GoalLocation);
	StepByPoly.Add(GoalPoly, FJY_NavFlowStep());

	TArray<JY_NavFlowField::FOpenPoly> OpenPolys;
	OpenPolys.HeapPush({ GoalPoly, 0.0f }, JY_NavFlowField::FCloserToGoal());

	TSet<NavNodeRef> ClosedPolys;
	TArray<FNavigationPortalEdge> PortalEdges;
	while (OpenPolys.Num() > 0)
	{
		JY_NavFlowField::FOpenPoly Current;
		OpenPolys.HeapPop(OUT Current, JY_NavFlowField::FCloserToGoal());

		/* 더 짧은 거리로 이미 처리한 다각형이 큐에 남아 있던 것 */
		bool bAlreadyClosed = false;
		ClosedPolys.Add(Current.PolyRef, OUT &bAlreadyClosed);
		if (bAlreadyClosed == true)
			continue;

		const FVector CurrentEntryLocation = EntryLocationByPoly.FindChecked(Current.PolyRef);

		/* 이웃 다각형과 맞닿은 경계선 — 목표 쪽(Current)에서 본 문이지만 같은 선분이다 (RecastNavMesh.h:1378) */
		PortalEdges.Reset();
		NavMesh.GetPolyNeighbors(Current.PolyRef, OUT PortalEdges);
		for (const FNavigationPortalEdge& PortalEdge : PortalEdges)
		{
			const NavNodeRef NeighborPoly = PortalEdge.ToRef;
			if (ClosedPolys.Contains(NeighborPoly) == true)
				continue;

			const FVector PortalCenter = PortalEdge.GetMiddlePoint();
			const float NeighborDistance = Current.DistanceToGoal + FVector::Dist(CurrentEntryLocation, PortalCenter);

			FJY_NavFlowStep* ExistingStep = StepByPoly.Find(NeighborPoly);
			if (ExistingStep != nullptr && ExistingStep->DistanceToGoal <= NeighborDistance)
				continue;

			FJY_NavFlowStep& NeighborStep = StepByPoly.FindOrAdd(NeighborPoly);
			NeighborStep.NextPoly = Current.PolyRef;
			NeighborStep.PortalLeft = PortalEdge.Left;
			NeighborStep.PortalRight = PortalEdge.Right;
			NeighborStep.DistanceToGoal = NeighborDistance;
			EntryLocationByPoly.Add(NeighborPoly, PortalCenter);

			OpenPolys.HeapPush({ NeighborPoly, NeighborDistance }, JY_NavFlowField::FCloserToGoal());
		}
	}
}

/* 위에서 본 2D 교차 — 문 위치 비율 t(0 = Left, 1 = Right)를 구해 문 범위로 자른다. 기둥에 막히면 t가 범위를 벗어나 기둥 쪽 끝으로 간다 */
FVector FJY_NavFlowStep::GetPortalCrossing(const FVector& From, const FVector& To, float EdgeInset) const
{
	const FVector2D PortalStart(PortalLeft);
	const FVector2D PortalDelta = FVector2D(PortalRight) - PortalStart;
	const double PortalLength = PortalDelta.Size();
	if (PortalLength <= UE_KINDA_SMALL_NUMBER)
		return GetPortalCenter();

	const FVector2D LineStart(From);
	const FVector2D LineDelta = FVector2D(To) - LineStart;
	const double Denominator = FVector2D::CrossProduct(LineDelta, PortalDelta);

	/* 직선이 문과 나란하면 내 위치에서 문으로 내린 수선의 발 */
	double PortalRatio = 0.0;
	if (FMath::Abs(Denominator) <= UE_KINDA_SMALL_NUMBER)
	{
		PortalRatio = FVector2D::DotProduct(LineStart - PortalStart, PortalDelta) / (PortalLength * PortalLength);
	}
	else
	{
		PortalRatio = FVector2D::CrossProduct(PortalStart - LineStart, LineDelta) / Denominator;
	}

	const double InsetRatio = FMath::Min(static_cast<double>(EdgeInset), PortalLength * 0.5) / PortalLength;
	PortalRatio = FMath::Clamp(PortalRatio, InsetRatio, 1.0 - InsetRatio);

	return FMath::Lerp(PortalLeft, PortalRight, PortalRatio);
}

void FJY_NavFlowField::Reset()
{
	GoalPoly = INVALID_NAVNODEREF;
	StepByPoly.Reset();
}
