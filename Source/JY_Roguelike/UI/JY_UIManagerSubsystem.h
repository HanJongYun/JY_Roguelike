#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"
#include "UI/JY_UITypes.h"
#include "JY_UIManagerSubsystem.generated.h"

class APlayerController;
class UJY_PrimaryLayout;
class UJY_ScreenWidget;

/* 로컬 플레이어의 화면과 ViewModel을 관리 */
UCLASS()
class UJY_UIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

public:
	/* 화면을 해당 층 스택 맨 위에 올림 (예: UI.Layer.Menu) */
	UJY_ScreenWidget* PushScreen(FGameplayTag LayerTag, TSubclassOf<UJY_ScreenWidget> ScreenClass);

	/* 확인 팝업을 Modal 층에 띄우고 결과를 콜백으로 돌려줌 */
	void ShowConfirm(const FText& Title, const FText& Body, FJY_OnConfirmResult OnResult);

	/* 배너·토스트 알림 (틀이 아직 없으면 생긴 뒤 표시) */
	void ShowNotification(EJY_NotificationType Type, const FText& Message);

	/* 정지 화면이 켜지거나 꺼질 때 호출 — 목록이 비었다↔안 비었다 바뀔 때만 서버에 정지 요청 */
	void SetPauseScreenActive(UJY_ScreenWidget* Screen, bool bActive);

	/* 요청한 ViewModel을 플레이어당 하나로 꺼냄 (없으면 생성) — Resolver와 C++이 같은 인스턴스를 받는다 */
	UMVVMViewModelBase* GetOrCreateViewModel(TSubclassOf<UMVVMViewModelBase> ViewModelClass);

	template <typename T>
	T* GetOrCreateViewModel() { return Cast<T>(GetOrCreateViewModel(T::StaticClass())); }


private:
	/* 화면 틀을 만들어 이 플레이어 화면에 붙임 */
	void CreatePrimaryLayout(APlayerController* PC);

	/* 화면 틀을 화면에서 떼고 버림 */
	void DestroyPrimaryLayout();

	/* 켜진 화면이 없을 때의 기본 입력(게임, 커서 표시)을 적용 */
	void ApplyDefaultInputConfig(APlayerController* PC);

private:
	UPROPERTY(Transient)
	TObjectPtr<UJY_PrimaryLayout> PrimaryLayout;

	/* 지금 켜져 있는 정지 화면들 — 개수 대신 목록: 켜짐·꺼짐 알림이 두 번 오거나 빠져도 어긋나지 않음 */
	TSet<TWeakObjectPtr<UJY_ScreenWidget>> PauseScreens;

	/* 틀이 생기기 전에 온 알림 (접속 직후 서버 배너 등) — 틀 생성 직후 표시 */
	TArray<TPair<EJY_NotificationType, FText>> PendingNotifications;

	/* 처음 열 때 끊기지 않게 미리 비동기로 올려 둔 화면 클래스들 (핸들이 살아 있는 동안 메모리 유지) */
	TSharedPtr<struct FStreamableHandle> PreloadHandle;

	/* 키 = 요청한 C++ ViewModel 클래스 (값은 GameData 교체 표의 BP 자식일 수 있음) */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UMVVMViewModelBase>> ViewModels;
};
