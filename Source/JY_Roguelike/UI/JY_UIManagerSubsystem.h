#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"
#include "UI/JY_UITypes.h"
#include "JY_UIManagerSubsystem.generated.h"

class APlayerController;
class UJY_GameData;
class UJY_PrimaryLayout;
class UJY_ScreenWidget;

/* 로컬 플레이어의 화면을 관리 — 클라에만 있다(데디 서버엔 로컬 플레이어가 없음) */
UCLASS()
class UJY_UIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

public:
	/* 화면을 해당 층 스택 맨 위에 올림 (예: UI.Layer.Menu). InitFunc는 화면이 켜지기 전에 한 번 불린다 */
	UJY_ScreenWidget* PushScreen(FGameplayTag LayerTag, TSubclassOf<UJY_ScreenWidget> ScreenClass, TFunctionRef<void(UJY_ScreenWidget&)> InitFunc = [](UJY_ScreenWidget&) {});

	/* 확인 팝업을 Modal 층에 띄우고 결과를 OnResult로 돌려줌 */
	void ShowConfirm(const FText& Title, const FText& Body, FJY_OnConfirmResult OnResult);

	/* 정지 화면이 켜지거나 꺼질 때 호출 — 목록이 비었다↔안 비었다 바뀔 때만 서버에 정지 요청 */
	void SetPauseScreenActive(UJY_ScreenWidget* Screen, bool bActive);

	/* 요청한 ViewModel을 플레이어당 하나로 꺼냄 (없으면 생성) — 어느 화면·코드가 요청해도 같은 인스턴스 */
	UMVVMViewModelBase* GetOrCreateViewModel(TSubclassOf<UMVVMViewModelBase> ViewModelClass);

	template <typename T>
	T* GetOrCreateViewModel() { return Cast<T>(GetOrCreateViewModel(T::StaticClass())); }

private:
	/* 화면 틀을 만들어 이 플레이어 화면에 붙이고 HUD까지 올림 */
	void CreatePrimaryLayout(APlayerController* PC);

	/* HUD를 Game 층에 올림 */
	void PushHUD(const UJY_GameData& GameData);

	/* 화면 틀을 화면에서 떼고 버림 */
	void DestroyPrimaryLayout();

private:
	UPROPERTY(Transient)
	TObjectPtr<UJY_PrimaryLayout> PrimaryLayout;

	/* 틀을 만든 PC — 맵 이동 후엔 위젯의 GetOwningPlayer가 새 PC를 돌려줘 비교에 못 쓴다 */
	TWeakObjectPtr<APlayerController> LayoutOwner;

	/* 지금 켜져 있는 정지 화면들 */
	TSet<TWeakObjectPtr<UJY_ScreenWidget>> PauseScreens;

	/* 만든 ViewModel 보관 — 키 = ViewModel 클래스, 클래스당 1개 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UMVVMViewModelBase>> ViewModels;
};
