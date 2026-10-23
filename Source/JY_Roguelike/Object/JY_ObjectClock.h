#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

namespace JY_ObjectClock
{
	// [Server] [SimulatedProxy]
	/* 서버·클라가 공유하는 서버 시각 — GameState가 오기 전엔 자기 월드 시각 */
	inline double GetSyncedSeconds(const UWorld* World)
	{
		const AGameStateBase* GameState = World->GetGameState();
		if (GameState == nullptr)
			return World->GetTimeSeconds();

		return GameState->GetServerWorldTimeSeconds();
	}
}
