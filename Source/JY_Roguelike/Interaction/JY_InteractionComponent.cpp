#include "Interaction/JY_InteractionComponent.h"
#include "Interaction/JY_InteractableActor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/SphereComponent.h"
#include "Engine/LocalPlayer.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "UI/ViewModels/JY_InteractionViewModel.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "System/JY_GameplayTags.h"

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
/*
 * 범위(구)는 물체가 갖고, 겹침은 폰 몸체(루트 캡슐) 쪽에서 받는다 — 감지부터 E까지 이 컴포넌트 한곳에서 흐른다.
 * 겹침은 양쪽 컴포넌트에 모두 알려진다(PrimitiveComponent.cpp BeginComponentOverlap·EndComponentOverlap).
 * 초기 겹침(UpdateInitialOverlaps)은 BeginPlay 뒤에 계산되므로 여기서 구독하면 놓치지 않는다(Actor.cpp:4801).
 */
// [Server] [OwnClient] [SimulatedProxy]
void UJY_InteractionComponent::BeginPlay() //override
{
	Super::BeginPlay();

	UPrimitiveComponent* OwnerBody = CastChecked<UPrimitiveComponent>(GetOwner()->GetRootComponent());
	OwnerBody->OnComponentBeginOverlap.AddDynamic(this, &UJY_InteractionComponent::HandleOwnerBeginOverlap);
	OwnerBody->OnComponentEndOverlap.AddDynamic(this, &UJY_InteractionComponent::HandleOwnerEndOverlap);
}

/*
 * 겹침은 서버·남의 클라에서도 일어난다 — 내가 조종하는 폰일 때만 후보에 넣는다 (대상 고르기는 로컬 일).
 * 물체의 메시 등 다른 부품과의 겹침은 제외 — "범위 안"은 그 물체의 InteractionRange와 겹친 것만.
 */
// [Server] [OwnClient] [SimulatedProxy]
void UJY_InteractionComponent::HandleOwnerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AJY_InteractableActor* InteractableActor = Cast<AJY_InteractableActor>(OtherActor);
	if (InteractableActor == nullptr || OtherComp != InteractableActor->GetInteractionRange())
		return;

	if (CastChecked<APawn>(GetOwner())->IsLocallyControlled() == false)
		return;

	AddCandidate(InteractableActor);
}

/*
 * 나갈 때는 조종 여부를 묻지 않는다 — 폰이 죽거나 맵 이동으로 컨트롤러가 먼저 떨어진 뒤 나가도 후보·안내가 남지 않게
 * (후보에 없으면 아무 일 없음).
 */
// [Server] [OwnClient] [SimulatedProxy]
void UJY_InteractionComponent::HandleOwnerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AJY_InteractableActor* InteractableActor = Cast<AJY_InteractableActor>(OtherActor);
	if (InteractableActor == nullptr || OtherComp != InteractableActor->GetInteractionRange())
		return;

	RemoveCandidate(InteractableActor);
}

// [OwnClient]
void UJY_InteractionComponent::AddCandidate(AJY_InteractableActor* InteractableActor)
{
	Candidates.Add(InteractableActor);
	RefreshFocus();
}

// [OwnClient]
void UJY_InteractionComponent::RemoveCandidate(AJY_InteractableActor* InteractableActor)
{
	Candidates.Remove(InteractableActor);
	RefreshFocus();
}

// [OwnClient]
void UJY_InteractionComponent::RefreshFocus()
{
	auto IsInvalidCandidate = [](const TWeakObjectPtr<AJY_InteractableActor>& Candidate) { return Candidate.IsValid() == false; };
	Candidates.RemoveAll(IsInvalidCandidate);

	AJY_InteractableActor* NewFocus = Candidates.Num() > 0 ? Candidates.Last().Get() : nullptr;
	if (NewFocus == FocusTarget.Get())
		return;

	FocusTarget = NewFocus;

	/*
	 * 맵 이동 때 폰은 컨트롤러가 먼저 떨어진 뒤 겹침이 끝난다:
	 * LoadMap → DestroyActor(폰)(UnrealEngine.cpp:16247) → APawn::Destroyed에서 컨트롤러 해제(Pawn.cpp:552)
	 * → 그다음 겹침 정리(LevelActor.cpp:981) → 여기. 이때 GetUIManager()는 nullptr이라 안내를 띄운 관리자로 지운다.
	 * ViewModel은 LocalPlayer에 남아 다음 맵 HUD로 넘어가므로, 못 지우면 던전에 "[E] 포털"이 남는다.
	 */
	UJY_UIManagerSubsystem* UIManager = GetUIManager();
	if (UIManager == nullptr)
	{
		UIManager = PromptUIManager.Get();
	}

	SetPromptName(UIManager, NewFocus != nullptr ? NewFocus->GetDisplayName() : FText::GetEmpty());
}

