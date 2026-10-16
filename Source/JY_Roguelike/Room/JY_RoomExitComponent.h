#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_RoomExitComponent.generated.h"

class ULevel;

/*
 * 출구 문 — 방을 깨기 전엔 잠겨 막고, 열린 뒤 플레이어가 지나가면 다음 방으로.
 * 방 흐름(UJY_RoomFlowSubsystem)의 "방 깸" 알림을 받아 스스로 열린다 — 방 흐름은 문 클래스를 모른다.
 * BP 부품은 Component Tags로 찾는다: PassTrigger(통과 감지 Box) · Barrier(막는 벽 Box) · Gate(창살 메시) · OpenLight(열림 불빛)
 */
UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_RoomExitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_RoomExitComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/* [Server] 어느 방이 깼거나 잠겼다 — 내 방이면 열거나 잠금 */
	void HandleRoomClearChanged(const ULevel* RoomLevel, bool bCleared);

	// [Server]
	void SetOpen(bool bNewOpen);

	UFUNCTION()
	void HandlePassBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_Open();

	/* 열림 상태에 맞게 통과 감지·막는 벽·창살·불빛을 바꿈 (서버·클라 공통) */
	void ApplyOpenState();

	/* BP 부품 — 저장하지 않고 쓸 때마다 찾는다 */
	template <typename T>
	T* FindPart(FName PartTag) const;

private:
	/* 복제 값 — ReplicatedUsing=OnRep_Open: 클라도 벽을 켜고 꺼야 이동 예측이 서버와 맞는다 */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Open)
	bool bOpen = false;

	FDelegateHandle RoomClearChangedHandle;
};
