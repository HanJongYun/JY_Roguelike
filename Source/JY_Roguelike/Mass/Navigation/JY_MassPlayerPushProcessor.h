#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "JY_MassPlayerPushProcessor.generated.h"

/* 플레이어와 몸이 겹친 적을 밖으로 밀어낸다 — 서 있는 적도 민다 (엔진 회피는 서 있는 적을 건너뜀) */
UCLASS()
class UJY_MassPlayerPushProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UJY_MassPlayerPushProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
