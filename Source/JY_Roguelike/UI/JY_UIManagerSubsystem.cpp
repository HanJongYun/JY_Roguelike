#include "UI/JY_UIManagerSubsystem.h"
#include "UI/Foundation/JY_PrimaryLayout.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "UI/Foundation/JY_HUDLayout.h"
#include "UI/Common/JY_ConfirmDialog.h"
#include "UI/Common/JY_NotificationWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "System/JY_GameplayTags.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "System/JY_WorldSettings.h"
#include "Player/JY_PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_UIManagerSubsystem)

// [OwnClient]
void UJY_UIManagerSubsystem::Deinitialize() //override
{
	DestroyPrimaryLayout();

	Super::Deinitialize();
}

/* 맵 이동(심리스 포함) 뒤 새 PC가 오면 불린다 — 옛 틀은 Outer가 GameInstance라 살아 있어 지우고 다시 만든다 (UserWidget.cpp:2753) */
// [OwnClient]
void UJY_UIManagerSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController) //override
{
	Super::PlayerControllerChanged(NewPlayerController);

	/* 같은 PC로 두 번 와도 다시 만들지 않음 — 위젯의 GetOwningPlayer와 비교하면 안 된다:
	   옛 월드가 사라지면 새 PC를 돌려줘 새 맵에서 틀이 안 만들어진다 (LocalPlayer.cpp:151) */
	if (PrimaryLayout != nullptr && LayoutOwner.Get() == NewPlayerController)
		return;

	DestroyPrimaryLayout();

	if (NewPlayerController == nullptr)
		return;

	CreatePrimaryLayout(NewPlayerController);
}

/* ZOrder 1000 — 기본 ZOrder 0으로 붙는 위젯보다 위. 틀은 맵당 1회라 동기 로드로 충분 */
// [OwnClient]
void UJY_UIManagerSubsystem::CreatePrimaryLayout(APlayerController* PC)
{
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	if (GameData == nullptr)
		return;

	const TSubclassOf<UJY_PrimaryLayout> LayoutClass = GameData->PrimaryLayoutClass.LoadSynchronous();
	if (LayoutClass == nullptr)
		return;

	PrimaryLayout = CreateWidget<UJY_PrimaryLayout>(PC, LayoutClass);
	if (PrimaryLayout == nullptr)
		return;

	PrimaryLayout->AddToPlayerScreen(1000);
	LayoutOwner = PC;

	/* 층은 CreateWidget 안의 NativeOnInitialized에서 이미 등록됨 → 만든 직후 바로 Push 가능 */
	PushHUD(PC, *GameData);
}

/* 스택이 위젯 생성(또는 풀 재사용) → InitFunc → Activate 순으로 부른다 (CommonActivatableWidgetContainer.cpp:191) */
// [OwnClient]
UJY_ScreenWidget* UJY_UIManagerSubsystem::PushScreen(FGameplayTag LayerTag, TSubclassOf<UJY_ScreenWidget> ScreenClass, TFunctionRef<void(UJY_ScreenWidget&)> InitFunc)
{
	if (PrimaryLayout == nullptr || ScreenClass == nullptr)
		return nullptr;

	UCommonActivatableWidgetContainerBase* Layer = PrimaryLayout->GetLayer(LayerTag);
	if (Layer == nullptr)
		return nullptr;

	return Layer->AddWidget<UJY_ScreenWidget>(ScreenClass, InitFunc);
}

/* 글자·결과 함수는 InitFunc로 켜지기 전에 채운다 — 풀에서 재사용한 팝업이 이전 값으로 켜지지 않게 */
// [OwnClient]
void UJY_UIManagerSubsystem::ShowConfirm(const FText& Title, const FText& Body, FJY_OnConfirmResult OnResult)
{
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	if (GameData == nullptr)
		return;

	const TSubclassOf<UJY_ConfirmDialog> DialogClass = GameData->ConfirmDialogClass.LoadSynchronous();

	/* 켜지기 전에 불릴 함수 — PushScreen 안에서 바로 불리고 끝나므로 지역 변수를 참조(&)로 잡아도 안전 */
	auto SetupBeforeActivate = [&Title, &Body, &OnResult](UJY_ScreenWidget& Screen)
	{
		UJY_ConfirmDialog* Dialog = CastChecked<UJY_ConfirmDialog>(&Screen);
		Dialog->SetupDialog(Title, Body, MoveTemp(OnResult));
	};

	PushScreen(JYGameplayTags::UI_Layer_Modal, DialogClass, SetupBeforeActivate);
}

