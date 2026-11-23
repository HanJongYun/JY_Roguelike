#include "Mass/Behavior/JY_MassDamageProcessor.h"

#include "Combat/JY_StatusEffectData.h"
#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "Mass/JY_MassTypes.h"
#include "Mass/JY_MassProcessorOrder.h"
#include "Mass/JY_MassStateTreeWake.h"
#include "MassExecutionContext.h"
#include "MassSignalSubsystem.h"
#include "System/JY_AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassDamageProcessor)

namespace JY_MassDamage
{
	/* 기간·틱 간격이 0인 상태이상은 Mass 적에게 걸지 않는다 — DA_JY_StatusEffectData 입력 실수를 드러낸다 */
	bool IsUsableForMass(const FJY_StatusEffectEntry& StatusEffectEntry)
	{
		const bool bHasMassValues = StatusEffectEntry.MassDurationSeconds > 0.0f && StatusEffectEntry.MassTickIntervalSeconds > 0.0f;
		return ensureMsgf(bHasMassValues == true, TEXT("DA_JY_StatusEffectData의 %s에 Mass 기간·틱 간격이 없음"), *StatusEffectEntry.StatusTag.ToString());
	}
}

UJY_MassDamageProcessor::UJY_MassDamageProcessor()
	: EntityQuery(*this)
{
	/* 체력은 서버만 정한다 — 엔진 기본값 Server|Standalone(MassProcessor.cpp:138)에서 Standalone을 뺌 */
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Server);

	ProcessingPhase = EMassProcessingPhase::PrePhysics;

	/* 체력·움찔·죽음은 적의 상태를 정하는 일이라 Behavior 단계 */
	JY_MassProcessorOrder::PlaceInBehavior(ExecutionOrder);

	/* 서브시스템 피해 목록과 복제 Actor(UObject)를 고치므로 게임 스레드에서만 */
	bRequiresGameThreadExecution = true;
}

void UJY_MassDamageProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) //override
{
	EntityQuery.AddRequirement<FJY_EnemyHealthFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FJY_EnemyStatusFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FJY_EnemyLastSentToClientFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJY_EnemyKnockbackFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FJY_EnemyBehaviorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);
}

