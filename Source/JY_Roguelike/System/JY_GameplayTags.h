#pragma once

#include "NativeGameplayTags.h"

/* C++이 이름으로 직접 쓰는 태그만 여기 둔다. 데이터로만 쓰는 태그(Meta.* 강화 종류)는 Config/DefaultGameplayTags.ini */
namespace JYGameplayTags
{
	/* SetByCaller — GE 적용 시 크기를 코드에서 넘기는 키 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Heal);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MetaValue);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Cooldown);

	/* Service — 상호작용 대상이 제공하는 서비스 (서버가 구매·환불 요청을 받은 물체가 그 서비스를 하는지 검증) */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Service_MetaAltar);

	/* UI.Layer — 화면 틀의 층 (아래일수록 위에 그려짐) */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);

	/* InputTag — InputConfig가 입력 액션과 짝짓는 태그. 어빌리티 입력은 AbilitySet도 같은 태그로 지급 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Interact);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Dash);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack);

	/* Status — 캐릭터 상태. 어빌리티 발동 차단 조건 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dying);

	/* Ability — 어빌리티 종류 (다른 어빌리티가 취소·차단 대상으로 씀) */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Attack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Skill);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Jump);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Start_Air);

	/* Event — 몽타주 노티파이·상호작용이 어빌리티로 보내는 이벤트 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Melee_Hit);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Melee_StepStart);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Skill_Hit);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Skill_DiveStart);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Skill_SlamPose);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Interact);

	/* State — 어빌리티가 실행 중에 붙이는 태그 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dashing);

	/* Gameplay — 데미지 규칙 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_DamageImmunity);

	/* Cheat — 개발용 (출시 빌드에선 무시) */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cheat_UnlimitedHealth);
}
