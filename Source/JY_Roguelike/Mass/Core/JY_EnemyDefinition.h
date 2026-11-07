#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "JY_EnemyDefinition.generated.h"

class UMassEntityConfigAsset;

/* 적 1종 — 엔티티 설계도·전투 값. UJY_GameData::EnemyDefinitions에 넣어 쓴다 */
UCLASS(Const)
class UJY_EnemyDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/* 설정값 — DA_Enemy_* 에셋에서 지정. 비율은 만분율 (10000 = 100%) */

	/* 엔티티 설계도 (DA_MassEnemy_*) — Trait마다 ValidTargetConfig로 서버(이동·복제)·클라(그리기)를 나눈다 */
	UPROPERTY(EditDefaultsOnly, Category = "Mass")
	TObjectPtr<const UMassEntityConfigAsset> EntityConfig;

	/* 플레이어를 때리는 피해 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", Meta = (ClampMin = "0"))
	float Attack = 10.0f;

	/* 받는 피해 감소 (2000 = 20% 덜 받음) */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", Meta = (ClampMax = "10000"))
	int32 DefenseRate = 0;

	/* 속성별 저항 (음수 = 약점, -5000 = 1.5배) */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", Meta = (Categories = "Element"))
	TMap<FGameplayTag, int32> ElementResistances;
};
