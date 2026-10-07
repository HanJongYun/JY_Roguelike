#include "UI/Foundation/JY_HUDLayout.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/JY_HealthSet.h"
#include "Player/JY_PlayerState.h"
#include "Components/ProgressBar.h"
#include "CommonTextBlock.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "System/JY_GameplayTags.h"
#include "Engine/LocalPlayer.h"
#include "Engine/AssetManager.h"
#include "ICommonInputModule.h"
#include "CommonInputSettings.h"
#include "Input/CommonUIInputTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_HUDLayout)

/* HUD는 켜져 있는 동안 캐릭터 이동·공격이 돼야 하고(Game), Tab으로 닫히면 안 된다 */
UJY_HUDLayout::UJY_HUDLayout(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EJY_ScreenInputMode::Game;
	bIsBackHandler = false;
}

/*
 * bIsBackHandler를 쓰지 않는 이유: 엔진이 등록하는 뒤로가기 바인딩은 Menu 모드 전용(CommonUIInputTypes.h:62 기본값)이고,
 * 라우터는 현재 입력 모드와 바인딩 모드가 같을 때만 실행한다(UIActionRouterTypes.cpp:851) → Game 모드인 HUD에선 무시된다.
 * 그래서 같은 뒤로가기 액션을 Game 모드로 직접 등록한다.
 * 정지 메뉴가 열려 있으면 입력 모드가 Menu라 이 바인딩은 무시되고, 메뉴의 뒤로가기(닫기)가 받는다.
 */
// [OwnClient]
void UJY_HUDLayout::NativeOnInitialized() //override
{
	Super::NativeOnInitialized();

	const UInputAction* BackAction = ICommonInputModule::GetSettings().GetEnhancedInputBackAction();
	if (BackAction != nullptr)
	{
		FBindUIActionArgs BindArgs(BackAction, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleBackAction));
		BindArgs.InputMode = ECommonInputMode::Game;
		RegisterUIActionBinding(BindArgs);
	}

	/* 정지 메뉴를 처음 열 때 끊기지 않게 지금 비동기로 미리 올려 둔다 — 핸들을 쥐고 있는 동안 로드한 클래스가 메모리에 남는다(StreamableManager.h:195) */
	if (EscapeMenuClass.IsNull() == false)
	{
		EscapeMenuPreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(EscapeMenuClass.ToSoftObjectPath());
	}
}

// [OwnClient]
void UJY_HUDLayout::HandleBackAction()
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (LocalPlayer == nullptr)
		return;

	UJY_UIManagerSubsystem* UIManagerSubsystem = LocalPlayer->GetSubsystem<UJY_UIManagerSubsystem>();
	if (UIManagerSubsystem == nullptr)
		return;

	/* 미리 로드가 끝났으면 즉시 반환, 아직이면 그 자리에서 마저 로드 */
	const TSubclassOf<UJY_ScreenWidget> MenuClass = EscapeMenuClass.LoadSynchronous();
	if (MenuClass == nullptr)
		return;

	UIManagerSubsystem->PushScreen(JYGameplayTags::UI_Layer_Menu, MenuClass);
}

// [OwnClient]
void UJY_HUDLayout::NativeDestruct() //override
{
	/* 위젯이 사라진 뒤 ASC가 지운 위젯을 부르지 않도록 구독 해제 */
	UnbindAbilitySystem();
	UnbindProgress();

	Super::NativeDestruct();
}

// [OwnClient]
void UJY_HUDLayout::BindAbilitySystem(UJY_AbilitySystemComponent* InAbilitySystemComponent)
{
	/* 같은 ASC로 두 번 불리면 구독이 두 겹이 되므로 먼저 끊고 다시 건다 */
	UnbindAbilitySystem();

	AbilitySystemComponent = InAbilitySystemComponent;
	if (InAbilitySystemComponent == nullptr)
		return;

	/* 값이 바뀔 때만 불림 — 가만히 있으면 호출 0번 (Tick 대신) */
	HealthChangedHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UJY_HealthSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);
	MaxHealthChangedHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UJY_HealthSet::GetMaxHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);

	/* 구독은 "다음 변화"부터라 지금 값은 직접 한 번 표시 */
	RefreshHealth();
}

// [OwnClient]
void UJY_HUDLayout::UnbindAbilitySystem()
{
	UJY_AbilitySystemComponent* OldAbilitySystemComponent = AbilitySystemComponent.Get();
	if (OldAbilitySystemComponent != nullptr)
	{
		OldAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UJY_HealthSet::GetHealthAttribute()).Remove(HealthChangedHandle);
		OldAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UJY_HealthSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
	}

	HealthChangedHandle.Reset();
	MaxHealthChangedHandle.Reset();
	AbilitySystemComponent.Reset();
}

// [OwnClient] ASC 알림 — 서버 적용 후 복제(OnRep_Health)로 값이 들어올 때
void UJY_HUDLayout::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	/* 체력이 바뀌어도 최대 체력과 함께 계산해야 해서 둘 다 다시 읽는다 */
	RefreshHealth();
}

// [OwnClient]
void UJY_HUDLayout::RefreshHealth()
{
	const UJY_AbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC == nullptr)
		return;

	const float CurrentHealth = ASC->GetNumericAttribute(UJY_HealthSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(UJY_HealthSet::GetMaxHealthAttribute());

	/* 최대 체력 0이면 나눗셈을 피해 0으로 */
	HealthBar->SetPercent(MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f);

	/* 번역 대상이 아닌 숫자 형식이라 INVTEXT */
	HealthText->SetText(FText::Format(INVTEXT("{0} / {1}"), FText::AsNumber(FMath::RoundToInt(CurrentHealth)), FText::AsNumber(FMath::RoundToInt(MaxHealth))));
}

// [OwnClient]
void UJY_HUDLayout::BindProgress(AJY_PlayerState* InPlayerState)
{
	/* BeginPlay·OnRep_PlayerState 두 번 불려도 구독이 두 겹이 되지 않게 먼저 끊는다 */
	UnbindProgress();

	ProgressPlayerState = InPlayerState;
	if (InPlayerState == nullptr)
		return;

	/* 서버 SetProgress → 복제 → OnRep_Progress에서 방송될 때만 불림 */
	ProgressChangedHandle = InPlayerState->OnProgressChanged.AddUObject(this, &ThisClass::RefreshSoulStones);

	/* 구독은 "다음 변화"부터라 지금 값은 직접 한 번 표시 */
	RefreshSoulStones();
}

// [OwnClient]
void UJY_HUDLayout::UnbindProgress()
{
	AJY_PlayerState* OldPlayerState = ProgressPlayerState.Get();
	if (OldPlayerState != nullptr)
	{
		OldPlayerState->OnProgressChanged.Remove(ProgressChangedHandle);
	}

	ProgressChangedHandle.Reset();
	ProgressPlayerState.Reset();
}

// [OwnClient]
void UJY_HUDLayout::RefreshSoulStones()
{
	const AJY_PlayerState* JYPlayerState = ProgressPlayerState.Get();
	if (JYPlayerState == nullptr)
		return;

	SoulText->SetText(FText::AsNumber(JYPlayerState->GetProgress().SoulStones));
}
