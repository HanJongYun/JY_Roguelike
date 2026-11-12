#include "Mass/Navigation/JY_EnemyFlowFieldProcessor.h"

#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "Mass/JY_MassProcessorOrder.h"
#include "Mass/JY_MassTypes.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_EnemyFlowFieldProcessor)

namespace JY_EnemyFlowField
{
	/* 문 위의 갈 곳에 이만큼 붙으면 문에 도착한 것으로 본다 — 0이면 방향이 0이 돼 멈춤(GetSafeNormal2D, Vector.h:656) */
	constexpr float PortalReachedTolerance = 0.1f;

	/* 문 끝에서 이만큼 안쪽 — NavMesh는 이미 몸 반경만큼 벽에서 떨어져 있어, 가장자리에 비비지 않을 여유만 */
	constexpr float PortalEdgeInset = 30.0f;
}

UJY_EnemyFlowFieldProcessor::UJY_EnemyFlowFieldProcessor()
	: EntityQuery(*this)
{
	/* 길은 서버가 정하고 클라는 위치만 받는다 */
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Server);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;

	JY_MassProcessorOrder::PlaceInTasks(ExecutionOrder);
}

void UJY_EnemyFlowFieldProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) //override
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJY_EnemyBehaviorFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJY_NavPolyFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FMassMovementParameters>(EMassFragmentPresence::All);
	EntityQuery.AddSubsystemRequirement<UJY_MassEnemySubsystem>(EMassFragmentAccess::ReadOnly);
}

/* 플로우 필드는 PrePhysics 시작 알림에서 게임 스레드가 다시 만든 뒤라, 여기서는 읽기만 한다 */
// [Server]
void UJY_EnemyFlowFieldProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) //override
{
	const UWorld* World = EntityManager.GetWorld();

	auto SetMoveTargets = [World](FMassExecutionContext& ChunkContext)
	{
		const UJY_MassEnemySubsystem& MassEnemySubsystem = ChunkContext.GetSubsystemChecked<UJY_MassEnemySubsystem>();
		const FJY_NavFlowField& FlowField = MassEnemySubsystem.GetFlowField();
		const FVector& PlayerLocation = MassEnemySubsystem.GetPlayerLocation();

		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FJY_EnemyBehaviorFragment> Behaviors = ChunkContext.GetFragmentView<FJY_EnemyBehaviorFragment>();
		const TConstArrayView<FJY_NavPolyFragment> NavPolys = ChunkContext.GetFragmentView<FJY_NavPolyFragment>();
		const TArrayView<FMassMoveTargetFragment> MoveTargets = ChunkContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const FMassMovementParameters& MovementParameters = ChunkContext.GetConstSharedFragment<FMassMovementParameters>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			if (Behaviors[EntityIt].State != EJY_EnemyState::Chase)
				continue;

			/* 목표에 이어지지 않은 다각형(다른 방·끊긴 바닥)이면 갈 곳을 바꾸지 않는다 */
			const NavNodeRef PolyRef = NavPolys[EntityIt].PolyRef;
			const FJY_NavFlowStep* FlowStep = FlowField.FindStep(PolyRef);
			if (FlowStep == nullptr)
				continue;

			/* 플레이어와 같은 다각형이면 사이에 벽이 없으니 바로 간다 */
			const FVector EnemyLocation = Transforms[EntityIt].GetTransform().GetLocation();
			const bool bInGoalPoly = PolyRef == FlowField.GetGoalPoly();
			/* 문 가운데가 아니라 플레이어 쪽 직선이 문을 지나는 점 — 트인 곳은 곧장, 기둥 앞은 모서리를 끼고 돈다 */
			FVector TargetLocation = bInGoalPoly ? PlayerLocation : FlowStep->GetPortalCrossing(EnemyLocation, PlayerLocation, JY_EnemyFlowField::PortalEdgeInset);

			/* 문 선 위에 서면 아직 이 다각형이라 갈 곳이 발밑이 된다 — 다음 다각형의 갈 곳으로 넘긴다. 일찍 넘기면 다음 갈 곳까지 직선이 모서리를 스친다 */
			if (bInGoalPoly == false && FVector::Dist2D(EnemyLocation, TargetLocation) < JY_EnemyFlowField::PortalReachedTolerance)
			{
				const FJY_NavFlowStep* NextStep = FlowField.FindStep(FlowStep->NextPoly);
				const bool bNextIsGoal = FlowStep->NextPoly == FlowField.GetGoalPoly();
				if (bNextIsGoal == true)
				{
					TargetLocation = PlayerLocation;
				}
				else if (NextStep != nullptr)
				{
					TargetLocation = NextStep->GetPortalCrossing(EnemyLocation, PlayerLocation, JY_EnemyFlowField::PortalEdgeInset);
				}
			}
			const float DistanceToTarget = FVector::Dist(EnemyLocation, TargetLocation);

			FMassMoveTargetFragment& MoveTarget = MoveTargets[EntityIt];
			if (MoveTarget.GetCurrentAction() != EMassMovementAction::Move)
			{
				MoveTarget.CreateNewAction(EMassMovementAction::Move, *World);
			}
			MoveTarget.Center = TargetLocation;
			MoveTarget.Forward = (TargetLocation - EnemyLocation).GetSafeNormal2D();
			MoveTarget.DistanceToGoal = bInGoalPoly ? DistanceToTarget : DistanceToTarget + FlowStep->DistanceToGoal;

			/* [임시코드] (2.5에서 StateTree 추격 Task가 채움) 엔진 Steering은 이 속도로 가고, 0이면 안 움직인다 (MassSteeringProcessors.cpp, MassNavMeshPathFollowTask.cpp:134) */
			MoveTarget.DesiredSpeed.Set(MovementParameters.DefaultDesiredSpeed);

			/* 남은 거리가 끝까지의 경로 거리라, 플레이어에 가까워질 때만 엔진이 감속해 선다 */
			MoveTarget.IntentAtGoal = EMassMovementAction::Stand;
		}
	};
	EntityQuery.ForEachEntityChunk(Context, SetMoveTargets);
}
