#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JY_InteractableActor.generated.h"

class APlayerController;
class USphereComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UJY_ScreenWidget;

/* 다가가면 "[E] 이름"이 뜨고 E로 쓰는 월드 물체 — 제단·기록판·포털의 부모 */
UCLASS()
class AJY_InteractableActor : public AActor
{
	GENERATED_BODY()

public:
	AJY_InteractableActor();

protected:
	virtual void BeginPlay() override;

public:
	/* 안내 표시 켜기·끄기 (로컬 플레이어가 초점을 맞췄을 때) */
	void SetPromptVisible(bool bVisible);

	/* 이 거리 안이면 사용 가능 — 서버가 요청을 믿기 전에 다시 확인 */
	bool IsWithinInteractRange(const AActor* User) const;

	/* 화면을 여는 대상인가 (아니면 서버에 요청하는 대상) */
	bool OpensScreen() const { return ScreenClass.IsNull() == false; }

	/* 서버 동작형: 서버가 검증을 통과시킨 뒤 호출 (자식이 재정의) */
	virtual void OnServerInteract(APlayerController* User) {}

	const FText& GetDisplayName() const { return DisplayName; }
	const TSoftClassPtr<UJY_ScreenWidget>& GetScreenClass() const { return ScreenClass; }
	const FText& GetConfirmText() const { return ConfirmText; }

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

	/* 머리 위 "[E] 이름" (화면 공간 — 줌과 무관하게 같은 크기). 데디 서버에선 위젯을 만들지 않는다 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Interaction")
	TObjectPtr<UWidgetComponent> PromptWidget;

protected:
	/* 설정값 — BP_JY_Interactable_* · BP_JY_Portal의 Class Defaults에서 수정 */

	/* 안내·팝업에 보일 이름 (예: 제단) */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction")
	FText DisplayName;

	/* 지정하면 E로 GameMenu 층에 이 화면을 연다 (제단·기록판). 비우면 서버 동작형 (포털) */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction")
	TSoftClassPtr<UJY_ScreenWidget> ScreenClass;

	/* 서버 동작형에서 먼저 물어볼 문구 (비우면 바로 실행). 예: 던전으로 출발할까요? */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction")
	FText ConfirmText;

	/* 서버 거리 검증 여유 (cm) */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction", meta = (ClampMin = "0.0"))
	float ServerRangeTolerance = 100.0f;

private:
	TSharedPtr<struct FStreamableHandle> ScreenPreloadHandle;
};
