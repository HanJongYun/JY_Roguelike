#include "UI/JY_UIManagerSubsystem.h"
#include "UI/Foundation/JY_PrimaryLayout.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Input/CommonUIActionRouterBase.h"
#include "CommonInputSubsystem.h"
#include "CommonInputActionDomain.h"
#include "Engine/LocalPlayer.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "UI/ViewModels/JY_StatsViewModel.h"
#include "System/JY_LogChannels.h"
#include "GameFramework/PlayerController.h"

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
	{
		UE_LOG(LogJY, Error, TEXT("JYUI Push 실패: 층 %s 없음"), *LayerTag.ToString());
		return nullptr;
	}

	UJY_ScreenWidget* Screen = Layer->AddWidget<UJY_ScreenWidget>(ScreenClass);
	UE_LOG(LogJY, Log, TEXT("JYUI Push %s %s"), *LayerTag.ToString(), *GetNameSafe(Screen));
	return Screen;
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
	{
		UE_LOG(LogJY, Error, TEXT("[%s] JYUI PrimaryLayoutClass 비어 있음 (DA_JY_GameData)"), JY_GetNetRoleTag(PC));
		return;
	}

	PrimaryLayout = CreateWidget<UJY_PrimaryLayout>(PC, LayoutClass);
	if (PrimaryLayout == nullptr)
		return;

	PrimaryLayout->AddToPlayerScreen(1000);
	UE_LOG(LogJY, Log, TEXT("[%s] JYUI Layout created %s (%s)"), JY_GetNetRoleTag(PC), *GetNameSafe(PrimaryLayout), *GetNameSafe(PC));
}

// [OwnClient]
void UJY_UIManagerSubsystem::DestroyPrimaryLayout()
{
	if (PrimaryLayout == nullptr)
		return;

	PrimaryLayout->RemoveFromParent();
	PrimaryLayout = nullptr;
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
	{
		UE_LOG(LogJY, Error, TEXT("[%s] JYUI ActionDomainTable 비어 있음 — DefaultGame.ini CommonInputSettings 확인"), JY_GetNetRoleTag(PC));
		return;
	}

	ActionRouter->SetActiveUIInputConfig(FUIInputConfig(DefaultTable->InputMode, DefaultTable->MouseCaptureMode, DefaultTable->bHideCursorDuringViewportCapture), DefaultTable);
}
