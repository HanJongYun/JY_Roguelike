#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "JY_MassDamageProcessor.generated.h"

/* 쌓인 피해와 상태이상 틱 피해를 적 체력에 반영하고, 체력이 0이 된 적을 지운다 — 적 체력을 바꾸는 유일한 곳 */
UCLASS()
class UJY_MassDamageProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UJY_MassDamageProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
