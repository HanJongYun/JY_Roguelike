#include "Mass/Navigation/JY_EnemyNavigationTrait.h"

#include "Mass/JY_MassTypes.h"
#include "MassCommonFragments.h"
#include "MassEntityTemplateRegistry.h"
#include "MassNavigationFragments.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_EnemyNavigationTrait)

/* 갈 곳은 엔진 MoveTarget에 쓴다 — 그 Fragment는 엔진 Steering Trait가 넣으므로 요구만 한다 (MassSteeringTrait.cpp:24, 엔진 NavMesh 길 Trait도 같음 MassNavMeshNavigationTrait.cpp:14) */
void UJY_EnemyNavigationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const //override
{
	BuildContext.RequireFragment<FTransformFragment>();
	BuildContext.RequireFragment<FMassMoveTargetFragment>();

	BuildContext.AddFragment<FJY_NavPolyFragment>();
}
