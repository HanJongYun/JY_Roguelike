#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "JY_PlayerState.generated.h"

class UAbilitySystemComponent;
class UJY_AbilitySystemComponent;
class UJY_HealthSet;

/** 플레이어의 ASC와 체력 속성을 가진 PlayerState */
UCLASS()
class AJY_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AJY_PlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface

	UFUNCTION(BlueprintCallable, Category = "JY|PlayerState")
	UJY_AbilitySystemComponent* GetJYAbilitySystemComponent() const { return AbilitySystemComponent; }

	const UJY_HealthSet* GetHealthSet() const { return HealthSet; }

private:
	/** 플레이어 능력·속성의 주인 */
	UPROPERTY(VisibleAnywhere, Category = "JY|PlayerState")
	TObjectPtr<UJY_AbilitySystemComponent> AbilitySystemComponent;

	/** 체력 속성 세트 (ASC가 InitializeComponent에서 찾아 등록) */
	UPROPERTY()
	TObjectPtr<const UJY_HealthSet> HealthSet;
};
