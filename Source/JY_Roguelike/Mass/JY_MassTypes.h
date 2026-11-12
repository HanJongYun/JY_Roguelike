#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "Mass/EntityElementTypes.h"
#include "AI/Navigation/NavigationTypes.h"
#include "JY_MassTypes.generated.h"

/* 적이 지금 무엇을 하나 — 서버가 정하고 클라는 표현(모션 고르기)에 쓴다 */
UENUM()
enum class EJY_EnemyState : uint8
{
	/* 막 생겨 예고 중 — 움직이지도 때리지도 않는다 */
	Spawning,

	/* 플레이어를 쫓는다 */
	Chase,
};

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
	EJY_EnemyState State = EJY_EnemyState::Spawning;

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

/* 행동 상태와 그 상태가 끝나는 시각 */
USTRUCT()
struct FJY_EnemyBehaviorFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY()
	EJY_EnemyState State = EJY_EnemyState::Spawning;

	/* Spawning이 끝나는 월드 시각 (초) */
	UPROPERTY()
	double SpawningEndSeconds = 0.0;
};

/* ---------- 클라 전용 ---------- */

/* 복제받은 표시 값 */
USTRUCT()
struct FJY_EnemyVisualFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY()
	EJY_EnemyState State = EJY_EnemyState::Spawning;

	UPROPERTY()
	uint8 HealthPct = 255;

	UPROPERTY()
	uint8 ShieldPct = 0;

	UPROPERTY()
	uint8 StatusFlags = 0;
};

/* 적이 밟고 있는 NavMesh 다각형 (서버) — 스폰 때 채우고, 이동하면 갱신 */
USTRUCT()
struct FJY_NavPolyFragment : public FMassFragment
{
	GENERATED_BODY()

	NavNodeRef PolyRef = INVALID_NAVNODEREF;

	/* NavMesh 위 마지막 위치 — 다음 프레임 NavMesh 붙이기의 출발점 */
	FVector Location = FVector::ZeroVector;
};
