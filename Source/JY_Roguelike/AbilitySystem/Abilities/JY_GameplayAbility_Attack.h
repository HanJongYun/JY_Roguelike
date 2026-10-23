#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility.h"
#include "JY_GameplayAbility_Attack.generated.h"


/* 공격 어빌리티 공통 — 대시 중이나 다른 공격을 끊을 수 없는 동안엔 시작하지 않음, 타격 원점 */
UCLASS(Abstract)
class UJY_GameplayAbility_Attack : public UJY_GameplayAbility
{
	GENERATED_BODY()

public:
	UJY_GameplayAbility_Attack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	/* [Server] [OwnClient] 실행 중인 이 공격이 지금 다른 공격의 시작을 막는지 */
	virtual bool IsBlockingOtherAttacks() const;

	/* [OwnClient] 공중이면 Ability.Start.Air, 땅이면 빈 태그 — 첫 방향과 함께 서버로 보냄 */
	FGameplayTag MakeStartTag() const;

	/* 아바타 위치 + 수평 방향 */
	FTransform GetAvatarHitOrigin() const;

	/* [Server] [OwnClient] 재생 중인 몽타주에서 이 뼈가 직전 SampleSeconds 동안 움직인 수평 방향 (월드). 못 구하면 0 */
	FVector GetMontageSwingDirection(FName BoneName, float SampleSeconds) const;

private:
	/* [Server] [OwnClient] 실행 중인 공격 중 다른 공격의 시작을 막는 것이 있는지 */
	static bool IsBlockingAttackActive(const FGameplayAbilityActorInfo* ActorInfo);
};
