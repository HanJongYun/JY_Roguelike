#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityHandle.h"

class UMassSignalSubsystem;
struct FMassExecutionContext;

/*
 * 적의 StateTree를 깨우는 유일한 통로 — Mass StateTree는 신호를 받은 적만 전환 조건을 다시 본다 (MassStateTreeProcessors.cpp:279)
 * 어떤 신호로 깨우는지는 이 파일만 안다. 부르는 쪽은 "상태가 바뀔 만한 일이 생겼다"만 알린다
 */
namespace JY_MassStateTreeWake
{
	/* 이 적들의 StateTree를 다음 Behavior 단계에 한 번 돌린다 — Processor·Task 안에서 부른다 */
	void WakeEntities(FMassExecutionContext& Context, UMassSignalSubsystem& SignalSubsystem, TConstArrayView<FMassEntityHandle> Entities);

	/* 이 적의 StateTree를 DelaySeconds 뒤에 한 번 돌린다 */
	void WakeEntityAfterDelay(FMassExecutionContext& Context, UMassSignalSubsystem& SignalSubsystem, FMassEntityHandle Entity, float DelaySeconds);
}
