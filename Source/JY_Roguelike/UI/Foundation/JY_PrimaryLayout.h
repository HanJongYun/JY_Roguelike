#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "UI/JY_UITypes.h"
#include "JY_PrimaryLayout.generated.h"

class UCommonActivatableWidgetContainerBase;
class UCommonActivatableWidgetStack;
class UJY_NotificationWidget;

/* 화면 전체 틀 — 층마다 화면 스택을 하나씩 가진다 */
UCLASS(Abstract, meta = (DisableNativeTick))
class UJY_PrimaryLayout : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

public:
	/* 층 태그에 스택을 등록 */
	void RegisterLayer(FGameplayTag LayerTag, UCommonActivatableWidgetContainerBase* LayerWidget);

	/* 층 태그로 스택을 찾음 (없으면 nullptr) */
	UCommonActivatableWidgetContainerBase* GetLayer(FGameplayTag LayerTag) const;

	/* 알림 종류에 맞는 알림 칸을 찾음 */
	UJY_NotificationWidget* GetNotificationWidget(EJY_NotificationType Type) const;

protected:
	/* WBP_JY_PrimaryLayout 안의 같은 이름 위젯과 연결 (BindWidget). 아래일수록 화면 위에 그려짐 */

	/* UI.Layer.Game — HUD */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> GameLayer;

	/* UI.Layer.GameMenu — 기록판·제단 (던전: 축복 선택·상점) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> GameMenuLayer;

	/* 지역 이름 배너 — 스택이 아닌 미리 배치한 위젯 (NotificationOverlay 안) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_NotificationWidget> BannerWidget;

	/* 우상단 토스트 — 배너와 같은 클래스, 모양만 WBP_JY_Toast (NotificationOverlay 안) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UJY_NotificationWidget> ToastWidget;

	/* UI.Layer.Menu — 정지 메뉴·설정 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> MenuLayer;

	/* UI.Layer.Modal — 확인 팝업 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> ModalLayer;

private:
	UPROPERTY(Transient, meta = (Categories = "UI.Layer"))
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>> Layers;
};
