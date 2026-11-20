#include "Mass/Navigation/JY_MassApplyMovementProcessor.h"

#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "Mass/JY_MassProcessorOrder.h"
#include "Mass/JY_MassStateTreeWake.h"
#include "Mass/JY_MassTypes.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassSignalSubsystem.h"
#include "NavMesh/RecastNavMesh.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassApplyMovementProcessor)

namespace JY_MassApplyMovement
{
	/*
	 * 넉백 한 프레임 — 수평은 바닥 점을 NavMesh 위로 옮기고(벽을 못 뚫음), 높이는 중력으로 따로 계산해 바닥 높이에 더한다.
	 * 공중에서는 수평 속도를 그대로 두고, 바닥에 붙으면 감속해 멈춘다. 멈췄으면 true
	 */
	// [Server]
	bool MoveKnockback(const ARecastNavMesh& NavMesh, float DeltaSeconds, float GravityZ, FTransform& Transform, FJY_NavPolyFragment& NavPoly, FJY_EnemyKnockbackFragment& Knockback, FMassVelocityFragment& Velocity)
	{
		/* Transform의 Z는 바닥 높이 + 뜬 높이라, 뜬 높이를 빼야 다각형 위의 점이 된다 */
		const FVector CurrentLocation = Transform.GetLocation();
		const FVector GroundLocation(CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z - Knockback.Height);
		const FNavLocation StartLocation(GroundLocation, NavPoly.PolyRef);
		const FVector HorizontalVelocity(Knockback.Velocity.X, Knockback.Velocity.Y, 0.0f);
		const FVector MovedLocation = GroundLocation + HorizontalVelocity * DeltaSeconds;

		FNavLocation SnappedLocation;
		if (NavMesh.FindMoveAlongSurface(StartLocation, MovedLocation, OUT SnappedLocation) == false)
		{
			SnappedLocation = StartLocation;
		}

		/* 벽에 막혀 못 간 만큼 수평 속도를 잃는다 — 안 그러면 벽에 붙은 채 남은 속도로 계속 미끄러진다 */
		const FVector MovedGroundDelta = SnappedLocation.Location - GroundLocation;
		Knockback.Velocity.X = MovedGroundDelta.X / DeltaSeconds;
		Knockback.Velocity.Y = MovedGroundDelta.Y / DeltaSeconds;

		Knockback.Velocity.Z += GravityZ * DeltaSeconds;
		Knockback.Height += Knockback.Velocity.Z * DeltaSeconds;

		/* 내려오는 중에 바닥에 닿아야 바닥 — 위로 띄우는 힘 없이 맞으면 첫 프레임부터 바닥이다 */
		bool bStopped = false;
		const bool bOnGround = Knockback.Height <= 0.0f && Knockback.Velocity.Z <= 0.0f;
		if (bOnGround == true)
		{
			Knockback.Height = 0.0f;
			Knockback.Velocity.Z = 0.0f;

			/* 바닥 마찰 — 방향은 그대로 두고 빠르기만 줄인다. 벽에 정면으로 막혀 속도를 다 잃은 적도 여기서 멈춘다 */
			const float GroundSpeed = Knockback.Velocity.Size2D();
			const float SlowedSpeed = GroundSpeed - Knockback.GroundDeceleration * DeltaSeconds;
			if (SlowedSpeed <= 0.0f)
			{
				Knockback.Velocity = FVector::ZeroVector;
				Knockback.bActive = false;
				bStopped = true;
			}
			else
			{
				const float SpeedScale = SlowedSpeed / GroundSpeed;
				Knockback.Velocity.X *= SpeedScale;
				Knockback.Velocity.Y *= SpeedScale;
			}
		}

		const FVector NewLocation = SnappedLocation.Location + FVector(0.0f, 0.0f, Knockback.Height);
		Velocity.Value = (NewLocation - CurrentLocation) / DeltaSeconds;
		Transform.SetLocation(NewLocation);
		NavPoly.PolyRef = SnappedLocation.NodeRef;

		return bStopped;
	}
}

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
	EntityQuery.AddRequirement<FJY_EnemyBehaviorFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJY_EnemyKnockbackFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<UJY_MassEnemySubsystem>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);

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
		const TConstArrayView<FJY_EnemyBehaviorFragment> Behaviors = ChunkContext.GetFragmentView<FJY_EnemyBehaviorFragment>();
		const TArrayView<FJY_EnemyKnockbackFragment> Knockbacks = ChunkContext.GetMutableFragmentView<FJY_EnemyKnockbackFragment>();
		const float GravityZ = MassEnemySubsystem.GetGravityZ();

		/* 넉백이 끝나 멈춘 적 — 다음에 쫓을지 설지는 적마다의 StateTree가 정한다 */
		TArray<FMassEntityHandle, TInlineAllocator<16>> StoppedEntities;

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FJY_NavPolyFragment& NavPoly = NavPolys[EntityIt];
			if (NavPoly.PolyRef == INVALID_NAVNODEREF)
				continue;

			/* 넉백 중인 적은 원하는 속도 대신 넉백 속도로 따로 옮긴다 */
			if (Behaviors[EntityIt].State == EJY_EnemyState::Knockback)
			{
				/* 멈춘 뒤 StateTree가 상태를 바꿀 때까지의 한 프레임 — 그 자리에 둔다 */
				if (Knockbacks[EntityIt].bActive == false)
				{
					Velocities[EntityIt].Value = FVector::ZeroVector;
					continue;
				}

				FTransform& KnockbackTransform = Transforms[EntityIt].GetMutableTransform();
				const bool bStopped = JY_MassApplyMovement::MoveKnockback(*NavMesh, DeltaSeconds, GravityZ, KnockbackTransform, NavPoly, Knockbacks[EntityIt], Velocities[EntityIt]);
				if (bStopped == true)
				{
					StoppedEntities.Add(ChunkContext.GetEntity(EntityIt));
				}
				continue;
			}

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

		UMassSignalSubsystem& SignalSubsystem = ChunkContext.GetMutableSubsystemChecked<UMassSignalSubsystem>();
		JY_MassStateTreeWake::WakeEntities(ChunkContext, SignalSubsystem, StoppedEntities);
	};
	EntityQuery.ForEachEntityChunk(Context, MoveOnNavMesh);
}
