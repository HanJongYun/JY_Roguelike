#pragma once

#include "NativeGameplayTags.h"

namespace JYGameplayTags
{
	/* SetByCaller — GE 적용 시 크기를 코드에서 넘기는 키 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Heal);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MetaValue);

	/* UI.Layer — 화면을 쌓는 층 이름 (UJY_PrimaryLayout) */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);

	/* Meta — 메타 강화 종류 (DA_JY_MetaUpgrades의 행과 1:1) */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Meta_Toughness);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Meta_Might);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Meta_Swiftness);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Meta_Fortune);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Meta_Resolve);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Meta_Insight);
}
