#include "UI/JY_UIManagerSubsystem.h"
#include "UI/Foundation/JY_PrimaryLayout.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "UI/Foundation/JY_HUDLayout.h"
#include "System/JY_GameplayTags.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Input/CommonUIActionRouterBase.h"
#include "CommonInputSubsystem.h"
#include "CommonInputActionDomain.h"
#include "Engine/LocalPlayer.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "UI/ViewModels/JY_StatsViewModel.h"
#include "UI/ViewModels/JY_ProgressViewModel.h"
#include "GameFramework/PlayerController.h"
#include "Player/JY_PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_UIManagerSubsystem)

// [OwnClient]
void UJY_UIManagerSubsystem::Deinitialize() //override
{
	DestroyPrimaryLayout();

	Super::Deinitialize();
}

/*
 * 엔진이 로컬 플레이어에 새 PC를 붙일 때마다 부른다 (LocalPlayer.cpp:745 ReceivedPlayerController).
 * 데디 서버에는 로컬 플레이어가 없어 이 서브시스템 자체가 없다 → 클라에서만 실행.
 * 맵 이동 뒤: 위젯 Outer가 GameInstance라 옛 레이아웃 포인터가 살아 있다(UserWidget.cpp:2752) → 지우고 새 PC로 다시 만든다.
 */
// [OwnClient]
void UJY_UIManagerSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController) //override
{
	Super::PlayerControllerChanged(NewPlayerController);

	/* 같은 PC로 두 번 알림이 와도(SetPlayer·ReceivedPlayer 둘 다 호출, Player.cpp:174 / PlayerController.cpp:954) 다시 만들지 않는다 */
	if (PrimaryLayout != nullptr && PrimaryLayout->GetOwningPlayer() == NewPlayerController)
		return;

	DestroyPrimaryLayout();

	if (NewPlayerController == nullptr)
		return;

	CreatePrimaryLayout(NewPlayerController);
	ApplyDefaultInputConfig(NewPlayerController);
}

/*
 * 층 스택에 넣으면 스택이 위젯을 만들고 켠다(Activate) → 화면의 GetDesiredInputConfig로 입력 모드가 바뀐다.
 * 닫기는 화면이 스스로 DeactivateWidget → 스택이 빼고 아래 화면(없으면 기본 입력)으로 돌아간다.
 */
// [OwnClient]
UJY_ScreenWidget* UJY_UIManagerSubsystem::PushScreen(FGameplayTag LayerTag, TSubclassOf<UJY_ScreenWidget> ScreenClass)
{
	if (PrimaryLayout == nullptr || ScreenClass == nullptr)
		return nullptr;

	UCommonActivatableWidgetContainerBase* Layer = PrimaryLayout->GetLayer(LayerTag);
	if (Layer == nullptr)
		return nullptr;

	UJY_ScreenWidget* Screen = Layer->AddWidget<UJY_ScreenWidget>(ScreenClass);
	return Screen;
}

/* 팝업 클래스는 GameData에서 — 팝업 모양을 바꿔도 부르는 쪽 코드는 그대로 */
// [OwnClient]
void UJY_UIManagerSubsystem::ShowConfirm(const FText& Title, const FText& Body, FJY_OnConfirmResult OnResult)
{
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const TSubclassOf<UJY_ConfirmDialog> DialogClass = GameData != nullptr ? GameData->ConfirmDialogClass.LoadSynchronous() : nullptr;
	if (DialogClass == nullptr)
		return;

	UJY_ConfirmDialog* Dialog = Cast<UJY_ConfirmDialog>(PushScreen(JYGameplayTags::UI_Layer_Modal, DialogClass));
	if (Dialog == nullptr)
		return;

	Dialog->SetupDialog(Title, Body, MoveTemp(OnResult));
}

/*
 * 알림 칸은 레이아웃에 미리 배치된 위젯 2개(배너·토스트)를 재사용 — 알림마다 위젯을 만들지 않는다.
 * 서버 Client RPC가 레이아웃보다 먼저 도착할 수 있어(접속 직후) 그때는 모아 뒀다가 틀을 만들 때 표시.
 */
// [OwnClient]
void UJY_UIManagerSubsystem::ShowNotification(EJY_NotificationType Type, const FText& Message)
{
	if (PrimaryLayout == nullptr)
	{
		PendingNotifications.Emplace(Type, Message);
		return;
	}

	UJY_BannerWidget* NotificationWidget = PrimaryLayout->GetNotificationWidget(Type);
	if (NotificationWidget == nullptr)
		return;

	NotificationWidget->EnqueueMessage(Message);
}

/*
 * 예: ESC 메뉴(정지) 위에 확인 팝업(정지) → 목록 2개. 팝업을 닫으면 1개 → 여전히 정지. 메뉴까지 닫아야 0개 → 해제.
 * 정지는 서버가 건다(SetPause는 서버 GameMode 권한). 월드가 멈춰도 PC·네트워크는 돌아 해제 RPC가 도착한다.
 */
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

	/* 닫히지 않고 사라진 화면 정리 */
	for (auto It = PauseScreens.CreateIterator(); It; ++It)
	{
		if (It->IsValid() == false)
		{
			It.RemoveCurrent();
		}
	}

	const bool bIsPaused = PauseScreens.Num() > 0;
	if (bWasPaused == bIsPaused)
		return;

	AJY_PlayerController* PC = Cast<AJY_PlayerController>(GetLocalPlayer()->PlayerController);
	if (PC == nullptr)
		return;

	PC->Server_SetUIPaused(bIsPaused);
}

