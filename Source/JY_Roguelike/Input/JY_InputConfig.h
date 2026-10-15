#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "JY_InputConfig.generated.h"

class UInputAction;

/* 입력 액션 하나와 그 입력 태그의 짝 */
USTRUCT(BlueprintType)
struct FJY_InputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/* 캐릭터 입력 목록. 직접 처리하는 입력과 어빌리티로 가는 입력을 나눠 담는다 */
UCLASS(Const)
class UJY_InputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/* 직접 처리 입력에서 태그에 맞는 입력 액션 찾기 */
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag) const;

public:
	/* 설정값 — DA_JY_InputConfig에서 지정 */

	/* 캐릭터 함수에 바로 연결 (이동·상호작용) */
	UPROPERTY(EditDefaultsOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FJY_InputAction> NativeInputActions;

	/* 같은 태그로 지급된 어빌리티를 발동 (점프·대시) */
	UPROPERTY(EditDefaultsOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FJY_InputAction> AbilityInputActions;
};
