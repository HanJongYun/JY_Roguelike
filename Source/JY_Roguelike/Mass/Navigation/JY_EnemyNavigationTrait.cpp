#include "Mass/Navigation/JY_EnemyNavigationTrait.h"

#include "Mass/JY_MassTypes.h"
#include "MassCommonFragments.h"
#include "MassEntityTemplateRegistry.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_EnemyNavigationTrait)

/* 갈 곳은 엔진 MoveTarget에 쓴다 — 그 Fragment는 엔진 Steering Trait가 넣으므로 요구만 한다 (MassSteeringTrait.cpp:24, 엔진 NavMesh 길 Trait도 같음 MassNavMeshNavigationTrait.cpp:14) */
void UJY_EnemyNavigationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const //override
{
	BuildContext.RequireFragment<FTransformFragment>();
	BuildContext.RequireFragment<FMassMoveTargetFragment>();
	BuildContext.RequireFragment<FMassDesiredMovementFragment>();
	BuildContext.RequireFragment<FMassVelocityFragment>();

	BuildContext.AddFragment<FJY_NavPolyFragment>();

	/* 위치는 UJY_MassApplyMovementProcessor가 NavMesh 위로만 옮긴다 — 엔진 이동이 이 적을 건너뛰게 (MassMovementProcessors.cpp:86) */
	BuildContext.AddTag<FMassCustomMovementTag>();
}
