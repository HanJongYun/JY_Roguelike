#include "UI/Screens/JY_MetaEntryWidget.h"
#include "UI/Foundation/JY_ButtonBase.h"
#include "UI/ViewModels/JY_MetaEntryViewModel.h"
#include "Player/JY_PlayerController.h"
#include "View/MVVMView.h"
#include "Components/Image.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MetaEntryWidget)

/* 줄 위젯은 보관함(풀)에서 다시 쓰여도 NativeOnInitialized는 처음 만들 때 1번 — 클릭 구독이 겹치지 않는다 */
// [OwnClient]
void UJY_MetaEntryWidget::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	BuyButton->OnClicked().AddUObject(this, &ThisClass::HandleBuyClicked);
}

/*
 * 같은 클래스 ViewModel이 줄마다 따로라 Resolver(클래스당 1개)가 아니라 Manual — 화면이 줄마다 직접 넣는다.
 * 넣는 순간 이 위젯의 MVVM View가 바인딩 5개를 그 ViewModel 값으로 한 번 실행하고, 이후엔 알림이 올 때마다 갱신.
 */
// [OwnClient]
void UJY_MetaEntryWidget::SetEntry(UJY_MetaEntryViewModel* InEntryViewModel, AActor* InAltar)
{
	EntryViewModel = InEntryViewModel;
	Altar = InAltar;

	/* 아이콘은 바뀌지 않아 바인딩 대신 1번 설정 — Soft 참조라 필요할 때 비동기로 읽힌다 */
	IconImage->SetBrushFromSoftTexture(InEntryViewModel->GetIcon());

	/* WBP에 ViewModel이 없으면(설정 실수) 바인딩이 안 걸려 줄이 기본 글자 그대로 보인다 */
	UMVVMView* View = GetExtension<UMVVMView>();
	if (ensureMsgf(View != nullptr, TEXT("%s: WBP에 ViewModel \"MetaEntry\"(Manual)가 없음"), *GetClass()->GetName()) == false)
		return;

	View->SetViewModelByClass(InEntryViewModel);
}

/*
 * 버튼이 꺼져 있는 건 표시일 뿐 — 가격·최대 단계·보유량·제단·거리는 서버가 다시 검증한다(AJY_GameMode::TryBuyMetaUpgrade).
 * 제단이 사라졌으면 nullptr이 가고 서버가 거절한다.
 */
// [OwnClient]
void UJY_MetaEntryWidget::HandleBuyClicked()
{
	AJY_PlayerController* PC = GetOwningPlayer<AJY_PlayerController>();
	const FGameplayTag& UpgradeTag = EntryViewModel->GetUpgradeTag();
	PC->Server_BuyMetaUpgrade(UpgradeTag, Altar.Get());
}
