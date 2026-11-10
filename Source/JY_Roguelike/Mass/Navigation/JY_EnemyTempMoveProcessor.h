#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "JY_EnemyTempMoveProcessor.generated.h"

/* 임시: 갈 곳(MoveTarget)으로 일정 속도로 걷고 NavMesh 위에 붙인다 — 2장 B(엔진 이동·회피) 들어오면 삭제 */
UCLASS()
class UJY_EnemyTempMoveProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UJY_EnemyTempMoveProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
