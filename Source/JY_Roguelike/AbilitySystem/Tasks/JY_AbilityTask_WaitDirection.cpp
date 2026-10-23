#include "AbilitySystem/Tasks/JY_AbilityTask_WaitDirection.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AbilityTask_WaitDirection)

namespace JY_AbilityTask_WaitDirection
{
	/* 방향만 필요해 회전만 담은 변환을 LocationInfo에 넣는다 */
	FGameplayAbilityTargetDataHandle MakeDirectionTargetData(const FVector& Direction)
	{
		const FRotator DirectionRotation = Direction.Rotation();

		FGameplayAbilityTargetData_LocationInfo* DirectionData = new FGameplayAbilityTargetData_LocationInfo();
		DirectionData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
		DirectionData->TargetLocation.LiteralTransform = FTransform(DirectionRotation);
		return FGameplayAbilityTargetDataHandle(DirectionData);
	}

	FVector GetDirectionFromTargetData(const FGameplayAbilityTargetDataHandle& DataHandle)
	{
		/* 서버에선 네트워크로 온 값이라 비어 있을 수 있다 */
		const FGameplayAbilityTargetData* DirectionData = DataHandle.Get(0);
		if (DirectionData == nullptr)
			return FVector::ZeroVector;

		const FTransform DirectionTransform = DirectionData->GetEndPointTransform();
		const FRotator DirectionRotation = DirectionTransform.Rotator();
		const FRotator YawRotation(0.0f, DirectionRotation.Yaw, 0.0f);
		return YawRotation.Vector();
	}
}

UJY_AbilityTask_WaitDirection* UJY_AbilityTask_WaitDirection::WaitDirection(UGameplayAbility* OwningAbility, bool bAcceptRepeated)
{
	UJY_AbilityTask_WaitDirection* Task = NewAbilityTask<UJY_AbilityTask_WaitDirection>(OwningAbility);
	Task->bAcceptRepeated = bAcceptRepeated;
	return Task;
}

// [Server] [OwnClient]
/* 서버는 클라 커서·이동 입력을 모른다 — 조종 클라가 TargetData로 보내고 서버는 실행 키로 구독해 기다린다 (Lyra RangedWeapon 방식) */
void UJY_AbilityTask_WaitDirection::Activate() //override
{
	Super::Activate();

	SpecHandle = GetAbilitySpecHandle();
	ActivationPredictionKey = GetActivationPredictionKey();

	UJY_AbilitySystemComponent* JYASC = CastChecked<UJY_AbilitySystemComponent>(AbilitySystemComponent.Get());
	FAbilityTargetDataSetDelegate& DirectionDelegate = JYASC->AbilityTargetDataSetDelegate(SpecHandle, ActivationPredictionKey);
	DirectionDelegateHandle = DirectionDelegate.AddUObject(this, &ThisClass::OnDirectionReceived);
}

// [OwnClient]
/* 실행 키 = 서버 구독 키(실행 동안 고정), 현재 키 = 서버가 같은 예측 창에서 처리하게 함 */
void UJY_AbilityTask_WaitDirection::SendDirection(const FVector& Direction, FGameplayTag ApplicationTag)
{
	const FGameplayAbilityTargetDataHandle DirectionDataHandle = JY_AbilityTask_WaitDirection::MakeDirectionTargetData(Direction);

	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (ActorInfo->IsNetAuthority() == false)
	{
		UJY_AbilitySystemComponent* JYASC = CastChecked<UJY_AbilitySystemComponent>(AbilitySystemComponent.Get());
		const FPredictionKey ScopedPredictionKey = JYASC->ScopedPredictionKey;
		JYASC->CallServerSetReplicatedTargetData(SpecHandle, ActivationPredictionKey, DirectionDataHandle, ApplicationTag, ScopedPredictionKey);
	}

	OnDirectionReceived(DirectionDataHandle, ApplicationTag);
}

// [Server] [OwnClient]
void UJY_AbilityTask_WaitDirection::OnDirectionReceived(const FGameplayAbilityTargetDataHandle& DirectionDataHandle, FGameplayTag ApplicationTag)
{
	/* 첫 방향만 받으면 먼저 끊는다 — 같은 키로 두 번 오면(변조 클라) 비용이 두 번 나간다 */
	if (bAcceptRepeated == false)
	{
		StopListening();
	}

	/* 아래 Consume이 ASC 저장본을 지우므로 복사. 비워 두면 다음 방향이 같은 키로 와도 새로 저장된다 */
	const FGameplayAbilityTargetDataHandle LocalDataHandle = DirectionDataHandle;
	UJY_AbilitySystemComponent* JYASC = CastChecked<UJY_AbilitySystemComponent>(AbilitySystemComponent.Get());
	JYASC->ConsumeClientReplicatedTargetData(SpecHandle, ActivationPredictionKey);

	if (ShouldBroadcastAbilityTaskDelegates() == false)
		return;

	const FVector Direction = JY_AbilityTask_WaitDirection::GetDirectionFromTargetData(LocalDataHandle);
	OnDirection.Broadcast(Direction, ApplicationTag);
}

void UJY_AbilityTask_WaitDirection::StopListening()
{
	UJY_AbilitySystemComponent* JYASC = CastChecked<UJY_AbilitySystemComponent>(AbilitySystemComponent.Get());
	FAbilityTargetDataSetDelegate& DirectionDelegate = JYASC->AbilityTargetDataSetDelegate(SpecHandle, ActivationPredictionKey);
	DirectionDelegate.Remove(DirectionDelegateHandle);
}

/* 어빌리티가 끝나면 엔진이 태스크도 끝낸다(GameplayAbility.cpp:852) — 구독·저장본을 여기서 정리 */
void UJY_AbilityTask_WaitDirection::OnDestroy(bool bInOwnerFinished) //override
{
	StopListening();

	UJY_AbilitySystemComponent* JYASC = CastChecked<UJY_AbilitySystemComponent>(AbilitySystemComponent.Get());
	JYASC->ConsumeClientReplicatedTargetData(SpecHandle, ActivationPredictionKey);

	Super::OnDestroy(bInOwnerFinished);
}