/* "지금 초점"은 복제 값이 아니라 내 컴퓨터에서만 정해지는 상태 → 가진 쪽이 ViewModel에 직접 넣는다 */
// [OwnClient]
void UJY_InteractionComponent::SetPromptName(UJY_UIManagerSubsystem* UIManager, const FText& Name)
{
	if (UIManager == nullptr)
		return;

	UIManager->GetOrCreateViewModel<UJY_InteractionViewModel>()->SetFocusedName(Name);
	PromptUIManager = Name.IsEmpty() == false ? UIManager : nullptr;
}

/* 서버에서 나감 판정이 와도 여기서 nullptr — 데디 서버엔 로컬 플레이어가 없다 (LocalPlayer.h:377) */
// [Server] [OwnClient]
UJY_UIManagerSubsystem* UJY_InteractionComponent::GetUIManager() const
{
	const APawn* Pawn = GetOwner<APawn>();
	const APlayerController* PC = Pawn != nullptr ? Pawn->GetController<APlayerController>() : nullptr;
	return PC != nullptr ? ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(PC->GetLocalPlayer()) : nullptr;
}

/*
 * 화면형(기록판·제단): 내 화면만 바뀌므로 서버에 묻지 않고 바로 연다. 값을 바꾸는 행동(구매)은 그 화면이 서버에 요청한다.
 * 서버형(포털): 클라의 "가까이 있다" 판정을 믿지 않고 반드시 서버를 거친다.
 */
// [OwnClient]
void UJY_InteractionComponent::Interact()
{
	AJY_InteractableActor* Target = FocusTarget.Get();
	if (Target == nullptr)
		return;

	switch (Target->GetInteractionType())
	{
	case EJY_InteractionType::OpenScreen:
	{
		/* 기획 실수(화면 미지정)를 조용히 넘기지 않는다 — 에디터에 어느 물체인지 띄우고 아무것도 안 함 */
		if (ensureMsgf(Target->GetScreenClass().IsNull() == false, TEXT("%s: InteractionType이 OpenScreen인데 ScreenClass가 비어 있음"), *Target->GetName()) == false)
			return;

		UJY_UIManagerSubsystem* UIManager = GetUIManager();
		if (UIManager == nullptr)
			return;

		/* BeginPlay에서 미리 로드해 둬 여기선 바로 돌려받는다 */
		TSubclassOf<UJY_ScreenWidget> ScreenClass = Target->GetScreenClass().LoadSynchronous();

		/* 화면이 켜지기 전에 "누가 열었나"를 넣는다 — 켜질 때(NativeOnActivated) 화면이 바로 쓸 수 있게 */
		auto SetSourceBeforeActivate = [Target](UJY_ScreenWidget& Screen) { Screen.SetSourceActor(Target); };
		UIManager->PushScreen(JYGameplayTags::UI_Layer_GameMenu, ScreenClass, SetSourceBeforeActivate);
		return;
	}
	case EJY_InteractionType::ServerAction:
	{
		if (Target->GetConfirmText().IsEmpty() == true)
		{
			Server_Interact(Target);
			return;
		}

		UJY_UIManagerSubsystem* UIManager = GetUIManager();
		if (UIManager == nullptr)
			return;

		/* 팝업이 떠 있는 동안 물체가 사라질 수 있어 weak로 넘긴다 — 결과가 올 때 살아 있는지 확인 */
		FJY_OnConfirmResult OnResult = FJY_OnConfirmResult::CreateUObject(this, &UJY_InteractionComponent::HandleConfirmResult, TWeakObjectPtr<AJY_InteractableActor>(Target));
		UIManager->ShowConfirm(Target->GetDisplayName(), Target->GetConfirmText(), MoveTemp(OnResult));
		return;
	}
	}
}

/* 확인 팝업 결과 — 확인일 때만 서버에 요청 */
// [OwnClient]
void UJY_InteractionComponent::HandleConfirmResult(bool bConfirmed, TWeakObjectPtr<AJY_InteractableActor> Target)
{
	if (bConfirmed == false || Target.IsValid() == false)
		return;

	Server_Interact(Target.Get());
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
