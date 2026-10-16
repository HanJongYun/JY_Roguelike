#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "JY_RegionData.generated.h"

/* 지역의 한 칸 — 이 칸에 올 수 있는 방 후보들 */
USTRUCT()
struct FJY_RoomSlot
{
	GENERATED_BODY()

	/* 후보 방 맵 — 지역 맵에 하위 레벨로 등록된 것만 켤 수 있다 */
	UPROPERTY(EditAnywhere, Category = "JY|Room")
	TArray<TSoftObjectPtr<UWorld>> CandidateRooms;
};

/* 지역 하나의 방 순서 (에셋 DA_JY_Region01 등) — 지역 맵 World Settings가 가리킴 */
UCLASS(Const)
class UJY_RegionData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	const TArray<FJY_RoomSlot>& GetRoomSlots() const { return RoomSlots; }

	float GetTransitionFadeSeconds() const { return TransitionFadeSeconds; }

private:
	/* 설정값 — DA_JY_Region* 에셋에서 수정 */

	/* 앞에서부터 차례로 지나는 칸. 지역 맵에 도착하면 첫 칸부터 켠다 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Room")
	TArray<FJY_RoomSlot> RoomSlots;

	/* 방을 넘어갈 때 화면이 어두워지는·밝아지는 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Room", Meta = (ClampMin = "0.0"))
	float TransitionFadeSeconds = 0.35f;
};
