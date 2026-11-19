#include "Mass/Net/JY_MassNetInterpProcessor.h"

#include "Mass/JY_MassProcessorOrder.h"
#include "Mass/JY_MassTypes.h"
#include "Mass/EntityFragments.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassNetInterpProcessor)

UJY_MassNetInterpProcessor::UJY_MassNetInterpProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Client);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	JY_MassProcessorOrder::PlaceBeforeRepresentation(ExecutionOrder);
}

void UJY_MassNetInterpProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) //override
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FJY_EnemyNetInterpFragment>(EMassFragmentAccess::ReadWrite);
}

/* 엔진 CMC의 Linear 보간과 같은 방식 — 구간에서 지난 시간의 비율만큼 출발 → 도착을 채운다 (CharacterMovementComponent.cpp:8420) */
// [OwnClient]
void UJY_MassNetInterpProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) //override
{
	auto MoveAlongSegments = [](FMassExecutionContext& ChunkContext)
	{
		const float DeltaSeconds = ChunkContext.GetDeltaTimeSeconds();
		const TArrayView<FTransformFragment> Transforms = ChunkContext.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FJY_EnemyNetInterpFragment> Interps = ChunkContext.GetMutableFragmentView<FJY_EnemyNetInterpFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			/* 도착한 적은 다음 값을 받을 때까지 그 자리에 둔다 */
			FJY_EnemyNetInterpFragment& Interp = Interps[EntityIt];
			if (Interp.Elapsed >= Interp.Duration)
				continue;

			Interp.Elapsed = FMath::Min(Interp.Elapsed + DeltaSeconds, Interp.Duration);
			const float Alpha = Interp.Elapsed / Interp.Duration;

			FTransform& Transform = Transforms[EntityIt].GetMutableTransform();
			Transform.SetLocation(FMath::Lerp(Interp.StartLocation, Interp.TargetLocation, Alpha));
			Transform.SetRotation(FQuat::Slerp(Interp.StartRotation, Interp.TargetRotation, Alpha));
		}
	};
	EntityQuery.ForEachEntityChunk(Context, MoveAlongSegments);
}
