#include "System/JY_GameMode.h"
#include "Character/JY_Character.h"
#include "Player/JY_PlayerController.h"
#include "Player/JY_PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameMode)

/* C++ 기본값. 실제 에셋(메시·입력)은 BP_JY_GameMode에서 BP 클래스로 덮어쓴다 */
AJY_GameMode::AJY_GameMode()
{
	PlayerStateClass = AJY_PlayerState::StaticClass();
	PlayerControllerClass = AJY_PlayerController::StaticClass();
	DefaultPawnClass = AJY_Character::StaticClass();
}
