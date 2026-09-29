#include "Interaction/JY_InteractionComponent.h"
#include "Interaction/JY_InteractableActor.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "UI/ViewModels/JY_InteractionViewModel.h"
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
 * 안내 ViewModel은 로컬 플레이어에 남아 맵을 넘어간다 — 컴포넌트가 사라질 때 지우지 않으면
 * 포털 범위 안에서 이동했을 때 던전 HUD에 "[E] 포털"이 남는다 (맵 이동 EndPlay는 겹침 종료를 보내지 않음, Actor.cpp:3233).
 * 사망·폰 교체로 사라질 때도 같은 이유로 지운다.
 */
// [OwnClient]
void UJY_InteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) //override
{
	SetPromptName(PromptUIManager.Get(), FText::GetEmpty());
	Candidates.Reset();
	FocusedInteractable.Reset();

	Super::EndPlay(EndPlayReason);
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

	FocusedInteractable = NewFocus;

	/* 빙의가 풀린 뒤(사망) 나가도 안내를 지울 수 있게, 컨트롤러가 없으면 안내를 띄운 관리자를 쓴다 */
	UJY_UIManagerSubsystem* UIManager = GetUIManager();
	if (UIManager == nullptr)
	{
		UIManager = PromptUIManager.Get();
	}

	SetPromptName(UIManager, NewFocus != nullptr ? NewFocus->GetDisplayName() : FText::GetEmpty());
}

/* 로컬 전용 상태라 ViewModel에 직접 값을 넣는다 (UIManager 헤더의 ViewModel 규칙 2) */
// [OwnClient]
void UJY_InteractionComponent::SetPromptName(UJY_UIManagerSubsystem* UIManager, const FText& Name)
{
	if (UIManager == nullptr)
		return;

	UIManager->GetOrCreateViewModel<UJY_InteractionViewModel>()->SetFocusedName(Name);
	PromptUIManager = Name.IsEmpty() == false ? UIManager : nullptr;
}

// [OwnClient]
UJY_UIManagerSubsystem* UJY_InteractionComponent::GetUIManager() const
{
	const APawn* Pawn = GetOwner<APawn>();
	const APlayerController* PC = Pawn != nullptr ? Pawn->GetController<APlayerController>() : nullptr;
	return PC != nullptr ? ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(PC->GetLocalPlayer()) : nullptr;
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

	UJY_UIManagerSubsystem* UIManager = GetUIManager();
	if (UIManager == nullptr)
		return;

	/* 화면에 "누가 열었나"를 넘긴다 — 제단 화면은 이 물체를 판매 대상으로 서버에 보낸다 */
	if (Target->OpensScreen() == true)
	{
		UIManager->PushScreen(JYGameplayTags::UI_Layer_GameMenu, Target->GetScreenClass().LoadSynchronous(), [Target](UJY_ScreenWidget& Screen)
		{
			Screen.SetSourceActor(Target);
		});
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
	if (Target == nullptr || Target->OpensScreen() == true)
		return;

	APawn* Pawn = GetOwner<APawn>();
	if (Pawn == nullptr || Target->IsWithinInteractRange(Pawn) == false)
		return;

	Target->OnServerInteract(Pawn->GetController<APlayerController>());
}
