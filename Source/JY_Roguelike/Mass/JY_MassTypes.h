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

	/* 플레이어 앞에 서서 공격 차례를 기다린다 */
	Wait,
};

/* 모션 목록(TP_JY_*)의 칸 순서 — 모든 적이 이 순서로 채운다. 그 적만의 모션은 뒤에 붙인다 */
UENUM()
enum class EJY_EnemyAnimSlot : uint8
{
	Idle,
	Run,
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

	/* 걸린 상태이상 — 비트 번호 = DA_JY_StatusEffectData 목록 번호 */
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

	/* 직접 피해를 받을 때마다 1 증가 — 255 다음은 0. 상태이상 틱으로는 안 오른다 */
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

	/* 플레이어가 이 적의 거리(FJY_EnemyRangeParams) 안에 있나 — 바뀌는 순간 StateTree를 깨운다 */
	UPROPERTY()
	bool bInsideRange = false;
};

/* 적 종류마다 다른 거리 — 값은 Mass Config의 JY Enemy Navigation Trait에서 정한다 */
USTRUCT()
struct FJY_EnemyRangeParams : public FMassConstSharedFragment
{
	GENERATED_BODY()

	/* 플레이어까지 남은 길이 이 값 이하가 되면 거리 안 (cm) */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float StopDistance = 150.0f;

	/* 거리 안이 된 뒤 StopDistance보다 이만큼 더 멀어져야 거리 밖 (cm) — 경계에서 안팎이 계속 바뀌지 않게 */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float ResumeMargin = 50.0f;
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

/* 받은 위치 사이를 채우는 구간 하나 — 출발(받는 순간 그려져 있던 자리) → 도착(받은 서버 자리) */
USTRUCT()
struct FJY_EnemyNetInterpFragment : public FMassFragment
{
	GENERATED_BODY()

	FVector StartLocation = FVector::ZeroVector;

	FVector TargetLocation = FVector::ZeroVector;

	FQuat StartRotation = FQuat::Identity;

	FQuat TargetRotation = FQuat::Identity;

	/* 이 구간을 가는 데 쓸 시간 (초) — 서버가 보낸 시각의 차이 */
	float Duration = 0.0f;

	/* 이 구간에서 지난 시간 (초) — Duration에 닿으면 도착 */
	float Elapsed = 0.0f;

	/* 이 적의 값을 마지막으로 받았을 때의 서버 전송 시각 (초) */
	float LastServerSeconds = 0.0f;
};

/* 적이 밟고 있는 NavMesh 다각형 (서버) — 스폰 때 채우고, 이동하면 갱신. 위치는 Transform 하나에만 둔다 */
USTRUCT()
struct FJY_NavPolyFragment : public FMassFragment
{
	GENERATED_BODY()

	NavNodeRef PolyRef = INVALID_NAVNODEREF;
};

/* ---------- 상태이상 (서버) ---------- */

namespace JY_Mass
{
	/* 적 1마리에 걸 수 있는 상태이상 종류 수 — 복제 값(FJY_EnemyState::StatusFlags)이 uint8 비트 */
	constexpr int32 MaxStatusCount = 8;
}

/* 상태이상 하나의 시각 (월드 시각, 초) */
struct FJY_EnemyStatusSlot
{
	/* 풀리는 시각 */
	double EndSeconds = 0.0;

	/* 다음에 체력을 깎을 시각 */
	double NextTickSeconds = 0.0;
};

/* 적에게 걸린 상태이상 — 처음부터 붙여 두고 값으로 켜고 끈다. 번호 = DA_JY_StatusEffectData 목록 번호 */
USTRUCT()
struct FJY_EnemyStatusFragment : public FMassFragment
{
	GENERATED_BODY()

	FJY_EnemyStatusSlot Slots[JY_Mass::MaxStatusCount];

	/* 지금 걸린 상태의 비트 — 그대로 복제 값 StatusFlags가 된다 */
	UPROPERTY()
	uint8 ActiveFlags = 0;
};

/* 적 1마리가 이번 프레임에 받은 것 */
struct FJY_PendingMassHit
{
	/* 직접 피해 합 */
	float Damage = 0.0f;

	/* 걸 상태이상의 비트 */
	uint8 StatusFlags = 0;
};
