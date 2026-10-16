#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Skill/JY_SkillTypes.h"
#include "UI/JY_UITypes.h"
#include "JY_GameData.generated.h"

class UJY_PrimaryLayout;
class UJY_HUDLayout;
class UJY_ConfirmDialog;
class UJY_MetaUpgradeData;
class UJY_ScreenWidget;
class UUserWidget;
class UJY_SkillDefinition;

/* 게임 전체가 같이 쓰는 데이터 한 벌 (에셋 DA_JY_GameData) */
UCLASS(BlueprintType, Const)
class UJY_GameData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/* 설정값 — DA_JY_GameData 에셋에서 지정 */

	/* 화면 전체 틀 위젯 (WBP_JY_PrimaryLayout) */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UJY_PrimaryLayout> PrimaryLayoutClass;

	/* 틀이 생기면 Game 층에 올릴 HUD (WBP_JY_HUD) */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UJY_HUDLayout> HUDLayoutClass;

	/* 확인 팝업 (WBP_JY_ConfirmDialog) — Modal 층에 올림 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UJY_ConfirmDialog> ConfirmDialogClass;

	/* 메타 강화 표 (DA_JY_MetaUpgrades) — 서버가 구매 가격·효과를 검증할 때 씀 */
	UPROPERTY(EditDefaultsOnly, Category = "Progress")
	TObjectPtr<const UJY_MetaUpgradeData> MetaUpgradeData;

	/* 접속·맵 이동 중 화면을 가리는 로딩창 (WBP_JY_LoadingScreen) — 뷰포트에 직접 붙음 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UUserWidget> LoadingScreenClass;

	/* 타이틀 화면 (WBP_JY_Title) — 타이틀 맵에서 Menu 층에 올림 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UJY_ScreenWidget> TitleScreenClass;

	/* 로비 맵 (Lvl_JY_Hub_MD) — 타이틀에서 시작하면 이동 */
	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSoftObjectPtr<UWorld> HubMap;

	/* 마우스 커서 그림 (강철 칼 / 누르면 금빛 칼) */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FJY_CursorData Cursor;

	/* 런 시작 스킬 슬롯 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Skill", Meta = (ClampMin = "1"))
	int32 StartSkillSlotCount = 1;

	/* 슬롯 번호 → 입력 태그 (InputTag.Skill.Slot1~). 개수 = 최대 슬롯 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Skill", Meta = (Categories = "InputTag.Skill"))
	TArray<FGameplayTag> SkillSlotInputTags;

	/* 강화 카드 선택지 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Skill", Meta = (ClampMin = "1"))
	int32 SkillCardChoiceCount = 3;

	/* 카드로 줄일 수 있는 쿨타임 하한 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Skill", Meta = (ClampMin = "0"))
	float MinSkillCooldown = 0.5f;

	/* 런 시작 때 슬롯 순서대로 장착할 스킬 — 보상으로 얻기 전 테스트용. 개수만큼 슬롯이 열림 */
	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	TArray<TObjectPtr<UJY_SkillDefinition>> StartSkills;

	/* 슬롯과 별개로 런 내내 가진 스킬 (우클릭 찌르기). 보상 풀(SkillDefinitions)에 넣지 않는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	TArray<FJY_FixedSkill> FixedSkills;

	/* 획득 가능한 스킬 (디버그 명령·보상 풀) */
	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	TArray<TObjectPtr<UJY_SkillDefinition>> SkillDefinitions;
};
