#pragma once

#include "CoreMinimal.h"
#include "Mass/JY_MassTypes.h"
#include "MassEntityTraitBase.h"
#include "JY_EnemyNavigationTrait.generated.h"

/* 플로우 필드를 따라 플레이어를 쫓는 적 — 밟은 다각형과 갈 곳을 붙인다 (서버 Config에만) */
UCLASS(meta = (DisplayName = "JY Enemy Navigation"))
class UJY_EnemyNavigationTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

protected:
	/* 설정값 — DA_MassEnemy_* 의 이 Trait에서 적 종류마다 정한다 */

	UPROPERTY(EditAnywhere, Category = "JY")
	FJY_EnemyRangeParams RangeParams;
};
