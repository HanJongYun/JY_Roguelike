#include "UI/ViewModels/JY_StatsViewModel.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/JY_HealthSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_StatsViewModel)

/* ASC 연결 전(클라에 PlayerState 도착 전) 표시 — 빈 글자 대신 "모름" 상태를 보여준다 */
UJY_StatsViewModel::UJY_StatsViewModel()
{
	HealthText = INVTEXT("-- / --");
}

/* 색은 BP 자식에서 바꾼 NormalHealthColor가 적용된 뒤 맞춰야 해서 생성자가 아니라 여기서 */
void UJY_StatsViewModel::PostInitProperties() //override
{
	Super::PostInitProperties();

	HealthBarColor = NormalHealthColor;
}

/*
 * 1장 HUD(UJY_HUDLayout::BindAbilitySystem)에 있던 구독을 그대로 옮겨 왔다 — 화면이 몇 개든 구독은 여기 1번.
 * ASC 값 변경 델리게이트는 서버 GE 적용과 클라 복제(OnRep) 둘 다에서 방송된다 (GameplayEffect.cpp:3783, 3945~3973).
 */
// [OwnClient]
void UJY_StatsViewModel::BindAbilitySystem(UJY_AbilitySystemComponent* InAbilitySystemComponent)
{
	/* 같은 ASC로 두 번 불리면 구독이 두 겹이 되므로 먼저 끊고 다시 건다 */
	UnbindAbilitySystem();

	AbilitySystemComponent = InAbilitySystemComponent;
	if (InAbilitySystemComponent == nullptr)
		return;

	HealthChangedHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UJY_HealthSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);
	MaxHealthChangedHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UJY_HealthSet::GetMaxHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);

	/* 구독은 "다음 변화"부터라 이미 복제돼 온 지금 값은 직접 한 번 채운다 */
	RefreshHealthValues();
}

// [OwnClient]
void UJY_StatsViewModel::UnbindAbilitySystem()
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

/* 체력이 바뀌어도 최대 체력과 함께 계산해야 해서 둘 다 ASC에서 다시 읽는다 */
// [OwnClient]
void UJY_StatsViewModel::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshHealthValues();
}

/*
 * HUD의 RefreshHealth와 같은 계산 — 다만 위젯에 그리지 않고 값만 바꾼다. 그리는 건 WBP 바인딩(5-5).
 * UE_MVVM_SET_PROPERTY_VALUE는 값이 실제로 바뀐 칸만 알린다 (MVVMViewModelBase.h:81).
 */
// [OwnClient]
void UJY_StatsViewModel::RefreshHealthValues()
{
	const UJY_AbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC == nullptr)
		return;

	const float NewCurrentHealth = ASC->GetNumericAttribute(UJY_HealthSet::GetHealthAttribute());
	const float NewMaxHealth = ASC->GetNumericAttribute(UJY_HealthSet::GetMaxHealthAttribute());
	UE_MVVM_SET_PROPERTY_VALUE(CurrentHealth, NewCurrentHealth);
	UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, NewMaxHealth);

	/* 최대 체력 0이면 나눗셈을 피해 0으로 */
	const float NewPercent = NewMaxHealth > 0.0f ? FMath::Clamp(NewCurrentHealth / NewMaxHealth, 0.0f, 1.0f) : 0.0f;
	UE_MVVM_SET_PROPERTY_VALUE(HealthPercent, NewPercent);

	/*
	 * FText는 새로 만들면 글자가 같아도 "다르다"로 비교돼(Text.cpp:1455~1475, 깊은 비교는 옵션) 매번 알림이 나간다
	 * → 반올림 정수가 바뀔 때만 글자를 만든다. 예: 99.6 → 99.8은 둘 다 100이라 알림 0번. 번역 대상이 아닌 숫자 형식이라 INVTEXT.
	 */
	const int32 RoundedCurrentHealth = FMath::RoundToInt(NewCurrentHealth);
	const int32 RoundedMaxHealth = FMath::RoundToInt(NewMaxHealth);
	if (RoundedCurrentHealth != DisplayedCurrentHealth || RoundedMaxHealth != DisplayedMaxHealth)
	{
		DisplayedCurrentHealth = RoundedCurrentHealth;
		DisplayedMaxHealth = RoundedMaxHealth;
		UE_MVVM_SET_PROPERTY_VALUE(HealthText, FText::Format(INVTEXT("{0} / {1}"), FText::AsNumber(RoundedCurrentHealth), FText::AsNumber(RoundedMaxHealth)));
	}

	/* 기준·색을 여기서 정해 위젯은 값만 받는다 — 화면이 여러 개여도 같은 기준·색 */
	const bool bNewLowHealth = NewPercent <= LowHealthRatio;
	UE_MVVM_SET_PROPERTY_VALUE(bLowHealth, bNewLowHealth);
	UE_MVVM_SET_PROPERTY_VALUE(HealthBarColor, bNewLowHealth ? LowHealthColor : NormalHealthColor);
}
