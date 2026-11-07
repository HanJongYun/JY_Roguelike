#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "JY_RoomData.generated.h"

/* 방 하나의 정보 (에셋 DA_Room_*) — 맵을 불러오기 전에도 읽힌다. 몬스터 배치는 맵 안 스폰 지점이 가진다 */
UCLASS(Const)
class UJY_RoomData : public UDataAsset
{
	GENERATED_BODY()

public:
	const TSoftObjectPtr<UWorld>& GetMap() const { return Map; }

	const FGameplayTag& GetRoomType() const { return RoomType; }

	/* 전투 방인가 (Room.Type.Combat) — 아니면 들어가자마자 문이 열린다 */
	bool IsCombatRoom() const;

	int32 GetWaveCount() const { return WaveCount; }

	float GetFirstWaveDelaySeconds() const { return FirstWaveDelaySeconds; }

	float GetSpawnWarningSeconds() const { return SpawnWarningSeconds; }

private:
	/* 설정값 — DA_Room_* 에셋에서 수정 */

	/* 방 맵 — 지역 맵에 하위 레벨로 등록된 것만 켤 수 있다 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Room")
	TSoftObjectPtr<UWorld> Map;

	/* 방의 성격 (Room.Type.* — Combat·Combat.Start·Combat.Boss·Shop) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Room", Meta = (Categories = "Room.Type"))
	FGameplayTag RoomType;

	/* 전투 방의 웨이브 수 — 맵 스폰 지점의 웨이브 번호는 1 ~ 이 값 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Wave", Meta = (ClampMin = "0"))
	int32 WaveCount = 1;

	/* 방에 들어와서 첫 웨이브가 나올 때까지 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Wave", Meta = (ClampMin = "0.0"))
	float FirstWaveDelaySeconds = 2.0f;

	/* 적이 생긴 뒤 움직이기 전까지 예고하는 시간 (초) — 이동·공격 안 함 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Wave", Meta = (ClampMin = "0.0"))
	float SpawnWarningSeconds = 0.5f;
};
