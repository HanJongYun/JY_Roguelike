#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_InteractionComponent.generated.h"

class AJY_InteractableActor;
class UJY_UIManagerSubsystem;

/* 플레이어 캐릭터에 붙어 가까운 상호작용 대상을 고르고 E 입력을 처리 (화면 아래 안내 표시도 겸함) */
UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_InteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_InteractionComponent();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/* 대상 범위에 들어옴 — 가장 최근에 들어온 대상이 초점 */
	void AddCandidate(AJY_InteractableActor* Interactable);

	/* 대상 범위에서 나감 — 초점이면 남은 대상 중 가장 최근 것으로 */
	void RemoveCandidate(AJY_InteractableActor* Interactable);

	/* E 입력 — 초점 대상의 동작 실행 */
	void Interact();

private:
	/* 초점이 바뀌었으면 화면 아래 안내를 바꿈 */
	void RefreshFocus();

	/* 화면 아래 안내 글자를 바꿈 (빈 글자면 숨김) */
	void SetPromptName(UJY_UIManagerSubsystem* UIManager, const FText& Name);

	/* 이 폰 주인(로컬 플레이어)의 UI 관리자 */
	UJY_UIManagerSubsystem* GetUIManager() const;

	/* 확인 팝업 결과 → 확인이면 서버에 요청 */
	void HandleConfirmResult(bool bConfirmed, TWeakObjectPtr<AJY_InteractableActor> Target);

	/* 서버에 사용 요청 — 서버가 거리·종류를 다시 검증 */
	UFUNCTION(Server, Reliable)
	void Server_Interact(AJY_InteractableActor* Target);

private:
	/* 범위 안 대상들 (들어온 순서). 초점 = 마지막 */
	TArray<TWeakObjectPtr<AJY_InteractableActor>> Candidates;

	/* 지금 안내가 떠 있는 대상 */
	TWeakObjectPtr<AJY_InteractableActor> FocusedInteractable;

	/* 안내를 띄운 UI 관리자 — 사라질 때(EndPlay) 폰의 컨트롤러가 이미 없어도 안내를 지울 수 있게 기억 */
	TWeakObjectPtr<UJY_UIManagerSubsystem> PromptUIManager;
};
