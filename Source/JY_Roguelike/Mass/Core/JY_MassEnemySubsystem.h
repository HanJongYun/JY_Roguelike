#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Mass/EntityHandle.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "Mass/Navigation/JY_NavFlowField.h"
#include "JY_MassEnemySubsystem.generated.h"

class ARecastNavMesh;
class UMassEntityConfigAsset;
class UJY_EnemyDefinition;
class AJY_RoomEnemyNetActor;
struct FJY_EnemyState;
struct FMassEntityManager;
struct FJY_FastArrayEnemyContainer;
struct FJY_HitShape;
struct FJY_CombatTarget;
struct FMassExecutionContext;

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

	/* 플레이어 쪽으로 가는 다각형별 다음 걸음 (서버) */
	const FJY_NavFlowField& GetFlowField() const { return FlowField; }

	/* 이번 프레임 시작 때 복사한 플레이어 위치 = 캡슐 중심 (서버) — 사거리·타격처럼 몸 기준일 때 */
	const FVector& GetPlayerLocation() const { return PlayerLocation; }

	/* 플레이어 발밑 NavMesh 점 (서버) — 이동의 갈 곳처럼 바닥 기준일 때 */
	const FVector& GetPlayerNavLocation() const { return PlayerNavLocation; }

	/* 이 월드의 NavMesh (없으면 nullptr) */
	const ARecastNavMesh* GetNavMesh() const;

	/* 모양 안의 맞을 수 있는 적(예고 중 제외)을 대상 목록에 더한다. Center = 모양 중심, Forward = 수평 앞 방향 */
	void FindEnemiesInShape(const FJY_HitShape& Shape, const FVector& Center, const FVector& Forward, TArray<FJY_CombatTarget>& OutTargets) const;

	/* 적 1마리가 받을 최종 피해를 쌓는다 — 다음 Mass 처리 때 피해 Processor가 체력에 반영 */
	void AddPendingHit(FMassEntityHandle Entity, float Damage);

	/* 쌓인 피해를 통째로 넘기고 비운다 */
	TMap<FMassEntityHandle, float> ConsumePendingHits();

	/* 죽은 적 1마리를 지운다 — 엔티티는 이번 Mass 단계 끝에, 복제 데이터는 바로 */
	void DestroyEnemy(FMassExecutionContext& Context, FMassEntityHandle Entity, int32 ReplicationID);

private:
	/* Mass PrePhysics 처리 직전 — 플레이어 위치를 복사하고, 적이 있을 때만 바닥 점과 플로우 필드를 갱신한다 */
	void HandlePrePhysicsStarted(float DeltaSeconds);

	/* 새로 만든 적마다 선 자리의 다각형을 넣는다 */
	void InitializeNavPolys(TConstArrayView<FMassEntityHandle> Entities);

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

	FVector PlayerLocation = FVector::ZeroVector;

	FVector PlayerNavLocation = FVector::ZeroVector;

	FJY_NavFlowField FlowField;

	/* 적별로 이번 프레임에 받은 최종 피해 합 — 공격(CombatQuery::ApplyHit)이 쌓고 UJY_MassDamageProcessor가 비운다 */
	TMap<FMassEntityHandle, float> PendingHitsBuffer;

	/* 서버·클라 — ReplicationID로 내 엔티티 찾기 */

	TMap<int32, FMassEntityHandle> EntityByReplicationID;
};

/* Processor는 플로우 필드·플레이어 위치를 읽기만 한다 — 바꾸는 건 PrePhysics 시작 알림(게임 스레드)뿐이라 작업 스레드에서 읽어도 된다 */
template<>
struct TMassExternalSubsystemTraits<UJY_MassEnemySubsystem> final
{
	enum
	{
		GameThreadOnly = false,
		ThreadSafeWrite = false,
	};
};
