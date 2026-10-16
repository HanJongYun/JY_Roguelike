#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "JY_RoomFlowSubsystem.generated.h"

class UJY_RegionData;
class ULevel;

/* 방을 깼거나(true) 새 방에 들어와 잠겼다(false) (서버) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FJY_OnRoomClearChanged, const ULevel* /*RoomLevel*/, bool /*bCleared*/);

/* 던전 지역 맵에서 방을 차례로 켜고 플레이어를 옮긴다 (서버) */
UCLASS()
class UJY_RoomFlowSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

public:
	/* 던전 지역 맵인가 (맵 World Settings에 RegionData가 있음) */
	bool IsRegionMap() const;

	/* 플레이어 캐릭터를 만들어도 되나 — 지역 맵이면 첫 방에 들어간 뒤부터, 아니면 항상 */
	bool CanStartPlayers() const;

	/* 지금 방 레벨 (지역 맵이 아니거나 첫 방 전이면 nullptr) */
	const ULevel* GetCurrentRoomLevel() const;

	/* 출구 문을 지났다 — 방을 깬 뒤면 다음 칸 방을 켜고 옮김. 마지막 칸이면 로비로 */
	void RequestNextRoom();

	/* 지금 방을 깸 — 출구 문이 열린다 */
	void ClearCurrentRoom();

	/* 이 방 레벨이 지금 방이고 깬 상태인가 */
	bool IsRoomCleared(const ULevel* RoomLevel) const;

	/* 방을 깼거나 새 방에 들어옴 */
	FJY_OnRoomClearChanged OnRoomClearChanged;

private:
	/* 이 칸의 방을 골라 켜기 시작 — 다 켜지면 TryEnterNextRoom */
	void LoadRoom(int32 SlotIndex);

	/* 방에 들어옴 — 적 스폰 지점이 없는 방(상점)은 바로 깬 것으로 친다 */
	void EnterRoom(const ULevel* RoomLevel);

	/* 다음 칸 후보 중 하나 — 이번 런에 이미 나온 방은 뺌 */
	TSoftObjectPtr<UWorld> PickNextRoom(int32 SlotIndex) const;

	/* 다음 방 레벨이 서버에 다 올라옴 (LoadStreamLevel 완료 콜백) */
	UFUNCTION()
	void HandleNextRoomLoaded();

	/* 모든 클라가 새 방을 띄웠고 화면이 다 어두워졌으면 옮기고, 아니면 잠시 뒤 다시 확인 */
	void TryEnterNextRoom();

	bool HaveAllClientsInitializedLevel(const ULevel* Level) const;

	/* 모든 플레이어의 캐릭터를 방 PlayerStart로 */
	void MovePlayersToStart(const ULevel* Level) const;

	/* 첫 방에 들어오기를 기다리던 플레이어의 캐릭터를 만든다 */
	void StartWaitingPlayers() const;

	/* 모든 플레이어 화면을 검게 덮거나(true) 걷음(false) */
	void FadePlayerScreens(bool bFadeOut) const;

	/* 마지막 칸을 지나면 로비로 */
	void ReturnToHub() const;

private:
	TObjectPtr<const UJY_RegionData> RegionData;

	/* 지금 방이 몇 번째 칸인지 */
	int32 CurrentSlotIndex = INDEX_NONE;

	TSoftObjectPtr<UWorld> CurrentRoom;

	bool bCurrentRoomCleared = false;

	/* 이번 런에 들어간 방 */
	TArray<TSoftObjectPtr<UWorld>> VisitedRooms;

	/* 켜는 중인 다음 방 */
	TSoftObjectPtr<UWorld> PendingRoom;

	/* 켜는 중인 방의 칸 번호 */
	int32 PendingSlotIndex = INDEX_NONE;

	/* 화면이 다 어두워지는 시각 (월드 시간) */
	double FadeOutEndSeconds = 0.0;

	FTimerHandle EnterRetryTimerHandle;

	/* LoadStreamLevel 완료 콜백 구분 번호 */
	int32 NextLatentUUID = 0;
};
