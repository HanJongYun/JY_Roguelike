#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "JY_MassFlowFieldProcessor.generated.h"

/* 적마다 플레이어까지 남은 길을 플로우 필드에서 읽는다 — 쫓는 적은 갈 곳(MoveTarget)을 넣고, 거리 안팎이 바뀐 적은 StateTree를 깨운다 */
UCLASS()
class UJY_MassFlowFieldProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UJY_MassFlowFieldProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
