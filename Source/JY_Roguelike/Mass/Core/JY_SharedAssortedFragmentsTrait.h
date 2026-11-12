#pragma once

#include "CoreMinimal.h"
#include "MassAssortedFragmentsTrait.h"
#include "JY_SharedAssortedFragmentsTrait.generated.h"

/* 서버·클라가 똑같이 가져야 하는 Fragment 목록 (몸 반경·높이 등) — 엔진 분류된 프래그먼트와 다른 종류라 서버용·클라용과 함께 둘 수 있다 */
UCLASS(meta = (DisplayName = "Shared Assorted Fragments"))
class UJY_SharedAssortedFragmentsTrait : public UMassAssortedFragmentsTrait
{
	GENERATED_BODY()
};
