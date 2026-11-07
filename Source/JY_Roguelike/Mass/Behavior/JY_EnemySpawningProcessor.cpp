#include "Mass/Behavior/JY_EnemySpawningProcessor.h"

#include "Mass/JY_MassFragments.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_EnemySpawningProcessor)

UJY_EnemySpawningProcessor::UJY_EnemySpawningProcessor()
	: EntityQuery(*this)
{
	/* 상태는 서버가 정해 복제로 보낸다 — 엔진 기본값 Server|Standalone(MassProcessor.cpp:138)에서 Standalone을 뺌 */
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Server);

	/* 이동보다 먼저 상태를 정해야 같은 프레임에 움직이기 시작한다 */
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void UJY_EnemySpawningProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) //override
{
	EntityQuery.AddRequirement<FJY_EnemyBehaviorFragment>(EMassFragmentAccess::ReadWrite);
}

/* 150마리 비교 한 번씩이라 태그로 거르지 않는다 — Spawning을 이 Processor로 둘지 StateTree 상태로 옮길지는 3-5에서 정한다 */
// [Server]
void UJY_EnemySpawningProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) //override
{
	const UWorld* World = EntityManager.GetWorld();
	const double NowSeconds = World->GetTimeSeconds();

	auto FinishSpawning = [NowSeconds](FMassExecutionContext& ChunkContext)
	{
		const TArrayView<FJY_EnemyBehaviorFragment> Behaviors = ChunkContext.GetMutableFragmentView<FJY_EnemyBehaviorFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FJY_EnemyBehaviorFragment& Behavior = Behaviors[EntityIt];
			if (Behavior.State != EJY_EnemyState::Spawning || NowSeconds < Behavior.SpawningEndSeconds)
				continue;

			Behavior.State = EJY_EnemyState::Chase;
		}
	};
	EntityQuery.ForEachEntityChunk(Context, FinishSpawning);
}
