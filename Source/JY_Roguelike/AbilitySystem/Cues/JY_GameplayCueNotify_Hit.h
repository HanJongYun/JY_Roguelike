#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "JY_GameplayCueNotify_Hit.generated.h"

class UCameraShakeBase;
class UNiagaraSystem;
class USoundBase;

/* 피격 연출 — 이펙트·소리 + 맞은 쪽 반응(몽타주·애니 멈춤·화면 흔들림). 큐 태그와 에셋은 GCN_JY_Hit_* BP에서 */
UCLASS(Abstract)
class UJY_GameplayCueNotify_Hit : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	/* 설정값 — GCN_JY_Hit_Melee 등 */

	/* 맞은 위치에 터지는 이펙트 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Hit")
	TObjectPtr<UNiagaraSystem> HitEffect;

	/* 무작위로 하나 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Hit")
	TArray<TObjectPtr<USoundBase>> HitSounds;

	/* 맞은 쪽 반응 세기 — 맞은 쪽 컴포넌트가 이 태그로 몽타주를 고른다 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Hit|Reaction", Meta = (Categories = "HitReaction"))
	FGameplayTag HitReactionTag;

	/* 맞은 쪽 애니가 멈추는 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Hit|Reaction", Meta = (ClampMin = "0.0"))
	float HitStopSeconds = 0.0f;

	/* 맞은 플레이어 화면만 흔들림 (때린 쪽 흔들림은 어빌리티가 따로) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Hit|Reaction")
	TSubclassOf<UCameraShakeBase> VictimCameraShake;
};
