#include "UI/Foundation/JY_PrimaryLayout.h"
#include "System/JY_GameplayTags.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "UI/Common/JY_NotificationWidget.h"

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

/*
 * 부르는 쪽(UIManager 등)은 종류만 넘기고, 어느 위젯이 그 종류를 맡는지는 틀만 안다.
 * 종류가 늘면: enum 값 1줄 → BindWidget 1개 → 여기 case 1줄 → WBP NotificationOverlay에 같은 이름 위젯 배치. 부르는 쪽은 그대로.
 */
// [OwnClient]
UJY_NotificationWidget* UJY_PrimaryLayout::GetNotificationWidget(EJY_NotificationType Type) const
{
	switch (Type)
	{
	case EJY_NotificationType::Banner:
		return BannerWidget;
	case EJY_NotificationType::Toast:
		return ToastWidget;
	}

	/* enum에 값을 추가하고 case를 빠뜨렸을 때 — 에디터에 경고를 띄우고 알림은 버린다 */
	ensureMsgf(false, TEXT("알림 종류 %d에 맞는 칸이 없음 — BindWidget과 case 추가"), static_cast<int32>(Type));
	return nullptr;
}
