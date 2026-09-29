#include "UI/JY_UIManagerSubsystem.h"
#include "UI/Foundation/JY_PrimaryLayout.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "UI/Foundation/JY_HUDLayout.h"
#include "UI/Common/JY_ConfirmDialog.h"
#include "UI/Common/JY_NotificationWidget.h"
#include "Engine/AssetManager.h"
#include "System/JY_GameplayTags.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Input/CommonUIActionRouterBase.h"
#include "CommonInputSubsystem.h"
#include "CommonInputActionDomain.h"
#include "Engine/LocalPlayer.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "GameFramework/PlayerController.h"
#include "Player/JY_PlayerController.h"
#include "System/JY_WorldSettings.h"

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
 *   새로 만드는 이유: 열려 있던 화면·HUD 표시 설정을 새 맵 기준으로 처음부터 맞춘다. 맵 이동은 허브↔던전뿐이라 비용은 로딩 중 1회.
 * 전제: 맵 이동은 hard travel(포털 ServerTravel). seamless travel을 켜면 클라 PC가 유지돼(World.cpp:8694) 이 함수가 불리지 않는다
 *   → 그때는 틀을 다시 만드는 다른 계기(월드 전환 알림)를 추가해야 한다. 던전 방 이동은 같은 맵 스트리밍이라 해당 없음.
 */
// [OwnClient]
void UJY_UIManagerSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController) //override
{
	Super::PlayerControllerChanged(NewPlayerController);

	/*
	 * 같은 PC로 두 번 알림이 와도(SetPlayer·ReceivedPlayer 둘 다 호출, Player.cpp:174 / PlayerController.cpp:954) 다시 만들지 않는다.
	 * 위젯의 GetOwningPlayer와 비교하면 안 된다 — 맵 이동으로 옛 월드가 사라지면 로컬 플레이어의 새 PC를 돌려줘(LocalPlayer.cpp:151)
	 * 새 맵에서 틀을 다시 만들지 않는다 (예: 포털로 던전에 가면 HUD·배너가 없음)
	 */
	if (PrimaryLayout != nullptr && LayoutOwner.Get() == NewPlayerController)
		return;

	DestroyPrimaryLayout();

	if (NewPlayerController == nullptr)
		return;

	CreatePrimaryLayout(NewPlayerController);
	ApplyDefaultInputConfig(NewPlayerController);
}

/*
 * 층 스택에 넣으면 스택이 위젯을 만들고(또는 풀에서 꺼내고) InitFunc → 켜기(Activate) 순서로 진행한다 (CommonActivatableWidgetContainer.cpp:191~196).
 * 켜질 때 화면의 GetDesiredInputConfig로 입력 모드가 바뀐다. 닫기는 화면이 스스로 DeactivateWidget → 스택이 빼고 아래 화면(없으면 기본 입력)으로.
 */
// [OwnClient]
UJY_ScreenWidget* UJY_UIManagerSubsystem::PushScreen(FGameplayTag LayerTag, TSubclassOf<UJY_ScreenWidget> ScreenClass, TFunctionRef<void(UJY_ScreenWidget&)> InitFunc)
{
	if (PrimaryLayout == nullptr || ScreenClass == nullptr)
		return nullptr;

	UCommonActivatableWidgetContainerBase* Layer = PrimaryLayout->GetLayer(LayerTag);
	if (Layer == nullptr)
		return nullptr;

	/*
	 * 게임 메뉴 층(제단·기록판·축복)은 같은 화면이 두 번 쌓이지 않게 — 이미 켜져 있으면 그것을 돌려준다.
	 * 다른 층은 쌓는 게 정상이라 적용하지 않는다: Modal은 팝업마다 결과 콜백이 달라 매번 새로, Menu는 정지 메뉴 위에 설정 화면.
	 */
	if (LayerTag == JYGameplayTags::UI_Layer_GameMenu)
	{
		UJY_ScreenWidget* OpenScreen = FindActiveScreen(*Layer, ScreenClass);
		if (OpenScreen != nullptr)
			return OpenScreen;
	}

	return Layer->AddWidget<UJY_ScreenWidget>(ScreenClass, InitFunc);
}

/*
 * 팝업 클래스는 GameData에서 — 팝업 모양을 바꿔도 부르는 쪽 코드는 그대로.
 * 글자·콜백은 초기화 함수로 켜지기 전에 채운다 → 켜지는 순간 빈 글자가 한 프레임 보이지 않는다.
 */
