#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "JY_InteractableActor.generated.h"

class APlayerController;
class USphereComponent;
class UStaticMeshComponent;
class UJY_ScreenWidget;
struct FStreamableHandle;

/* E를 눌렀을 때 하는 일의 종류 */
UENUM()
enum class EJY_InteractionType : uint8
{
	/* 서버에 묻지 않고 내 화면에 창을 연다 (기록판·제단) — ScreenClass 필수 */
	OpenScreen,

	/* 서버에 요청하고 서버가 검증 후 실행 (포털) — OnServerInteract를 재정의한 C++ 자식만 */
	ServerAction
};

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
	EJY_InteractionType GetInteractionType() const { return InteractionType; }
	const TSoftClassPtr<UJY_ScreenWidget>& GetScreenClass() const { return ScreenClass; }
	const FText& GetConfirmText() const { return ConfirmText; }
	const FGameplayTag& GetServiceTag() const { return ServiceTag; }
	const USphereComponent* GetInteractionRange() const { return InteractionRange; }

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

	/* E를 눌렀을 때 하는 일 — 서버 동작형은 C++ 자식 생성자가 정하므로 BP에서는 보기만 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Interaction")
	EJY_InteractionType InteractionType = EJY_InteractionType::OpenScreen;

	/* OpenScreen일 때 E로 GameMenu 층에 여는 화면 (예: WBP_JY_RecordBoard) */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction", meta = (EditCondition = "InteractionType == EJY_InteractionType::OpenScreen", EditConditionHides))
	TSoftClassPtr<UJY_ScreenWidget> ScreenClass;

	/* ServerAction일 때 서버에 요청하기 전 확인 팝업 문구 (예: 던전으로 출발할까요?). 비우면 바로 요청 */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction", meta = (EditCondition = "InteractionType == EJY_InteractionType::ServerAction", EditConditionHides))
	FText ConfirmText;

	/* 이 물체가 제공하는 서비스 (제단 = Service.MetaAltar). 서버가 구매·환불 요청의 판매 대상인지 이것으로 확인 */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction", meta = (Categories = "Service"))
	FGameplayTag ServiceTag;

	/* 서버 거리 검증 여유 (cm) */
	UPROPERTY(EditAnywhere, Category = "JY|Interaction", meta = (ClampMin = "0.0"))
	float ServerRangeTolerance = 100.0f;

private:
	/* 화면 클래스 미리 로드 — 핸들을 쥐고 있어야 로드된 클래스가 메모리에 남는다 */
	TSharedPtr<FStreamableHandle> ScreenPreloadHandle;
};