/*
 * ViewModel을 LocalPlayer 단위로 두는 이유 — 위젯·레벨이 바뀌어도 값이 유지되고, 여러 화면이 같은 ViewModel을 공유.
 * 새 ViewModel 추가 = 클래스만 만들면 된다(여기 수정 없음). 색·기준값을 BP 자식으로 바꾸려면 DA_JY_GameData 교체 표에 지정.
 */
// [OwnClient]
UMVVMViewModelBase* UJY_UIManagerSubsystem::GetOrCreateViewModel(TSubclassOf<UMVVMViewModelBase> ViewModelClass)
{
	if (ViewModelClass == nullptr)
		return nullptr;

	const TObjectPtr<UMVVMViewModelBase>* FoundViewModel = ViewModels.Find(ViewModelClass);
	if (FoundViewModel != nullptr)
		return *FoundViewModel;

	/* 한 번만 생성되고 계속 보관되므로 동기 로드 1회 비용은 무시할 수준 */
	TSubclassOf<UMVVMViewModelBase> InstanceClass = ViewModelClass;
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const TSoftClassPtr<UMVVMViewModelBase>* OverrideClass = GameData != nullptr ? GameData->ViewModelClassOverrides.Find(ViewModelClass) : nullptr;
	if (OverrideClass != nullptr)
	{
		const TSubclassOf<UMVVMViewModelBase> LoadedClass = OverrideClass->LoadSynchronous();
		if (LoadedClass != nullptr && LoadedClass->IsChildOf(ViewModelClass))
		{
			InstanceClass = LoadedClass;
		}
	}

	UMVVMViewModelBase* NewViewModel = NewObject<UMVVMViewModelBase>(this, InstanceClass);
	ViewModels.Add(ViewModelClass, NewViewModel);
	return NewViewModel;
}

// [OwnClient] 로컬 플레이어만
void UJY_UIManagerSubsystem::BindAbilitySystem(UJY_AbilitySystemComponent* InASC)
{
	GetOrCreateViewModel<UJY_StatsViewModel>()->BindAbilitySystem(InASC);
}

// [OwnClient] 로컬 플레이어만
void UJY_UIManagerSubsystem::BindPlayerState(AJY_PlayerState* InPlayerState)
{
	GetOrCreateViewModel<UJY_ProgressViewModel>()->BindPlayerState(InPlayerState);
}

/*
 * ZOrder 1000 — 옛 경로로 붙는 위젯(ZOrder 0)보다 위. 틀 자체는 투명하고 클릭을 통과시킨다(UserWidget 기본 SelfHitTestInvisible).
 * 클래스는 Soft 참조라 여기서 처음 로드된다. 틀은 플레이어당 1개·맵당 1회라 동기 로드로 충분.
 */
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

	/* 층은 CreateWidget 안의 NativeOnInitialized에서 이미 등록됨 → 바로 Push 가능. 맵마다 레이아웃과 함께 새로 생긴다 */
	const TSubclassOf<UJY_HUDLayout> HUDClass = GameData->HUDLayoutClass.LoadSynchronous();
	if (HUDClass == nullptr)
		return;

	PushScreen(JYGameplayTags::UI_Layer_Game, HUDClass);

	for (const TPair<EJY_NotificationType, FText>& Pending : PendingNotifications)
	{
		ShowNotification(Pending.Key, Pending.Value);
	}
	PendingNotifications.Reset();
}

// [OwnClient]
void UJY_UIManagerSubsystem::DestroyPrimaryLayout()
{
	if (PrimaryLayout == nullptr)
		return;

	PrimaryLayout->RemoveFromParent();
	PrimaryLayout = nullptr;

	/* 맵 이동 시 새 월드는 정지 상태로 시작하지 않는다 → 목록도 비운다 */
	PauseScreens.Reset();
}

/*
 * 켜진 화면이 하나도 없으면 CommonUI는 마지막 입력 모드를 그대로 둔다 → 메뉴를 닫아도 이동이 막힌 채 남는다.
 * 엔진은 이때 ActionDomainTable 기본값을 적용한다(CommonUIActionRouterBase.cpp:1834 RefreshActionDomainLeafNodeConfig) →
 * 그 에셋(DA_JY_InputDefaults)에 "게임 모드·커서 표시"를 두고, 시작할 때도 같은 값을 여기서 한 번 적용한다(시작 시엔 엔진이 안 부름).
 */
// [OwnClient]
void UJY_UIManagerSubsystem::ApplyDefaultInputConfig(APlayerController* PC)
{
	UCommonUIActionRouterBase* ActionRouter = ULocalPlayer::GetSubsystem<UCommonUIActionRouterBase>(GetLocalPlayer());
	if (ActionRouter == nullptr)
		return;

	/* 라우터가 쓰는 것과 같은 곳에서 꺼낸다 (CommonUIActionRouterBase.cpp:1091) — CDO(GetDefault)는 에셋이 로드 안 된 상태 */
	const UCommonInputSubsystem* CommonInputSubsystem = ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(GetLocalPlayer());
	const UCommonInputActionDomainTable* DefaultTable = CommonInputSubsystem != nullptr ? CommonInputSubsystem->GetActionDomainTable() : nullptr;
	if (DefaultTable == nullptr)
		return;

	ActionRouter->SetActiveUIInputConfig(FUIInputConfig(DefaultTable->InputMode, DefaultTable->MouseCaptureMode, DefaultTable->bHideCursorDuringViewportCapture), DefaultTable);
}
