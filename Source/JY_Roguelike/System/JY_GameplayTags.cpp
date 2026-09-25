#include "System/JY_GameplayTags.h"

namespace JYGameplayTags
{
	/* SetByCaller */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage,  "SetByCaller.Damage",  "데미지 크기. 데미지 GE가 HealthSet.Damage(메타 속성)에 더한다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Heal,    "SetByCaller.Heal",    "회복 크기. 회복 GE가 HealthSet.Healing(메타 속성)에 더한다.");

	/* UI.Layer */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Game,       "UI.Layer.Game",       "HUD 층. 게임 입력 유지.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_GameMenu,   "UI.Layer.GameMenu",   "게임 안 메뉴 층. 기록판·제단·축복 선택.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Menu,       "UI.Layer.Menu",       "시스템 메뉴 층. ESC 메뉴·설정.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Modal,      "UI.Layer.Modal",      "맨 위 팝업 층. 확인 팝업.");
}
