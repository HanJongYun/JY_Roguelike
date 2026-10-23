#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_RoomExitComponent.generated.h"

class ULevel;

/* 출구 문 — 방을 깨면 열리고 플레이어가 지나가면 다음 방으로 (부품 태그: PassTrigger·Barrier·Gate·OpenLight) */
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

	/* BP 부품을 태그로 찾음 */
	template <typename T>
	T* FindPart(FName PartTag) const;

private:
	/* 복제 — 열림 상태 */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Open)
	bool bOpen = false;

	FDelegateHandle RoomClearChangedHandle;
};
