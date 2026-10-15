#include "Player/JY_PlayerController.h"
#include "Player/JY_PlayerState.h"
#include "Player/JY_CheatManager.h"
#include "Camera/JY_CameraOcclusionFadeComponent.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "System/JY_GameMode.h"
#include "Skill/JY_SkillLoadoutComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "UI/ViewModels/JY_StatsViewModel.h"
#include "UI/ViewModels/JY_ProgressViewModel.h"
#include "UI/ViewModels/JY_MetaUpgradeViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_PlayerController)

AJY_PlayerController::AJY_PlayerController()
{
	CameraOcclusionFadeComponent = CreateDefaultSubobject<UJY_CameraOcclusionFadeComponent>(TEXT("CameraOcclusionFade"));
	CheatClass = UJY_CheatManager::StaticClass();
}

/* 서버·혼자 플레이는 BeginPlay 때 PlayerState가 이미 있어 OnRep이 안 온다 → 여기서 연결 */
// [OwnClient] 로컬 컨트롤러만
void AJY_PlayerController::BeginPlay() //override
{
	Super::BeginPlay();

	InitializeLocalUI();
}

/* 클라는 PlayerState가 BeginPlay보다 늦게 복제돼 올 수 있어 도착 시점에 다시 연결 */
// [OwnClient]
void AJY_PlayerController::OnRep_PlayerState() //override
{
	Super::OnRep_PlayerState();

	InitializeLocalUI();
}

/*
 * 복제 상태 → ViewModel 연결은 여기 한 곳. 새 복제 ViewModel도 여기 1줄 추가.
 * BeginPlay·OnRep 둘 다 불러도 안전 — PlayerState가 없으면 그냥 돌아가고, Bind 함수들은 먼저 끊고 다시 건다.
 */
// [OwnClient] 로컬 컨트롤러만
void AJY_PlayerController::InitializeLocalUI()
{
	if (IsLocalPlayerController() == false)
		return;

	UJY_UIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(GetLocalPlayer());
	if (UIManager == nullptr)
		return;

	AJY_PlayerState* JYPlayerState = GetPlayerState<AJY_PlayerState>();
	if (JYPlayerState == nullptr)
		return;

	UJY_StatsViewModel* StatsViewModel = UIManager->GetOrCreateViewModel<UJY_StatsViewModel>();
	StatsViewModel->BindAbilitySystem(JYPlayerState->GetJYAbilitySystemComponent());

	UJY_ProgressViewModel* ProgressViewModel = UIManager->GetOrCreateViewModel<UJY_ProgressViewModel>();
	ProgressViewModel->BindPlayerState(JYPlayerState);

	UJY_MetaUpgradeViewModel* MetaUpgradeViewModel = UIManager->GetOrCreateViewModel<UJY_MetaUpgradeViewModel>();
	MetaUpgradeViewModel->BindPlayerState(JYPlayerState);
}

/*
 * 맵에 도착한 뒤 PlayerState(영혼석·체력 원본)와 캐릭터가 둘 다 복제돼 올 때까지 로딩창 유지.
 * 하나라도 없으면 HUD가 빈 값이거나 캐릭터 없는 빈 방이 보인다.
 * 지금은 죽어도 캐릭터가 사라지지 않는다 — 부활(폰 교체)을 만들면 이 조건을 다시 본다.
 */
// [OwnClient]
bool AJY_PlayerController::ShouldShowLoadingScreen() const //override
{
	return PlayerState == nullptr || GetPawn() == nullptr;
}

/*
 * 클라는 GameMode가 없어 엔진이 CheatManager를 안 만든다(PlayerController.cpp:1162) — 콘솔 명령을 받아 서버로 보낼 쪽이라 만든다(Lyra AddCheats LyraPlayerController.cpp:323).
 * 실행 여부는 서버 PC의 CheatManager가 정한다 — 서버는 엔진 판단(AllowCheats) 그대로.
 */
// [OwnClient]
void AJY_PlayerController::AddCheats(bool bForce) //override
{
	const bool bClient = GetNetMode() == NM_Client;
	Super::AddCheats(bForce == true || bClient == true);
}

