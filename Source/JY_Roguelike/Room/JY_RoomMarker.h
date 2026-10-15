#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "JY_RoomMarker.generated.h"

class UArrowComponent;
class UBillboardComponent;
class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/* 방 안의 위치 표시 (적 스폰·보상·출구 문) — 게임 중엔 안 보이고 위치·방향만 쓰임. 입장 위치는 엔진 PlayerStart */
UCLASS(Abstract)
class AJY_RoomMarker : public AActor
{
	GENERATED_BODY()

public:
	AJY_RoomMarker();

#if WITH_EDITORONLY_DATA
private:
	/* 에디터에서만 보이는 아이콘 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Room")
	TObjectPtr<UBillboardComponent> SpriteComponent;

	/* 에디터에서만 보이는 앞쪽 화살표 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Room")
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif
};

/* 적이 나타나는 곳 — 웨이브 데이터가 SpawnTag로 고름 */
UCLASS()
class AJY_RoomEnemySpawnPoint : public AJY_RoomMarker
{
	GENERATED_BODY()

public:
	const FGameplayTag& GetSpawnTag() const { return SpawnTag; }

private:
	/* 설정값 — 레벨에 놓은 액터의 디테일 패널에서 수정 */

	/* 이 지점을 쓰는 웨이브 (Room.Spawn.Wave1 등) */
	UPROPERTY(EditInstanceOnly, Category = "JY|Room", Meta = (Categories = "Room.Spawn"))
	FGameplayTag SpawnTag;
};

/* 방을 깨면 보상이 나타나는 곳 */
UCLASS()
class AJY_RoomRewardPoint : public AJY_RoomMarker
{
	GENERATED_BODY()
};

/* 다음 방으로 가는 출구 문 자리 — 화살표가 방 바깥쪽. 방을 깨기 전엔 잠겨 막고, 열린 뒤 플레이어가 지나가면 다음 방으로 */
UCLASS()
class AJY_RoomExitDoor : public AJY_RoomMarker
{
	GENERATED_BODY()

public:
	AJY_RoomExitDoor();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	const USceneComponent* GetRewardIconAnchor() const { return RewardIconAnchor; }

	/* 문 열기·잠그기 (서버) */
	void SetOpen(bool bNewOpen);

private:
	UFUNCTION()
	void HandlePassBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_Open();

	/* 열림 상태에 맞게 막는 벽·창살·불빛을 바꿈 (서버·클라 공통) */
	void ApplyOpenState();

private:
	/* 문 위에 다음 방 보상 아이콘이 뜨는 자리 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Room")
	TObjectPtr<USceneComponent> RewardIconAnchor;

	/* 문 통과 감지 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Room")
	TObjectPtr<UBoxComponent> PassTrigger;

	/* 잠겼을 때 문틀을 막는 보이지 않는 벽 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Room")
	TObjectPtr<UBoxComponent> Barrier;

	/* 잠김 표시 창살 — 메시는 BP_JY_RoomExitDoor 컴포넌트에서 지정 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Room")
	TObjectPtr<UStaticMeshComponent> GateMesh;

	/* 열림 표시 불빛 — 색·밝기는 BP_JY_RoomExitDoor 컴포넌트에서 지정 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Room")
	TObjectPtr<UPointLightComponent> OpenLight;

	/* 복제 값 — ReplicatedUsing=OnRep_Open: 클라도 벽을 켜고 꺼야 이동 예측이 서버와 맞는다 */
	UPROPERTY(ReplicatedUsing = OnRep_Open)
	bool bOpen = false;
};
