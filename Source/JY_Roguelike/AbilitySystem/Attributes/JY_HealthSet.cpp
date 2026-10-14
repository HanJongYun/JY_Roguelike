#include "AbilitySystem/Attributes/JY_HealthSet.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "System/JY_GameplayTags.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_HealthSet)

UJY_HealthSet::UJY_HealthSet()
	: Health(100.0f)
	, MaxHealth(100.0f)
{
	bOutOfHealth = false;
	MaxHealthBeforeAttributeChange = 0.0f;
	HealthBeforeAttributeChange = 0.0f;
}

/* REPNOTIFY_Always — 값이 같아도 OnRep을 불러 클라 ASC의 값 변경 델리게이트가 빠짐없이 돌게 한다 (Lyra 동일) */
void UJY_HealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UJY_HealthSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UJY_HealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

// [OwnClient] [SimulatedProxy]
/* 방송만 한다. 클라가 속성을 바꾸는 코드는 금지 (LyraHealthSet.cpp OnRep_Health와 같은 원칙) */
void UJY_HealthSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	/* 클라 ASC 값 갱신 + 값 변경 델리게이트 방송 */
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJY_HealthSet, Health, OldValue);

	const float CurrentHealth = GetHealth();
	const float EstimatedMagnitude = CurrentHealth - OldValue.GetCurrentValue();

	/* 클라에선 가해자를 모름 → nullptr */
	OnHealthChanged.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentHealth);

	if (bOutOfHealth == false && CurrentHealth <= 0.0f)
	{
		OnOutOfHealth.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentHealth);
	}

	bOutOfHealth = (CurrentHealth <= 0.0f);
}

// [OwnClient] [SimulatedProxy]
void UJY_HealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJY_HealthSet, MaxHealth, OldValue);

	OnMaxHealthChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxHealth() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxHealth());
}

// [Server]
/* 무적이면 들어오는 데미지를 0으로 (LyraHealthSet::PreGameplayEffectExecute와 같음. 자폭·GodMode 예외는 쓰는 곳이 없어 뺐다) */
bool UJY_HealthSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) //override
{
	if (Super::PreGameplayEffectExecute(Data) == false)
		return false;

	const bool bIsIncomingDamage = Data.EvaluatedData.Attribute == GetDamageAttribute() && Data.EvaluatedData.Magnitude > 0.0f;
	if (bIsIncomingDamage == true && Data.Target.HasMatchingGameplayTag(JYGameplayTags::Gameplay_DamageImmunity) == true)
	{
		Data.EvaluatedData.Magnitude = 0.0f;
		return false;
	}

	/* 변경 전 값 저장 → Post에서 실제로 바뀌었는지 비교 */
	HealthBeforeAttributeChange = GetHealth();
	MaxHealthBeforeAttributeChange = GetMaxHealth();

	return true;
}

// [Server]
/* 메타 속성(Damage·Healing)을 Health 변화로 바꾸는 유일한 곳 */
void UJY_HealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) //override
{
	Super::PostGameplayEffectExecute(Data);

	const float MinimumHealth = 0.0f;

	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
	AActor* Instigator = EffectContext.GetOriginalInstigator();
	AActor* Causer = EffectContext.GetEffectCauser();

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		/* 데미지 → -Health, 쓴 메타 값은 0으로 */
		SetHealth(FMath::Clamp(GetHealth() - GetDamage(), MinimumHealth, GetMaxHealth()));
		SetDamage(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetHealingAttribute())
	{
		/* 회복 → +Health, 최대 체력에서 멈춤 */
		SetHealth(FMath::Clamp(GetHealth() + GetHealing(), MinimumHealth, GetMaxHealth()));
		SetHealing(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), MinimumHealth, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		OnMaxHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, MaxHealthBeforeAttributeChange, GetMaxHealth());
	}

	if (GetHealth() != HealthBeforeAttributeChange)
	{
		OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
	}

	if (GetHealth() <= 0.0f && bOutOfHealth == false)
	{
		OnOutOfHealth.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
	}

	/* 위 방송 중 체력이 다시 바뀌었을 수 있어 한 번 더 확인 */
	bOutOfHealth = (GetHealth() <= 0.0f);
}

void UJY_HealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const //override
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, OUT NewValue);
}

void UJY_HealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) //override
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, OUT NewValue);
}

void UJY_HealthSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) //override
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	/*
	 * 최대 체력이 줄어 현재 체력이 넘치면 현재 체력을 새 최대치로 덮어쓴다 (Lyra LyraHealthSet::PostAttributeChange와 같음).
	 * 늘 때 현재 체력을 채우는 건 전역 규칙이 아니다 — 메타 강화는 AJY_PlayerState::RefreshMetaEffects, 축복은 축복 쪽에서 정한다.
	 * 속성 값은 서버 GE로만 바뀌고 클라는 복제로 받는다 (예측 GE가 생기면 클라에서도 불릴 수 있어 그때 권한 검사를 넣는다).
	 */
	if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
	{
		UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponent();
		check(JYASC);

		JYASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Override, NewValue);
	}

	/* 회복으로 0 위로 올라오면 다음 0 도달 때 다시 방송하도록 */
	if (bOutOfHealth == true && GetHealth() > 0.0f)
	{
		bOutOfHealth = false;
	}
}

void UJY_HealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}
