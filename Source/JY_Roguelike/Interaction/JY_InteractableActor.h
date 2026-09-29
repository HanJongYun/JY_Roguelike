#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "JY_InteractableActor.generated.h"

class APlayerController;
class USphereComponent;
class UStaticMeshComponent;

/* 다가가서 E로 쓰는 월드 물체 — 제단·기록판·포털의 부모 */
UCLASS()
class AJY_InteractableActor : public AActor
{
	GENERATED_BODY()

public:
	AJY_InteractableActor();

protected:
	virtual void BeginPlay() override;

public:
	/* 이 거리 안이면 사용 가능 — 서버가 요청을 믿기 전에 다시 확인 */
	bool IsWithinInteractRange(const AActor* User) const;

	/* 서버가 거리 검증을 통과시킨 뒤 호출 (자식이 재정의 — 포털은 맵 이동) */
	virtual void OnServerInteract(APlayerController* User) {}

	const FText& GetDisplayName() const { return DisplayName; }
	const FGameplayTag& GetServiceTag() const { return ServiceTag; }

private:
	UFUNCTION()
	void HandleRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	/* 겉모습. 메시 에셋은 BP에서 지정 (지금은 엔진 큐브) */
	UPROPERTY(VisibleAnywhere, Category = "JY|Interaction")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/* 들어오면 사용 가능 범위 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Interaction")
	TObjectPtr<USphereComponent> InteractionRange;

protected:
	/* 설정값 — BP_JY_Interactable_* · BP_JY_Portal의 Class Defaults에서 수정 */

	/* 이 물체의 이름 (예: 제단) — 나중에 화면 아래 "[E] 제단" 안내에 쓴다 */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction")
	FText DisplayName;

	/* 이 물체가 제공하는 서비스 (제단 = Service.MetaAltar). 서버가 구매·환불 요청의 판매 대상인지 이것으로 확인 */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction", meta = (Categories = "Service"))
	FGameplayTag ServiceTag;

	/* 서버 거리 검증 여유 (cm) */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction", meta = (ClampMin = "0.0"))
	float ServerRangeTolerance = 100.0f;
};
