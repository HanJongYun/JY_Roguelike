#include "System/JY_GameplayTags.h"

namespace JYGameplayTags
{
	/* SetByCaller */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage,    "SetByCaller.Damage",    "데미지 크기. 데미지 GE가 HealthSet.Damage(메타 속성)에 더한다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Heal,      "SetByCaller.Heal",      "회복 크기. 회복 GE가 HealthSet.Healing(메타 속성)에 더한다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_MetaValue, "SetByCaller.MetaValue", "메타 강화 효과 크기 = 단계 × 단계당 값.");

	/* Service — 새 서비스(상점·축복 NPC)는 새 태그 1줄 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Service_MetaAltar,     "Service.MetaAltar",     "메타 강화 구매·환불을 받는 제단.");
}
