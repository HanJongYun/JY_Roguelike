#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "JY_GameData.generated.h"

class UJY_MetaUpgradeData;

/* 게임 전체가 같이 쓰는 데이터 한 벌 (에셋 DA_JY_GameData) */
UCLASS(BlueprintType, Const)
class UJY_GameData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/* 설정값 — DA_JY_GameData 에셋에서 지정 */

	/* 메타 강화 표 (DA_JY_MetaUpgrades) — 서버가 구매 가격·효과를 검증할 때 씀 */
	UPROPERTY(EditDefaultsOnly, Category = "Progress")
	TObjectPtr<const UJY_MetaUpgradeData> MetaUpgradeData;
};
