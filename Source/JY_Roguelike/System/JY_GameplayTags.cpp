#include "System/JY_GameplayTags.h"

namespace JYGameplayTags
{
	/* SetByCaller */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage,  "SetByCaller.Damage",  "데미지 크기. 데미지 GE가 HealthSet.Damage(메타 속성)에 더한다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Heal,    "SetByCaller.Heal",    "회복 크기. 회복 GE가 HealthSet.Healing(메타 속성)에 더한다.");
}
