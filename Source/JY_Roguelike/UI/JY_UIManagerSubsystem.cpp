#include "UI/JY_UIManagerSubsystem.h"
#include "UI/JY_ScreenWidget.h"
#include "UI/JY_UISettings.h"
#include "UI/ViewModels/JY_HealthViewModel.h"
#include "System/JY_LogChannels.h"
#include "GameFramework/PlayerController.h"
#include "MVVMSubsystem.h"
#include "View/MVVMView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_UIManagerSubsystem)

/*
 * ViewModel을 LocalPlayer 단위로 두는 이유 — 위젯·레벨이 바뀌어도 값이 유지되고, 여러 화면이 같은 ViewModel을 공유.
 * 클래스는 프로젝트 설정(JY UI)에서 고른다. BP 자식으로 색·기준값을 바꿔 끼울 수 있게 (비어 있으면 C++ 기본 클래스).
 */
UJY_HealthViewModel* UJY_UIManagerSubsystem::GetHealthViewModel()
{
	if (HealthViewModel == nullptr)
	{
		/* 한 번만 생성되고 계속 보관되므로 동기 로드 1회 비용은 무시할 수준 */
		TSubclassOf<UJY_HealthViewModel> ViewModelClass = GetDefault<UJY_UISettings>()->HealthViewModelClass.LoadSynchronous();
		if (ViewModelClass == nullptr)
		{
			ViewModelClass = UJY_HealthViewModel::StaticClass();
		}

		HealthViewModel = NewObject<UJY_HealthViewModel>(this, ViewModelClass);
	}

	return HealthViewModel;
}

// [OwnClient] 로컬 플레이어만
void UJY_UIManagerSubsystem::ShowHUD(APlayerController* PC, TSubclassOf<UJY_ScreenWidget> HUDClass)
{
	if (PC == nullptr)
		return;

	if (HUDClass == nullptr)
	{
		UE_LOG(LogJY, Warning, TEXT("[%s] JYHealth ShowHUD: HUDWidgetClass 비어 있음 (%s)"), JY_GetNetRoleTag(PC), *GetNameSafe(PC));
		return;
	}

	if (HUDWidget != nullptr)
		return;

	HUDWidget = CreateWidget<UJY_ScreenWidget>(PC, HUDClass);
	if (HUDWidget == nullptr)
		return;

	/*
	 * View는 CreateWidget 안의 NativeOnInitialized에서 만들어진다 (UserWidget.cpp:1843 → MVVMViewClass.cpp:785).
	 * 그래서 생성 직후, 화면에 붙이기 전에 꽂는다. 위젯이 이 ViewModel을 안 쓰면 false = 정상.
	 */
	UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(HUDWidget);
	if (View != nullptr)
	{
		const bool bSet = View->SetViewModelByClass(GetHealthViewModel());
		UE_LOG(LogJY, Log, TEXT("[%s] JYHealth HUD SetViewModel %s"), JY_GetNetRoleTag(PC), bSet ? TEXT("OK") : TEXT("NotUsed"));
	}

	/* 화면 전체가 아니라 이 플레이어 영역에 붙인다 — 분할 화면 협동 대비 */
	HUDWidget->AddToPlayerScreen();
}

// [OwnClient] 로컬 플레이어만
void UJY_UIManagerSubsystem::BindAbilitySystem(UJY_AbilitySystemComponent* InASC)
{
	GetHealthViewModel()->BindAbilitySystem(InASC);
}
