#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "JY_EnemySpawningProcessor.generated.h"

/* 예고 시간이 지난 서버 적을 Spawning에서 Chase로 바꾼다 */
UCLASS()
class UJY_EnemySpawningProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UJY_EnemySpawningProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
