#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "JY_AbilitySystemComponent.generated.h"

/* 프로젝트 전용 ASC. 입력 태그로 어빌리티를 찾아 발동한다 */
UCLASS()
class UJY_AbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UJY_AbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;

public:
	/* 이 입력 태그를 가진 어빌리티를 눌림·누름 목록에 넣기 */
	void AbilityInputTagPressed(const FGameplayTag& InputTag);

	/* 이 입력 태그를 가진 어빌리티를 뗌 목록에 넣고 누름 목록에서 빼기 */
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	/* 이번 프레임 입력 목록으로 어빌리티 발동·입력 전달 */
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

private:
	/* 실행 중 어빌리티에 누름·뗌 복제 이벤트 내기 */
	void InvokeInputEventIfActive(FGameplayAbilitySpec& Spec, EAbilityGenericReplicatedEvent::Type EventType);

private:
	/* 이번 프레임에 눌린 어빌리티 */
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	/* 이번 프레임에 뗀 어빌리티 */
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	/* 누르고 있는 어빌리티 (뗄 때까지 유지) */
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};
