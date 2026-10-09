#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_InteractionComponent.generated.h"

class AJY_InteractableActor;
class UJY_UIManagerSubsystem;

/* 플레이어 캐릭터에 붙어 가까운 상호작용 대상을 고르고 E 입력을 처리 */
UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_InteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_InteractionComponent();

protected:
	virtual void BeginPlay() override;

public:
	/* E 입력 — 초점 대상의 동작 실행 */
	void Interact();

private:
	/* 폰 몸체(캡슐)가 무언가와 겹치기 시작 — 상호작용 물체의 범위면 후보에 넣음 */
	UFUNCTION()
	void HandleOwnerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/* 폰 몸체(캡슐)의 겹침이 끝남 — 상호작용 물체의 범위면 후보에서 뺌 */
	UFUNCTION()
	void HandleOwnerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/* 대상 범위에 들어옴 — 가장 최근에 들어온 대상이 초점 */
	void AddCandidate(AJY_InteractableActor* Interactable);

	/* 대상 범위에서 나감 — 초점이면 남은 대상 중 가장 최근 것으로 */
	void RemoveCandidate(AJY_InteractableActor* Interactable);

	/* 후보 목록에서 초점 대상을 다시 고르고, 바뀌었으면 안내를 갱신 */
	void RefreshFocus();

	/* 화면 아래 안내에 대상 이름을 넣음 (빈 글자면 숨김) */
	void SetPromptName(UJY_UIManagerSubsystem* UIManager, const FText& Name);

	/* 내 화면 관리자 — 폰 → PC → 로컬 플레이어. 서버·컨트롤러 없음이면 nullptr */
	UJY_UIManagerSubsystem* GetUIManager() const;

	/* 서버 동작 전 확인 팝업 결과 — 확인이면 서버에 요청 */
	void HandleConfirmResult(bool bConfirmed, TWeakObjectPtr<AJY_InteractableActor> Target);

	/* 서버에 사용 요청 — 서버가 거리를 다시 검증 */
	UFUNCTION(Server, Reliable)
	void Server_Interact(AJY_InteractableActor* Target);

private:
	/* 범위 안 대상들 (들어온 순서). 초점 = 마지막 */
	TArray<TWeakObjectPtr<AJY_InteractableActor>> Candidates;

	/* 지금 초점인 대상 (E를 누르면 쓰일 대상) */
	TWeakObjectPtr<AJY_InteractableActor> FocusedInteractable;

	/* 안내를 띄운 화면 관리자 — 컨트롤러가 먼저 떨어진 뒤(맵 이동)에도 안내를 지울 수 있게 */
	TWeakObjectPtr<UJY_UIManagerSubsystem> PromptUIManager;
};
