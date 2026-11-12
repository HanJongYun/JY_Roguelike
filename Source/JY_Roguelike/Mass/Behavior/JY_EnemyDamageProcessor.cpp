#include "Mass/Behavior/JY_EnemyDamageProcessor.h"

#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "Mass/JY_MassTypes.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_EnemyDamageProcessor)

UJY_EnemyDamageProcessor::UJY_EnemyDamageProcessor()
	: EntityQuery(*this)
{
	/* 체력은 서버만 정한다 — 엔진 기본값 Server|Standalone(MassProcessor.cpp:138)에서 Standalone을 뺌 */
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Server);

	ProcessingPhase = EMassProcessingPhase::PrePhysics;

	/* 서브시스템 피해 목록과 복제 Actor(UObject)를 고치므로 게임 스레드에서만 */
	bRequiresGameThreadExecution = true;
}

void UJY_EnemyDamageProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) //override
{
	EntityQuery.AddRequirement<FJY_EnemyHealthFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FJY_EnemyLastSentToClientFragment>(EMassFragmentAccess::ReadOnly);
}

/* 피해 목록에 있는 적만 체력을 깎는다 — 목록에 있어도 이미 지워진 적은 쿼리에 없어 그냥 버려진다 */
// [Server]
void UJY_EnemyDamageProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) //override
{
	UWorld* World = EntityManager.GetWorld();
	UJY_MassEnemySubsystem* MassEnemySubsystem = World->GetSubsystem<UJY_MassEnemySubsystem>();
	const TMap<FMassEntityHandle, float> PendingHits = MassEnemySubsystem->ConsumePendingHits();
	if (PendingHits.Num() == 0)
		return;

	auto ApplyPendingHits = [MassEnemySubsystem, &PendingHits](FMassExecutionContext& ChunkContext)
	{
		const TArrayView<FJY_EnemyHealthFragment> Healths = ChunkContext.GetMutableFragmentView<FJY_EnemyHealthFragment>();
		const TConstArrayView<FJY_EnemyLastSentToClientFragment> LastSentToClients = ChunkContext.GetFragmentView<FJY_EnemyLastSentToClientFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const FMassEntityHandle Entity = ChunkContext.GetEntity(EntityIt);
			const float* Damage = PendingHits.Find(Entity);
			if (Damage == nullptr)
				continue;

			/* 실드·방어 규칙은 아직 없다 — 계산이 끝난 최종 피해를 체력에서만 뺀다 */
			FJY_EnemyHealthFragment& Health = Healths[EntityIt];
			Health.Current = FMath::Max(Health.Current - *Damage, 0.0f);
			++Health.HitSerial;

			/* 임시 사망 — 사망 상태·연출 없이 바로 지운다 */
			if (Health.Current <= 0.0f)
			{
				MassEnemySubsystem->DestroyEnemy(ChunkContext, Entity, LastSentToClients[EntityIt].ReplicationID);
			}
		}
	};
	EntityQuery.ForEachEntityChunk(Context, ApplyPendingHits);
}
