#include "Input/JY_InputConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_InputConfig)

const UInputAction* UJY_InputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FJY_InputAction& Action : NativeInputActions)
	{
		if (Action.InputTag == InputTag)
			return Action.InputAction;
	}

	return nullptr;
}
