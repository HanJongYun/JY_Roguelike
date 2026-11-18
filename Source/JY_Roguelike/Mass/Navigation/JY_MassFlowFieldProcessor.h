#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "JY_MassFlowFieldProcessor.generated.h"

/* 쫓는 적마다 밟은 다각형의 다음 문을 플로우 필드에서 읽어 갈 곳(MoveTarget)에 넣는다 */
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
