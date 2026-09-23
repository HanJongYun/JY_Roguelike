#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/JY_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "JY_HealthSet.generated.h"

/* 체력·최대 체력과 들어오는 데미지·회복을 담는 속성 세트 */
UCLASS(BlueprintType)
class UJY_HealthSet : public UJY_AttributeSet
{
	GENERATED_BODY()

public:
	UJY_HealthSet();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

public:
	ATTRIBUTE_ACCESSORS_BASIC(UJY_HealthSet, Health);
	ATTRIBUTE_ACCESSORS_BASIC(UJY_HealthSet, MaxHealth);
	ATTRIBUTE_ACCESSORS_BASIC(UJY_HealthSet, Healing);
	ATTRIBUTE_ACCESSORS_BASIC(UJY_HealthSet, Damage);

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	/* 체력은 0~최대, 최대 체력은 1 이상으로 자르기 */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

public:
	/* 델리게이트 */

	/* 체력이 바뀌면 방송 (클라에선 가해자 정보 없음) */
	mutable FJY_AttributeEvent OnHealthChanged;

	/* 최대 체력이 바뀌면 방송 */
	mutable FJY_AttributeEvent OnMaxHealthChanged;

	/* 체력이 0이 되면 한 번 방송 */
	mutable FJY_AttributeEvent OnOutOfHealth;

private:
	/* 복제 속성 — 서버에서 GE로만 바뀌고, 클라는 OnRep으로 받는다 */

	/* 현재 체력 (복제, ReplicatedUsing=OnRep_Health) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "JY|Health", Meta = (HideFromModifiers, AllowPrivateAccess = true))
	FGameplayAttributeData Health;

	/* 최대 체력 (복제, ReplicatedUsing=OnRep_MaxHealth) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "JY|Health", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHealth;

private:
	/*
	 * 메타 속성 — 복제 안 함. GE가 여기에 값을 넣으면
	 * 서버 PostGameplayEffectExecute에서 Health 변화로 바뀌고 0으로 돌아간다
	 */

	/* 들어오는 회복량 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Health", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Healing;

	/* 들어오는 데미지량 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Health", Meta = (HideFromModifiers, AllowPrivateAccess = true))
	FGameplayAttributeData Damage;

private:
	/* 체력 0 도달 여부 (OnOutOfHealth 중복 방송 방지) */
	bool bOutOfHealth;

	/* GE 적용 직전 값 */
	float MaxHealthBeforeAttributeChange;

	float HealthBeforeAttributeChange;
};
