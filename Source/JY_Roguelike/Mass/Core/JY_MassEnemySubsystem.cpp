#include "Mass/Core/JY_MassEnemySubsystem.h"

#include "Mass/Core/JY_EnemyDefinition.h"
#include "Mass/JY_MassFragments.h"
#include "Mass/Net/JY_EnemyNetWriteProcessor.h"
#include "Mass/Net/JY_RoomEnemyNetActor.h"
#include "Mass/EntityFragments.h"
#include "MassEntitySubsystem.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "MassEntityConfigAsset.h"
#include "MassSpawnerSubsystem.h"
#include "MassSpawnerTypes.h"
#include "MassSpawnLocationProcessor.h"
#include "Room/JY_RoomFlowSubsystem.h"
#include "StructUtils/InstancedStruct.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassEnemySubsystem)

namespace JY_MassEnemy
{
	/* 서버 Trait(ValidTargetConfig = Server)는 Standalone 월드에서 빠져 Fragment 없는 엔티티가 생긴다 (MassEntityTemplateRegistry.cpp:124) — 기준 실행이 Play As Client라 Standalone은 적을 만들지 않는다 */
	bool IsEnemyServer(const UWorld& World)
	{
		const ENetMode NetMode = World.GetNetMode();
		return NetMode == NM_DedicatedServer || NetMode == NM_ListenServer;
	}
}

/* 복제 Actor는 퍼시스턴트 레벨에 두어 방이 바뀌어도 남는다 */
// [Server]
void UJY_MassEnemySubsystem::OnWorldBeginPlay(UWorld& InWorld) //override
{
	Super::OnWorldBeginPlay(InWorld);

	if (JY_MassEnemy::IsEnemyServer(InWorld) == false)
		return;

	RoomEnemyNetActor = InWorld.SpawnActor<AJY_RoomEnemyNetActor>();
}

/* Processor 실행 중에는 부르면 안 된다 — 타이머·치트처럼 Mass 처리 바깥에서만 (MassEntityManager.cpp:846) */
// [Server]
TArray<FMassEntityHandle> UJY_MassEnemySubsystem::SpawnWave(const UJY_EnemyDefinition& EnemyDefinition, TConstArrayView<FVector> Locations)
{
	TArray<FMassEntityHandle> SpawnedEntities;
	UWorld* World = GetWorld();
	if (JY_MassEnemy::IsEnemyServer(*World) == false)
		return SpawnedEntities;

	/* 클라는 이 번호로 같은 배열에서 같은 정의를 찾는다 — 배열에 없는 정의는 클라가 만들 수 없다. 복제 칸이 uint8이라 256종까지 */
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const int32 DefinitionIndex = GameData->EnemyDefinitions.IndexOfByKey(&EnemyDefinition);
	check(DefinitionIndex != INDEX_NONE);
	check(DefinitionIndex <= MAX_uint8);

	/* 복제 위치는 값이 작을수록 비트를 덜 쓴다 (QuantizedVectorSerialization.cpp:90) — 방에 하나씩 있는 PlayerStart를 기준점으로 쓴다 */
	const UJY_RoomFlowSubsystem* RoomFlow = World->GetSubsystem<UJY_RoomFlowSubsystem>();
	const FVector RoomOrigin = RoomFlow->GetCurrentRoomStartLocation();
	RoomEnemyNetActor->SetRoomOrigin(RoomOrigin);

	SpawnedEntities = SpawnEntitiesAt(*EnemyDefinition.EntityConfig, Locations);

	FMassEntityManager& EntityManager = GetEntityManager();
	for (const FMassEntityHandle& Entity : SpawnedEntities)
	{
		/* 엔티티가 실제로 선 위치로 복제 데이터를 만든다 — 위치 배열 순서와 엔티티 순서가 같다고 가정하지 않음 */
		const FTransformFragment& TransformFragment = EntityManager.GetFragmentDataChecked<FTransformFragment>(Entity);
		const FTransform& Transform = TransformFragment.GetTransform();
		const FJY_EnemyHealthFragment& Health = EntityManager.GetFragmentDataChecked<FJY_EnemyHealthFragment>(Entity);
		const FJY_EnemyState EnemyState = UJY_EnemyNetWriteProcessor::MakeEnemyState(Transform, Health, RoomOrigin);
		const int32 ReplicationID = RoomEnemyNetActor->AddEnemy(static_cast<uint8>(DefinitionIndex), EnemyState);

		/* 서버 엔티티 → 복제 데이터 연결. 클라는 반대 방향(EntityByReplicationID)으로 찾는다 */
		FJY_EnemyLastSentToClientFragment& LastSentToClient = EntityManager.GetFragmentDataChecked<FJY_EnemyLastSentToClientFragment>(Entity);
		LastSentToClient.ReplicationID = ReplicationID;
		LastSentToClient.LastSentState = EnemyState;

		EntityByReplicationID.Add(ReplicationID, Entity);
	}

	return SpawnedEntities;
}

