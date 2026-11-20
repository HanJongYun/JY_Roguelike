#include "Mass/JY_MassStateTreeWake.h"

#include "MassExecutionContext.h"
#include "MassSignalSubsystem.h"
#include "MassStateTreeTypes.h"

namespace JY_MassStateTreeWake
{
	/* 엔진 StateTree Processor는 정해진 신호 이름만 듣는다 — 엔진이 "트리를 다시 돌려라"로 쓰는 신호를 같이 쓴다 (MassStateTreeProcessors.cpp:281, :405) */
	const FName WakeSignalName = UE::Mass::Signals::NewStateTreeTaskRequired;
}

/* 처리 중에 바로 보내지 않고 단계 끝으로 미룬다 — 다른 스레드의 Processor와 겹치지 않게 (MassSignalSubsystem.cpp:124) */
void JY_MassStateTreeWake::WakeEntities(FMassExecutionContext& Context, UMassSignalSubsystem& SignalSubsystem, TConstArrayView<FMassEntityHandle> Entities)
{
	if (Entities.Num() == 0)
		return;

	SignalSubsystem.SignalEntitiesDeferred(Context, WakeSignalName, Entities);
}

/* 처리 바깥이라 미룰 단계가 없다 — 바로 보내도 다른 Processor와 겹치지 않는다 */
void JY_MassStateTreeWake::WakeEntitiesOutsideProcessing(UMassSignalSubsystem& SignalSubsystem, TConstArrayView<FMassEntityHandle> Entities)
{
	if (Entities.Num() == 0)
		return;

	SignalSubsystem.SignalEntities(WakeSignalName, Entities);
}

void JY_MassStateTreeWake::WakeEntityAfterDelay(FMassExecutionContext& Context, UMassSignalSubsystem& SignalSubsystem, FMassEntityHandle Entity, float DelaySeconds)
{
	SignalSubsystem.DelaySignalEntityDeferred(Context, WakeSignalName, Entity, DelaySeconds);
}
