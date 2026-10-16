#include "System/JY_WorldSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_WorldSettings)

/* 맵 파일 값이라 클라가 복제를 기다리지 않고 맵이 열리는 순간 읽는다. 서브레벨 World Settings는 엔진이 무시 */
AJY_WorldSettings::AJY_WorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
