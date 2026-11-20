#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UI/JY_UITypes.h"
#include "JY_GameData.generated.h"

class UJY_PrimaryLayout;
class UJY_HUDLayout;
class UJY_ConfirmDialog;
class UJY_MetaUpgradeData;
class UJY_ScreenWidget;
class UUserWidget;
class UJY_SkillData;
class UJY_StatusEffectData;
class UGameplayEffect;
class UJY_EnemyDefinition;
class UJY_BlessingDefinition;

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

	/* 모든 스킬 정보·시작 배치·쿨타임 GE (DA_JY_SkillData) */
	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	TObjectPtr<UJY_SkillData> SkillData;

	/* 모든 상태이상 수치 (DA_JY_StatusEffectData) — 맞힐 때 Payload의 상태 태그로 찾는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TObjectPtr<UJY_StatusEffectData> StatusEffectData;

	/* 피해 GE (GE_JY_Damage) — 크기는 SetByCaller.Damage */
	UPROPERTY(EditDefaultsOnly, Category = "GameplayEffect")
	TSubclassOf<UGameplayEffect> DamageGameplayEffect;

	/* 회복 GE (GE_JY_Heal) — 크기는 SetByCaller.Heal */
	UPROPERTY(EditDefaultsOnly, Category = "GameplayEffect")
	TSubclassOf<UGameplayEffect> HealGameplayEffect;

	/* 모든 축복 (DA_JY_Blessing_*) — 치트·3택1이 여기서 고른다 */
	UPROPERTY(EditDefaultsOnly, Category = "Blessing")
	TArray<TObjectPtr<const UJY_BlessingDefinition>> BlessingDefinitions;

	/* Mass 적 종류 — 배열 번호가 복제되는 정의 번호(DefinitionIndex). 서버·클라가 같은 순서로 읽는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Mass")
	TArray<TObjectPtr<const UJY_EnemyDefinition>> EnemyDefinitions;

	/* Mass 적 값을 클라로 보내는 간격 (초) — 클라는 이 사이를 보간으로 채운다 */
	UPROPERTY(EditDefaultsOnly, Category = "Mass", Meta = (ClampMin = "0.0"))
	float EnemyNetSendInterval = 0.05f;

	/* 플레이어와 겹친 Mass 적이 밖으로 밀려나는 빠르기 (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "Mass", Meta = (ClampMin = "0.0"))
	float EnemyPlayerPushSpeed = 300.0f;

	/* 겹친 깊이가 이 값보다 얕아지면 밀려나는 빠르기가 줄어든다 (cm) — 다 빠져나올 때 미끄러지지 않게 */
	UPROPERTY(EditDefaultsOnly, Category = "Mass", Meta = (ClampMin = "1.0"))
	float EnemyPlayerPushSlowdownDistance = 40.0f;

	/* 플레이어 둘레 이 거리 안의 Mass 적에게만 길막이 캡슐을 놓는다 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "Mass", Meta = (ClampMin = "0.0"))
	float EnemyBlockerSearchRadius = 300.0f;
};
