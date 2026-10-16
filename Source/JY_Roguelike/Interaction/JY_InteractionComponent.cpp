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
 * 가장 가까운 대상을 매 프레임 계산하지 않고 "마지막으로 들어온 대상"을 초점으로 둔다.
 * 로비 대상은 3개이고 범위가 겹치지 않게 배치 — 거리 폴링(Tick)은 비용만 든다. 겹치게 늘어나면 그때 거리 비교를 추가.
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
 * 물체의 메시 등 다른 부품과의 겹침은 제외 — 상호작용 인터페이스를 구현한 범위 컴포넌트와 겹친 것만.
 */
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

/*
 * 나갈 때는 조종 여부를 묻지 않는다 — 폰이 죽거나 맵 이동으로 컨트롤러가 먼저 떨어진 뒤 나가도 후보·안내가 남지 않게
 * (후보에 없으면 아무 일 없음).
 */
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

/*
 * Lyra와 같은 흐름(LyraGameplayAbility_Interact.cpp:114 TriggerInteraction) — 이벤트 데이터에 대상 액터를 실어 GA를 발동.
 * 조종 클라에서 이벤트로 발동한 예측 GA는 이벤트 데이터째 서버에도 발동을 요청한다 (AbilitySystemComponent_Abilities.cpp:1937 ServerTryActivateAbilityWithEventData)
 */
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
