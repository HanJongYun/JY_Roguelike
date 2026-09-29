#include "UI/Screens/JY_MetaAltarScreen.h"
#include "UI/Screens/JY_MetaEntryWidget.h"
#include "UI/Foundation/JY_ButtonBase.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "UI/ViewModels/JY_ProgressViewModel.h"
#include "Player/JY_PlayerController.h"
#include "Components/DynamicEntryBox.h"
#include "Engine/LocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MetaAltarScreen)

/* 메뉴 입력 · 뒤로가기로 닫힘 · 정지 안 함(허브에서 구경하는 화면) */
UJY_MetaAltarScreen::UJY_MetaAltarScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	InputMode = EJY_ScreenInputMode::Menu;
}

/*
 * 항목 위젯은 화면이 처음 만들어질 때 1번만 채운다. 화면은 층 스택의 위젯 풀에 남아 다시 열 때 같은 인스턴스를 쓰고
 * (CommonActivatableWidgetContainer.cpp:193 GetOrCreateInstance), 값은 항목 ViewModel이 바뀔 때만 갱신된다.
 */
// [OwnClient]
void UJY_MetaAltarScreen::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	RefundButton->OnClicked().AddUObject(this, &UJY_MetaAltarScreen::HandleRefundClicked);

	UJY_UIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(GetOwningLocalPlayer());
	if (UIManager == nullptr)
		return;

	for (UJY_MetaEntryViewModel* EntryViewModel : UIManager->GetOrCreateViewModel<UJY_ProgressViewModel>()->GetMetaEntries())
	{
		UJY_MetaEntryWidget* EntryWidget = EntryBox->CreateEntry<UJY_MetaEntryWidget>();
		if (EntryWidget != nullptr)
		{
			EntryWidget->SetEntryViewModel(EntryViewModel);
		}
	}
}

// [OwnClient]
void UJY_MetaAltarScreen::HandleRefundClicked()
{
	UJY_UIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(GetOwningLocalPlayer());
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
