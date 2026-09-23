#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "JY_UISettings.generated.h"

class UJY_HealthViewModel;

/** UI가 쓸 ViewModel 클래스 지정 (프로젝트 설정 > Game > JY UI) */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "JY UI"))
class UJY_UISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 체력 ViewModel 클래스 (BP 자식으로 색·기준값을 바꿔 끼움) */
	UPROPERTY(Config, EditAnywhere, Category = "ViewModel")
	TSoftClassPtr<UJY_HealthViewModel> HealthViewModelClass;
};
