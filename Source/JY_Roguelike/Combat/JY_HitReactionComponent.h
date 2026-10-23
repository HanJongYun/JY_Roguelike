#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "JY_HitReactionComponent.generated.h"

class UAnimMontage;
class UAnimSequence;
class UCameraShakeBase;
class UMaterialInterface;

/* 맞은 방향별 반응 몽타주 */
USTRUCT(BlueprintType)
struct FJY_HitReactMontages
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Front;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Back;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Left;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Right;
};

/* 맞은 방향별로 지금 자세에 더하는 짧은 움찔 애니 (Additive 시퀀스) */
USTRUCT(BlueprintType)
struct FJY_HitReactAdditives
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimSequence> Front;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimSequence> Back;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimSequence> Left;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimSequence> Right;
};

/* 피격 큐가 넘기는 반응 정보 */
USTRUCT()
struct FJY_HitReactionParams
{
	GENERATED_BODY()

	/* 맞은 수평 방향 (때린 쪽 → 맞은 쪽). 0이면 앞에서 맞은 것으로 */
	FVector HitDirection = FVector::ZeroVector;

	/* 반응 세기 (HitReaction.Light 등). 비우면 기본 세기 */
	FGameplayTag ReactionTag;

	float Damage = 0.0f;

	/* 애니가 멈추는 시간 (초) */
	float HitStopSeconds = 0.0f;

	/* 맞은 플레이어 화면만 흔들림 */
	TSubclassOf<UCameraShakeBase> CameraShake;
};

/* 맞은 쪽 반응 — 밀려남(서버)과 몽타주·애니 멈춤·번쩍임·화면 흔들림(클라). 플레이어·적 공용 */
UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_HitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_HitReactionComponent();

public:
	/* [Server] 밀려나기 — 높이가 있으면 포물선으로 날아감 (저항만큼 줄임) */
	void ReceiveKnockback(const FVector2f& Impulse, float Height);

	/* [Client] 피격 반응 연출 — 피격 큐가 부름 */
	void PlayHitReaction(const FJY_HitReactionParams& Params);

	/* 애님이 지금 더할 움찔 애니와 그 재생 위치 (없으면 false) */
	bool GetHitAdditivePose(UAnimSequence*& OutSequence, float& OutTime) const;

private:
	/* 큐가 정한 세기 — 큰 피해면 HeavyReactionTag */
	FGameplayTag ResolveReactionTag(const FJY_HitReactionParams& Params) const;

	/* 맞은 방향·세기에 맞는 몽타주 (없으면 nullptr) */
	UAnimMontage* FindHitReactMontage(const FJY_HitReactionParams& Params) const;

	/* 맞은 방향·세기에 맞는 움찔 애니 (없으면 nullptr) */
	UAnimSequence* FindHitReactAdditive(const FJY_HitReactionParams& Params) const;

	/* 때린 쪽 방향에 맞는 칸 고르기 (앞·뒤·왼·오) */
	template <typename T>
	T* SelectByHitDirection(const FVector& HitDirection, T* Front, T* Back, T* Left, T* Right) const;

	void StartHitAdditive(const FJY_HitReactionParams& Params);

	bool IsHitReactMontage(const UAnimMontage* Montage) const;

	void PlayHitReactMontage(const FJY_HitReactionParams& Params);

	void StartHitStop(float Seconds);
	void EndHitStop();

	void StartHitFlash();
	void EndHitFlash();

	void StartCameraShake(TSubclassOf<UCameraShakeBase> CameraShake) const;

	/* 서버·조종 클라가 같은 루트모션 소스를 건다 */
	void ApplyKnockback(const FVector& Velocity, float Height);

	/* [Server -> Client RPC] 조종 클라에도 같은 밀림 */
	UFUNCTION(Client, Reliable)
	void Client_ApplyKnockback(FVector_NetQuantize10 Velocity, float Height);

private:
	/* 설정값 — BP_JY_Character·BP_JY_TrainingDummy 등의 컴포넌트 디테일 */

	/* 반응 세기 태그별 몽타주 — 하던 동작을 멈추고 크게 맞는 반응 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|HitReaction", Meta = (Categories = "HitReaction"))
	TMap<FGameplayTag, FJY_HitReactMontages> ReactionMontages;

	/* 반응 세기 태그별 움찔 애니 — 하던 동작 위에 상체만 잠깐 튄다 (애님 그래프가 Additive로 더함) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|HitReaction", Meta = (Categories = "HitReaction"))
	TMap<FGameplayTag, FJY_HitReactAdditives> ReactionAdditives;

	/* 움찔 애니 재생 속도 — 높을수록 짧고 튀게 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|HitReaction", Meta = (ClampMin = "0.1"))
	float AdditivePlayRate = 1.0f;

	/* 큐가 세기를 안 정했거나 그 세기 몽타주가 없을 때 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|HitReaction", Meta = (Categories = "HitReaction"))
	FGameplayTag DefaultReactionTag;

	/* 이 피해 이상이면 HeavyReactionTag로 (0이면 안 씀) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|HitReaction", Meta = (ClampMin = "0.0"))
	float HeavyHitDamage = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "JY|HitReaction", Meta = (Categories = "HitReaction", EditCondition = "HeavyHitDamage > 0"))
	FGameplayTag HeavyReactionTag;

	/* 몽타주·애니 멈춤·화면 흔들림을 다시 하기까지 (초) — 지속 피해(화염)에 매번 움찔하지 않게 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|HitReaction", Meta = (ClampMin = "0.0"))
	float ReactCooldownSeconds = 0.8f;

	/* 맞을 때마다 메시에 잠깐 덮는 재질 (비우면 번쩍임 없음) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|HitReaction")
	TObjectPtr<UMaterialInterface> HitFlashMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "JY|HitReaction", Meta = (ClampMin = "0.0", EditCondition = "HitFlashMaterial != nullptr"))
	float HitFlashSeconds = 0.12f;

	/* 밀려남 저항 (0 = 그대로, 1 = 안 밀림) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|HitReaction|Knockback", Meta = (ClampMin = "0", ClampMax = "1"))
	float KnockbackResistance = 0.0f;

	/* 밀려나는 시간 (초). 거리 = 미는 속도 × 이 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|HitReaction|Knockback", Meta = (ClampMin = "0.05"))
	float KnockbackDuration = 0.25f;

	/* 떠서 날아가는 시간 (초). 거리 = 미는 속도 × 이 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|HitReaction|Knockback", Meta = (ClampMin = "0.05"))
	float AirKnockbackDuration = 0.5f;

private:
	/* 지금 밀려나는 루트모션 소스 (없으면 0) — 다시 맞으면 바꿔 끼운다 */
	uint16 KnockbackRootMotionSourceID = 0;

	/* 마지막으로 몽타주·애니 멈춤을 한 시각 (ReactCooldownSeconds용) */
	double LastReactSeconds = -1.0;

	/* 지금 더하는 움찔 애니 (끝나면 비움) */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> HitAdditiveSequence;

	double HitAdditiveStartSeconds = 0.0;

	/* 번쩍임 전에 덮여 있던 재질 — 끝나면 되돌린다 */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> OverlayMaterialBeforeFlash;

	bool bHitFlashActive = false;

	FTimerHandle HitStopTimerHandle;
	FTimerHandle HitFlashTimerHandle;
};
