#include "Mass/Net/JY_MassDebugTextProcessor.h"

#include "Mass/JY_MassTypes.h"
#include "Mass/EntityFragments.h"
#include "DrawDebugHelpers.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassDebugTextProcessor)

namespace JY_MassDebugText
{
	const FVector TextOffset(0.0, 0.0, 120.0);
}

UJY_MassDebugTextProcessor::UJY_MassDebugTextProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Client);
	ProcessingPhase = EMassProcessingPhase::PostPhysics;

	/* DrawDebugString은 게임 스레드에서만 HUD 목록을 고칠 수 있다 */
	bRequiresGameThreadExecution = true;
}

void UJY_MassDebugTextProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) //override
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJY_EnemyVisualFragment>(EMassFragmentAccess::ReadOnly);
}

// [OwnClient]
void UJY_MassDebugTextProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) //override
{
	const UWorld* World = EntityManager.GetWorld();

	const UEnum* StateEnum = StaticEnum<EJY_EnemyState>();

	auto DrawStateTexts = [World, StateEnum](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FJY_EnemyVisualFragment> Visuals = ChunkContext.GetFragmentView<FJY_EnemyVisualFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const FVector TextLocation = Transforms[EntityIt].GetTransform().GetLocation() + JY_MassDebugText::TextOffset;
			const FJY_EnemyVisualFragment& Visual = Visuals[EntityIt];
			const FString StateName = StateEnum->GetNameStringByValue(static_cast<int64>(Visual.State));
			FString DebugText = FString::Printf(TEXT("%s %d"), *StateName, Visual.HealthPct);

			/* 걸린 상태이상이 있으면 비트 값을 붙인다 (예: Chase 255 S2) */
			if (Visual.StatusFlags != 0)
			{
				DebugText += FString::Printf(TEXT(" S%d"), Visual.StatusFlags);
			}

			/* 시간 0 = 이번 프레임만 그린다 (HUD.cpp:764) */
			DrawDebugString(World, TextLocation, DebugText, nullptr, FColor::Green, 0.0f);
		}
	};
	EntityQuery.ForEachEntityChunk(Context, DrawStateTexts);
}
