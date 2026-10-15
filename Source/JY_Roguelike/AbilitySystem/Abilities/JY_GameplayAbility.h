#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "JY_GameplayAbility.generated.h"

class UJY_AbilitySystemComponent;
class UCameraShakeBase;
struct FJY_HitShape;

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

	/* 이 어빌리티를 가진 ASC (프로젝트 ASC 타입) */
	UJY_AbilitySystemComponent* GetJYAbilitySystemComponentFromActorInfo() const;

protected:
	/* [OwnClient] 커서가 가리키는 수평 방향 (커서 광선 ∩ 발 높이 평면). 실패하면 몸 앞 */
	FVector ComputeCursorDirection() const;

	/* 수평 방향을 TargetData로 묶기 */
	static FGameplayAbilityTargetDataHandle MakeDirectionTargetData(const FVector& Direction);

	/* TargetData에서 수평 방향 꺼내기 (없으면 0 벡터) */
	static FVector GetDirectionFromTargetData(const FGameplayAbilityTargetDataHandle& DataHandle);

	/* [OwnClient] TargetData를 서버로 (지금 예측 창의 키). 서버에선 아무것도 안 함 */
	void SendTargetDataToServer(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ApplicationTag) const;

	/* [Server] [OwnClient] 몸을 수평 방향으로 돌리기 */
	void FaceDirection(const FVector& Direction) const;

	/* [Server] [OwnClient] 모양 안 적 판정 — 서버는 피해·밀기·피격 연출, 조종 클라는 맞으면 화면만 흔듦 */
	void ApplyHitInShape(const FJY_HitShape& Shape, const FTransform& Origin, float Damage, float KnockbackSpeed, FGameplayTag HitCueTag, TSubclassOf<UCameraShakeBase> HitCameraShake) const;

protected:
	/* 설정값 — 각 어빌리티 생성자 또는 GA_ 블루프린트에서 */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JY|Ability Activation")
	EJY_AbilityActivationPolicy ActivationPolicy = EJY_AbilityActivationPolicy::OnInputTriggered;
};
