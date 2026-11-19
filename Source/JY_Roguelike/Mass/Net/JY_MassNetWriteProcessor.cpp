#include "Mass/Net/JY_MassNetWriteProcessor.h"

#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "Mass/JY_MassTypes.h"
#include "Mass/Net/JY_RoomEnemyNetActor.h"
#include "Mass/EntityFragments.h"
#include "MassExecutionContext.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassNetWriteProcessor)

namespace JY_MassNetWrite
{
	constexpr float MaxRatioByte = 255.0f;

	/* 회복으로 최대치를 넘어도 uint8이 감겨 엉뚱한 값이 되지 않게 0~255로 자른다 */
	uint8 ToRatioByte(float Value, float MaxValue)
	{
		const int32 RatioByte = FMath::RoundToInt32(MaxRatioByte * Value / MaxValue);
		const int32 ClampedRatioByte = FMath::Clamp(RatioByte, 0, MAX_uint8);
		return static_cast<uint8>(ClampedRatioByte);
	}
}

UJY_MassNetWriteProcessor::UJY_MassNetWriteProcessor()
	: EntityQuery(*this)
{
	/* 클라는 받기만 한다 — 엔진 기본값 Server|Standalone(MassProcessor.cpp:138)에서 Standalone을 뺌 */
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Server);

	/* 이동이 끝난 뒤의 위치를 보내야 하므로 물리 뒤 단계 */
	ProcessingPhase = EMassProcessingPhase::PostPhysics;

	/* 복제 Actor(UObject)를 고치므로 게임 스레드에서만 */
	bRequiresGameThreadExecution = true;
}

void UJY_MassNetWriteProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) //override
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJY_EnemyHealthFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJY_EnemyBehaviorFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJY_EnemyStatusFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJY_EnemyLastSentToClientFragment>(EMassFragmentAccess::ReadWrite);
}

/* 비교는 청크 안의 Fragment끼리 끝내고, 바뀐 적만 Actor 표(TMap)로 찾아 고친다 — 보낼 값끼리 비교하므로 1cm 미만 이동은 보내지 않음 */
// [Server]
void UJY_MassNetWriteProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) //override
{
	UWorld* World = EntityManager.GetWorld();

	/* 매 프레임이 아니라 전송 간격마다만 비교한다 — 사이는 클라 보간(UJY_MassNetInterpProcessor)이 채운다 */
	const double WorldSeconds = World->GetTimeSeconds();
	if (WorldSeconds < NextSendSeconds)
		return;

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	NextSendSeconds = FMath::Max(NextSendSeconds + GameData->EnemyNetSendInterval, WorldSeconds);

	UJY_MassEnemySubsystem* MassEnemySubsystem = World->GetSubsystem<UJY_MassEnemySubsystem>();
	AJY_RoomEnemyNetActor* RoomEnemyNetActor = MassEnemySubsystem->GetRoomEnemyNetActor();
	const FVector RoomOrigin = RoomEnemyNetActor->GetRoomOrigin();

	bool bAnyStateChanged = false;
	auto WriteChangedStates = [RoomEnemyNetActor, RoomOrigin, &bAnyStateChanged](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FJY_EnemyHealthFragment> Healths = ChunkContext.GetFragmentView<FJY_EnemyHealthFragment>();
		const TConstArrayView<FJY_EnemyBehaviorFragment> Behaviors = ChunkContext.GetFragmentView<FJY_EnemyBehaviorFragment>();
		const TConstArrayView<FJY_EnemyStatusFragment> Statuses = ChunkContext.GetFragmentView<FJY_EnemyStatusFragment>();
		const TArrayView<FJY_EnemyLastSentToClientFragment> LastSentToClients = ChunkContext.GetMutableFragmentView<FJY_EnemyLastSentToClientFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FJY_EnemyLastSentToClientFragment& LastSentToClient = LastSentToClients[EntityIt];
			const FTransform& Transform = Transforms[EntityIt].GetTransform();
			const FJY_EnemyState EnemyState = MakeEnemyState(Transform, Healths[EntityIt], Behaviors[EntityIt], Statuses[EntityIt].ActiveFlags, RoomOrigin);
			if (EnemyState == LastSentToClient.LastSentState)
				continue;

			RoomEnemyNetActor->UpdateEnemy(LastSentToClient.ReplicationID, EnemyState);
			LastSentToClient.LastSentState = EnemyState;
			bAnyStateChanged = true;
		}
	};
	EntityQuery.ForEachEntityChunk(Context, WriteChangedStates);

	/* 보낼 적이 있을 때만 시각을 고친다 — 전부 서 있으면 시각도 안 간다 */
	if (bAnyStateChanged == true)
	{
		RoomEnemyNetActor->SetLastSendServerSeconds(static_cast<float>(WorldSeconds));
	}
}

/* FVector_NetQuantize는 받는 쪽에서 1cm로 반올림된다 (NetSerialization.h:430) — 서버도 미리 반올림해야 소수점만 바뀐 값을 안 보낸다 */
FJY_EnemyState UJY_MassNetWriteProcessor::MakeEnemyState(const FTransform& Transform, const FJY_EnemyHealthFragment& Health, const FJY_EnemyBehaviorFragment& Behavior, uint8 StatusFlags, const FVector& RoomOrigin)
{
	const FVector RelativeLocation = Transform.GetLocation() - RoomOrigin;

	const double RoundedX = FMath::RoundToDouble(RelativeLocation.X);
	const double RoundedY = FMath::RoundToDouble(RelativeLocation.Y);
	const double RoundedZ = FMath::RoundToDouble(RelativeLocation.Z);
	const FRotator Rotation = Transform.Rotator();

	FJY_EnemyState EnemyState;
	EnemyState.RelativeLocation = FVector(RoundedX, RoundedY, RoundedZ);
	EnemyState.Yaw = FRotator::CompressAxisToByte(Rotation.Yaw);
	EnemyState.State = Behavior.State;

	/* 최대 체력 0은 에셋(DA_MassEnemy_*) 값 실수 — 0으로 나누지 않고 드러낸다 */
	if (ensureMsgf(Health.Max > 0.0f, TEXT("FJY_EnemyHealthFragment::Max가 0 이하 — DA_MassEnemy_* 서버 Trait 값 확인")))
	{
		EnemyState.HealthPct = JY_MassNetWrite::ToRatioByte(Health.Current, Health.Max);
	}
	if (Health.ShieldMax > 0.0f)
	{
		EnemyState.ShieldPct = JY_MassNetWrite::ToRatioByte(Health.Shield, Health.ShieldMax);
	}
	EnemyState.StatusFlags = StatusFlags;
	EnemyState.HitSerial = Health.HitSerial;
	return EnemyState;
}
