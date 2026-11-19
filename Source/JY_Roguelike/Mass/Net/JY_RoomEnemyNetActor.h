#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Engine/NetSerialization.h"
#include "Mass/JY_MassTypes.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "JY_RoomEnemyNetActor.generated.h"

class AJY_RoomEnemyNetActor;

/* 적 1마리의 복제 데이터 */
USTRUCT()
struct FJY_FastArrayEnemyItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	/* 서버·클라가 같은 적을 가리키는 번호는 부모의 ReplicationID를 쓴다 */
	UPROPERTY()
	FJY_EnemyState EnemyState;

	/* 적 종류 — UJY_GameData::EnemyDefinitions 배열 번호 */
	UPROPERTY()
	uint8 DefinitionIndex = 0;
};

/* 적 복제 데이터 목록 — 바뀐 원소만 보낸다 */
USTRUCT()
struct FJY_FastArrayEnemyContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	FJY_FastArrayEnemyContainer() = default;
	FJY_FastArrayEnemyContainer(AJY_RoomEnemyNetActor* InOwnerActor) : OwnerActor(InOwnerActor) {}

	/* 이번 수신에서 새로 생긴 적 데이터를 한꺼번에 받는다 */
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);

	/* 이번 수신에서 값이 바뀐 적 데이터를 한꺼번에 받는다 */
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);

	/* 이번 수신에서 지워질 적 데이터를 한꺼번에 받는다 — 원소가 아직 배열에 있을 때 불린다 */
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FJY_FastArrayEnemyItem, FJY_FastArrayEnemyContainer>(Items, DeltaParams, *this);
	}

	UPROPERTY()
	TArray<FJY_FastArrayEnemyItem> Items;

	UPROPERTY(NotReplicated)
	TObjectPtr<AJY_RoomEnemyNetActor> OwnerActor = nullptr;
};

/* 엔진은 이 콜백을 가상 함수가 아니라 이름으로 부른다 — 이름·인자가 틀리면 부모의 빈 함수가 조용히 불리므로 컴파일 때 확인 (FastArraySerializer.h:1165) */
static_assert(std::is_same_v<decltype(&FJY_FastArrayEnemyContainer::PostReplicatedAdd), void (FJY_FastArrayEnemyContainer::*)(const TArrayView<int32>&, int32)>);
static_assert(std::is_same_v<decltype(&FJY_FastArrayEnemyContainer::PostReplicatedChange), void (FJY_FastArrayEnemyContainer::*)(const TArrayView<int32>&, int32)>);
static_assert(std::is_same_v<decltype(&FJY_FastArrayEnemyContainer::PreReplicatedRemove), void (FJY_FastArrayEnemyContainer::*)(const TArrayView<int32>&, int32)>);

template<>
struct TStructOpsTypeTraits<FJY_FastArrayEnemyContainer> : public TStructOpsTypeTraitsBase2<FJY_FastArrayEnemyContainer>
{
	enum { WithNetDeltaSerializer = true };
};

/* 지금 방의 Mass 적 목록을 플레이어 클라에 보내는 복제 Actor — 런 동안 1개 */
UCLASS()
class AJY_RoomEnemyNetActor : public AInfo
{
	GENERATED_BODY()

public:
	AJY_RoomEnemyNetActor();

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	/* 적 1마리 데이터 추가 — 돌려주는 값 = 그 적의 ReplicationID */
	int32 AddEnemy(uint8 DefinitionIndex, const FJY_EnemyState& EnemyState);

	/* 적 1마리 복제 데이터를 고쳐 클라로 보낸다 */
	void UpdateEnemy(int32 ReplicationID, const FJY_EnemyState& EnemyState);

	/* 적 1마리 데이터를 지워 클라도 그 적을 지우게 한다 */
	void RemoveEnemy(int32 ReplicationID);

	/* 적 데이터를 전부 지워 클라도 지우게 한다 */
	void RemoveAllEnemies();

	void SetRoomOrigin(const FVector& InRoomOrigin);

	const FVector& GetRoomOrigin() const { return RoomOrigin; }

	void SetLastSendServerSeconds(float InLastSendServerSeconds) { LastSendServerSeconds = InLastSendServerSeconds; }

	float GetLastSendServerSeconds() const { return LastSendServerSeconds; }

private:
	/* 복제 — ReplicatedUsing 없음, RelativeLocation을 월드 위치로 되돌릴 때 쓴다 */
	UPROPERTY(Replicated)
	FVector_NetQuantize RoomOrigin = FVector::ZeroVector;

	/* 복제 — ReplicatedUsing 없음, 적 값을 마지막으로 보낸 서버 시각 (초). 클라가 보간 구간 길이를 구할 때 쓴다 */
	UPROPERTY(Replicated)
	float LastSendServerSeconds = 0.0f;

	/* 복제 — ReplicatedUsing 없음, 목록 콜백(FJY_FastArrayEnemyContainer::PostReplicatedAdd·Change)이 받는다 */
	UPROPERTY(Replicated)
	FJY_FastArrayEnemyContainer EnemyContainer;

	/* 서버 — ReplicationID로 EnemyContainer.Items 위치 찾기 */

	TMap<int32, int32> ItemIndexByReplicationID;
};
