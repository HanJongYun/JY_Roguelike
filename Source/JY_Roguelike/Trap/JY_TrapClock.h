#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

namespace JY_TrapClock
{
	// [Server] [SimulatedProxy]
	/*
	 * 주기 함정(가시·도끼)의 박자 — 서버 시각 기준이라 서버·클라가 복제 없이 같은 위치를 그린다.
	 * 클라는 GameState가 아직 안 왔으면 자기 월드 시각을 써서 그동안만 박자가 어긋난다.
	 */
	inline double GetSyncedSeconds(const UWorld* World)
	{
		const AGameStateBase* GameState = World->GetGameState();
		if (GameState == nullptr)
			return World->GetTimeSeconds();

		return GameState->GetServerWorldTimeSeconds();
	}
}
