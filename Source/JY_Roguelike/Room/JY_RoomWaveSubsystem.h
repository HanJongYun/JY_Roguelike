#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/Deque.h"
#include "JY_RoomWaveSubsystem.generated.h"

class UJY_EnemyDefinition;
class AJY_RoomEnemySpawnPoint;
class UJY_RoomData;
class ULevel;

/* 지금 방의 웨이브를 차례로 낸다 — 방 흐름의 입장 알림을 듣고 Mass 적을 만든다 (서버) */
UCLASS()
class UJY_RoomWaveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/* 새 방에 들어옴 — 남은 적을 지우고, 전투 방이면 첫 웨이브 타이머를 건다 */
	void HandleRoomEntered(const ULevel* RoomLevel, const UJY_RoomData* RoomData);

	/* 이 번호의 스폰 지점마다 적 구성대로 자리를 정해 대기열에 넣는다 (1부터) */
	void StartWave(int32 WaveNumber);

	/* 지점 반경 안 네비메시 위의 자리 — 지점에서 길이 이어져 벽 너머로는 안 나간다 */
	FVector FindSpawnLocation(const AJY_RoomEnemySpawnPoint& SpawnPoint) const;

	/* 대기열에서 한 틱 몫을 꺼내 만든다 — 남으면 다음 틱에 이어서 */
	void SpawnQueuedEnemies();

private:
	/* 만들 차례를 기다리는 적 1마리 — 정의는 방 맵의 스폰 지점이 붙잡고 있다 */
	struct FJY_QueuedEnemySpawn
	{
		const UJY_EnemyDefinition* EnemyDefinition = nullptr;
		FVector Location = FVector::ZeroVector;
	};

	TWeakObjectPtr<const ULevel> CurrentRoomLevel;

	/* 지금 방의 정보 */
	UPROPERTY()
	TObjectPtr<const UJY_RoomData> CurrentRoomData;

	TDeque<FJY_QueuedEnemySpawn> QueuedEnemySpawns;

	FTimerHandle FirstWaveTimerHandle;

	FTimerHandle SpawnTimerHandle;
};
