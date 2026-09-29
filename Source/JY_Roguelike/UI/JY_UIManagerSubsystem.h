#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"
#include "UI/JY_UITypes.h"
#include "JY_UIManagerSubsystem.generated.h"

class APlayerController;
class UCommonActivatableWidgetContainerBase;
class UJY_GameData;
class UJY_PrimaryLayout;
class UJY_ScreenWidget;

/*
 * 로컬 플레이어의 화면과 ViewModel을 관리 — 클라에만 있다(데디 서버엔 로컬 플레이어가 없음)
 *
 * 맡는 일: ① 맵마다 화면 틀(PrimaryLayout)을 만들고 HUD를 올림 ② 화면 열기(PushScreen)·확인 팝업 ③ 배너·토스트 알림
 *          ④ 정지 화면 목록 → 서버 정지 요청 ⑤ ViewModel 보관(플레이어당 클래스별 1개)
 *
 * ViewModel에 값을 넣는 규칙 3가지 (새 화면을 만들 때 이 중 하나를 고른다)
 *   1. 복제 상태(체력·영혼석)   → ViewModel이 원본을 구독한다. 연결은 AJY_PlayerController::InitializeLocalUI 한 곳
 *   2. 로컬 전용 상태(상호작용 초점) → 그 상태를 가진 게임 코드가 GetOrCreateViewModel로 직접 값을 넣는다
 *   3. 목록 항목(제단 줄)       → 화면이 항목 위젯마다 ViewModel을 직접 넣는다 (WBP Creation Type = Manual)
 *   값 → 위젯: ViewModel의 FieldNotify 값이 바뀌면 WBP View Binding이 위젯 속성을 자동으로 바꾼다
 *
 * ViewModel 수명 규칙: ViewModel은 맵을 넘어 남는다 → 월드 물체가 채운 값은 그 물체가 EndPlay에서 비운다
 *   (예: 상호작용 컴포넌트가 사라질 때 "[E] 포털" 안내를 지움 — 안 지우면 던전 HUD에 남는다)
 */
UCLASS()
class UJY_UIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

public:
	/* 화면을 해당 층 스택 맨 위에 올림 (예: UI.Layer.Menu). InitFunc는 화면이 켜지기 전에 한 번 불린다 (여는 물체·글자 전달) */
	UJY_ScreenWidget* PushScreen(FGameplayTag LayerTag, TSubclassOf<UJY_ScreenWidget> ScreenClass, TFunctionRef<void(UJY_ScreenWidget&)> InitFunc = [](UJY_ScreenWidget&) {});

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
	/* 화면 틀을 만들어 이 플레이어 화면에 붙이고 기본 화면(HUD)까지 올림 */
	void CreatePrimaryLayout(APlayerController* PC);

	/* 틀이 없을 때 모아 둔 알림을 표시 */
	void ShowPendingNotifications();

	/* 자주 여는 화면(확인 팝업)을 미리 비동기 로드 */
	void PreloadCommonScreens(const UJY_GameData& GameData);

	/* 이 맵이 HUD를 쓰면 Game 층에 올림 */
	void PushHUD(APlayerController* PC, const UJY_GameData& GameData);

	/* 화면 틀을 화면에서 떼고 버림 */
	void DestroyPrimaryLayout();

	/* 켜진 화면이 없을 때의 기본 입력(게임, 커서 표시)을 적용 */
	void ApplyDefaultInputConfig(APlayerController* PC);

	/* 층에 이미 켜져 있는 같은 클래스 화면 (없으면 nullptr) */
	static UJY_ScreenWidget* FindActiveScreen(const UCommonActivatableWidgetContainerBase& Layer, TSubclassOf<UJY_ScreenWidget> ScreenClass);

private:
	UPROPERTY(Transient)
	TObjectPtr<UJY_PrimaryLayout> PrimaryLayout;

	/* 틀을 만든 PC — 맵 이동 후엔 위젯의 GetOwningPlayer가 새 PC를 돌려줘 비교에 못 쓴다 */
	TWeakObjectPtr<APlayerController> LayoutOwner;

	/* 지금 켜져 있는 정지 화면들 — 개수 대신 목록: 켜짐·꺼짐 알림이 두 번 오거나 빠져도 어긋나지 않음 */
	TSet<TWeakObjectPtr<UJY_ScreenWidget>> PauseScreens;

	/* 틀이 생기기 전에 온 알림 (접속 직후 서버 배너가 틀보다 먼저 도착하는 경우) — 틀 생성 직후 표시 */
	TArray<TPair<EJY_NotificationType, FText>> PendingNotifications;

	/* 처음 열 때 끊기지 않게 미리 비동기로 올려 둔 확인 팝업 클래스 (핸들이 살아 있는 동안 메모리 유지) */
	TSharedPtr<struct FStreamableHandle> PreloadHandle;

	/* 키 = 가장 가까운 C++ ViewModel 클래스 (값은 GameData 교체 표의 BP 자식일 수 있음) */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UMVVMViewModelBase>> ViewModels;
};
