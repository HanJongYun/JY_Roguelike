#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "JY_AbilitySet.generated.h"

class UJY_GameplayAbility;
class UJY_AbilitySystemComponent;

/* 지급할 어빌리티 하나와 그 입력 태그 */
USTRUCT(BlueprintType)
struct FJY_AbilitySet_GameplayAbility
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UJY_GameplayAbility> Ability = nullptr;

	UPROPERTY(EditDefaultsOnly)
	int32 AbilityLevel = 1;

	/* 비우면 입력으로 발동하지 않음 */
	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/* 지급한 어빌리티 핸들. 회수할 때 쓴다 */
USTRUCT(BlueprintType)
struct FJY_AbilitySet_GrantedHandles
{
	GENERATED_BODY()

public:
	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);

	/* 지급했던 어빌리티를 전부 회수 */
	void TakeFromAbilitySystem(UJY_AbilitySystemComponent* AbilitySystemComponent);

private:
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
};

/* 한 번에 지급하는 어빌리티 묶음 */
UCLASS(BlueprintType, Const)
class UJY_AbilitySet : public UDataAsset
{
	GENERATED_BODY()

public:
	/* 서버에서 어빌리티 지급. 입력 태그는 스펙에 심어 ASC가 입력으로 찾게 한다 */
	void GiveToAbilitySystem(UJY_AbilitySystemComponent* AbilitySystemComponent, FJY_AbilitySet_GrantedHandles& OutGrantedHandles, UObject* SourceObject) const;

protected:
	/* 설정값 — DA_JY_HeroAbilitySet에서 지정 */

	UPROPERTY(EditDefaultsOnly, Category = "Abilities", Meta = (TitleProperty = "Ability"))
	TArray<FJY_AbilitySet_GameplayAbility> GrantedGameplayAbilities;
};
