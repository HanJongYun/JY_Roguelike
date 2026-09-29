#include "UI/Screens/JY_MetaEntryWidget.h"
#include "UI/Foundation/JY_ButtonBase.h"
#include "UI/ViewModels/JY_MetaEntryViewModel.h"
#include "Player/JY_PlayerController.h"
#include "Interaction/JY_InteractableActor.h"
#include "View/MVVMView.h"
#include "Components/Image.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MetaEntryWidget)

// [OwnClient]
void UJY_MetaEntryWidget::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	BuyButton->OnClicked().AddUObject(this, &UJY_MetaEntryWidget::HandleBuyClicked);
}

/* 같은 클래스 ViewModel이 항목마다 따로라 Resolver(클래스당 1개)가 아니라 Manual로 넣는다 (UIManager 헤더의 ViewModel 규칙 3) */
// [OwnClient]
void UJY_MetaEntryWidget::SetEntry(UJY_MetaEntryViewModel* InViewModel, AJY_InteractableActor* InSeller)
{
	EntryViewModel = InViewModel;
	Seller = InSeller;

	/* 아이콘은 바뀌지 않아 바인딩 대신 1번 설정 — Soft라 필요할 때 비동기로 읽힌다 */
	if (InViewModel != nullptr)
	{
		IconImage->SetBrushFromSoftTexture(InViewModel->GetIcon());
	}

	UMVVMView* View = GetExtension<UMVVMView>();
	if (View == nullptr)
		return;

	View->SetViewModelByClass(InViewModel);
}

/*
 * 구매 가능 여부·가격·판매 대상은 서버가 다시 검증한다 — 버튼 비활성은 표시일 뿐.
 * 판매 대상이 없을 때 버튼을 끄지 않고 여기서 멈추는 이유: 버튼 활성은 WBP 바인딩(bCanBuy → bIsEnabled)이 계속 덮어쓴다.
 */
// [OwnClient]
void UJY_MetaEntryWidget::HandleBuyClicked()
{
	AJY_PlayerController* PC = GetOwningPlayer<AJY_PlayerController>();
	if (PC == nullptr || EntryViewModel.IsValid() == false || Seller.IsValid() == false)
		return;

	PC->Server_BuyMetaUpgrade(EntryViewModel->GetUpgradeTag(), Seller.Get());
}
