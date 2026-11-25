#include "Mass/Navigation/JY_MassFlowFieldProcessor.h"

#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "Mass/JY_MassProcessorOrder.h"
#include "Mass/JY_MassStateTreeWake.h"
#include "Mass/JY_MassTypes.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassSignalSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassFlowFieldProcessor)

namespace JY_MassFlowField
{
	/*
	 * 문 위의 갈 곳에 이만큼 붙으면 문에 도착한 것으로 보고 다음 다각형을 기준으로 바꾼다.
	 * 여러 마리가 같은 문으로 몰리면 서로 밀려 갈 곳에 딱 닿지 못한다 — 너무 작으면(예전 0.1cm) 문 앞에서 맴돌며 못 넘어간다.
	 * 문 끝 여유(PortalEdgeInset)와 같게 둬, 일찍 바꿔도 다음 갈 곳까지 직선이 기둥 모서리를 넘지 않게
	 */
	constexpr float PortalReachedTolerance = 30.0f;

	/* 문 끝에서 이만큼 안쪽 — NavMesh는 이미 몸 반경만큼 벽에서 떨어져 있어, 가장자리에 비비지 않을 여유만 */
	constexpr float PortalEdgeInset = 30.0f;

	/* 그 다각형에 있는 적이 갈 곳 — 플레이어 다각형이면 사이에 벽이 없으니 플레이어, 아니면 플레이어 쪽 직선이 문을 지나는 점(트인 곳은 곧장, 기둥 앞은 모서리를 끼고 돈다) */
	FVector GetTargetLocation(const FJY_NavFlowField& FlowField, NavNodeRef Poly, const FJY_NavFlowStep& FlowStep, const FVector& EnemyLocation, const FVector& PlayerNavLocation)
	{
		if (Poly == FlowField.GetGoalPoly())
			return PlayerNavLocation;

		return FlowStep.GetPortalCrossing(EnemyLocation, PlayerNavLocation, PortalEdgeInset);
	}
}

UJY_MassFlowFieldProcessor::UJY_MassFlowFieldProcessor()
	: EntityQuery(*this)
{
	/* 길은 서버가 정하고 클라는 위치만 받는다 */
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Server);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;

	JY_MassProcessorOrder::PlaceInTasks(ExecutionOrder);
}

void UJY_MassFlowFieldProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) //override
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJY_EnemyBehaviorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FJY_NavPolyFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FJY_EnemyRangeParams>(EMassFragmentPresence::All);
	EntityQuery.AddSubsystemRequirement<UJY_MassEnemySubsystem>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);
}

