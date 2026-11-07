#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/JY_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "JY_CombatSet.generated.h"

/* 플레이어 공격 값 — 기본 공격력·공격력 증가 %·치명 확률 %·치명 배율 % */
UCLASS(BlueprintType)
class UJY_CombatSet : public UJY_AttributeSet
{
	GENERATED_BODY()

public:
	UJY_CombatSet();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	ATTRIBUTE_ACCESSORS_BASIC(UJY_CombatSet, BaseAttack);
	ATTRIBUTE_ACCESSORS_BASIC(UJY_CombatSet, AttackBonus);
	ATTRIBUTE_ACCESSORS_BASIC(UJY_CombatSet, CritChance);
	ATTRIBUTE_ACCESSORS_BASIC(UJY_CombatSet, CritMultiplier);

protected:
	UFUNCTION()
	void OnRep_BaseAttack(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AttackBonus(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CritChance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CritMultiplier(const FGameplayAttributeData& OldValue);

private:
	/* 복제 속성 — 본인에게만 복제 (HUD 표시용) */

	/* 기본 공격력 (복제, ReplicatedUsing=OnRep_BaseAttack) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseAttack, Category = "JY|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseAttack;

	/* 공격력 증가 % (복제, ReplicatedUsing=OnRep_AttackBonus). 제단·축복 GE가 더한다 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackBonus, Category = "JY|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData AttackBonus;

	/* 치명 확률 % (복제, ReplicatedUsing=OnRep_CritChance) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritChance, Category = "JY|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData CritChance;

	/* 치명 배율 % (복제, ReplicatedUsing=OnRep_CritMultiplier). 200 = 2배 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritMultiplier, Category = "JY|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData CritMultiplier;
};
