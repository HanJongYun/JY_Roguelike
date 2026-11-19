#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "JY_MassNetInterpProcessor.generated.h"

/* 클라 적을 받은 위치 사이에서 매 프레임 조금씩 옮긴다 — 구간은 UJY_MassEnemySubsystem::ApplyEnemyState가 적는다 */
UCLASS()
class UJY_MassNetInterpProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UJY_MassNetInterpProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
