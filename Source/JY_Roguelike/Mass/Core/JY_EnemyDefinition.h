#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "JY_EnemyDefinition.generated.h"

class UMassEntityConfigAsset;

/* 적 1종 — 엔티티 설계도를 담는다. UJY_GameData::EnemyDefinitions에 넣어 쓴다 */
UCLASS(Const)
class UJY_EnemyDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/* 설정값 — DA_Enemy_* 에셋에서 지정 */

	/* 엔티티 설계도 (DA_MassEnemy_*) — Trait마다 ValidTargetConfig로 서버(이동·복제)·클라(그리기)를 나눈다 */
	UPROPERTY(EditDefaultsOnly, Category = "Mass")
	TObjectPtr<const UMassEntityConfigAsset> EntityConfig;
};
