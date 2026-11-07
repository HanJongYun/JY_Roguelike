#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Mass/EntityHandle.h"
#include "JY_MassEnemySubsystem.generated.h"

class UMassEntityConfigAsset;
class UJY_EnemyDefinition;
class AJY_RoomEnemyNetActor;
struct FJY_EnemyState;
struct FMassEntityManager;
struct FJY_FastArrayEnemyContainer;

/* Mass 적을 만들고 지우는 곳 — 서버는 웨이브·치트로, 클라는 복제 데이터를 받아 자기 엔티티를 만든다 */
UCLASS()
class UJY_MassEnemySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

public:
	/* 적 종류의 설계도로 위치마다 1마리씩 만들고 복제 데이터를 붙인다 — SpawnWarningSeconds 동안 예고(Spawning) 뒤 움직인다 */
	TArray<FMassEntityHandle> SpawnWave(const UJY_EnemyDefinition& EnemyDefinition, TConstArrayView<FVector> Locations, float SpawnWarningSeconds);

	/* 서버 적을 전부 지운다 — 클라 적도 복제로 따라 지워진다 */
	void DestroyAllEnemies();

	/* 서버 적 전부를 한 방향으로 옮긴다 (치트 시험용) */
	void MoveAllEnemies(const FVector& Offset);

	/* 서버 적 전부의 체력을 깎는다 (치트 시험용) */
	void DamageAllEnemies(float Damage);

	/* 새로 받은 복제 데이터마다 그 종류의 설계도로 엔티티를 만든다 */
	void SpawnReplicatedEnemies(const FJY_FastArrayEnemyContainer& EnemyContainer, TConstArrayView<int32> AddedIndices);

	/* 값이 바뀐 복제 데이터를 클라 엔티티에 반영한다 */
	void UpdateReplicatedEnemies(const FJY_FastArrayEnemyContainer& EnemyContainer, TConstArrayView<int32> ChangedIndices);

	/* 지워질 복제 데이터의 클라 엔티티를 지운다 */
	void DestroyReplicatedEnemies(const FJY_FastArrayEnemyContainer& EnemyContainer, TConstArrayView<int32> RemovedIndices);

	AJY_RoomEnemyNetActor* GetRoomEnemyNetActor() const { return RoomEnemyNetActor; }

private:
	/* 이 월드의 엔티티와 Fragment를 가진 엔진 저장소 */
	FMassEntityManager& GetEntityManager() const;

	TArray<FMassEntityHandle> SpawnEntitiesAt(const UMassEntityConfigAsset& EnemyConfig, TConstArrayView<FVector> Locations);

	/* 복제 데이터의 위치·방향·표시 값을 클라 엔티티에 넣는다 */
	void ApplyEnemyState(FMassEntityHandle Entity, const FJY_EnemyState& EnemyState, const FVector& RoomOrigin);

private:
	/* 서버 */

	UPROPERTY()
	TObjectPtr<AJY_RoomEnemyNetActor> RoomEnemyNetActor;

	/* 서버·클라 — ReplicationID로 내 엔티티 찾기 */

	TMap<int32, FMassEntityHandle> EntityByReplicationID;
};
