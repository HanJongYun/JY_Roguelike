#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_InteractionComponent.generated.h"

class AJY_InteractableActor;

/* 플레이어 캐릭터에 붙어 가까운 상호작용 대상을 고르고 E 입력을 처리 */
UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_InteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_InteractionComponent();

public:
	/* 대상 범위에 들어옴 — 가장 최근에 들어온 대상이 초점 */
	void AddCandidate(AJY_InteractableActor* Interactable);

	/* 대상 범위에서 나감 — 초점이면 남은 대상 중 가장 최근 것으로 */
	void RemoveCandidate(AJY_InteractableActor* Interactable);

	/* E 입력 — 초점 대상의 동작 실행 */
	void Interact();

private:
	/* 초점이 바뀌었으면 안내 표시를 옮김 */
	void RefreshFocus();

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
};
