#pragma once

#include "NativeGameplayTags.h"

/* C++이 이름으로 직접 쓰는 태그만 여기 둔다. 데이터로만 쓰는 태그(Meta.* 강화 종류)는 Config/DefaultGameplayTags.ini */
namespace JYGameplayTags
{
	/* SetByCaller — GE 적용 시 크기를 코드에서 넘기는 키 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Heal);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MetaValue);

	/* Service — 상호작용 대상이 제공하는 서비스 (서버가 구매·환불 요청의 판매 대상을 검증) */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Service_MetaAltar);

	/* UI.Layer — 화면 틀의 층 (아래일수록 위에 그려짐) */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);
}
