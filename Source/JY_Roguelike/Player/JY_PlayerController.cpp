#include "Player/JY_PlayerController.h"
#include "Player/JY_PlayerState.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Effects/JY_DebugGameplayEffects.h"
#include "System/JY_GameplayTags.h"
#include "System/JY_GameMode.h"
#include "GameFramework/CheatManagerDefines.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "UI/ViewModels/JY_StatsViewModel.h"
#include "UI/ViewModels/JY_ProgressViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_PlayerController)

AJY_PlayerController::AJY_PlayerController()
{
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

	UIManager->GetOrCreateViewModel<UJY_StatsViewModel>()->BindAbilitySystem(JYPlayerState->GetJYAbilitySystemComponent());
	UIManager->GetOrCreateViewModel<UJY_ProgressViewModel>()->BindPlayerState(JYPlayerState);
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

// [OwnClient] 콘솔 입력을 받은 로컬 컨트롤러
/* 0 이하는 무시 — 방향은 명령 이름이 정하고, 값은 크기만 받는다 */
void AJY_PlayerController::JYDamage(float Amount)
{
	if (Amount <= 0.0f)
		return;

	Server_DebugApplyHealthDelta(-Amount);
}

// [OwnClient] 콘솔 입력을 받은 로컬 컨트롤러
void AJY_PlayerController::JYHeal(float Amount)
{
	if (Amount <= 0.0f)
		return;

	Server_DebugApplyHealthDelta(Amount);
}

/*
 * 체력은 서버에서 GE로만 바꾼다. 클라가 직접 속성을 바꾸지 않는다.
 * UFUNCTION은 #if 블록 안에 둘 수 없어 선언은 두고, 본문만 치트 매니저가 없는 빌드에서 막는다.
 * CheatManager 검사 = Lyra ServerCheat(LyraPlayerController.cpp:332~341)와 같은 관문.
 *   서버 PC의 AddCheats는 GameMode AllowCheats일 때만 CheatManager를 만든다 (PIE는 항상 허용, 배포 서버는 막힘).
 *   이게 없으면 개발 빌드 전용 서버에서 개조 클라가 이 RPC로 무한 회복할 수 있다.
 */
// [Server]
void AJY_PlayerController::Server_DebugApplyHealthDelta_Implementation(float Delta)
{
#if UE_WITH_CHEAT_MANAGER
	if (CheatManager == nullptr)
		return;

	/* NaN·무한대 차단 */
	if (Delta == 0.0f || FMath::IsFinite(Delta) == false)
		return;

	AJY_PlayerState* JYPlayerState = GetPlayerState<AJY_PlayerState>();
	if (JYPlayerState == nullptr)
		return;

	UJY_AbilitySystemComponent* JYASC = JYPlayerState->GetJYAbilitySystemComponent();
	if (JYASC == nullptr)
		return;

	/* 음수=데미지 GE, 양수=회복 GE. 크기는 항상 양수로 SetByCaller에 싣는다 */
	const bool bIsDamage = Delta < 0.0f;
	const TSubclassOf<UGameplayEffect> EffectClass = bIsDamage ? UJY_GameplayEffect_DebugDamage::StaticClass() : UJY_GameplayEffect_DebugHeal::StaticClass();
	const FGameplayTag MagnitudeTag = bIsDamage ? JYGameplayTags::SetByCaller_Damage : JYGameplayTags::SetByCaller_Heal;

	FGameplayEffectContextHandle EffectContext = JYASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = JYASC->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
	if (SpecHandle.IsValid() == false)
		return;

	SpecHandle.Data->SetSetByCallerMagnitude(MagnitudeTag, FMath::Abs(Delta));
	JYASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
#endif
}

/* 값 검증(판매 대상·거리·가격)은 전부 GameMode — PC는 요청을 전달만 한다 */
// [Server]
void AJY_PlayerController::Server_BuyMetaUpgrade_Implementation(FGameplayTag UpgradeTag, AJY_InteractableActor* Seller)
{
	AJY_GameMode* GameMode = GetWorld()->GetAuthGameMode<AJY_GameMode>();
	if (GameMode == nullptr)
		return;

	GameMode->TryBuyMetaUpgrade(this, UpgradeTag, Seller);
}

// [Server]
void AJY_PlayerController::Server_RefundAllMetaUpgrades_Implementation(AJY_InteractableActor* Seller)
{
	AJY_GameMode* GameMode = GetWorld()->GetAuthGameMode<AJY_GameMode>();
	if (GameMode == nullptr)
		return;

	GameMode->RefundAllMetaUpgrades(this, Seller);
}

// [OwnClient] 콘솔 입력을 받은 로컬 컨트롤러
void AJY_PlayerController::JYAddSoul(int32 Amount)
{
	if (Amount <= 0)
		return;

	Server_DebugAddSoulStones(Amount);
}

/* 체력 치트와 같은 관문 — 치트가 허용된 서버(PIE)에서만 */
// [Server]
void AJY_PlayerController::Server_DebugAddSoulStones_Implementation(int32 Amount)
{
#if UE_WITH_CHEAT_MANAGER
	if (CheatManager == nullptr || Amount <= 0)
		return;

	AJY_GameMode* GameMode = GetWorld()->GetAuthGameMode<AJY_GameMode>();
	if (GameMode == nullptr)
		return;

	GameMode->AddSoulStones(this, Amount);
#endif
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
