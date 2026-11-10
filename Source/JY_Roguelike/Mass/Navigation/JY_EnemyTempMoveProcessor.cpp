#include "Mass/Navigation/JY_EnemyTempMoveProcessor.h"

#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "Mass/JY_MassFragments.h"
#include "Mass/Navigation/JY_EnemyFlowFieldProcessor.h"
#include "Mass/Navigation/JY_NavigationFragments.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassNavigationFragments.h"
#include "NavMesh/RecastNavMesh.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_EnemyTempMoveProcessor)

namespace JY_EnemyTempMove
{
	constexpr float MoveSpeed = 350.0f;

	/* 플레이어와 겹치지 않게 이 거리 안이면 멈춘다 */
	constexpr float StopDistance = 120.0f;
}

UJY_EnemyTempMoveProcessor::UJY_EnemyTempMoveProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Server);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteAfter.Add(UJY_EnemyFlowFieldProcessor::StaticClass()->GetFName());

	/* NavMesh(UObject) 질의를 하므로 게임 스레드에서 */
	bRequiresGameThreadExecution = true;
}

void UJY_EnemyTempMoveProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) //override
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FJY_EnemyBehaviorFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJY_NavPolyFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadOnly);
}

/* 임시(2장 B에서 삭제): 바닥 높이·벽은 FindMoveAlongSurface가 NavMesh 위로 맞춘다 (PImplRecastNavMesh.cpp:1967) */
// [Server]
void UJY_EnemyTempMoveProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) //override
{
	const UWorld* World = EntityManager.GetWorld();
	const UJY_MassEnemySubsystem* MassEnemySubsystem = World->GetSubsystem<UJY_MassEnemySubsystem>();
	const ARecastNavMesh* NavMesh = MassEnemySubsystem->GetNavMesh();
	if (NavMesh == nullptr)
		return;

	const FVector PlayerLocation = MassEnemySubsystem->GetPlayerLocation();
	const float DeltaSeconds = Context.GetDeltaTimeSeconds();

	auto MoveEnemies = [NavMesh, PlayerLocation, DeltaSeconds](FMassExecutionContext& ChunkContext)
	{
		const TArrayView<FTransformFragment> Transforms = ChunkContext.GetMutableFragmentView<FTransformFragment>();
		const TConstArrayView<FJY_EnemyBehaviorFragment> Behaviors = ChunkContext.GetFragmentView<FJY_EnemyBehaviorFragment>();
		const TArrayView<FJY_NavPolyFragment> NavPolys = ChunkContext.GetMutableFragmentView<FJY_NavPolyFragment>();
		const TConstArrayView<FMassMoveTargetFragment> MoveTargets = ChunkContext.GetFragmentView<FMassMoveTargetFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const FMassMoveTargetFragment& MoveTarget = MoveTargets[EntityIt];
			if (Behaviors[EntityIt].State != EJY_EnemyState::Chase || MoveTarget.GetCurrentAction() != EMassMovementAction::Move)
				continue;

			FTransform& Transform = Transforms[EntityIt].GetMutableTransform();
			const FVector EnemyLocation = Transform.GetLocation();
			if (FVector::Dist2D(EnemyLocation, PlayerLocation) <= JY_EnemyTempMove::StopDistance)
				continue;

			const FVector MoveDirection = (MoveTarget.Center - EnemyLocation).GetSafeNormal2D();
			const FVector DesiredLocation = EnemyLocation + MoveDirection * JY_EnemyTempMove::MoveSpeed * DeltaSeconds;

			FJY_NavPolyFragment& NavPoly = NavPolys[EntityIt];
			const FNavLocation StartLocation(EnemyLocation, NavPoly.PolyRef);
			FNavLocation ResultLocation;
			if (NavMesh->FindMoveAlongSurface(StartLocation, DesiredLocation, OUT ResultLocation) == false)
				continue;

			Transform.SetLocation(ResultLocation.Location);
			NavPoly.PolyRef = ResultLocation.NodeRef;
			if (MoveDirection.IsNearlyZero() == false)
			{
				Transform.SetRotation(MoveDirection.ToOrientationQuat());
			}
		}
	};
	EntityQuery.ForEachEntityChunk(Context, MoveEnemies);
}
