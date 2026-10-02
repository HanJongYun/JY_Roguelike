#include "UI/JY_UIManagerSubsystem.h"
#include "UI/Foundation/JY_PrimaryLayout.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "UI/Foundation/JY_HUDLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Player/JY_PlayerState.h"
#include "System/JY_GameplayTags.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "GameFramework/PlayerController.h"

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
 * 층 스택에 넣으면 스택이 위젯을 만들고(또는 풀에서 꺼내고) 켠다(Activate).
 * 켜질 때 화면의 GetDesiredInputConfig로 입력 모드가 바뀐다.
 */
// [OwnClient]
UJY_ScreenWidget* UJY_UIManagerSubsystem::PushScreen(FGameplayTag LayerTag, TSubclassOf<UJY_ScreenWidget> ScreenClass)
{
	if (PrimaryLayout == nullptr || ScreenClass == nullptr)
		return nullptr;

	UCommonActivatableWidgetContainerBase* Layer = PrimaryLayout->GetLayer(LayerTag);
	if (Layer == nullptr)
		return nullptr;

	return Layer->AddWidget<UJY_ScreenWidget>(ScreenClass);
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

