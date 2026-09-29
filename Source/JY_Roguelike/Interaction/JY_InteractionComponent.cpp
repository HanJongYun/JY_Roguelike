#include "Interaction/JY_InteractionComponent.h"
#include "Interaction/JY_InteractableActor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_InteractionComponent)

/* Server RPC를 보내려면 복제되는 컴포넌트여야 한다 (주인 = 내 폰 → 내 연결로 전송) */
UJY_InteractionComponent::UJY_InteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

/*
 * 가장 가까운 대상을 매 프레임 계산하지 않고 "마지막으로 들어온 대상"을 초점으로 둔다.
 * 허브 대상은 3개이고 범위가 겹치지 않게 배치 — 거리 폴링(Tick)은 비용만 든다. 겹치게 늘어나면 그때 거리 비교를 추가.
 */
// [OwnClient]
void UJY_InteractionComponent::AddCandidate(AJY_InteractableActor* Interactable)
{
	Candidates.Remove(Interactable);
	Candidates.Add(Interactable);
	RefreshFocus();
}

// [OwnClient]
void UJY_InteractionComponent::RemoveCandidate(AJY_InteractableActor* Interactable)
{
	Candidates.Remove(Interactable);
	RefreshFocus();
}

// [OwnClient]
void UJY_InteractionComponent::RefreshFocus()
{
	Candidates.RemoveAll([](const TWeakObjectPtr<AJY_InteractableActor>& Candidate) { return Candidate.IsValid() == false; });

	FocusedInteractable = Candidates.Num() > 0 ? Candidates.Last().Get() : nullptr;
}

/* 클라의 "가까이 있다" 판정을 믿지 않고 반드시 서버를 거친다 */
// [OwnClient]
void UJY_InteractionComponent::Interact()
{
	AJY_InteractableActor* Target = FocusedInteractable.Get();
	if (Target == nullptr)
		return;

	Server_Interact(Target);
}

/* 개조 클라가 먼 대상을 보내도 여기서 거절된다 */
// [Server]
void UJY_InteractionComponent::Server_Interact_Implementation(AJY_InteractableActor* Target)
{
	if (Target == nullptr)
		return;

	APawn* Pawn = GetOwner<APawn>();
	if (Pawn == nullptr || Target->IsWithinInteractRange(Pawn) == false)
		return;

	Target->OnServerInteract(Pawn->GetController<APlayerController>());
}
