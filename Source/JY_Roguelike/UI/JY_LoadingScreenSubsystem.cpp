#include "UI/JY_LoadingScreenSubsystem.h"
#include "UI/JY_LoadingProcessInterface.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_LoadingScreenSubsystem)

namespace JYLoadingScreen
{
	/* 틀(1000)·다른 뷰포트 위젯보다 위 (Lyra LoadingScreenZOrder 기본값과 같음) */
	constexpr int32 ZOrder = 10000;
}

/* 데디 서버엔 화면(뷰포트)이 없다 */
bool UJY_LoadingScreenSubsystem::ShouldCreateSubsystem(UObject* Outer) const //override
{
	const UGameInstance* GameInstance = CastChecked<UGameInstance>(Outer);
	return GameInstance->IsDedicatedServerInstance() == false;
}

/* 하드 이동은 게임 스레드를 막아 Tick이 안 돈다 → PreLoadMap에서 직접 켠다 */
/* 심리스는 PreLoadMap이 오지 않아 시작 방송을 따로 받는다 */
// [OwnClient]
void UJY_LoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection) //override
{
	Super::Initialize(Collection);

	FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &ThisClass::HandlePreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
	FWorldDelegates::OnSeamlessTravelStart.AddUObject(this, &ThisClass::HandleSeamlessTravelStart);
}

// [OwnClient]
void UJY_LoadingScreenSubsystem::Deinitialize() //override
{
	FCoreUObjectDelegates::PreLoadMapWithContext.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	FWorldDelegates::OnSeamlessTravelStart.RemoveAll(this);

	HideLoadingScreen();

	Super::Deinitialize();
}

/* 조건이 여러 곳(이동 상태·복제 도착)에서 바뀌어 이벤트 하나로 못 잡으므로 매 프레임 검사 (Lyra 방식) */
// [OwnClient]
void UJY_LoadingScreenSubsystem::Tick(float DeltaTime) //override
{
	if (bWaitingForNewController == true)
	{
		const APlayerController* LocalController = GetGameInstance()->GetFirstLocalPlayerController();
		if (LocalController != ControllerBeforeSeamlessTravel.Get())
		{
			bWaitingForNewController = false;
			ControllerBeforeSeamlessTravel.Reset();
		}
	}

	UpdateLoadingScreen();
}

/* FTickableGameObject는 생성자에서 등록돼 클래스 기본 객체(CDO)도 틱 목록에 들어간다 — CDO엔 GameInstance가 없다 (Lyra LoadingScreenManager.cpp:165) */
ETickableTickType UJY_LoadingScreenSubsystem::GetTickableTickType() const //override
{
	if (IsTemplate() == true)
		return ETickableTickType::Never;

	return ETickableTickType::Conditional;
}

/* 시작 직후엔 뷰포트가 아직 없다 — 붙일 곳이 생긴 뒤부터 검사 */
bool UJY_LoadingScreenSubsystem::IsTickable() const //override
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance->GetGameViewportClient() != nullptr;
}

TStatId UJY_LoadingScreenSubsystem::GetStatId() const //override
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UJY_LoadingScreenSubsystem, STATGROUP_Tickables);
}

// [OwnClient]
void UJY_LoadingScreenSubsystem::UpdateLoadingScreen()
{
	const bool bShouldShow = IsLoading();
	const bool bShowing = LoadingScreenWidget.IsValid();
	if (bShouldShow == bShowing)
		return;

	if (bShouldShow == true)
	{
		ShowLoadingScreen();
	}
	else
	{
		HideLoadingScreen();
	}
}

