#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "Input/JY_InputConfig.h"
#include "JY_InputComponent.generated.h"

/* InputConfig 목록으로 입력을 한 번에 묶는 입력 컴포넌트 */
UCLASS()
class UJY_InputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	/* 태그에 맞는 직접 처리 입력을 함수에 묶기 */
	template<class UserClass, typename FuncType>
	void BindNativeAction(const UJY_InputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func);

	/* 어빌리티 입력 전부를 눌림·뗌 함수에 태그와 함께 묶기 */
	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const UJY_InputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc);
};

template<class UserClass, typename FuncType>
void UJY_InputComponent::BindNativeAction(const UJY_InputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func)
{
	const UInputAction* InputAction = InputConfig->FindNativeInputActionForTag(InputTag);
	if (ensureMsgf(InputAction != nullptr, TEXT("%s에 %s 입력이 없음"), *GetNameSafe(InputConfig), *InputTag.ToString()) == false)
		return;

	BindAction(InputAction, TriggerEvent, Object, Func);
}

/* 눌림은 Started — 누르고 있는 동안 매 프레임 오는 Triggered로 묶으면 끝난 어빌리티가 계속 다시 발동된다(대시 자동 연타). 계속 누름이 필요한 어빌리티는 ASC의 누름 목록이 처리 */
template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UJY_InputComponent::BindAbilityActions(const UJY_InputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc)
{
	for (const FJY_InputAction& Action : InputConfig->AbilityInputActions)
	{
		if (ensureMsgf(Action.InputAction != nullptr && Action.InputTag.IsValid() == true, TEXT("%s에 비어 있는 어빌리티 입력 칸"), *GetNameSafe(InputConfig)) == false)
			continue;

		BindAction(Action.InputAction, ETriggerEvent::Started, Object, PressedFunc, Action.InputTag);
		BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag);
	}
}
