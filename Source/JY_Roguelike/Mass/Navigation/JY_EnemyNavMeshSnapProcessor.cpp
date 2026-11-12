#include "Mass/Navigation/JY_EnemyNavMeshSnapProcessor.h"

#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "Mass/JY_MassProcessorOrder.h"
#include "Mass/JY_MassTypes.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "NavMesh/RecastNavMesh.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_EnemyNavMeshSnapProcessor)

UJY_EnemyNavMeshSnapProcessor::UJY_EnemyNavMeshSnapProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Server);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;

	JY_MassProcessorOrder::PlaceInPositionCorrection(ExecutionOrder);
}

void UJY_EnemyNavMeshSnapProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) //override
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FJY_NavPolyFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<UJY_MassEnemySubsystem>(EMassFragmentAccess::ReadOnly);
}

/* 지난 프레임 NavMesh 위 위치 → 엔진이 옮긴 위치를 NavMesh 표면을 따라 잇는다. 방 NavMesh 붙이기·떼기는 프레임 끝이라 여기와 안 겹친다 (LevelTick.cpp:1865) */
// [Server]
void UJY_EnemyNavMeshSnapProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) //override
{
	const float DeltaSeconds = Context.GetDeltaTimeSeconds();

	auto SnapToNavMesh = [DeltaSeconds](FMassExecutionContext& ChunkContext)
	{
		const UJY_MassEnemySubsystem& MassEnemySubsystem = ChunkContext.GetSubsystemChecked<UJY_MassEnemySubsystem>();
		const ARecastNavMesh* NavMesh = MassEnemySubsystem.GetNavMesh();
		if (NavMesh == nullptr)
			return;

		const TArrayView<FTransformFragment> Transforms = ChunkContext.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FJY_NavPolyFragment> NavPolys = ChunkContext.GetMutableFragmentView<FJY_NavPolyFragment>();
		const TArrayView<FMassVelocityFragment> Velocities = ChunkContext.GetMutableFragmentView<FMassVelocityFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FJY_NavPolyFragment& NavPoly = NavPolys[EntityIt];
			if (NavPoly.PolyRef == INVALID_NAVNODEREF)
				continue;

			FTransform& Transform = Transforms[EntityIt].GetMutableTransform();
			const FVector MovedLocation = Transform.GetLocation();
			const FNavLocation StartLocation(NavPoly.Location, NavPoly.PolyRef);

			/* 벽에 막히면 벽을 따라 미끄러지고, Z는 그 자리 NavMesh 높이 (PImplRecastNavMesh.cpp FindMoveAlongSurface: moveAlongSurface + getPolyHeight) */
			FNavLocation SnappedLocation;
			if (NavMesh->FindMoveAlongSurface(StartLocation, MovedLocation, OUT SnappedLocation) == false)
			{
				SnappedLocation = StartLocation;
			}

			Transform.SetLocation(SnappedLocation.Location);

			/* 실제 속도 = 실제로 움직인 양 — 엔진은 원하는 속도를 그대로 적어 두므로, 벽에 막힌 만큼 여기서 고친다. 엔진 회피가 이 값을 읽는다 (MassAvoidanceProcessors.cpp:401) */
			if (DeltaSeconds > 0.0f)
			{
				Velocities[EntityIt].Value = (SnappedLocation.Location - NavPoly.Location) / DeltaSeconds;
			}

			NavPoly.Location = SnappedLocation.Location;
			NavPoly.PolyRef = SnappedLocation.NodeRef;
		}
	};
	EntityQuery.ForEachEntityChunk(Context, SnapToNavMesh);
}
