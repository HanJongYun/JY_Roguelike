#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/JY_UITypes.h"
#include "JY_GameData.generated.h"

class UJY_PrimaryLayout;
class UJY_HUDLayout;
class UJY_ConfirmDialog;
class UJY_MetaUpgradeData;

/* 게임 전체가 같이 쓰는 데이터 한 벌 (에셋 DA_JY_GameData) */
UCLASS(BlueprintType, Const)
class UJY_GameData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/* 설정값 — DA_JY_GameData 에셋에서 지정. UI 클래스는 Soft 참조(서버에서 로드 안 됨) */

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

	/* 마우스 커서 그림 (강철 칼 / 누르면 금빛 칼) */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FJY_CursorData Cursor;
};
