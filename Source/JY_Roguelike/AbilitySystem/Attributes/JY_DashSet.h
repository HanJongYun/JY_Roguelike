#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/JY_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "JY_DashSet.generated.h"

/* 대시 남은 횟수와 최대 횟수 */
UCLASS(BlueprintType)
class UJY_DashSet : public UJY_AttributeSet
{
	GENERATED_BODY()

public:
	UJY_DashSet();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

public:
	ATTRIBUTE_ACCESSORS_BASIC(UJY_DashSet, DashCharges);
	ATTRIBUTE_ACCESSORS_BASIC(UJY_DashSet, MaxDashCharges);

protected:
	UFUNCTION()
	void OnRep_DashCharges(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxDashCharges(const FGameplayAttributeData& OldValue);

private:
	/* 남은 횟수를 0~최대, 최대 횟수를 1 이상으로 */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:
	/* 복제 속성 — 본인에게만 복제 */

	/* 남은 대시 횟수 (복제, ReplicatedUsing=OnRep_DashCharges) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DashCharges, Category = "JY|Dash", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData DashCharges;

	/* 최대 대시 횟수 (복제, ReplicatedUsing=OnRep_MaxDashCharges). 축복이 GE로 올린다 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxDashCharges, Category = "JY|Dash", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxDashCharges;
};
