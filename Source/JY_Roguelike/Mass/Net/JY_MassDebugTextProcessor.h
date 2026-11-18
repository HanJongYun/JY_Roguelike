#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "JY_MassDebugTextProcessor.generated.h"

/* 클라 적 머리 위에 복제받은 상태·체력 값을 글자로 그린다 (체력바·상태 모션이 생기면 지운다) */
UCLASS()
class UJY_MassDebugTextProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UJY_MassDebugTextProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
