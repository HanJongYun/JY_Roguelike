#include "AbilitySystem/Tasks/JY_AbilityTask_WaitMoveInput.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AbilityTask_WaitMoveInput)

UJY_AbilityTask_WaitMoveInput::UJY_AbilityTask_WaitMoveInput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/* 엔진에 이동 입력 이벤트가 없어 Tick — 공격 1회 동안만 살아 비용은 무시할 수준 */
	bTickingTask = true;
}

UJY_AbilityTask_WaitMoveInput* UJY_AbilityTask_WaitMoveInput::WaitMoveInput(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<UJY_AbilityTask_WaitMoveInput>(OwningAbility);
}

// [OwnClient]
/* 끝내지 않고 계속 알린다 — 공격 동작 중 입력은 어빌리티가 무시하고 후딜레이에서 다시 본다 */
void UJY_AbilityTask_WaitMoveInput::TickTask(float DeltaTime) //override
{
	Super::TickTask(DeltaTime);

	const APawn* AvatarPawn = CastChecked<APawn>(GetAvatarActor());
	const FVector LastMovementInput = AvatarPawn->GetLastMovementInputVector();
	if (LastMovementInput.IsNearlyZero() == true)
		return;

	if (ShouldBroadcastAbilityTaskDelegates() == false)
		return;

	OnMoveInput.Broadcast();
}