// [OwnClient]
void UJY_UIManagerSubsystem::ShowConfirm(const FText& Title, const FText& Body, FJY_OnConfirmResult OnResult)
{
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const TSubclassOf<UJY_ConfirmDialog> DialogClass = GameData != nullptr ? GameData->ConfirmDialogClass.LoadSynchronous() : nullptr;
	if (DialogClass == nullptr)
		return;

	PushScreen(JYGameplayTags::UI_Layer_Modal, DialogClass, [&Title, &Body, &OnResult](UJY_ScreenWidget& Screen)
	{
		CastChecked<UJY_ConfirmDialog>(&Screen)->SetupDialog(Title, Body, MoveTemp(OnResult));
	});
}

/*
 * 알림 칸은 레이아웃에 미리 배치된 위젯 2개(배너·토스트)를 재사용 — 알림마다 위젯을 만들지 않는다.
 * 틀이 없을 때 온 알림(접속 직후·맵 이동 중)은 버리지 않고 모아 뒀다가 틀을 만들 때 표시.
 */
// [OwnClient]
void UJY_UIManagerSubsystem::ShowNotification(EJY_NotificationType Type, const FText& Message)
{
	/* 틀이 계속 안 생기는 설정 오류에서도 쌓이지 않게 상한 — 넘치면 가장 오래된 것부터 버림(알림 칸 대기열과 같은 규칙) */
	if (PrimaryLayout == nullptr)
	{
		constexpr int32 MaxPendingNotifications = 8;
		if (PendingNotifications.Num() >= MaxPendingNotifications)
		{
			PendingNotifications.RemoveAt(0);
		}

		PendingNotifications.Emplace(Type, Message);
		return;
	}

	UJY_NotificationWidget* NotificationWidget = PrimaryLayout->GetNotificationWidget(Type);
	if (NotificationWidget == nullptr)
		return;

	NotificationWidget->EnqueueMessage(Message);
}

/*
 * 예: 정지 메뉴(정지) 위에 확인 팝업(정지) → 목록 2개. 팝업을 닫으면 1개 → 여전히 정지. 메뉴까지 닫아야 0개 → 해제.
 * 정지는 서버가 건다(SetPause는 서버 GameMode 권한). 월드가 멈춰도 PC·네트워크는 돌아 해제 RPC가 도착한다.
 * 1인 게임 전제 — 여러 클라가 각자 정지를 걸면 서로 막히므로, 협동을 넣을 때 정지 규칙을 다시 정한다.
 * 주의: 같은 층에 정지 화면을 겹쳐 올리면 가려진 화면이 먼저 꺼져 목록이 잠깐 0개 → 해제·정지 요청이 연달아 나간다(한 프레임 풀림).
 *       지금은 층마다 정지 화면이 하나라 생기지 않는다 — 생기면 판정을 다음 프레임으로 미룬다.
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

	/* 맵 이동으로 옛 레이아웃이 정리되며 화면이 꺼질 때는 보내지 않는다 — 옛 PC는 곧 사라지고 새 월드는 정지 없이 시작 */
	AJY_PlayerController* PC = Cast<AJY_PlayerController>(GetLocalPlayer()->PlayerController);
	if (PC == nullptr || PC->IsActorBeingDestroyed() == true || PC->GetWorld()->bIsTearingDown == true)
		return;

	PC->Server_SetUIPaused(bIsPaused);
}

/*
 * ViewModel을 LocalPlayer 단위로 두는 이유 — 위젯·레벨이 바뀌어도 값이 유지되고, 여러 화면이 같은 ViewModel을 공유.
 * 새 ViewModel 추가 = 클래스만 만들면 된다(여기 수정 없음). 색·기준값을 BP 자식으로 바꾸려면 DA_JY_GameData 교체 표에 지정.
 * 키는 가장 가까운 C++ 클래스로 맞춘다 — WBP에서 BP 자식을 직접 골라도 C++ 코드와 같은 인스턴스를 받는다
 * (안 맞추면 인스턴스가 2개 생겨, C++이 값을 넣는 쪽과 위젯이 보는 쪽이 갈린다 → 체력바가 계속 "--").
 */
