#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "Mass/EntityElementTypes.h"
#include "JY_MassFragments.generated.h"

/* ---------- 서버 → 클라: 복제로 넘기는 데이터 ---------- */

/* 적 1마리 복제 데이터 — 필드별로 바뀐 것만 전송된다 */
USTRUCT()
struct FJY_EnemyState
{
	GENERATED_BODY()

	bool operator==(const FJY_EnemyState& Other) const = default;

	/* 방 기준점(AJY_RoomEnemyNetActor::RoomOrigin)에서 떨어진 위치, 1cm 단위 */
	UPROPERTY()
	FVector_NetQuantize RelativeLocation = FVector::ZeroVector;

	/* 256단계, 1단계 약 1.4도 */
	UPROPERTY()
	uint8 Yaw = 0;

	UPROPERTY()
	uint8 State = 0;

	/* 체력 비율 0~255 */
	UPROPERTY()
	uint8 HealthPct = 255;

	/* 보호막 비율 0~255 */
	UPROPERTY()
	uint8 ShieldPct = 0;

	UPROPERTY()
	uint8 StatusFlags = 0;

	/* 맞을 때마다 1 증가 */
	UPROPERTY()
	uint8 HitSerial = 0;
};

/* ---------- 서버 전용 ---------- */

/* 서버 엔티티가 자기 복제 데이터(EnemyContainer.Items 원소)를 찾는 번호와 마지막으로 보낸 값 */
USTRUCT()
struct FJY_EnemyLastSentToClientFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY()
	int32 ReplicationID = INDEX_NONE;

	UPROPERTY()
	FJY_EnemyState LastSentState;
};

/* 체력 — 값은 Mass Config의 Assorted Fragments Trait에서 정한다 */
USTRUCT()
struct FJY_EnemyHealthFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float Current = 1.0f;

	UPROPERTY(EditAnywhere)
	float Max = 1.0f;

	UPROPERTY(EditAnywhere)
	float Shield = 0.0f;

	UPROPERTY(EditAnywhere)
	float ShieldMax = 0.0f;

	/* 맞을 때마다 1 증가 — 255 다음은 0 */
	UPROPERTY()
	uint8 HitSerial = 0;
};

/* ---------- 클라 전용 ---------- */

/* 복제받은 표시 값 */
USTRUCT()
struct FJY_EnemyVisualFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 HealthPct = 255;

	UPROPERTY()
	uint8 ShieldPct = 0;

	UPROPERTY()
	uint8 StatusFlags = 0;
};
