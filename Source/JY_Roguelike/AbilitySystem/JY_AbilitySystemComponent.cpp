#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AbilitySystemComponent)

UJY_AbilitySystemComponent::UJY_AbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

// [OwnClient]
/* 엔진 기본은 눌림을 로컬로만 전하는데 WaitInputPress/Release 태스크는 복제 이벤트를 기다려 여기서 낸다 (Lyra와 같음) */
void UJY_AbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) //override
{
	Super::AbilitySpecInputPressed(Spec);

	InvokeInputEventIfActive(Spec, EAbilityGenericReplicatedEvent::InputPressed);
}

// [OwnClient]
void UJY_AbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) //override
{
	Super::AbilitySpecInputReleased(Spec);

	InvokeInputEventIfActive(Spec, EAbilityGenericReplicatedEvent::InputReleased);
}

// [OwnClient]
/* 대표 인스턴스는 InstancedPerActor에만 있다 — BP에서 인스턴싱 정책을 바꿀 수 있어 없으면 건너뛴다 */
void UJY_AbilitySystemComponent::InvokeInputEventIfActive(FGameplayAbilitySpec& Spec, EAbilityGenericReplicatedEvent::Type EventType)
{
	if (Spec.IsActive() == false)
		return;

	const UGameplayAbility* PrimaryInstance = Spec.GetPrimaryInstance();
	if (PrimaryInstance == nullptr)
		return;

	const FGameplayAbilityActivationInfo ActivationInfo = PrimaryInstance->GetCurrentActivationInfo();
	const FPredictionKey OriginalPredictionKey = ActivationInfo.GetActivationPredictionKey();
	InvokeReplicatedEvent(EventType, Spec.Handle, OriginalPredictionKey);
}

// [OwnClient]
void UJY_AbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag) == true)
		{
			InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
		}
	}
}

// [OwnClient]
void UJY_AbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag) == true)
		{
			InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.Remove(AbilitySpec.Handle);
		}
	}
}

// [OwnClient] AJY_PlayerController::PostProcessInput에서 매 프레임
/* 한 프레임 입력을 다 모은 뒤 발동 — 같은 프레임의 이동 입력(대시 방향)을 어빌리티가 읽을 수 있다 */
void UJY_AbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		if (AbilitySpec == nullptr || AbilitySpec->IsActive() == true)
			continue;

		const UJY_GameplayAbility* AbilityCDO = Cast<UJY_GameplayAbility>(AbilitySpec->Ability);
		if (AbilityCDO != nullptr && AbilityCDO->GetActivationPolicy() == EJY_AbilityActivationPolicy::WhileInputActive)
		{
			AbilitiesToActivate.AddUnique(SpecHandle);
		}
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		if (AbilitySpec == nullptr)
			continue;

		AbilitySpec->InputPressed = true;
		if (AbilitySpec->IsActive() == true)
		{
			AbilitySpecInputPressed(*AbilitySpec);
			continue;
		}

		const UJY_GameplayAbility* AbilityCDO = Cast<UJY_GameplayAbility>(AbilitySpec->Ability);
		if (AbilityCDO != nullptr && AbilityCDO->GetActivationPolicy() == EJY_AbilityActivationPolicy::OnInputTriggered)
		{
			AbilitiesToActivate.AddUnique(SpecHandle);
		}
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(SpecHandle);
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		if (AbilitySpec == nullptr)
			continue;

		AbilitySpec->InputPressed = false;
		if (AbilitySpec->IsActive() == true)
		{
			AbilitySpecInputReleased(*AbilitySpec);
		}
	}

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}
