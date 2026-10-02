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
#include "UI/Foundation/JY_HUDLayout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_PlayerController)

AJY_PlayerController::AJY_PlayerController()
{
}

// [OwnClient] 로컬 컨트롤러만
/* [UI 과외 임시] 0-3 방식 — 위젯을 만들어 뷰포트에 바로 올린다 */
void AJY_PlayerController::BeginPlay() //override
{
	Super::BeginPlay();

	if (IsLocalPlayerController() == false || HUDClass == nullptr)
		return;

	HUDWidget = CreateWidget<UJY_HUDLayout>(this, HUDClass);
	if (HUDWidget == nullptr)
		return;

	HUDWidget->AddToViewport();

	/* 클라는 이 시점에 PlayerState가 아직 없을 수 있다 — 없으면 OnRep_PlayerState에서 연결 */
	BindHUDToPlayerState();
}

// [OwnClient] 복제로 PlayerState가 도착(또는 교체)했을 때. 서버에서는 불리지 않음
void AJY_PlayerController::OnRep_PlayerState() //override
{
	Super::OnRep_PlayerState();

	BindHUDToPlayerState();
}

// [OwnClient]
void AJY_PlayerController::BindHUDToPlayerState()
{
	if (HUDWidget == nullptr)
		return;

	/* 체력(ASC)과 영혼석(진행 값)은 PlayerState가 가진다. BeginPlay·OnRep 두 번 불려도 Bind 함수들이 먼저 끊고 다시 건다 */
	AJY_PlayerState* JYPlayerState = GetPlayerState<AJY_PlayerState>();
	if (JYPlayerState == nullptr)
		return;

	HUDWidget->BindAbilitySystem(JYPlayerState->GetJYAbilitySystemComponent());
	HUDWidget->BindProgress(JYPlayerState);
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