/* 치트 시험용 — 실제 이동은 2장 이동 Processor가 한다 */
// [Server]
void UJY_MassEnemySubsystem::MoveAllEnemies(const FVector& Offset)
{
	FMassEntityManager& EntityManager = GetEntityManager();
	for (const TPair<int32, FMassEntityHandle>& Pair : EntityByReplicationID)
	{
		FTransformFragment& TransformFragment = EntityManager.GetFragmentDataChecked<FTransformFragment>(Pair.Value);
		FTransform& Transform = TransformFragment.GetMutableTransform();
		Transform.AddToTranslation(Offset);
	}
}

/* 치트 시험용 — 실제 피해는 3장 피해 Processor가 준다 */
// [Server]
void UJY_MassEnemySubsystem::DamageAllEnemies(float Damage)
{
	FMassEntityManager& EntityManager = GetEntityManager();
	for (const TPair<int32, FMassEntityHandle>& Pair : EntityByReplicationID)
	{
		FJY_EnemyHealthFragment& Health = EntityManager.GetFragmentDataChecked<FJY_EnemyHealthFragment>(Pair.Value);
		Health.Current = FMath::Max(Health.Current - Damage, 0.0f);
		++Health.HitSerial;
	}
}

// [OwnClient]
void UJY_MassEnemySubsystem::SpawnReplicatedEnemies(const FJY_FastArrayEnemyContainer& EnemyContainer, TConstArrayView<int32> AddedIndices)
{
	/* 한 번의 수신에 여러 종류가 섞여 올 수 있다 — 종류마다 한 번씩 만든다 */
	TMap<uint8, TArray<int32>> AddedIndicesByDefinition;
	for (const int32 EnemyIndex : AddedIndices)
	{
		const uint8 DefinitionIndex = EnemyContainer.Items[EnemyIndex].DefinitionIndex;
		AddedIndicesByDefinition.FindOrAdd(DefinitionIndex).Add(EnemyIndex);
	}

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const FVector RoomOrigin = EnemyContainer.OwnerActor->GetRoomOrigin();
	for (const TPair<uint8, TArray<int32>>& Pair : AddedIndicesByDefinition)
	{
		const UJY_EnemyDefinition* EnemyDefinition = GameData->EnemyDefinitions[Pair.Key];
		const TArray<int32>& DefinitionEnemyIndices = Pair.Value;

		TArray<FVector> Locations;
		Locations.Reserve(DefinitionEnemyIndices.Num());
		for (const int32 EnemyIndex : DefinitionEnemyIndices)
		{
			Locations.Add(RoomOrigin + EnemyContainer.Items[EnemyIndex].EnemyState.RelativeLocation);
		}

		TArray<FMassEntityHandle> SpawnedEntities = SpawnEntitiesAt(*EnemyDefinition->EntityConfig, Locations);
		for (int32 Index = 0; Index < SpawnedEntities.Num(); ++Index)
		{
			const FJY_FastArrayEnemyItem& EnemyItem = EnemyContainer.Items[DefinitionEnemyIndices[Index]];
			EntityByReplicationID.Add(EnemyItem.ReplicationID, SpawnedEntities[Index]);
			ApplyEnemyState(SpawnedEntities[Index], EnemyItem.EnemyState, RoomOrigin);
		}
	}
}

