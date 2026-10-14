#include "System/JY_GameplayTags.h"

namespace JYGameplayTags
{
	/* SetByCaller */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage,    "SetByCaller.Damage",    "데미지 크기. 데미지 GE가 HealthSet.Damage(메타 속성)에 더한다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Heal,      "SetByCaller.Heal",      "회복 크기. 회복 GE가 HealthSet.Healing(메타 속성)에 더한다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_MetaValue, "SetByCaller.MetaValue", "메타 강화 효과 크기 = 단계 × 단계당 값.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Cooldown,  "SetByCaller.Cooldown",  "쿨타임 길이(초). 스킬 쿨타임 GE 길이.");

	/* Service — 새 서비스(상점·축복 NPC)는 새 태그 1줄 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Service_MetaAltar,     "Service.MetaAltar",     "메타 강화 구매·환불을 받는 제단.");

	/* UI.Layer — 새 층은 태그 1줄 + JY_PrimaryLayout BindWidget 1개 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Game,         "UI.Layer.Game",         "HUD 층. 게임 입력 유지.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_GameMenu,     "UI.Layer.GameMenu",     "게임 안 메뉴 층. 기록판·제단·축복 선택.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Menu,         "UI.Layer.Menu",         "시스템 메뉴 층. 정지 메뉴·설정.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Modal,        "UI.Layer.Modal",        "맨 위 팝업 층. 확인 팝업.");

	/* InputTag — 새 입력은 태그 1줄 + InputConfig 1줄 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move,           "InputTag.Move",           "이동 (WASD).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Interact,       "InputTag.Interact",       "상호작용 (E).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump,           "InputTag.Jump",           "점프 어빌리티 (Space).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Dash,           "InputTag.Dash",           "대시 어빌리티 (Shift).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Attack,         "InputTag.Attack",         "기본 공격 어빌리티 (좌클릭).");

	/* Status */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Death,            "Status.Death",            "죽음 상태 전체. 모든 어빌리티가 이 태그로 막힌다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Death_Dying,      "Status.Death.Dying",      "죽기 시작함 (UJY_HealthComponent::StartDeath).");

	/* Ability */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_Attack,     "Ability.Type.Attack",     "기본 공격. 대시·점프·스킬이 이 태그로 공격을 취소한다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_Skill,      "Ability.Type.Skill",      "스킬 공통. 스킬끼리 겹치지 않게 막는 태그.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_Jump,       "Ability.Type.Jump",       "점프. 스킬 중엔 막힌다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Start_Air,       "Ability.Start.Air",       "공중에서 시작 표시 (첫 방향 TargetData의 ApplicationTag) — 공중 콤보·공중 내려찍기.");

	/* Event — 새 시점은 태그 1줄 + 몽타주 노티파이 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Melee_Hit,         "Event.Melee.Hit",         "기본 공격 타격 시점.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Melee_StepStart,   "Event.Melee.StepStart",   "콤보 각 타 시작 직후 (몸 돌리기).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Skill_Hit,         "Event.Skill.Hit",         "스킬 타격 시점.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Skill_DiveStart,   "Event.Skill.DiveStart",   "공중 내려찍기 하강 시작.");

	/* State */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dashing,           "State.Dashing",           "대시 중. 점프를 막는다.");

	/* Gameplay */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_DamageImmunity, "Gameplay.DamageImmunity", "데미지 무시 (UJY_HealthSet::PreGameplayEffectExecute).");
}
