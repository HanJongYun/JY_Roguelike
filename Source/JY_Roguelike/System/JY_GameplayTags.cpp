#include "System/JY_GameplayTags.h"

namespace JYGameplayTags
{
	/* SetByCaller */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage,  "SetByCaller.Damage",  "데미지 크기. 데미지 GE가 HealthSet.Damage(메타 속성)에 더한다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Heal,    "SetByCaller.Heal",    "회복 크기. 회복 GE가 HealthSet.Healing(메타 속성)에 더한다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_MetaValue, "SetByCaller.MetaValue", "메타 강화 효과 크기 = 단계 × 단계당 값.");

	/* UI.Layer */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Game,       "UI.Layer.Game",       "HUD 층. 게임 입력 유지.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_GameMenu,   "UI.Layer.GameMenu",   "게임 안 메뉴 층. 기록판·제단·축복 선택.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Menu,       "UI.Layer.Menu",       "시스템 메뉴 층. ESC 메뉴·설정.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Modal,      "UI.Layer.Modal",      "맨 위 팝업 층. 확인 팝업.");

	/* Meta */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Meta_Toughness,      "Meta.Toughness",      "강인함 — 최대 체력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Meta_Might,          "Meta.Might",          "힘 — 공격력 (던전 조각에서 연결).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Meta_Swiftness,      "Meta.Swiftness",      "민첩 — 대시 재사용 (던전 조각에서 연결).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Meta_Fortune,        "Meta.Fortune",        "행운 — 영혼석 획득량 (던전 조각에서 연결).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Meta_Resolve,        "Meta.Resolve",        "불굴 — 부활 횟수 (던전 조각에서 연결).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Meta_Insight,        "Meta.Insight",        "통찰 — 축복 선택지 수 (던전 조각에서 연결).");
}
