#include "Interaction/JY_InteractionComponent.h"
#include "Interaction/JY_InteractableActor.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "System/JY_GameplayTags.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

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

	AJY_InteractableActor* NewFocus = Candidates.Num() > 0 ? Candidates.Last().Get() : nullptr;
	if (NewFocus == FocusedInteractable.Get())
		return;

	if (FocusedInteractable.IsValid() == true)
	{
		FocusedInteractable->SetPromptVisible(false);
	}

	FocusedInteractable = NewFocus;

	if (NewFocus != nullptr)
	{
		NewFocus->SetPromptVisible(true);
	}
}

/*
 * 화면형은 내 화면만 바뀌므로 서버에 묻지 않는다(구매 같은 값 변경은 그 화면이 서버에 요청).
 * 서버형은 반드시 서버를 거친다 — 클라의 "가까이 있다" 판정을 믿지 않는다.
 */
// [OwnClient]
void UJY_InteractionComponent::Interact()
{
	AJY_InteractableActor* Target = FocusedInteractable.Get();
	if (Target == nullptr)
		return;

	const APawn* Pawn = GetOwner<APawn>();
	const APlayerController* PC = Pawn != nullptr ? Pawn->GetController<APlayerController>() : nullptr;
	UJY_UIManagerSubsystem* UIManager = PC != nullptr ? ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(PC->GetLocalPlayer()) : nullptr;
	if (UIManager == nullptr)
		return;

	if (Target->GetInteractionType() == EJY_InteractionType::OpenScreen)
	{
		UIManager->PushScreen(JYGameplayTags::UI_Layer_GameMenu, Target->GetScreenClass().LoadSynchronous());
		return;
	}

	if (Target->GetConfirmText().IsEmpty() == true)
	{
		Server_Interact(Target);
		return;
	}

	UIManager->ShowConfirm(Target->GetDisplayName(), Target->GetConfirmText(), FJY_OnConfirmResult::CreateUObject(this, &UJY_InteractionComponent::HandleConfirmResult, TWeakObjectPtr<AJY_InteractableActor>(Target)));
}

// [OwnClient]
void UJY_InteractionComponent::HandleConfirmResult(bool bConfirmed, TWeakObjectPtr<AJY_InteractableActor> Target)
{
	if (bConfirmed == false || Target.IsValid() == false)
		return;

	Server_Interact(Target.Get());
}

/* 개조 클라가 먼 대상·화면형 대상을 보내도 여기서 거절된다 */
// [Server]
void UJY_InteractionComponent::Server_Interact_Implementation(AJY_InteractableActor* Target)
{
	if (Target == nullptr || Target->GetInteractionType() != EJY_InteractionType::ServerAction)
		return;

	APawn* Pawn = GetOwner<APawn>();
	if (Pawn == nullptr || Target->IsWithinInteractRange(Pawn) == false)
		return;

	Target->OnServerInteract(Pawn->GetController<APlayerController>());
}