/* 맵을 넘어 남아 여러 화면이 공유한다 — 월드 물체가 채운 값은 그 물체가 사라질 때 비울 것 (안 비우면 다음 맵 HUD에 남음) */
/* 값 넣기: 복제 상태는 InitializeLocalUI에서 구독, 로컬 상태는 게임 코드가 직접, 목록 항목은 화면이 직접 */
// [OwnClient]
UMVVMViewModelBase* UJY_UIManagerSubsystem::GetOrCreateViewModel(TSubclassOf<UMVVMViewModelBase> ViewModelClass)
{
	if (ViewModelClass == nullptr)
		return nullptr;

	const TObjectPtr<UMVVMViewModelBase>* FoundViewModel = ViewModels.Find(ViewModelClass);
	if (FoundViewModel != nullptr)
		return *FoundViewModel;

	/* Outer = 이 서브시스템 — UPROPERTY 표가 붙잡고 있어 GC되지 않는다 */
	UMVVMViewModelBase* NewViewModel = NewObject<UMVVMViewModelBase>(this, ViewModelClass);
	ViewModels.Add(ViewModelClass, NewViewModel);
	return NewViewModel;
}

/* HUD는 올리기만 한다 — 원본 연결은 AJY_PlayerController::InitializeLocalUI라 틀·PlayerState 도착 순서 무관 */
// [OwnClient]
void UJY_UIManagerSubsystem::PushHUD(APlayerController* PC, const UJY_GameData& GameData)
{
	const AJY_WorldSettings* WorldSettings = Cast<AJY_WorldSettings>(PC->GetWorldSettings());
	if (ensureMsgf(WorldSettings != nullptr, TEXT("World Settings가 AJY_WorldSettings가 아님 — DefaultEngine.ini WorldSettingsClassName 확인")) == false)
		return;

	if (WorldSettings->ShouldShowHUD() == false)
		return;

	const TSubclassOf<UJY_HUDLayout> HUDClass = GameData.HUDLayoutClass.LoadSynchronous();
	if (HUDClass == nullptr)
		return;

	PushScreen(JYGameplayTags::UI_Layer_Game, HUDClass);
}

// [OwnClient]
void UJY_UIManagerSubsystem::DestroyPrimaryLayout()
{
	if (PrimaryLayout == nullptr)
		return;

	PrimaryLayout->RemoveFromParent();
	PrimaryLayout = nullptr;
	LayoutOwner.Reset();
}

/* 틀 null 검사 없음 — PC가 오면 틀을 먼저 만들고 그 PC의 RPC는 그 뒤 실행된다 (NetConnection.cpp:5258) */
/* PC 도착 전에 부르는 RPC 아닌 호출자가 생기면 대기열을 추가할 것 */
// [OwnClient]
void UJY_UIManagerSubsystem::ShowNotification(EJY_NotificationType Type, const FText& Message)
{
	PrimaryLayout->GetNotificationWidget(Type)->EnqueueMessage(Message);
}

/* 개수 대신 목록 — 같은 화면의 켜짐·꺼짐 알림이 두 번 와도 어긋나지 않는다 */
/* 1인 게임 전제 — 정지는 서버가 걸며, 여러 클라가 각자 걸면 서로 막힌다 */
/* 주의: 같은 층에 정지 화면을 겹치면 가려진 화면이 먼저 꺼져 목록이 잠깐 0이 된다 (CommonActivatableWidgetContainer.cpp:165) */
// [OwnClient]
void UJY_UIManagerSubsystem::SetPauseScreenActive(UJY_ScreenWidget* Screen, bool bActive)
{
	const bool bWasPaused = PauseScreens.Num() > 0;

	if (bActive == true)
	{
		PauseScreens.Add(Screen);
	}
	else
	{
		PauseScreens.Remove(Screen);
	}

	const bool bIsPaused = PauseScreens.Num() > 0;
	if (bWasPaused == bIsPaused)
		return;

	/* 맵 이동 정리 중엔 보내지 않는다 — LoadMap이 BeginTearingDown을 PC 파괴보다 먼저 한다 (UnrealEngine.cpp:16183) */
	AJY_PlayerController* PC = Cast<AJY_PlayerController>(GetLocalPlayer()->PlayerController);
	if (PC == nullptr || PC->GetWorld()->bIsTearingDown == true)
		return;

	PC->Server_SetUIPaused(bIsPaused);
}
