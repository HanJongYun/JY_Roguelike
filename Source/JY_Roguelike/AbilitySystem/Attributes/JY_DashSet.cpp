#include "AbilitySystem/Attributes/JY_DashSet.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_DashSet)

/* 시작 횟수 (임시 2회) */
UJY_DashSet::UJY_DashSet()
	: DashCharges(2.0f)
	, MaxDashCharges(2.0f)
{
}

void UJY_DashSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UJY_DashSet, DashCharges, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UJY_DashSet, MaxDashCharges, COND_OwnerOnly, REPNOTIFY_Always);
}

// [OwnClient]
void UJY_DashSet::OnRep_DashCharges(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJY_DashSet, DashCharges, OldValue);
}

// [OwnClient]
void UJY_DashSet::OnRep_MaxDashCharges(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJY_DashSet, MaxDashCharges, OldValue);
}

void UJY_DashSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const //override
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, OUT NewValue);
}

void UJY_DashSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) //override
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, OUT NewValue);
}

/*
 * 최대 횟수가 줄면 남은 횟수를 깎기만 한다
 * 늘 때 채우는 건 최대 횟수를 올리는 축복 GE 몫
 */
void UJY_DashSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) //override
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxDashChargesAttribute() && GetDashCharges() > NewValue)
	{
		UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponent();
		check(JYASC);

		JYASC->ApplyModToAttribute(GetDashChargesAttribute(), EGameplayModOp::Override, NewValue);
	}
}

void UJY_DashSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetDashChargesAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxDashCharges());
	}
	else if (Attribute == GetMaxDashChargesAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}
