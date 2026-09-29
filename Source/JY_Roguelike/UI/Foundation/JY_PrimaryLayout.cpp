#include "UI/Foundation/JY_PrimaryLayout.h"
#include "System/JY_GameplayTags.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "UI/Common/JY_BannerWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_PrimaryLayout)

/*
 * 층 등록을 BP 그래프(Lyra 방식) 대신 C++ BindWidget으로 — 로직은 C++, WBP는 배치만.
 * 층이 늘면: 태그 추가 → BindWidget 변수 추가 → 여기 RegisterLayer 1줄 → WBP에 같은 이름 스택 배치.
 */
// [OwnClient]
void UJY_PrimaryLayout::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	RegisterLayer(JYGameplayTags::UI_Layer_Game, GameLayer);
	RegisterLayer(JYGameplayTags::UI_Layer_GameMenu, GameMenuLayer);
	RegisterLayer(JYGameplayTags::UI_Layer_Menu, MenuLayer);
	RegisterLayer(JYGameplayTags::UI_Layer_Modal, ModalLayer);
}

/*
 * 전환 애니메이션 시간을 0으로 — Lyra와 같은 이유: 전환 중에는 입력 가드가 켜져 포커스가 새 화면으로 늦게 넘어간다.
 * 디자이너 미리보기(IsDesignTime)에서는 등록하지 않는다.
 */
// [OwnClient]
void UJY_PrimaryLayout::RegisterLayer(FGameplayTag LayerTag, UCommonActivatableWidgetContainerBase* LayerWidget)
{
	if (IsDesignTime() == true)
		return;

	if (LayerWidget == nullptr)
		return;

	LayerWidget->SetTransitionDuration(0.f);
	Layers.Add(LayerTag, LayerWidget);
}

// [OwnClient]
UCommonActivatableWidgetContainerBase* UJY_PrimaryLayout::GetLayer(FGameplayTag LayerTag) const
{
	return Layers.FindRef(LayerTag);
}

// [OwnClient]
UJY_BannerWidget* UJY_PrimaryLayout::GetNotificationWidget(EJY_NotificationType Type) const
{
	switch (Type)
	{
	case EJY_NotificationType::Banner:
		return BannerWidget;
	case EJY_NotificationType::Toast:
		return ToastWidget;
	default:
		ensureMsgf(false, TEXT("알림 종류에 맞는 칸이 없음 — BindWidget과 case 추가"));
		return nullptr;
	}
}
