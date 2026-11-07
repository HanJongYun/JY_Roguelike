#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JY_RoomMarker.generated.h"

class UArrowComponent;
class UBillboardComponent;
class UJY_EnemyDefinition;

/* 방 안의 위치 표시 (적 스폰·보상) — 게임 중엔 안 보이고 위치·방향만 쓰임. 입장 위치는 엔진 PlayerStart */
UCLASS(Abstract)
class AJY_RoomMarker : public AActor
{
	GENERATED_BODY()

public:
	AJY_RoomMarker();

#if WITH_EDITORONLY_DATA
private:
	/* 에디터에서만 보이는 아이콘 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Room")
	TObjectPtr<UBillboardComponent> SpriteComponent;

	/* 에디터에서만 보이는 앞쪽 화살표 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Room")
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif
};

/* 스폰 지점 한 곳에서 나올 적 한 종류 */
USTRUCT()
struct FJY_SpawnPointEnemy
{
	GENERATED_BODY()

	/* UJY_GameData::EnemyDefinitions에 들어 있는 것만 — 클라가 그 배열 번호로 같은 적을 만든다 */
	UPROPERTY(EditAnywhere, Category = "JY|Room")
	TObjectPtr<const UJY_EnemyDefinition> EnemyDefinition;

	UPROPERTY(EditAnywhere, Category = "JY|Room", Meta = (ClampMin = "1"))
	int32 Count = 1;
};

/* 적이 나타나는 곳 — 무엇이 몇 마리 나올지도 이 지점이 정한다 */
UCLASS()
class AJY_RoomEnemySpawnPoint : public AJY_RoomMarker
{
	GENERATED_BODY()

public:
	int32 GetWaveNumber() const { return WaveNumber; }

	float GetSpawnRadius() const { return SpawnRadius; }

	const TArray<FJY_SpawnPointEnemy>& GetEnemies() const { return Enemies; }

private:
	/* 설정값 — 레벨에 놓은 액터의 디테일 패널에서 수정 */

	/* 이 지점이 쓰이는 웨이브 (1부터) — 방의 DA_Room_* 웨이브 수를 넘으면 안 된다 */
	UPROPERTY(EditInstanceOnly, Category = "JY|Room", Meta = (ClampMin = "1"))
	int32 WaveNumber = 1;

	/* 이 반경 안 네비메시 위에 흩어져 나온다 — 벽 너머로는 안 나간다 (cm) */
	UPROPERTY(EditInstanceOnly, Category = "JY|Room", Meta = (ClampMin = "0.0"))
	float SpawnRadius = 150.0f;

	UPROPERTY(EditInstanceOnly, Category = "JY|Room")
	TArray<FJY_SpawnPointEnemy> Enemies;
};

/* 방을 깨면 보상이 나타나는 곳 */
UCLASS()
class AJY_RoomRewardPoint : public AJY_RoomMarker
{
	GENERATED_BODY()
};
