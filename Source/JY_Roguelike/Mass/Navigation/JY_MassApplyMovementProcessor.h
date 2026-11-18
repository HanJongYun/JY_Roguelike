#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "JY_MassApplyMovementProcessor.generated.h"

/* 적을 원하는 속도로 옮기되 NavMesh 위로만 — 벽을 못 뚫고 바닥 높이를 따른다 (엔진 이동 대신) */
UCLASS()
class UJY_MassApplyMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UJY_MassApplyMovementProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
