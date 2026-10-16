#pragma once

#include "CoreMinimal.h"
#include "Object/JY_Action.h"
#include "JY_EffectAction.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/* 이펙트 — BP의 이펙트 부품을 상태 동안 켜거나(화염판 불기둥), 그 자리에 시스템을 새로 재생 (폭발·독구름·점화) */
UCLASS(meta = (DisplayName = "Effect"))
class UJY_EffectAction : public UJY_Action
{
	GENERATED_BODY()

public:
	virtual void OnClientEnter(AJY_Object* Object, bool bCatchUp) override;
	virtual void OnClientExit(AJY_Object* Object) override;

private:
	/* 설정값 */

	/* 상태 동안 켤 이펙트 부품의 Component Tags — 비우면 안 씀 */
	UPROPERTY(EditAnywhere, Category = "JY|Effect")
	FName EffectPartTag;

	/* 상태에 들어갈 때 물체 위치에 새로 재생 — 늦게 받은 상태면 재생 안 함 */
	UPROPERTY(EditAnywhere, Category = "JY|Effect")
	TObjectPtr<UNiagaraSystem> SpawnSystem;

	/* 상태에서 나갈 때 새로 재생한 시스템도 끔 (반복 시스템: 독구름·점화 불꽃) */
	UPROPERTY(EditAnywhere, Category = "JY|Effect", Meta = (EditCondition = "SpawnSystem != nullptr"))
	bool bStopSpawnedOnExit = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> SpawnedEffect;
};
