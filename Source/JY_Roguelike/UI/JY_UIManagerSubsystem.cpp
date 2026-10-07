#include "UI/JY_UIManagerSubsystem.h"
#include "UI/Foundation/JY_PrimaryLayout.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "UI/Foundation/JY_HUDLayout.h"
#include "UI/Common/JY_ConfirmDialog.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Player/JY_PlayerState.h"
#include "System/JY_GameplayTags.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "Player/JY_PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_UIManagerSubsystem)

// [OwnClient]
void UJY_UIManagerSubsystem::Deinitialize() //override
{
	DestroyPrimaryLayout();

	Super::Deinitialize();
}

/*
 * 엔진이 로컬 플레이어에 새 PC를 붙일 때마다 부른다 (LocalPlayer.cpp:749 ReceivedPlayerController).
 * 데디 서버에는 로컬 플레이어가 없어 이 서브시스템 자체가 없다 → 클라에서만 실행.
 * 맵 이동 뒤: 위젯 Outer가 GameInstance라 옛 틀 포인터가 살아 있다(UserWidget.cpp:2753) → 지우고 새 PC로 다시 만든다.
 * 전제: 맵 이동은 hard travel(포털 ServerTravel). seamless travel이면 클라 PC가 유지돼 이 함수가 불리지 않는다.
 */
// [OwnClient]
void UJY_UIManagerSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController) //override
{
	Super::PlayerControllerChanged(NewPlayerController);

	/*
	 * 같은 PC로 두 번 알림이 와도(SetPlayer·ReceivedPlayer, Player.cpp:174 / PlayerController.cpp:954) 다시 만들지 않는다.
	 * 위젯의 GetOwningPlayer와 비교하면 안 된다 — 옛 월드가 사라지면 로컬 플레이어의 새 PC를 돌려줘(LocalPlayer.cpp:151)
	 * 새 맵에서 틀을 다시 만들지 않는다 (예: 포털로 던전에 가면 HUD가 없음)
	 */
	if (PrimaryLayout != nullptr && LayoutOwner.Get() == NewPlayerController)
		return;

	DestroyPrimaryLayout();

	if (NewPlayerController == nullptr)
		return;

	CreatePrimaryLayout(NewPlayerController);
}

/*
 * ZOrder 1000 — 옛 경로로 붙는 위젯(ZOrder 0)보다 위. 틀 자체는 투명하고 클릭을 통과시킨다.
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
	LayoutOwner = PC;

	/* 층은 CreateWidget 안의 NativeOnInitialized에서 이미 등록됨 → 만든 직후 바로 Push 가능 */
	PushHUD(PC, *GameData);
}

/*
 * 층 스택에 넣으면 스택이 위젯을 만들고(또는 풀에서 꺼내고) → InitFunc → 켠다(Activate) (CommonActivatableWidgetContainer.cpp:191~196).
 * 켜질 때 화면의 GetDesiredInputConfig로 입력 모드가 바뀐다.
 */
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

/*
 * 팝업 클래스는 GameData에서 — 팝업 모양을 바꿔도 부르는 쪽(종료·환불·포털 등)은 제목·본문·결과 함수만 넘긴다.
 * 글자·결과 함수는 InitFunc로 켜지기 전에 채운다 — 풀에서 재사용한 팝업이 이전 글자·결과 함수를 가진 채 켜지지 않는다.
 */
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

/*
 * 틀(PC가 붙을 때)과 PlayerState(클라는 복제로 늦게 올 수 있음) 중 무엇이 먼저 올지 정해져 있지 않다.
 * → HUD를 올릴 때 PlayerState가 있으면 여기서 연결, 없으면 AJY_PlayerController::OnRep_PlayerState가 BindHUDToPlayerState로 연결.
 */
// [OwnClient]
void UJY_UIManagerSubsystem::PushHUD(APlayerController* PC, const UJY_GameData& GameData)
{
	const TSubclassOf<UJY_HUDLayout> HUDClass = GameData.HUDLayoutClass.LoadSynchronous();
	if (HUDClass == nullptr)
		return;

	HUDLayout = Cast<UJY_HUDLayout>(PushScreen(JYGameplayTags::UI_Layer_Game, HUDClass));

	BindHUDToPlayerState(PC->GetPlayerState<AJY_PlayerState>());
}

// [OwnClient]
void UJY_UIManagerSubsystem::BindHUDToPlayerState(AJY_PlayerState* PlayerState)
{
	UJY_HUDLayout* HUD = HUDLayout.Get();
	if (HUD == nullptr || PlayerState == nullptr)
		return;

	/* 두 번 불려도 Bind 함수들이 먼저 끊고 다시 건다 */
	HUD->BindAbilitySystem(PlayerState->GetJYAbilitySystemComponent());
	HUD->BindProgress(PlayerState);
}

// [OwnClient]
void UJY_UIManagerSubsystem::DestroyPrimaryLayout()
{
	if (PrimaryLayout == nullptr)
		return;

	PrimaryLayout->RemoveFromParent();
	PrimaryLayout = nullptr;
	LayoutOwner.Reset();
	HUDLayout.Reset();
}

/*
 * 예: 정지 메뉴(정지) 위에 확인 팝업(정지) → 목록 2개. 팝업을 닫으면 1개 → 여전히 정지. 메뉴까지 닫아야 0개 → 해제.
 * 개수(int) 대신 목록 — 같은 화면의 켜짐·꺼짐 알림이 두 번 와도 어긋나지 않는다 (개수면 1 → 0 → -1).
 * 정지는 서버가 건다(SetPause는 서버 GameMode 권한, PlayerController.cpp:1931). 월드가 멈춰도 PC·네트워크는 돌아 해제 RPC가 도착한다.
 * 1인 게임 전제 — 여러 클라가 각자 정지를 걸면 서로 막히므로, 협동을 넣을 때 정지 규칙을 다시 정한다.
 * 주의: 같은 층에 정지 화면을 겹쳐 올리면 가려진 화면이 먼저 꺼져(CommonActivatableWidgetContainer.cpp:165) 목록이 잠깐 0개 → 해제·정지 요청이 연달아 나간다.
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

	const bool bIsPaused = PauseScreens.Num() > 0;
	if (bWasPaused == bIsPaused)
		return;

	/*
	 * 맵 이동으로 옛 틀이 정리되며 화면이 꺼질 때는 보내지 않는다 — 옛 PC는 곧 사라지고 새 월드는 정지 없이 시작.
	 * LoadMap이 BeginTearingDown(UnrealEngine.cpp:16183)을 PC 파괴(:16249)보다 먼저 하므로 bIsTearingDown 하나로 걸러진다.
	 */
	AJY_PlayerController* PC = Cast<AJY_PlayerController>(GetLocalPlayer()->PlayerController);
	if (PC == nullptr || PC->GetWorld()->bIsTearingDown == true)
		return;

	PC->Server_SetUIPaused(bIsPaused);
}
