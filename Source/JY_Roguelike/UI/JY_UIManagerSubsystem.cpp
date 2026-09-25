#include "UI/JY_UIManagerSubsystem.h"
#include "UI/Foundation/JY_PrimaryLayout.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "UI/JY_UISettings.h"
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
}

/*
 * ViewModel을 LocalPlayer 단위로 두는 이유 — 위젯·레벨이 바뀌어도 값이 유지되고, 여러 화면이 같은 ViewModel을 공유.
 * 클래스는 프로젝트 설정(JY UI)에서 고른다. BP 자식으로 색·기준값을 바꿔 끼울 수 있게 (비어 있으면 C++ 기본 클래스).
 */
UJY_StatsViewModel* UJY_UIManagerSubsystem::GetStatsViewModel()
{
	if (StatsViewModel == nullptr)
	{
		/* 한 번만 생성되고 계속 보관되므로 동기 로드 1회 비용은 무시할 수준 */
		TSubclassOf<UJY_StatsViewModel> ViewModelClass = GetDefault<UJY_UISettings>()->StatsViewModelClass.LoadSynchronous();
		if (ViewModelClass == nullptr)
		{
			ViewModelClass = UJY_StatsViewModel::StaticClass();
		}

		StatsViewModel = NewObject<UJY_StatsViewModel>(this, ViewModelClass);
	}

	return StatsViewModel;
}

// [OwnClient] 로컬 플레이어만
void UJY_UIManagerSubsystem::BindAbilitySystem(UJY_AbilitySystemComponent* InASC)
{
	GetStatsViewModel()->BindAbilitySystem(InASC);
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
