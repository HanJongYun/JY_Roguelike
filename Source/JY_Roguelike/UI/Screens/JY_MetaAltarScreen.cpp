#include "UI/Screens/JY_MetaAltarScreen.h"
#include "UI/Screens/JY_MetaEntryWidget.h"
#include "UI/Foundation/JY_ButtonBase.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "UI/ViewModels/JY_ProgressViewModel.h"
#include "UI/ViewModels/JY_MetaEntryViewModel.h"
#include "Player/JY_PlayerController.h"
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
 * 화면은 층 스택의 위젯 풀에 남아 다시 열 때 같은 인스턴스를 쓴다(CommonActivatableWidgetContainer.cpp:193 GetOrCreateInstance).
 * 항목 위젯은 모자란 만큼만 만든다 → 두 번째부터는 아무것도 만들지 않고, 값은 항목 ViewModel이 바뀔 때만 갱신된다.
 * 열 때마다 확인하는 이유: PlayerState가 아직 연결되기 전에 처음 열렸다면 항목 ViewModel이 아직 없다.
 */
// [OwnClient]
void UJY_MetaAltarScreen::NativeOnActivated() //override
{
	Super::NativeOnActivated();

	UJY_UIManagerSubsystem* UIManager = GetUIManager();
	if (UIManager == nullptr)
		return;

	const TArray<TObjectPtr<UJY_MetaEntryViewModel>>& EntryViewModels = UIManager->GetOrCreateViewModel<UJY_ProgressViewModel>()->GetMetaEntries();
	for (int32 Index = EntryBox->GetNumEntries(); Index < EntryViewModels.Num(); ++Index)
	{
		UJY_MetaEntryWidget* EntryWidget = EntryBox->CreateEntry<UJY_MetaEntryWidget>();
		if (EntryWidget != nullptr)
		{
			EntryWidget->SetEntryViewModel(EntryViewModels[Index]);
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

	AJY_PlayerController* PC = GetOwningPlayer<AJY_PlayerController>();
	if (PC == nullptr)
		return;

	PC->Server_RefundAllMetaUpgrades();
}
