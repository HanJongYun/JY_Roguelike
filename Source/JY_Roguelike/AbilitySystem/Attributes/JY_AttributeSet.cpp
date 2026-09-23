#include "AbilitySystem/Attributes/JY_AttributeSet.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AttributeSet)

UJY_AttributeSet::UJY_AttributeSet()
{
}

/* Lyra LyraAttributeSet.cpp와 동일 — Outer(PlayerState)의 월드를 돌려준다 */
UWorld* UJY_AttributeSet::GetWorld() const //override
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

UJY_AbilitySystemComponent* UJY_AttributeSet::GetJYAbilitySystemComponent() const
{
	return Cast<UJY_AbilitySystemComponent>(GetOwningAbilitySystemComponent());
}