/* 플로우 필드는 PrePhysics 시작 알림에서 게임 스레드가 다시 만든 뒤라, 여기서는 읽기만 한다 */
// [Server]
void UJY_MassFlowFieldProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) //override
{
	auto SetMoveTargets = [](FMassExecutionContext& ChunkContext)
	{
		const UJY_MassEnemySubsystem& MassEnemySubsystem = ChunkContext.GetSubsystemChecked<UJY_MassEnemySubsystem>();
		const FJY_NavFlowField& FlowField = MassEnemySubsystem.GetFlowField();
		const FVector& PlayerNavLocation = MassEnemySubsystem.GetPlayerNavLocation();

		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TArrayView<FJY_EnemyBehaviorFragment> Behaviors = ChunkContext.GetMutableFragmentView<FJY_EnemyBehaviorFragment>();
		const TConstArrayView<FJY_NavPolyFragment> NavPolys = ChunkContext.GetFragmentView<FJY_NavPolyFragment>();
		const TArrayView<FMassMoveTargetFragment> MoveTargets = ChunkContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const FJY_EnemyRangeParams& RangeParams = ChunkContext.GetConstSharedFragment<FJY_EnemyRangeParams>();

		/* 거리 안팎이 바뀐 적 — 무엇을 할지는 적마다의 StateTree가 정한다 */
		TArray<FMassEntityHandle, TInlineAllocator<16>> EntitiesToWake;

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			/* 목표에 이어지지 않은 다각형(다른 방·끊긴 바닥)이면 갈 곳을 바꾸지 않는다 */
			const NavNodeRef PolyRef = NavPolys[EntityIt].PolyRef;
			const FJY_NavFlowStep* FlowStep = FlowField.FindStep(PolyRef);
			if (FlowStep == nullptr)
				continue;

			/* 갈 곳과 남은 거리는 같은 다각형(Guide)에서 꺼낸다 — 평소엔 내가 서 있는 다각형 */
			const FVector EnemyLocation = Transforms[EntityIt].GetTransform().GetLocation();
			NavNodeRef GuidePoly = PolyRef;
			const FJY_NavFlowStep* GuideStep = FlowStep;
			FVector TargetLocation = JY_MassFlowField::GetTargetLocation(FlowField, GuidePoly, *GuideStep, EnemyLocation, PlayerNavLocation);

			/* 문 위 갈 곳에 거의 닿았으면(PortalReachedTolerance) 아직 이 다각형이어도 다음 다각형을 기준으로 바꾼다 — 안 바꾸면 갈 곳이 발밑이 돼 문 앞에서 맴돈다 */
			if (GuidePoly != FlowField.GetGoalPoly() && FVector::Dist2D(EnemyLocation, TargetLocation) < JY_MassFlowField::PortalReachedTolerance)
			{
				/* NextPoly는 Rebuild가 표에 먼저 넣은 다각형이라 항상 있다 (JY_NavFlowField.cpp:69) */
				GuidePoly = FlowStep->NextPoly;
				GuideStep = FlowField.FindStep(GuidePoly);
				TargetLocation = JY_MassFlowField::GetTargetLocation(FlowField, GuidePoly, *GuideStep, EnemyLocation, PlayerNavLocation);
			}
			const float DistanceToTarget = FVector::Dist(EnemyLocation, TargetLocation);

			/* 플레이어 다각형의 DistanceToGoal은 0이라 따로 가르지 않는다 (JY_NavFlowField.cpp:32) */
			const float DistanceToPlayer = DistanceToTarget + GuideStep->DistanceToGoal;

			/* 거리는 쫓지 않는 적도 잰다 — 서 있는 적이 "플레이어가 멀어졌다"를 알아야 다시 쫓는다. 나가는 선을 더 멀리 둬 경계에서 안팎이 계속 바뀌지 않게 */
			FJY_EnemyBehaviorFragment& Behavior = Behaviors[EntityIt];
			const float RangeDistance = Behavior.bInsideRange == true ? RangeParams.StopDistance + RangeParams.ResumeMargin : RangeParams.StopDistance;
			const bool bInsideRange = DistanceToPlayer <= RangeDistance;
			if (bInsideRange != Behavior.bInsideRange)
			{
				Behavior.bInsideRange = bInsideRange;
				EntitiesToWake.Add(ChunkContext.GetEntity(EntityIt));
			}

			/* 갈 곳은 쫓는 적만 바꾼다 — 이동을 켜고 끄는 것은 StateTree Task */
			if (Behavior.State != EJY_EnemyState::Chase)
				continue;

			FMassMoveTargetFragment& MoveTarget = MoveTargets[EntityIt];
			MoveTarget.Center = TargetLocation;
			MoveTarget.Forward = (TargetLocation - EnemyLocation).GetSafeNormal2D();
			MoveTarget.DistanceToGoal = DistanceToPlayer;
		}

		UMassSignalSubsystem& SignalSubsystem = ChunkContext.GetMutableSubsystemChecked<UMassSignalSubsystem>();
		JY_MassStateTreeWake::WakeEntities(ChunkContext, SignalSubsystem, EntitiesToWake);
	};
	EntityQuery.ForEachEntityChunk(Context, SetMoveTargets);
}
