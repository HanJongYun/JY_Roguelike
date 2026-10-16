#pragma once

#include "CoreMinimal.h"
#include "Object/JY_Action.h"
#include "JY_MovePartAction.generated.h"

/* 부품 이동 — BP 부품을 상대 위치로 옮김 (가시 숨음·경고·솟음, 화염판 발판 눌림). 화면 연출 전용, 서버 판정은 범위 피해가 한다 */
UCLASS(meta = (DisplayName = "Move Part"))
class UJY_MovePartAction : public UJY_Action
{
	GENERATED_BODY()

public:
	virtual void OnClientEnter(AJY_Object* Object, bool bCatchUp) override;
	virtual void OnClientExit(AJY_Object* Object) override;
	virtual bool TickClient(AJY_Object* Object, float DeltaSeconds) override;

private:
	/* 설정값 */

	/* 옮길 부품의 Component Tags */
	UPROPERTY(EditAnywhere, Category = "JY|MovePart")
	FName PartTag;

	/* 목표 상대 위치 (부모 부품 기준, 액터 배율 적용 전) */
	UPROPERTY(EditAnywhere, Category = "JY|MovePart")
	FVector TargetLocation = FVector::ZeroVector;

	/* 따라가는 속도 (FInterpTo, 클수록 빠름) — 0이면 바로 */
	UPROPERTY(EditAnywhere, Category = "JY|MovePart", Meta = (ClampMin = "0.0"))
	float MoveSpeed = 0.0f;
};
