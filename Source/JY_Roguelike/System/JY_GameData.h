#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "JY_GameData.generated.h"

class UJY_PrimaryLayout;

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
};
