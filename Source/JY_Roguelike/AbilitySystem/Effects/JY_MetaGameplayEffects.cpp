#include "AbilitySystem/Effects/JY_MetaGameplayEffects.h"
#include "AbilitySystem/Attributes/JY_HealthSet.h"
#include "System/JY_GameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MetaGameplayEffects)

/* Infinite — ASC가 PlayerState에 있어 폰이 다시 생겨도 남는다. 단계가 바뀌면 크기만 바꾼다(AJY_PlayerState::RefreshMetaEffects) */
UJY_GameplayEffect_MetaMaxHealth::UJY_GameplayEffect_MetaMaxHealth()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = JYGameplayTags::SetByCaller_MetaValue;

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute = UJY_HealthSet::GetMaxHealthAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::AddBase;
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(ModifierInfo);
}
