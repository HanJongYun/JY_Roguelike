#include "AbilitySystem/JY_AbilitySet.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AbilitySet)

void FJY_AbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	AbilitySpecHandles.Add(Handle);
}

// [Server]
void FJY_AbilitySet_GrantedHandles::TakeFromAbilitySystem(UJY_AbilitySystemComponent* AbilitySystemComponent)
{
	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		AbilitySystemComponent->ClearAbility(Handle);
	}

	AbilitySpecHandles.Reset();
}

// [Server]
void UJY_AbilitySet::GiveToAbilitySystem(UJY_AbilitySystemComponent* AbilitySystemComponent, FJY_AbilitySet_GrantedHandles& OutGrantedHandles, UObject* SourceObject) const
{
	for (const FJY_AbilitySet_GameplayAbility& Entry : GrantedGameplayAbilities)
	{
		if (ensureMsgf(Entry.Ability != nullptr, TEXT("%s에 비어 있는 어빌리티 칸"), *GetNameSafe(this)) == false)
			continue;

		FGameplayAbilitySpec AbilitySpec(Entry.Ability, Entry.AbilityLevel);
		AbilitySpec.SourceObject = SourceObject;

		/* ASC::AbilityInputTagPressed가 이 태그로 어빌리티를 찾는다 */
		if (Entry.InputTag.IsValid() == true)
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(Entry.InputTag);
		}

		const FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(AbilitySpec);
		OutGrantedHandles.AddAbilitySpecHandle(Handle);
	}
}