// [OwnClient]
void UJY_MassEnemySubsystem::UpdateReplicatedEnemies(const FJY_FastArrayEnemyContainer& EnemyContainer, TConstArrayView<int32> ChangedIndices)
{
	const FVector RoomOrigin = EnemyContainer.OwnerActor->GetRoomOrigin();
	for (const int32 EnemyIndex : ChangedIndices)
	{
		const FJY_FastArrayEnemyItem& EnemyItem = EnemyContainer.Items[EnemyIndex];
		const FMassEntityHandle Entity = EntityByReplicationID.FindChecked(EnemyItem.ReplicationID);
		ApplyEnemyState(Entity, EnemyItem.EnemyState, RoomOrigin);
	}
}

// [OwnClient]
void UJY_MassEnemySubsystem::ApplyEnemyState(FMassEntityHandle Entity, const FJY_EnemyState& EnemyState, const FVector& RoomOrigin)
{
	FMassEntityManager& EntityManager = GetEntityManager();

	const float Yaw = FRotator::DecompressAxisFromByte(EnemyState.Yaw);
	const FRotator Rotation(0.0, Yaw, 0.0);
	FTransformFragment& TransformFragment = EntityManager.GetFragmentDataChecked<FTransformFragment>(Entity);
	FTransform& Transform = TransformFragment.GetMutableTransform();
	Transform.SetLocation(RoomOrigin + EnemyState.RelativeLocation);
	Transform.SetRotation(Rotation.Quaternion());

	FJY_EnemyVisualFragment& Visual = EntityManager.GetFragmentDataChecked<FJY_EnemyVisualFragment>(Entity);
	Visual.HealthPct = EnemyState.HealthPct;
	Visual.ShieldPct = EnemyState.ShieldPct;
	Visual.StatusFlags = EnemyState.StatusFlags;
}

FMassEntityManager& UJY_MassEnemySubsystem::GetEntityManager() const
{
	UWorld* World = GetWorld();
	UMassEntitySubsystem* MassEntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	return MassEntitySubsystem->GetMutableEntityManager();
}

TArray<FMassEntityHandle> UJY_MassEnemySubsystem::SpawnEntitiesAt(const UMassEntityConfigAsset& EnemyConfig, TConstArrayView<FVector> Locations)
{
	TArray<FMassEntityHandle> SpawnedEntities;
	if (Locations.Num() == 0)
		return SpawnedEntities;

	UWorld* World = GetWorld();

	/* Trait 목록 → 이 적이 가질 Fragment 묶음(템플릿). 처음 한 번 만들고 이후엔 캐시를 돌려준다 */
	const FMassEntityTemplate& EnemyTemplate = EnemyConfig.GetOrCreateEntityTemplate(*World);

	FInstancedStruct SpawnData = FInstancedStruct::Make<FMassTransformsSpawnData>();
	FMassTransformsSpawnData& TransformsSpawnData = SpawnData.GetMutable<FMassTransformsSpawnData>();

	/* 기본값은 위치를 섞어 나눠 준다 — 그러면 N번째 엔티티가 N번째 위치에 서지 않는다 (MassSpawnerTypes.h:29) */
	TransformsSpawnData.bRandomize = false;
	TransformsSpawnData.Transforms.Reserve(Locations.Num());
	for (const FVector& Location : Locations)
	{
		TransformsSpawnData.Transforms.Add(FTransform(Location));
	}

	/* 위치는 생성 직후 이 Processor가 TransformFragment에 넣는다 — 엔진 생성기와 같은 조합 (MassEntityEQSSpawnPointsGenerator.cpp:75) */
	UMassSpawnerSubsystem* SpawnerSubsystem = World->GetSubsystem<UMassSpawnerSubsystem>();
	SpawnerSubsystem->SpawnEntities(EnemyTemplate.GetTemplateID(), Locations.Num(), FConstStructView(SpawnData), UMassSpawnLocationProcessor::StaticClass(), OUT SpawnedEntities);

	return SpawnedEntities;
}
