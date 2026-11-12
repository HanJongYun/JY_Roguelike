#include "Mass/Net/JY_RoomEnemyNetActor.h"

#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_RoomEnemyNetActor)

/* 배열 수신 단계(엔진 네트 수신)는 Mass 처리 바깥이라 여기서 바로 엔티티를 만들어도 된다 */
// [OwnClient]
void FJY_FastArrayEnemyContainer::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	/* 원소를 복사하면 ReplicationID가 지워지므로(FastArraySerializer.h:308) 목록과 번호를 그대로 넘긴다 */
	UWorld* World = OwnerActor->GetWorld();
	UJY_MassEnemySubsystem* MassEnemySubsystem = World->GetSubsystem<UJY_MassEnemySubsystem>();
	MassEnemySubsystem->SpawnReplicatedEnemies(*this, AddedIndices);
}

// [OwnClient]
void FJY_FastArrayEnemyContainer::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	UWorld* World = OwnerActor->GetWorld();
	UJY_MassEnemySubsystem* MassEnemySubsystem = World->GetSubsystem<UJY_MassEnemySubsystem>();
	MassEnemySubsystem->UpdateReplicatedEnemies(*this, ChangedIndices);
}

/* 지워지기 직전에 불려 원소의 ReplicationID를 아직 읽을 수 있다 (FastArraySerializer.h:1148) */
// [OwnClient]
void FJY_FastArrayEnemyContainer::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	UWorld* World = OwnerActor->GetWorld();
	UJY_MassEnemySubsystem* MassEnemySubsystem = World->GetSubsystem<UJY_MassEnemySubsystem>();
	MassEnemySubsystem->DestroyReplicatedEnemies(*this, RemovedIndices);
}

AJY_RoomEnemyNetActor::AJY_RoomEnemyNetActor()
	: EnemyContainer(this)
{
	bReplicates = true;

	/* 방 하나가 플레이어 화면에 통째로 보인다 — 적이 멀어도 끊기지 않게 거리와 상관없이 보낸다 */
	bAlwaysRelevant = true;
}

void AJY_RoomEnemyNetActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AJY_RoomEnemyNetActor, RoomOrigin);
	DOREPLIFETIME(AJY_RoomEnemyNetActor, EnemyContainer);
}

/* 처음 MarkItemDirty 때 엔진이 ReplicationID를 붙이고, 이 번호는 클라에도 같은 값으로 간다 (FastArraySerializer.h:213) */
// [Server]
int32 AJY_RoomEnemyNetActor::AddEnemy(uint8 DefinitionIndex, const FJY_EnemyState& EnemyState)
{
	FJY_FastArrayEnemyItem& EnemyItem = EnemyContainer.Items.AddDefaulted_GetRef();
	EnemyItem.EnemyState = EnemyState;
	EnemyItem.DefinitionIndex = DefinitionIndex;
	EnemyContainer.MarkItemDirty(EnemyItem);
	ItemIndexByReplicationID.Add(EnemyItem.ReplicationID, EnemyContainer.Items.Num() - 1);
	return EnemyItem.ReplicationID;
}

/* 매 프레임 전부가 아니라 값이 바뀐 적만 불린다 — TMap 조회는 엔진 엔티티 찾기와 같은 방식 (MassArchetypeData.h:392) */
// [Server]
void AJY_RoomEnemyNetActor::UpdateEnemy(int32 ReplicationID, const FJY_EnemyState& EnemyState)
{
	const int32 ItemIndex = ItemIndexByReplicationID.FindChecked(ReplicationID);
	FJY_FastArrayEnemyItem& EnemyItem = EnemyContainer.Items[ItemIndex];
	EnemyItem.EnemyState = EnemyState;
	EnemyContainer.MarkItemDirty(EnemyItem);
}

/* 클라는 원소를 ReplicationID로 구분하므로 자리를 바꿔 빼도 지워진 1마리만 PreReplicatedRemove를 받는다 */
// [Server]
void AJY_RoomEnemyNetActor::RemoveEnemy(int32 ReplicationID)
{
	const int32 ItemIndex = ItemIndexByReplicationID.FindAndRemoveChecked(ReplicationID);
	EnemyContainer.Items.RemoveAtSwap(ItemIndex);

	/* 맨 끝 원소가 빈자리로 옮겨왔으면 그 원소의 위치 번호를 고친다 — 지운 게 맨 끝이었으면 옮겨온 원소가 없다 */
	if (ItemIndex < EnemyContainer.Items.Num())
	{
		const int32 MovedReplicationID = EnemyContainer.Items[ItemIndex].ReplicationID;
		ItemIndexByReplicationID[MovedReplicationID] = ItemIndex;
	}
	EnemyContainer.MarkArrayDirty();
}

/* 원소를 빼고 배열 전체를 바뀐 것으로 표시하면 클라는 없어진 원소마다 PreReplicatedRemove를 받는다 */
// [Server]
void AJY_RoomEnemyNetActor::RemoveAllEnemies()
{
	EnemyContainer.Items.Reset();
	EnemyContainer.MarkArrayDirty();
	ItemIndexByReplicationID.Reset();
}

/* 일반 속성은 FastArray보다 먼저 받으므로, 같은 틱에 추가한 적을 풀 때 클라에 이미 새 기준점이 있다 (DataReplication.cpp:1052) */
// [Server]
void AJY_RoomEnemyNetActor::SetRoomOrigin(const FVector& InRoomOrigin)
{
	RoomOrigin = InRoomOrigin;
}
