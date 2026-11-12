#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "JY_EnemyNavMeshSnapProcessor.generated.h"

/* 엔진이 옮긴 적을 NavMesh 위로 붙인다 — 벽을 못 뚫고 바닥 높이를 따른다 */
UCLASS()
class UJY_EnemyNavMeshSnapProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UJY_EnemyNavMeshSnapProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
