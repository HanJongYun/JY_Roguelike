#include "System/JY_WorldSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_WorldSettings)

/*
 * 맵별 표시 값을 GameMode가 아니라 여기 두는 이유:
 * 맵 파일은 서버·클라가 모두 직접 로드한다 → 클라가 복제를 기다리지 않고 맵이 열리는 순간 바로 읽는다.
 * (GameMode 기본값은 GameState의 GameModeClass 복제가 도착해야 읽혀, 원격 클라에선 HUD가 먼저 판정하는 경쟁이 생긴다 — GameStateBase.cpp:53~58, 76)
 * 사용 설정: DefaultEngine.ini WorldSettingsClassName. 기존 맵은 열 때 엔진이 이 클래스로 바꿔 값을 복사한다(World.cpp:2241~2268) → 한 번 저장해야 파일에 남는다.
 * Lyra ALyraWorldSettings와 같은 자리 (Lyra는 서버만 읽고 Experience를 복제 — 우리는 맵 단위 표시값뿐이라 양쪽이 직접 읽는다).
 * 방 단위로 바뀌는 UI(보스 HP바 등)는 여기가 아니라 방 데이터·복제 상태가 맡는다.
 */
AJY_WorldSettings::AJY_WorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
