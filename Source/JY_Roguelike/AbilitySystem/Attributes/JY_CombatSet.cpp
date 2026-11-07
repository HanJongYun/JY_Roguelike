#include "AbilitySystem/Attributes/JY_CombatSet.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_CombatSet)

/* 시작 값 (임시) — 공격력 10이면 행동 배율 10000 = 피해 10 */
UJY_CombatSet::UJY_CombatSet()
	: BaseAttack(10.0f)
	, AttackBonus(0.0f)
	, CritChance(10.0f)
	, CritMultiplier(200.0f)
{
}

void UJY_CombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UJY_CombatSet, BaseAttack, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UJY_CombatSet, AttackBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UJY_CombatSet, CritChance, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UJY_CombatSet, CritMultiplier, COND_OwnerOnly, REPNOTIFY_Always);
}

// [OwnClient]
void UJY_CombatSet::OnRep_BaseAttack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJY_CombatSet, BaseAttack, OldValue);
}

// [OwnClient]
void UJY_CombatSet::OnRep_AttackBonus(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJY_CombatSet, AttackBonus, OldValue);
}

// [OwnClient]
void UJY_CombatSet::OnRep_CritChance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJY_CombatSet, CritChance, OldValue);
}

// [OwnClient]
void UJY_CombatSet::OnRep_CritMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UJY_CombatSet, CritMultiplier, OldValue);
}
