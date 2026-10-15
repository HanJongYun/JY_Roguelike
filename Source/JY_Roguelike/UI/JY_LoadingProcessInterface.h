#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "JY_LoadingProcessInterface.generated.h"

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UJY_LoadingProcessInterface : public UInterface
{
	GENERATED_BODY()
};

/* 로딩창을 붙잡을 수 있는 물체 — 준비가 덜 됐으면 true를 돌려 로딩창을 유지 */
class IJY_LoadingProcessInterface
{
	GENERATED_BODY()

public:
	/* 아직 준비 중이라 로딩창이 떠 있어야 하면 true */
	virtual bool ShouldShowLoadingScreen() const = 0;
};
