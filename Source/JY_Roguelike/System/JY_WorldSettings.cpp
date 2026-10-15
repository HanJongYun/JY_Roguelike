#include "System/JY_WorldSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_WorldSettings)

/*
 * 맵 파일은 서버·클라가 모두 직접 로드한다 → 여기 둔 값은 클라가 복제를 기다리지 않고 맵이 열리는 순간 읽는다.
 * 사용 설정: DefaultEngine.ini WorldSettingsClassName. 퍼시스턴트 맵 것만 쓰인다(서브레벨 World Settings는 엔진이 무시).
 * Lyra ALyraWorldSettings와 같은 자리.
 */
AJY_WorldSettings::AJY_WorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
