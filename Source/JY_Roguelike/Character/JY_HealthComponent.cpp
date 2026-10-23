#include "Character/JY_HealthComponent.h"
#include "AbilitySystem/Attributes/JY_HealthSet.h"
#include "System/JY_LogChannels.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "System/JY_GameplayTags.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_HealthComponent)

UJY_HealthComponent::UJY_HealthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/* 이벤트로만 동작 */
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	DeathState = EJY_DeathState::NotDead;
}

void UJY_HealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UJY_HealthComponent, DeathState);
}

void UJY_HealthComponent::OnUnregister() //override
{
	UninitializeFromAbilitySystem();

	Super::OnUnregister();
}

// [Server] [OwnClient] [SimulatedProxy] — 캐릭터 InitializeAbilitySystem에서 호출
/* 클라는 OnRep_PlayerState가 여러 번 올 수 있어 같은 ASC면 다시 구독하지 않는다 */
void UJY_HealthComponent::InitializeWithAbilitySystem(UJY_AbilitySystemComponent* InASC)
{
	if (InASC == nullptr)
		return;

	if (AbilitySystemComponent == InASC)
		return;

	/* 다른 ASC로 바뀐 경우 이전 구독부터 끊는다 */
	UninitializeFromAbilitySystem();

	HealthSet = InASC->GetSet<UJY_HealthSet>();
	if (HealthSet == nullptr)
	{
		UE_LOG(LogJY, Error, TEXT("[%s] JYHealth %s: ASC에 HealthSet 없음"), JY_GetNetRoleTag(GetOwner()), *GetNameSafe(GetOwner()));
		return;
	}

	AbilitySystemComponent = InASC;
	HealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);
}

void UJY_HealthComponent::UninitializeFromAbilitySystem()
{
	if (HealthSet != nullptr)
	{
		HealthSet->OnOutOfHealth.RemoveAll(this);
	}

	HealthSet = nullptr;
	AbilitySystemComponent = nullptr;
}

// [Server]
/* 클라 OnRep_Health도 OnOutOfHealth를 방송하므로 권한 검사로 걸러낸다 */
void UJY_HealthComponent::HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	if (GetOwner()->HasAuthority() == false)
		return;

	StartDeath();
}

// [Server] [OwnClient] [SimulatedProxy]
/* 서버: 상태 변경 + 복제 재촉. 클라: OnRep_DeathState에서 불려 같은 이벤트를 방송 */
void UJY_HealthComponent::StartDeath()
{
	if (DeathState != EJY_DeathState::NotDead)
		return;

	DeathState = EJY_DeathState::DeathStarted;

	AActor* Owner = GetOwner();
	check(Owner);

	/* 모든 어빌리티 차단 — 클라도 여기를 지나 예측도 막힌다 */
	/* 시뮬 프록시는 죽음 복제가 PlayerState(ASC)보다 먼저 올 수 있다 */
	if (AbilitySystemComponent != nullptr)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(JYGameplayTags::Status_Death_Dying, 1);
	}

	OnDeathStarted.Broadcast(Owner);

	/* 죽음은 바로 보여야 해서 다음 업데이트 주기를 기다리지 않는다 */
	Owner->ForceNetUpdate();
}

// [OwnClient] [SimulatedProxy]
/* Lyra 방식 — 값을 이전으로 되돌린 뒤 StartDeath를 거쳐 서버와 같은 경로로 진입 */
void UJY_HealthComponent::OnRep_DeathState(EJY_DeathState OldDeathState)
{
	const EJY_DeathState NewDeathState = DeathState;
	DeathState = OldDeathState;

	if (OldDeathState == EJY_DeathState::NotDead && NewDeathState != EJY_DeathState::NotDead)
	{
		StartDeath();
	}

	/* DeathFinished는 아직 처리 없음. 서버 값에 맞춰만 둔다 */
	DeathState = NewDeathState;
}