// [OwnClient]
UMVVMViewModelBase* UJY_UIManagerSubsystem::GetOrCreateViewModel(TSubclassOf<UMVVMViewModelBase> ViewModelClass)
{
	UClass* NativeClass = ViewModelClass.Get();
	while (NativeClass != nullptr && NativeClass->HasAnyClassFlags(CLASS_Native) == false)
	{
		NativeClass = NativeClass->GetSuperClass();
	}

	if (NativeClass == nullptr)
		return nullptr;

	const TObjectPtr<UMVVMViewModelBase>* FoundViewModel = ViewModels.Find(NativeClass);
	if (FoundViewModel != nullptr)
		return *FoundViewModel;

	/* 한 번만 생성되고 계속 보관되므로 동기 로드 1회 비용은 무시할 수준 */
	TSubclassOf<UMVVMViewModelBase> InstanceClass = NativeClass;
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const TSoftClassPtr<UMVVMViewModelBase>* OverrideClass = GameData != nullptr ? GameData->ViewModelClassOverrides.Find(NativeClass) : nullptr;
	if (OverrideClass != nullptr)
	{
		const TSubclassOf<UMVVMViewModelBase> LoadedClass = OverrideClass->LoadSynchronous();
		if (LoadedClass != nullptr && LoadedClass->IsChildOf(NativeClass))
		{
			InstanceClass = LoadedClass;
		}
	}

	UMVVMViewModelBase* NewViewModel = NewObject<UMVVMViewModelBase>(this, InstanceClass);
	ViewModels.Add(NativeClass, NewViewModel);
	return NewViewModel;
}

/*
 * ZOrder 1000 — 옛 경로로 붙는 위젯(ZOrder 0)보다 위. 틀 자체는 투명하고 클릭을 통과시킨다(UserWidget 기본 SelfHitTestInvisible).
 * 클래스는 Soft 참조라 여기서 처음 로드된다. 틀은 플레이어당 1개·맵당 1회라 동기 로드로 충분.
 * 층은 CreateWidget 안의 NativeOnInitialized에서 이미 등록됨 → 만든 직후 바로 Push 가능.
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
	LayoutOwner = PC;

	/* 순서: 알림 먼저 — HUD 설정이 빠져도(타이틀 맵 등) 알림은 나가게 */
	ShowPendingNotifications();
	PreloadCommonScreens(*GameData);
	PushHUD(PC, *GameData);
}

// [OwnClient]
void UJY_UIManagerSubsystem::ShowPendingNotifications()
{
	TArray<TPair<EJY_NotificationType, FText>> NotificationsToShow = MoveTemp(PendingNotifications);
	for (const TPair<EJY_NotificationType, FText>& Pending : NotificationsToShow)
	{
		ShowNotification(Pending.Key, Pending.Value);
	}
}

/* 확인 팝업은 첫 사용 때 끊기지 않게 지금 비동기로 미리 올려 둔다 (맵 로딩 직후라 체감 없음) */
// [OwnClient]
void UJY_UIManagerSubsystem::PreloadCommonScreens(const UJY_GameData& GameData)
{
	if (GameData.ConfirmDialogClass.IsNull() == true)
		return;

	PreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(GameData.ConfirmDialogClass.ToSoftObjectPath());
}

/* 맵마다 레이아웃과 함께 새로 생긴다. HUD를 쓸지는 이 맵의 World Settings가 정한다 (타이틀 맵 = 안 씀) */
// [OwnClient]
void UJY_UIManagerSubsystem::PushHUD(APlayerController* PC, const UJY_GameData& GameData)
{
	const AJY_WorldSettings* WorldSettings = Cast<AJY_WorldSettings>(PC->GetWorldSettings());
	if (WorldSettings != nullptr && WorldSettings->ShouldShowHUD() == false)
		return;

	const TSubclassOf<UJY_HUDLayout> HUDClass = GameData.HUDLayoutClass.LoadSynchronous();
	if (HUDClass == nullptr)
		return;

	PushScreen(JYGameplayTags::UI_Layer_Game, HUDClass);
}

/* 스택은 맨 위 하나만 켜져 있다 — 켜진(IsActivated) 것만 "열려 있음"으로 본다. 풀에 남은 꺼진 인스턴스는 제외 */
// [OwnClient]
UJY_ScreenWidget* UJY_UIManagerSubsystem::FindActiveScreen(const UCommonActivatableWidgetContainerBase& Layer, TSubclassOf<UJY_ScreenWidget> ScreenClass)
{
	for (UCommonActivatableWidget* Widget : Layer.GetWidgetList())
	{
		if (Widget != nullptr && Widget->GetClass() == ScreenClass && Widget->IsActivated() == true)
			return Cast<UJY_ScreenWidget>(Widget);
	}

	return nullptr;
}

// [OwnClient]
void UJY_UIManagerSubsystem::DestroyPrimaryLayout()
{
	if (PrimaryLayout == nullptr)
		return;

	PrimaryLayout->RemoveFromParent();
	PrimaryLayout = nullptr;
	LayoutOwner.Reset();

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
