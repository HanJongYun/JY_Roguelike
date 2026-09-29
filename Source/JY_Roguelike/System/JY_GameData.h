#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "JY_GameData.generated.h"

class UJY_PrimaryLayout;
class UMVVMViewModelBase;

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

	/* ViewModel 교체 표 — 키: C++ ViewModel 클래스, 값: 대신 만들 BP 자식(색·기준값 조정). 없는 키는 C++ 클래스 그대로 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TMap<TSubclassOf<UMVVMViewModelBase>, TSoftClassPtr<UMVVMViewModelBase>> ViewModelClassOverrides;
};
