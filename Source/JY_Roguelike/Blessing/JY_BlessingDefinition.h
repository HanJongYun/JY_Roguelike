#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "JY_BlessingDefinition.generated.h"

/* 축복 1개 — 어느 행동에 붙어 무엇을 바꾸는지. UJY_GameData::BlessingDefinitions에 넣어 쓴다 */
UCLASS(Const)
class UJY_BlessingDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/* 설정값 — DA_JY_Blessing_* 에셋에서 지정 */

	/* 계열 — 이 축복을 낀 행동의 피해 속성이 된다 */
	UPROPERTY(EditDefaultsOnly, Category = "Blessing", Meta = (Categories = "Element"))
	FGameplayTag ElementTag;

	/* 붙는 행동 (Skill.BasicAttack 등) */
	UPROPERTY(EditDefaultsOnly, Category = "Blessing", Meta = (Categories = "Skill"))
	FGameplayTag ActionTag;

	/* 그 행동으로 맞힌 적에 거는 상태이상 (비우면 안 건다) — 수치는 DA_JY_StatusEffectData */
	UPROPERTY(EditDefaultsOnly, Category = "Blessing", Meta = (Categories = "Status.Effect"))
	FGameplayTag StatusTag;

	/* 그 행동의 피해 증가 (만분율, 2000 = +20%) */
	UPROPERTY(EditDefaultsOnly, Category = "Blessing", Meta = (ClampMin = "0"))
	int32 ActionIncreaseRate = 0;
};
