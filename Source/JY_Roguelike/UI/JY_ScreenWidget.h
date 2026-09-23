#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JY_ScreenWidget.generated.h"

/* 모든 화면 위젯의 공통 부모 (층·입력 모드는 나중에 추가) */
UCLASS(Abstract)
class UJY_ScreenWidget : public UUserWidget
{
	GENERATED_BODY()
};
