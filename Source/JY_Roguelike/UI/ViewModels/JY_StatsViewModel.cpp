#include "UI/ViewModels/JY_StatsViewModel.h"
#include "AbilitySystem/Attributes/JY_HealthSet.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_StatsViewModel)

/* ASC 연결 전(클라에 PlayerState 도착 전) 표시 — 빈 막대 대신 "모름" 상태를 보여준다 */
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

// [OwnClient] 로컬 플레이어만 (UIManager가 LocalPlayer에만 있음)
/*
 * HealthSet 이벤트 대신 ASC 값 변경 델리게이트를 쓰는 이유:
 * 클라 OnRep 경로(GAMEPLAYATTRIBUTE_REPNOTIFY)와 서버 GE 경로 둘 다에서 방송된다 (GameplayEffect.cpp:3783, 3945~3973).
 * → 서버에서 직접 바뀌든 복제로 받든 같은 코드로 갱신된다.
 */
void UJY_StatsViewModel::BindAbilitySystem(UJY_AbilitySystemComponent* InASC)
{
	if (InASC == nullptr)
		return;

	if (AbilitySystemComponent.Get() != InASC)
	{
		UnbindAbilitySystem();

		AbilitySystemComponent = InASC;
		HealthChangedHandle = InASC->GetGameplayAttributeValueChangeDelegate(UJY_HealthSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);
		MaxHealthChangedHandle = InASC->GetGameplayAttributeValueChangeDelegate(UJY_HealthSet::GetMaxHealthAttribute()).AddUObject(this, &ThisClass::HandleMaxHealthChanged);
	}

	/* 구독 전에 이미 복제된 값이 있으므로 현재 값으로 한 번 채운다 (접속 직후 0으로 뜨는 문제 방지) */
	RefreshDisplayValues(InASC->GetNumericAttribute(UJY_HealthSet::GetHealthAttribute()), InASC->GetNumericAttribute(UJY_HealthSet::GetMaxHealthAttribute()));
}

void UJY_StatsViewModel::UnbindAbilitySystem()
{
	UJY_AbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC != nullptr)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UJY_HealthSet::GetHealthAttribute()).Remove(HealthChangedHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(UJY_HealthSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
	}

	HealthChangedHandle.Reset();
	MaxHealthChangedHandle.Reset();
	AbilitySystemComponent.Reset();
}

void UJY_StatsViewModel::BeginDestroy() //override
{
	UnbindAbilitySystem();

	Super::BeginDestroy();
}

void UJY_StatsViewModel::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshDisplayValues(ChangeData.NewValue, MaxHealth);
}

void UJY_StatsViewModel::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshDisplayValues(CurrentHealth, ChangeData.NewValue);
}

/* 값이 실제로 바뀐 필드만 알림이 나간다 (SetPropertyValue가 비교) */
void UJY_StatsViewModel::RefreshDisplayValues(float NewCurrentHealth, float NewMaxHealth)
{
	UE_MVVM_SET_PROPERTY_VALUE(CurrentHealth, NewCurrentHealth);
	UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, NewMaxHealth);

	/* 최대 체력 0이면 나눗셈 방지 */
	const float NewPercent = NewMaxHealth > 0.0f ? FMath::Clamp(NewCurrentHealth / NewMaxHealth, 0.0f, 1.0f) : 0.0f;
	UE_MVVM_SET_PROPERTY_VALUE(HealthPercent, NewPercent);

	const FText NewText = FText::Format(INVTEXT("{0} / {1}"), FText::AsNumber(FMath::RoundToInt(NewCurrentHealth)), FText::AsNumber(FMath::RoundToInt(NewMaxHealth)));
	UE_MVVM_SET_PROPERTY_VALUE(HealthText, NewText);

	const bool bNewLowHealth = NewPercent <= LowHealthRatio;
	UE_MVVM_SET_PROPERTY_VALUE(bLowHealth, bNewLowHealth);

	/* 색을 ViewModel에서 정해 위젯은 값만 받는다 — 여러 화면이 같은 기준·색을 공유 */
	UE_MVVM_SET_PROPERTY_VALUE(HealthBarColor, bNewLowHealth ? LowHealthColor : NormalHealthColor);
}
