#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "JY_GameplayCueNotify_Hit.generated.h"

class UNiagaraSystem;
class USoundBase;

/* 피격 연출 — 이펙트·소리·적 반응 몽타주. 큐 태그와 에셋은 GCN_JY_Hit_* BP에서 */
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
};
