#pragma once

#include "CoreMinimal.h"
#include "Object/JY_Action.h"
#include "JY_BreakAction.generated.h"

class UGeometryCollection;
class UGeometryCollectionComponent;
class UNiagaraSystem;
class UPrimitiveComponent;

/* 몸체를 숨기고 화면마다 파편을 튀게 함 (항아리·상자·통) */
UCLASS(meta = (DisplayName = "Break"))
class UJY_BreakAction : public UJY_Action
{
	GENERATED_BODY()

public:
	virtual void OnServerEnter(AJY_Object* Object) override;
	virtual void OnClientEnter(AJY_Object* Object, bool bCatchUp) override;

private:
	/* [Server] [Client] 몸체(와 붙은 부품)를 숨기고 충돌을 끔 */
	void HideBody(AJY_Object* Object) const;

	/* [Client] 파편을 만들어 바깥으로 튀게 함 (복제 안 함) */
	void SpawnDebris(AJY_Object* Object);

	void DestroyDebris();

private:
	/* 설정값 */

	/* 숨길 몸체의 Component Tags — 비우면 루트 (붙은 부품도 함께 숨김) */
	UPROPERTY(EditAnywhere, Category = "JY|Break")
	FName BodyPartTag;

	/* 부서질 때 그 자리에 한 번 재생 (먼지·파편) */
	UPROPERTY(EditAnywhere, Category = "JY|Break")
	TObjectPtr<UNiagaraSystem> BreakFX;

	/* 쪼개져 흩어지는 파편 (GC_JY_*). 비우면 파편 없이 사라짐 */
	UPROPERTY(EditAnywhere, Category = "JY|Break")
	TObjectPtr<UGeometryCollection> BreakGeometry;

	/* 파편이 바깥으로 튀는 속도 (cm/s) */
	UPROPERTY(EditAnywhere, Category = "JY|Break", Meta = (ClampMin = "0.0", EditCondition = "BreakGeometry != nullptr"))
	float DebrisSpeed = 400.0f;

	/* 파편을 지우는 최대 시간 (초) — 멈추면 에셋 설정(Remove on Sleep)으로 먼저 사라짐 */
	UPROPERTY(EditAnywhere, Category = "JY|Break", Meta = (ClampMin = "0.5", EditCondition = "BreakGeometry != nullptr"))
	float DebrisMaxLifeSeconds = 6.0f;

private:
	/* 부서진 뒤에만 클라에서 만드는 파편 */
	UPROPERTY(Transient)
	TObjectPtr<UGeometryCollectionComponent> DebrisComponent;

	FTimerHandle DebrisTimerHandle;
};
