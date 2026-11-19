#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "JY_MassNetWriteProcessor.generated.h"

struct FJY_EnemyBehaviorFragment;
struct FJY_EnemyHealthFragment;
struct FJY_EnemyState;

/* 서버 적의 복제 데이터가 바뀌면 복제 Actor의 데이터를 고쳐 클라로 보내게 한다 */
UCLASS()
class UJY_MassNetWriteProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UJY_MassNetWriteProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

public:
	/* 서버 엔티티 값을 보낼 형태(방 기준 상대 위치, 0~255 비율)로 바꾼다 */
	static FJY_EnemyState MakeEnemyState(const FTransform& Transform, const FJY_EnemyHealthFragment& Health, const FJY_EnemyBehaviorFragment& Behavior, uint8 StatusFlags, const FVector& RoomOrigin);

private:
	FMassEntityQuery EntityQuery;

	/* 다음에 비교·전송할 월드 시각 (초) */
	double NextSendSeconds = 0.0;
};
