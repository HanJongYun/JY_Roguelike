#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "JY_RoomMarker.generated.h"

class UArrowComponent;
class UBillboardComponent;

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

/* 적이 나타나는 곳 — 웨이브 데이터가 SpawnTag로 고름 */
UCLASS()
class AJY_RoomEnemySpawnPoint : public AJY_RoomMarker
{
	GENERATED_BODY()

public:
	const FGameplayTag& GetSpawnTag() const { return SpawnTag; }

private:
	/* 설정값 — 레벨에 놓은 액터의 디테일 패널에서 수정 */

	/* 이 지점을 쓰는 웨이브 (Room.Spawn.Wave1 등) */
	UPROPERTY(EditInstanceOnly, Category = "JY|Room", Meta = (Categories = "Room.Spawn"))
	FGameplayTag SpawnTag;
};

/* 방을 깨면 보상이 나타나는 곳 */
UCLASS()
class AJY_RoomRewardPoint : public AJY_RoomMarker
{
	GENERATED_BODY()
};