// [OwnClient] 로컬 컨트롤러만
/* 템플릿과 같은 자리 — 로컬 플레이어가 붙은 뒤 호출되어 서브시스템을 확실히 얻는다 */
void AJY_PlayerController::SetupInputComponent() //override
{
	Super::SetupInputComponent();

	if (IsLocalPlayerController() == false)
		return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem == nullptr)
		return;

	for (UInputMappingContext* MappingContext : DefaultMappingContexts)
	{
		if (MappingContext != nullptr)
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}
}

// [OwnClient] 로컬 컨트롤러만 — 입력 바인딩 실행 뒤 엔진이 호출 (UPlayerInput::ProcessInputStack, PlayerInput.cpp:1310)
void AJY_PlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused) //override
{
	/* 클라는 PlayerState가 복제로 늦게 올 수 있다 */
	AJY_PlayerState* JYPlayerState = GetPlayerState<AJY_PlayerState>();
	if (JYPlayerState != nullptr)
	{
		UJY_AbilitySystemComponent* JYASC = JYPlayerState->GetJYAbilitySystemComponent();
		JYASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

/* 값 검증(제단·거리·가격)은 전부 GameMode — PC는 요청을 전달만 한다 */
// [Server]
void AJY_PlayerController::Server_BuyMetaUpgrade_Implementation(FGameplayTag UpgradeTag, AJY_InteractableActor* Altar)
{
	AJY_GameMode* GameMode = GetWorld()->GetAuthGameMode<AJY_GameMode>();
	if (GameMode == nullptr)
		return;

	GameMode->TryBuyMetaUpgrade(this, UpgradeTag, Altar);
}

// [Server]
void AJY_PlayerController::Server_RefundAllMetaUpgrades_Implementation(AJY_InteractableActor* Altar)
{
	AJY_GameMode* GameMode = GetWorld()->GetAuthGameMode<AJY_GameMode>();
	if (GameMode == nullptr)
		return;

	GameMode->RefundAllMetaUpgrades(this, Altar);
}

// [OwnClient]
void AJY_PlayerController::JYChooseSkillCard(int32 ChoiceIndex)
{
	Server_ChooseSkillCard(ChoiceIndex);
}

// [Server]
void AJY_PlayerController::Server_ChooseSkillCard_Implementation(int32 ChoiceIndex)
{
	AJY_PlayerState* JYPlayerState = GetPlayerState<AJY_PlayerState>();
	if (JYPlayerState == nullptr)
		return;

	UJY_SkillLoadoutComponent* SkillLoadout = JYPlayerState->GetSkillLoadoutComponent();
	SkillLoadout->ChooseUpgradeCard(ChoiceIndex);
}

/*
 * 엔진 ServerPause는 토글이라(PlayerController.cpp:1980 SetPause(!IsPaused())) 요청이 두 번 오면 반대가 된다 → 원하는 상태를 bool로 받는다.
 * 1인 게임이라 클라 요청을 그대로 받는다. SetPause → GameMode::SetPause → WorldSettings 정지 정보 복제 → 클라 월드도 멈춤.
 */
// [Server]
void AJY_PlayerController::Server_SetUIPaused_Implementation(bool bPaused)
{
	/* 정지를 건 PC에 해제 조건을 묶어 둔다 — 이 PC가 나가면 엔진이 그 정지를 지운다 */
	SetPause(bPaused, FCanUnpause::CreateUObject(this, &APlayerController::DefaultCanUnpause));
}

/*
 * 서버가 이 PC에만 보내는 일회성 알림 — 놓쳐도 게임 상태가 틀어지지 않는 것 전용.
 * 서버가 여는 화면(축복 선택 등)은 RPC가 아니라 복제 상태로 연다 (늦게 접속·재접속해도 상태로 복원되게).
 */
// [Server -> Client RPC]
void AJY_PlayerController::Client_ShowNotification_Implementation(EJY_NotificationType Type, const FText& Message)
{
	ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(GetLocalPlayer())->ShowNotification(Type, Message);
}
