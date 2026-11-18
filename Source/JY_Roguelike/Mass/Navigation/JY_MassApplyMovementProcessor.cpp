#include "Mass/Navigation/JY_MassApplyMovementProcessor.h"

#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "Mass/JY_MassProcessorOrder.h"
#include "Mass/JY_MassTypes.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "NavMesh/RecastNavMesh.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassApplyMovementProcessor)

UJY_MassApplyMovementProcessor::UJY_MassApplyMovementProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Server);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;

	JY_MassProcessorOrder::PlaceInMovement(ExecutionOrder);
}

void UJY_MassApplyMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) //override
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FJY_NavPolyFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddSubsystemRequirement<UJY_MassEnemySubsystem>(EMassFragmentAccess::ReadOnly);

	/* 엔진 이동은 이 태그가 붙은 적을 건너뛴다 (MassMovementProcessors.cpp:86) — 위치는 여기서만 한 번 쓴다 */
	EntityQuery.AddTagRequirement<FMassCustomMovementTag>(EMassFragmentPresence::All);
}

/* 엔진 이동(위치 += 원하는 속도 × 시간)과 NavMesh 붙이기를 한 바퀴에 — 방 NavMesh 붙이기·떼기는 프레임 끝이라 여기와 안 겹친다 (LevelTick.cpp:1865) */
// [Server]
void UJY_MassApplyMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) //override
{
	/* 첫 프레임처럼 시간이 크게 튈 때 멀리 튀어 나가지 않게 — 엔진 이동과 같은 상한 (MassMovementProcessors.cpp:99) */
	const float DeltaSeconds = FMath::Min(0.1f, Context.GetDeltaTimeSeconds());
	if (DeltaSeconds <= 0.0f)
		return;

	auto MoveOnNavMesh = [DeltaSeconds](FMassExecutionContext& ChunkContext)
	{
		const UJY_MassEnemySubsystem& MassEnemySubsystem = ChunkContext.GetSubsystemChecked<UJY_MassEnemySubsystem>();
		const ARecastNavMesh* NavMesh = MassEnemySubsystem.GetNavMesh();
		if (NavMesh == nullptr)
			return;

		const TArrayView<FTransformFragment> Transforms = ChunkContext.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FJY_NavPolyFragment> NavPolys = ChunkContext.GetMutableFragmentView<FJY_NavPolyFragment>();
		const TArrayView<FMassVelocityFragment> Velocities = ChunkContext.GetMutableFragmentView<FMassVelocityFragment>();
		const TConstArrayView<FMassDesiredMovementFragment> DesiredMovements = ChunkContext.GetFragmentView<FMassDesiredMovementFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FJY_NavPolyFragment& NavPoly = NavPolys[EntityIt];
			if (NavPoly.PolyRef == INVALID_NAVNODEREF)
				continue;

			/* 안 움직이는 적(등장 예고 중·서 있는 적)은 NavMesh 검색을 건너뛴다 — 출발점과 도착점이 같아 결과가 같다 */
			const FVector& DesiredVelocity = DesiredMovements[EntityIt].DesiredVelocity;
			if (DesiredVelocity.IsNearlyZero() == true)
			{
				Velocities[EntityIt].Value = FVector::ZeroVector;
				continue;
			}

			/* 위치를 옮기는 곳은 여기뿐이라 Transform은 항상 PolyRef 다각형 위에 있다 */
			FTransform& Transform = Transforms[EntityIt].GetMutableTransform();
			const FNavLocation StartLocation(Transform.GetLocation(), NavPoly.PolyRef);
			const FVector MovedLocation = StartLocation.Location + DesiredVelocity * DeltaSeconds;

			/* 벽에 막히면 벽을 따라 미끄러지고, Z는 그 자리 NavMesh 높이 (PImplRecastNavMesh.cpp FindMoveAlongSurface: moveAlongSurface + getPolyHeight) */
			FNavLocation SnappedLocation;
			if (NavMesh->FindMoveAlongSurface(StartLocation, MovedLocation, OUT SnappedLocation) == false)
			{
				SnappedLocation = StartLocation;
			}

			/* 실제 속도 = 실제로 움직인 양 — 벽에 막힌 만큼 원하는 속도보다 작다. 3장에서 넣을 엔진 회피가 읽을 값 (MassAvoidanceProcessors.cpp:401), 지금은 읽는 곳 없음 */
			Velocities[EntityIt].Value = (SnappedLocation.Location - StartLocation.Location) / DeltaSeconds;

			Transform.SetLocation(SnappedLocation.Location);
			NavPoly.PolyRef = SnappedLocation.NodeRef;
		}
	};
	EntityQuery.ForEachEntityChunk(Context, MoveOnNavMesh);
}
