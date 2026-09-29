#include "UI/Screens/JY_MetaEntryWidget.h"
#include "UI/Foundation/JY_ButtonBase.h"
#include "UI/ViewModels/JY_MetaEntryViewModel.h"
#include "Player/JY_PlayerController.h"
#include "View/MVVMView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MetaEntryWidget)

// [OwnClient]
void UJY_MetaEntryWidget::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	BuyButton->OnClicked().AddUObject(this, &UJY_MetaEntryWidget::HandleBuyClicked);
}

/* 같은 클래스 ViewModel이 항목마다 따로라 Resolver(클래스당 1개)가 아니라 Manual로 넣는다 */
// [OwnClient]
void UJY_MetaEntryWidget::SetEntryViewModel(UJY_MetaEntryViewModel* InViewModel)
{
	EntryViewModel = InViewModel;

	UMVVMView* View = GetExtension<UMVVMView>();
	if (View == nullptr)
		return;

	View->SetViewModelByClass(InViewModel);
}

/* 구매 가능 여부·가격은 서버가 다시 검증한다 — 버튼 비활성은 표시일 뿐 */
// [OwnClient]
void UJY_MetaEntryWidget::HandleBuyClicked()
{
	AJY_PlayerController* PC = GetOwningPlayer<AJY_PlayerController>();
	if (PC == nullptr || EntryViewModel.IsValid() == false)
		return;

	PC->Server_BuyMetaUpgrade(EntryViewModel->GetUpgradeTag());
}
