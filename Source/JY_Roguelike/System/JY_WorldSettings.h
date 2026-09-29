#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "JY_WorldSettings.generated.h"

/* 맵마다 다른 설정을 담는 곳 — 에디터 World Settings 패널에서 맵별로 수정 (지금은 비어 있음, UI 과정에서 표시 설정을 추가) */
UCLASS()
class AJY_WorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	AJY_WorldSettings(const FObjectInitializer& ObjectInitializer);
};
