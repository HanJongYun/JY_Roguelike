#include "Mass/Core/JY_SharedAssortedFragmentsTrait.h"

/*
 * 동작은 엔진 분류된 프래그먼트 그대로 — 상속만 하는 이유:
 * Config는 같은 클래스 Trait가 대상(서버·클라)이 겹치면 하나만 남기는데, 비교가 IsA가 아니라 정확한 클래스라 (MassEntityConfigAsset.cpp:181)
 * 서버용·클라용 엔진 분류된 프래그먼트와 함께 AllNetModes로 둘 수 있다
 */

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_SharedAssortedFragmentsTrait)
