#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityElementTypes.h"
#include "AI/Navigation/NavigationTypes.h"
#include "JY_NavigationFragments.generated.h"

/* 적이 밟고 있는 NavMesh 다각형 (서버) — 스폰 때 채우고, 이동하면 갱신 */
USTRUCT()
struct FJY_NavPolyFragment : public FMassFragment
{
	GENERATED_BODY()

	NavNodeRef PolyRef = INVALID_NAVNODEREF;
};
