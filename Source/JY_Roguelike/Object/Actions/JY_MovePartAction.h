#pragma once

#include "CoreMinimal.h"
#include "Object/JY_Action.h"
#include "Kismet/KismetMathLibrary.h"
#include "JY_MovePartAction.generated.h"

/* BP 부품을 상대 위치로 옮기는 화면 연출 (가시 솟음, 화염판 발판 눌림) */
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

	/* 목표까지 걸리는 시간 (초) — 0이면 바로 */
	UPROPERTY(EditAnywhere, Category = "JY|MovePart", Meta = (ClampMin = "0.0"))
	float MoveSeconds = 0.0f;

	/* 움직임 모양 — ExpoOut(처음에 확 튀어나옴), EaseIn(천천히 출발해 빨라짐) 등 */
	UPROPERTY(EditAnywhere, Category = "JY|MovePart", Meta = (EditCondition = "MoveSeconds > 0"))
	TEnumAsByte<EEasingFunc::Type> EasingFunc = EEasingFunc::Linear;

	/* Ease 계열의 곡선 세기 (클수록 급함) */
	UPROPERTY(EditAnywhere, Category = "JY|MovePart", Meta = (ClampMin = "1.0", EditCondition = "MoveSeconds > 0"))
	float BlendExp = 2.0f;

private:
	/* 이번 이동의 출발 위치·지난 시간 */
	FVector StartLocation = FVector::ZeroVector;
	float ElapsedSeconds = 0.0f;
};
