#include "UI/Screens/JY_MetaAltarScreen.h"
#include "UI/Screens/JY_MetaEntryWidget.h"
#include "UI/Foundation/JY_ButtonBase.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "Player/JY_PlayerController.h"
#include "UI/ViewModels/JY_MetaUpgradeViewModel.h"
#include "UI/ViewModels/JY_MetaEntryViewModel.h"
#include "Interaction/JY_InteractableActor.h"
#include "Components/DynamicEntryBox.h"
#include "Engine/LocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MetaAltarScreen)

// [OwnClient]
void UJY_MetaAltarScreen::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	RefundButton->OnClicked().AddUObject(this, &ThisClass::HandleRefundClicked);
	CloseButton->OnClicked().AddUObject(this, &ThisClass::HandleCloseClicked);
}

/*
 * 열 때마다 줄을 다시 채운다. Reset(false)는 줄 위젯을 지우지 않고 박스의 보관함(풀)에 돌려놓고(DynamicEntryBoxBase.cpp:38),
 * CreateEntry가 보관함에서 다시 꺼낸다 → 두 번째로 열 때부터는 줄 위젯을 새로 만들지 않는다.
 * 처음 1번(NativeOnInitialized)만 채우지 않는 이유: PlayerState가 연결되기 전에 처음 열렸다면 줄 ViewModel이 아직 0개다.
 */
// [OwnClient]
void UJY_MetaAltarScreen::NativeOnActivated() //override
{
	Super::NativeOnActivated();

	UJY_UIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(GetOwningLocalPlayer());
	UJY_MetaUpgradeViewModel* MetaUpgradeViewModel = UIManager->GetOrCreateViewModel<UJY_MetaUpgradeViewModel>();
	const TArray<TObjectPtr<UJY_MetaEntryViewModel>>& EntryViewModels = MetaUpgradeViewModel->GetEntries();

	/* 구매 요청에 실을 제단 — Interact가 켜지기 전에 넣어 둠(SetSourceActor) */
	AJY_InteractableActor* Altar = Cast<AJY_InteractableActor>(GetSourceActor());

	EntryBox->Reset(false);
	for (UJY_MetaEntryViewModel* EntryViewModel : EntryViewModels)
	{
		UJY_MetaEntryWidget* EntryWidget = EntryBox->CreateEntry<UJY_MetaEntryWidget>();
		EntryWidget->SetEntry(EntryViewModel, Altar);
	}
}

/* 되돌릴 수 없는 행동이라 4장 확인 팝업을 거친다 */
// [OwnClient]
void UJY_MetaAltarScreen::HandleRefundClicked()
{
	UJY_UIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(GetOwningLocalPlayer());
	FJY_OnConfirmResult OnResult = FJY_OnConfirmResult::CreateUObject(this, &ThisClass::HandleRefundConfirmResult);
	UIManager->ShowConfirm(RefundConfirmTitle, RefundConfirmBody, MoveTemp(OnResult));
}

/* 제단·거리 검증과 환불 합계 계산은 서버(AJY_GameMode::RefundAllMetaUpgrades) — 제단이 사라졌으면 nullptr이 가고 서버가 거절한다 */
// [OwnClient]
void UJY_MetaAltarScreen::HandleRefundConfirmResult(bool bConfirmed)
{
	if (bConfirmed == false)
		return;

	AJY_PlayerController* PC = GetOwningPlayer<AJY_PlayerController>();
	AJY_InteractableActor* Altar = Cast<AJY_InteractableActor>(GetSourceActor());
	PC->Server_RefundAllMetaUpgrades(Altar);
}

/* 마우스로 닫기 — Tab(뒤로가기)과 같은 길: 스택이 이 화면을 끄고 아래 화면(HUD) 입력 모드로 돌아간다 */
// [OwnClient]
void UJY_MetaAltarScreen::HandleCloseClicked()
{
	DeactivateWidget();
}