/* 걸린 상태이상의 틱과 죽는 모션이 끝난 적 지우기가 있어 매 프레임 모든 적을 돈다 — 피해 목록에 있어도 이미 지워진 적은 쿼리에 없어 그냥 버려진다 */
// [Server]
void UJY_MassDamageProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) //override
{
	UWorld* World = EntityManager.GetWorld();
	UJY_MassEnemySubsystem* MassEnemySubsystem = World->GetSubsystem<UJY_MassEnemySubsystem>();
	const TMap<FMassEntityHandle, FJY_PendingMassHit> PendingHits = MassEnemySubsystem->ConsumePendingHits();

	const UJY_StatusEffectData* StatusEffectData = UJY_AssetManager::Get().GetStatusEffectData();
	const TArray<FJY_StatusEffectEntry>& StatusEffects = StatusEffectData->StatusEffects;
	const int32 StatusCount = FMath::Min(StatusEffects.Num(), JY_Mass::MaxStatusCount);
	const double NowSeconds = World->GetTimeSeconds();

	auto ApplyHitsAndStatusTicks = [MassEnemySubsystem, &PendingHits, &StatusEffects, StatusCount, NowSeconds](FMassExecutionContext& ChunkContext)
	{
		const TArrayView<FJY_EnemyHealthFragment> Healths = ChunkContext.GetMutableFragmentView<FJY_EnemyHealthFragment>();
		const TArrayView<FJY_EnemyStatusFragment> Statuses = ChunkContext.GetMutableFragmentView<FJY_EnemyStatusFragment>();
		const TConstArrayView<FJY_EnemyLastSentToClientFragment> LastSentToClients = ChunkContext.GetFragmentView<FJY_EnemyLastSentToClientFragment>();
		const TArrayView<FJY_EnemyKnockbackFragment> Knockbacks = ChunkContext.GetMutableFragmentView<FJY_EnemyKnockbackFragment>();
		const TArrayView<FJY_EnemyBehaviorFragment> Behaviors = ChunkContext.GetMutableFragmentView<FJY_EnemyBehaviorFragment>();

		/* 날아가기·움찔하기·죽기 — 상태를 바꾸는 것은 적마다의 StateTree라 깨워 준다 */
		TArray<FMassEntityHandle> WokenEntities;

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const FMassEntityHandle Entity = ChunkContext.GetEntity(EntityIt);
			FJY_EnemyHealthFragment& Health = Healths[EntityIt];
			FJY_EnemyStatusFragment& Status = Statuses[EntityIt];
			FJY_EnemyBehaviorFragment& Behavior = Behaviors[EntityIt];

			/* 죽는 중 — 피해도 상태이상 틱도 받지 않고, 죽는 모션이 끝나면 지운다 */
			if (Health.Current <= 0.0f)
			{
				if (NowSeconds >= Behavior.DeathEndSeconds)
				{
					MassEnemySubsystem->DestroyEnemy(ChunkContext, Entity, LastSentToClients[EntityIt].ReplicationID);
				}
				continue;
			}

			const FJY_PendingMassHit* PendingHit = PendingHits.Find(Entity);

			/* 걸린 상태도 새로 받은 것도 없는 적이 대부분이다 */
			if (PendingHit == nullptr && Status.ActiveFlags == 0)
				continue;

			float TotalDamage = 0.0f;
			uint8 AppliedStatusFlags = 0;
			if (PendingHit != nullptr)
			{
				TotalDamage = PendingHit->Damage;
				AppliedStatusFlags = PendingHit->StatusFlags;

				/* 독구름처럼 피해 없이 상태만 다시 거는 것은 타격으로 세지 않는다 */
				if (PendingHit->Damage > 0.0f)
				{
					++Health.HitSerial;
				}
			}

			for (int32 StatusIndex = 0; StatusIndex < StatusCount; ++StatusIndex)
			{
				const uint8 StatusBit = static_cast<uint8>(1 << StatusIndex);
				const FJY_StatusEffectEntry& StatusEffectEntry = StatusEffects[StatusIndex];
				FJY_EnemyStatusSlot& Slot = Status.Slots[StatusIndex];

				if ((AppliedStatusFlags & StatusBit) != 0 && JY_MassDamage::IsUsableForMass(StatusEffectEntry) == true)
				{
					/* 다시 걸리면 기간만 되돌린다 — 틱 박자는 그대로 (플레이어 GE와 같은 규칙) */
					Slot.EndSeconds = NowSeconds + StatusEffectEntry.MassDurationSeconds;
					if ((Status.ActiveFlags & StatusBit) == 0)
					{
						/* 첫 틱은 걸리는 즉시 — 아래에서 바로 들어간다 */
						Slot.NextTickSeconds = NowSeconds;
						Status.ActiveFlags |= StatusBit;
					}
				}

				if ((Status.ActiveFlags & StatusBit) == 0)
					continue;

				/* 끝나는 시각 정각의 틱까지 넣는다 — 플레이어 GE와 같게 (GameplayEffect.cpp:5441) */
				if (NowSeconds >= Slot.NextTickSeconds && Slot.NextTickSeconds <= Slot.EndSeconds)
				{
					/* 저항·공격자 증가분이 생기면 여기서 곱한다 */
					TotalDamage += StatusEffectEntry.MassTickDamage;
					Slot.NextTickSeconds += StatusEffectEntry.MassTickIntervalSeconds;
				}

				if (NowSeconds >= Slot.EndSeconds)
				{
					Status.ActiveFlags &= ~StatusBit;
				}
			}

			if (TotalDamage > 0.0f)
			{
				/* 실드·방어 규칙은 아직 없다 — 계산이 끝난 최종 피해를 체력에서만 뺀다 */
				Health.Current = FMath::Max(Health.Current - TotalDamage, 0.0f);

				/* 죽이는 타격은 넉백 없이 죽는 모션으로 — 걸린 상태이상도 같이 끈다 */
				if (Health.Current <= 0.0f)
				{
					Behavior.DeathEndSeconds = NowSeconds + Behavior.DeathSeconds;

					/* 움찔 시간이 남은 채 죽으면 죽는 중에 움찔 상태로 넘어간다 */
					Behavior.HitEndSeconds = 0.0;
					Status.ActiveFlags = 0;
					WokenEntities.Add(Entity);
					continue;
				}
			}

			if (PendingHit == nullptr)
				continue;

			if (PendingHit->KnockbackForce.IsNearlyZero() == false)
			{
				/* 띄우는 힘이 없으면 바닥에 붙어 밀린다. 날아가는 중에 또 맞으면 높이는 그대로 두고 속도만 새로 정한다 */
				FJY_EnemyKnockbackFragment& Knockback = Knockbacks[EntityIt];
				Knockback.Velocity = PendingHit->KnockbackForce / Knockback.Mass;
				Knockback.bActive = true;

				/* 날아가는 타격은 움찔하지 않는다 — 착지한 뒤 남은 움찔 시간으로 서 있지 않게 */
				Behavior.HitEndSeconds = 0.0;
				WokenEntities.Add(Entity);
				continue;
			}

			/* 독구름처럼 피해 없는 것과 날아가는 중의 타격은 움찔하지 않는다. 움찔하는 중에 또 맞으면 끝나는 시각만 뒤로 밀린다 */
			if (PendingHit->Damage > 0.0f && Behavior.State != EJY_EnemyState::Knockback)
			{
				Behavior.HitEndSeconds = NowSeconds + Behavior.HitSeconds;
				WokenEntities.Add(Entity);
			}
		}

		UMassSignalSubsystem& SignalSubsystem = ChunkContext.GetMutableSubsystemChecked<UMassSignalSubsystem>();
		JY_MassStateTreeWake::WakeEntities(ChunkContext, SignalSubsystem, WokenEntities);
	};
	EntityQuery.ForEachEntityChunk(Context, ApplyHitsAndStatusTicks);
}
