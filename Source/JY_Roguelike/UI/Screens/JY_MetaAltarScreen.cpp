#include "UI/Screens/JY_MetaAltarScreen.h"
#include "UI/Screens/JY_MetaEntryWidget.h"
#include "UI/Foundation/JY_ButtonBase.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "UI/ViewModels/JY_ProgressViewModel.h"
#include "UI/ViewModels/JY_MetaEntryViewModel.h"
#include "Player/JY_PlayerController.h"
#include "Interaction/JY_InteractableActor.h"
#include "Components/DynamicEntryBox.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MetaAltarScreen)

// [OwnClient]
void UJY_MetaAltarScreen::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	RefundButton->OnClicked().AddUObject(this, &UJY_MetaAltarScreen::HandleRefundClicked);
	CloseButton->OnClicked().AddUObject(this, &UJY_MetaAltarScreen::HandleCloseClicked);
}

// [OwnClient]
void UJY_MetaAltarScreen::HandleCloseClicked()
{
	DeactivateWidget();
}

/*
 * 열 때마다 항목을 다시 채운다: Reset(false)는 항목 위젯을 지우지 않고 박스 풀에 돌려놓고(DynamicEntryBoxBase.cpp:36),
 * CreateEntry가 그 풀에서 다시 꺼낸다 → 두 번째부터는 위젯을 새로 만들지 않는다. 강화 수가 줄어도 남는 줄이 없다.
 * 열 때마다 하는 이유: PlayerState가 아직 연결되기 전에 처음 열렸다면 항목 ViewModel이 아직 없고, 판매 대상(이 화면을 연 제단)도 매번 다르다.
 * 상점·축복 목록도 이 모양을 그대로 쓴다.
 */
// [OwnClient]
void UJY_MetaAltarScreen::NativeOnActivated() //override
{
	Super::NativeOnActivated();

	UJY_UIManagerSubsystem* UIManager = GetUIManager();
	if (UIManager == nullptr)
		return;

	AJY_InteractableActor* Seller = Cast<AJY_InteractableActor>(GetSourceActor());

	EntryBox->Reset(false);
	for (UJY_MetaEntryViewModel* EntryViewModel : UIManager->GetOrCreateViewModel<UJY_ProgressViewModel>()->GetMetaEntries())
	{
		UJY_MetaEntryWidget* EntryWidget = EntryBox->CreateEntry<UJY_MetaEntryWidget>();
		if (EntryWidget != nullptr)
		{
			EntryWidget->SetEntry(EntryViewModel, Seller);
		}
	}
}

// [OwnClient]
void UJY_MetaAltarScreen::HandleRefundClicked()
{
	UJY_UIManagerSubsystem* UIManager = GetUIManager();
	if (UIManager == nullptr)
		return;

	UIManager->ShowConfirm(RefundConfirmTitle, RefundConfirmBody, FJY_OnConfirmResult::CreateUObject(this, &UJY_MetaAltarScreen::HandleRefundConfirmResult));
}

// [OwnClient]
void UJY_MetaAltarScreen::HandleRefundConfirmResult(bool bConfirmed)
{
	if (bConfirmed == false)
		return;

	/* 판매 대상(제단)이 없으면 서버가 어차피 거절한다 — 보내지 않는다 */
	AJY_InteractableActor* Seller = Cast<AJY_InteractableActor>(GetSourceActor());
	AJY_PlayerController* PC = GetOwningPlayer<AJY_PlayerController>();
	if (PC == nullptr || Seller == nullptr)
		return;

	PC->Server_RefundAllMetaUpgrades(Seller);
}
