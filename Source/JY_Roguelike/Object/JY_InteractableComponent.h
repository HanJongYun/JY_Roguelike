#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameplayTagContainer.h"
#include "Interfaces/JY_InteractableTarget.h"
#include "JY_InteractableComponent.generated.h"

/* E키 발동 범위 구 — 플레이어가 겹치면 상호작용 후보, 쓰면 물체를 발동 */
UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_InteractableComponent : public USphereComponent, public IJY_InteractableTarget
{
	GENERATED_BODY()

public:
	UJY_InteractableComponent(const FObjectInitializer& ObjectInitializer);

public:
	//~IJY_InteractableTarget
	virtual FText GetInteractionName() const override { return DisplayName; }
	virtual FText GetConfirmText() const override { return ConfirmText; }
	virtual FGameplayTag GetServiceTag() const override { return ServiceTag; }
	virtual bool IsWithinInteractRange(const AActor* User) const override;
	virtual void InteractLocal(APawn* User) override;
	virtual void InteractServer(APawn* User) override;
	//~End of IJY_InteractableTarget

private:
	/* 설정값 — BP 컴포넌트 디테일에서 수정 */

	/* 이 물체의 이름 (예: 제단) — 화면 아래 "[E] 제단" 안내 */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction")
	FText DisplayName;

	/* 쓰기 전 확인 팝업 문구 (예: 던전으로 출발할까요?). 비우면 바로 씀 */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction")
	FText ConfirmText;

	/* 이 물체가 제공하는 서비스 (제단 = Service.MetaAltar) */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction", Meta = (Categories = "Service"))
	FGameplayTag ServiceTag;

	/* 서버 거리 검증 여유 (cm) */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction", Meta = (ClampMin = "0.0"))
	float ServerRangeTolerance = 100.0f;
};
