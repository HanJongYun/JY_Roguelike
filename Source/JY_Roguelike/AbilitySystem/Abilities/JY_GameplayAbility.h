#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "JY_GameplayAbility.generated.h"

/* 어빌리티가 입력에 어떻게 반응해 발동하는지 */
UENUM(BlueprintType)
enum class EJY_AbilityActivationPolicy : uint8
{
	/* 누른 순간 한 번 */
	OnInputTriggered,

	/* 누르고 있는 동안, 끝나면 다시 */
	WhileInputActive
};

/* 프로젝트 어빌리티 공용 부모 */
UCLASS(Abstract)
class UJY_GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UJY_GameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	EJY_AbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

protected:
	/* 설정값 — 각 어빌리티 생성자 또는 GA_ 블루프린트에서 */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JY|Ability Activation")
	EJY_AbilityActivationPolicy ActivationPolicy = EJY_AbilityActivationPolicy::OnInputTriggered;
};
