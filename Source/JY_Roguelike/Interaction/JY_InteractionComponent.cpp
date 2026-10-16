#include "Interaction/JY_InteractionComponent.h"
#include "Interfaces/JY_InteractableTarget.h"
#include "System/JY_GameplayTags.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "UI/ViewModels/JY_InteractionViewModel.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_InteractionComponent)

UJY_InteractionComponent::UJY_InteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/*
 * 범위가 겹치지 않게 배치해 거리 폴링 없이 "마지막으로 들어온 대상"을 초점으로 둔다.
 * 초기 겹침은 BeginPlay 뒤에 계산되므로 여기서 구독해도 놓치지 않는다 (Actor.cpp:4801)
 */
// [Server] [OwnClient] [SimulatedProxy]
void UJY_InteractionComponent::BeginPlay() //override
{
	Super::BeginPlay();

	UPrimitiveComponent* OwnerBody = CastChecked<UPrimitiveComponent>(GetOwner()->GetRootComponent());
	OwnerBody->OnComponentBeginOverlap.AddDynamic(this, &UJY_InteractionComponent::HandleOwnerBeginOverlap);
	OwnerBody->OnComponentEndOverlap.AddDynamic(this, &UJY_InteractionComponent::HandleOwnerEndOverlap);
}

/* 겹침은 서버·남의 클라에서도 일어난다 — 내가 조종하는 폰이 범위 컴포넌트와 겹친 것만 후보 */
// [Server] [OwnClient] [SimulatedProxy]
void UJY_InteractionComponent::HandleOwnerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<IJY_InteractableTarget>(OtherComp) == nullptr)
		return;

	if (CastChecked<APawn>(GetOwner())->IsLocallyControlled() == false)
		return;

	Candidates.Add(OtherComp);
	RefreshFocus();
}

/* 나갈 때는 조종 여부를 묻지 않는다 — 컨트롤러가 먼저 떨어진 뒤(사망·맵 이동) 나가도 후보·안내가 남지 않게 */
// [Server] [OwnClient] [SimulatedProxy]
void UJY_InteractionComponent::HandleOwnerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Cast<IJY_InteractableTarget>(OtherComp) == nullptr)
		return;

	Candidates.Remove(OtherComp);
	RefreshFocus();
}

// [OwnClient]
void UJY_InteractionComponent::RefreshFocus()
{
	auto IsInvalidCandidate = [](const TWeakObjectPtr<UPrimitiveComponent>& Candidate) { return Candidate.IsValid() == false; };
	Candidates.RemoveAll(IsInvalidCandidate);

	UPrimitiveComponent* NewFocus = Candidates.Num() > 0 ? Candidates.Last().Get() : nullptr;
	if (NewFocus == FocusTarget.Get())
		return;

	FocusTarget = NewFocus;

	/*
	 * 맵 이동 땐 컨트롤러가 먼저 떨어진 뒤 겹침이 끝나 GetUIManager()가 nullptr (Pawn.cpp:552) — 안내를 띄운 관리자로 지운다.
	 * 못 지우면 LocalPlayer의 ViewModel이 다음 맵으로 넘어가 던전에 "[E] 포털"이 남는다
	 */
	UJY_UIManagerSubsystem* UIManager = GetUIManager();
	if (UIManager == nullptr)
	{
		UIManager = PromptUIManager.Get();
	}

	const IJY_InteractableTarget* Target = Cast<IJY_InteractableTarget>(NewFocus);
	SetPromptName(UIManager, Target != nullptr ? Target->GetInteractionName() : FText::GetEmpty());
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

/* 확인 문구가 있는 대상(포털)은 내 화면에 팝업을 먼저 띄운다 — 팝업은 내 화면 일이라 GA 밖에서 */
// [OwnClient]
void UJY_InteractionComponent::Interact()
{
	UPrimitiveComponent* TargetComponent = FocusTarget.Get();
	const IJY_InteractableTarget* Target = Cast<IJY_InteractableTarget>(TargetComponent);
	if (Target == nullptr)
		return;

	if (Target->GetConfirmText().IsEmpty() == true)
	{
		TriggerInteraction(TargetComponent);
		return;
	}

	UJY_UIManagerSubsystem* UIManager = GetUIManager();
	if (UIManager == nullptr)
		return;

	/* 팝업이 떠 있는 동안 물체가 사라질 수 있어 weak로 넘긴다 — 결과가 올 때 살아 있는지 확인 */
	FJY_OnConfirmResult OnResult = FJY_OnConfirmResult::CreateUObject(this, &UJY_InteractionComponent::HandleConfirmResult, TWeakObjectPtr<UPrimitiveComponent>(TargetComponent));
	UIManager->ShowConfirm(Target->GetInteractionName(), Target->GetConfirmText(), MoveTemp(OnResult));
}

/* 확인 팝업 결과 — 확인일 때만 사용 */
// [OwnClient]
void UJY_InteractionComponent::HandleConfirmResult(bool bConfirmed, TWeakObjectPtr<UPrimitiveComponent> Target)
{
	if (bConfirmed == false || Target.IsValid() == false)
		return;

	TriggerInteraction(Target.Get());
}

/* 이벤트로 발동한 예측 GA는 이벤트 데이터째 서버에도 발동된다 (AbilitySystemComponent_Abilities.cpp:1937) */
// [OwnClient]
void UJY_InteractionComponent::TriggerInteraction(UPrimitiveComponent* Target) const
{
	APawn* Pawn = GetOwner<APawn>();
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (ASC == nullptr)
		return;

	FGameplayEventData Payload;
	Payload.EventTag = JYGameplayTags::Event_Interact;
	Payload.Instigator = Pawn;
	Payload.Target = Target->GetOwner();
	ASC->HandleGameplayEvent(Payload.EventTag, &Payload);
}