/* 순서: 엔진 이동 상태 → 월드 준비 → 내 PC 준비 (복제 도착 대기) */
// [OwnClient]
bool UJY_LoadingScreenSubsystem::IsLoading() const
{
	if (bLoadingMap == true || bWaitingForNewController == true)
		return true;

	const UGameInstance* GameInstance = GetGameInstance();
	const FWorldContext* WorldContext = GameInstance->GetWorldContext();
	if (WorldContext == nullptr)
		return true;

	if (WorldContext->PendingNetGame != nullptr || WorldContext->TravelURL.IsEmpty() == false)
		return true;

	const UWorld* World = WorldContext->World();
	if (World == nullptr || World->HasBegunPlay() == false || World->IsInSeamlessTravel() == true)
		return true;

	if (World->GetGameState() == nullptr)
		return true;

	const APlayerController* LocalController = GameInstance->GetFirstLocalPlayerController(World);
	if (LocalController == nullptr)
		return true;

	/* 이 PC가 더 기다릴 게 있는지 (예: AJY_PlayerController = 캐릭터 도착 전). 타이틀 PC는 구현하지 않아 바로 준비 */
	const IJY_LoadingProcessInterface* LoadingProcess = Cast<IJY_LoadingProcessInterface>(LocalController);
	return LoadingProcess != nullptr && LoadingProcess->ShouldShowLoadingScreen() == true;
}

/* Outer = GameInstance, 뷰포트에 직접 붙여 LoadMap에도 안 떨어진다 (GameViewportClient.cpp:472) */
/* 곧 LoadMap이 게임 스레드를 막으므로 Slate를 한 번 돌려 바로 그린다 (PIE는 생략) */
// [OwnClient]
void UJY_LoadingScreenSubsystem::ShowLoadingScreen()
{
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	if (GameData == nullptr)
		return;

	const TSubclassOf<UUserWidget> WidgetClass = GameData->LoadingScreenClass.LoadSynchronous();
	if (ensureMsgf(WidgetClass != nullptr, TEXT("DA_JY_GameData의 LoadingScreenClass가 비어 있음")) == false)
		return;

	UGameInstance* GameInstance = GetGameInstance();
	UUserWidget* UserWidget = UUserWidget::CreateWidgetInstance(*GameInstance, WidgetClass, NAME_None);
	LoadingScreenWidget = UserWidget->TakeWidget();

	UGameViewportClient* ViewportClient = GameInstance->GetGameViewportClient();
	ViewportClient->AddViewportWidgetContent(LoadingScreenWidget.ToSharedRef(), JYLoadingScreen::ZOrder);

	if (GIsEditor == false)
	{
		FSlateApplication::Get().Tick();
	}
}

/* PIE를 로딩 중에 끄면 뷰포트가 먼저 사라진 뒤 여기가 불린다 (Deinitialize) */
// [OwnClient]
void UJY_LoadingScreenSubsystem::HideLoadingScreen()
{
	if (LoadingScreenWidget.IsValid() == false)
		return;

	UGameViewportClient* ViewportClient = GetGameInstance()->GetGameViewportClient();
	if (ViewportClient != nullptr)
	{
		ViewportClient->RemoveViewportWidgetContent(LoadingScreenWidget.ToSharedRef());
	}

	LoadingScreenWidget.Reset();
}

/* PIE 한 프로세스에선 서버 월드의 LoadMap도 이 전역 방송으로 온다 → 내 GameInstance 것만 */
// [OwnClient]
void UJY_LoadingScreenSubsystem::HandlePreLoadMap(const FWorldContext& WorldContext, const FString& MapName)
{
	if (WorldContext.OwningGameInstance != GetGameInstance())
		return;

	bLoadingMap = true;
	UpdateLoadingScreen();
}

// [OwnClient]
void UJY_LoadingScreenSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (LoadedWorld == nullptr || LoadedWorld->GetGameInstance() != GetGameInstance())
		return;

	bLoadingMap = false;
}

/* 서버는 도착 맵에서 PC를 새로 만든다(GameModeBase HandleSeamlessTravelPlayer) — 그 PC가 올 때까지 옛 PC 화면을 가린다 */
// [OwnClient]
void UJY_LoadingScreenSubsystem::HandleSeamlessTravelStart(UWorld* TravelWorld, const FString& MapName)
{
	if (TravelWorld->GetGameInstance() != GetGameInstance())
		return;

	bWaitingForNewController = true;
	ControllerBeforeSeamlessTravel = GetGameInstance()->GetFirstLocalPlayerController(TravelWorld);
}
