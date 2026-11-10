#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "JY_EnemyNavigationTrait.generated.h"

/* 플로우 필드를 따라 플레이어를 쫓는 적 — 밟은 다각형과 갈 곳을 붙인다 (서버 Config에만) */
UCLASS(meta = (DisplayName = "JY Enemy Navigation"))
class UJY_EnemyNavigationTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
