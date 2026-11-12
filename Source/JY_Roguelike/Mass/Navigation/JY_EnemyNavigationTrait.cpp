#include "Mass/Navigation/JY_EnemyNavigationTrait.h"

#include "Mass/JY_MassTypes.h"
#include "MassCommonFragments.h"
#include "MassEntityTemplateRegistry.h"
#include "MassNavigationFragments.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_EnemyNavigationTrait)

/* 갈 곳은 엔진 MoveTarget에 쓴다 — 엔진 Steering(B)이 이 Fragment를 읽어 힘을 낸다 (MassSteeringTrait.cpp:24) */
void UJY_EnemyNavigationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const //override
{
	BuildContext.RequireFragment<FTransformFragment>();

	BuildContext.AddFragment<FJY_NavPolyFragment>();
	BuildContext.AddFragment<FMassMoveTargetFragment>();
}
