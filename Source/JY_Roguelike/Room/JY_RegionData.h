#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "JY_RegionData.generated.h"

class UJY_RoomData;

/* 지역 하나의 방 목록과 칸 종류 순서 (에셋 DA_JY_Region01 등) — 지역 맵 World Settings가 가리킴 */
UCLASS(Const)
class UJY_RegionData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	const TArray<TObjectPtr<const UJY_RoomData>>& GetRooms() const { return Rooms; }

	const TArray<FGameplayTag>& GetSlotTypes() const { return SlotTypes; }

	float GetTransitionFadeSeconds() const { return TransitionFadeSeconds; }

private:
	/* 설정값 — DA_JY_Region* 에셋에서 수정 */

	/* 이 지역에 나올 수 있는 방 (DA_Room_*) — 순서 상관없음 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Room")
	TArray<TObjectPtr<const UJY_RoomData>> Rooms;

	/* 칸마다 나올 방 종류, 앞에서부터 차례로 지남. 칸마다 Rooms 중 종류가 정확히 같고 아직 안 나온 방 하나를 뽑는다 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Room", Meta = (Categories = "Room.Type"))
	TArray<FGameplayTag> SlotTypes;

	/* 방을 넘어갈 때 화면이 어두워지는·밝아지는 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Room", Meta = (ClampMin = "0.0"))
	float TransitionFadeSeconds = 0.35f;
};
