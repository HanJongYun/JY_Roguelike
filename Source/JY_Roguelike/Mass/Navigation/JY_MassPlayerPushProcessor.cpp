#include "Mass/Navigation/JY_MassPlayerPushProcessor.h"

#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "Mass/JY_MassProcessorOrder.h"
#include "Mass/JY_MassTypes.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassPlayerPushProcessor)

namespace JY_MassPlayerPush
{
	/* 밀려나는 빠르기에 맞추는 세기 (1/초) — 엔진 서기 조향이 거는 브레이크(MassSteeringProcessors.cpp:343)를 이기고 몇 프레임 안에 그 빠르기가 되게 */
	constexpr float VelocityGain = 20.0f;
}

UJY_MassPlayerPushProcessor::UJY_MassPlayerPushProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Server);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;

	/* 설정값을 GameData(UObject)에서 읽으므로 게임 스레드에서만 */
	bRequiresGameThreadExecution = true;

	JY_MassProcessorOrder::PlaceAfterAvoidance(ExecutionOrder);
}

void UJY_MassPlayerPushProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) //override
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAgentRadiusFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAgentHeightFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJY_EnemyBehaviorFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FJY_EnemyKnockbackFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassForceFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<UJY_MassEnemySubsystem>(EMassFragmentAccess::ReadOnly);
}

/*
 * 위치를 직접 옮기지 않고 힘만 더한다 — 힘은 엔진 ApplyForce가 속도로, 우리 이동이 위치로 바꾸면서 벽을 막아 준다.
 * 힘은 "밀려나는 빠르기 − 지금 속도"로 정해, 다 빠져나오면 스스로 0이 된다 (일정한 힘이면 빠져나온 뒤에도 미끄러짐)
 */
// [Server]
void UJY_MassPlayerPushProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) //override
{
	const float DeltaSeconds = Context.GetDeltaTimeSeconds();
	if (DeltaSeconds <= 0.0f)
		return;

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const float PushSpeed = GameData->EnemyPlayerPushSpeed;
	const float SlowdownDistance = GameData->EnemyPlayerPushSlowdownDistance;

	/* 프레임이 길 때 한 번에 목표 속도를 넘겨 버리지 않게 */
	const float VelocityGain = FMath::Min(JY_MassPlayerPush::VelocityGain, 1.0f / DeltaSeconds);

	auto PushOverlappingEnemies = [PushSpeed, SlowdownDistance, VelocityGain](FMassExecutionContext& ChunkContext)
	{
		const UJY_MassEnemySubsystem& MassEnemySubsystem = ChunkContext.GetSubsystemChecked<UJY_MassEnemySubsystem>();
		const FVector& PlayerLocation = MassEnemySubsystem.GetPlayerLocation();
		const float PlayerRadius = MassEnemySubsystem.GetPlayerRadius();
		const float PlayerHalfHeight = MassEnemySubsystem.GetPlayerHalfHeight();

		/* 플레이어를 아직 한 번도 못 읽었으면 밀 기준이 없다 */
		if (PlayerRadius <= 0.0f)
			return;

		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FAgentRadiusFragment> Radii = ChunkContext.GetFragmentView<FAgentRadiusFragment>();
		const TConstArrayView<FAgentHeightFragment> Heights = ChunkContext.GetFragmentView<FAgentHeightFragment>();
		const TConstArrayView<FJY_EnemyBehaviorFragment> Behaviors = ChunkContext.GetFragmentView<FJY_EnemyBehaviorFragment>();
		const TConstArrayView<FJY_EnemyKnockbackFragment> Knockbacks = ChunkContext.GetFragmentView<FJY_EnemyKnockbackFragment>();
		const TConstArrayView<FMassDesiredMovementFragment> DesiredMovements = ChunkContext.GetFragmentView<FMassDesiredMovementFragment>();
		const TArrayView<FMassForceFragment> Forces = ChunkContext.GetMutableFragmentView<FMassForceFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			/* 예고 중·죽는 중인 적은 몸이 없고, 맞고 밀리는 적은 넉백 속도로만 움직인다 */
			const EJY_EnemyState EnemyState = Behaviors[EntityIt].State;
			if (EnemyState == EJY_EnemyState::Spawning || EnemyState == EJY_EnemyState::Dying || Knockbacks[EntityIt].bActive == true)
				continue;

			/* Transform은 발 위치 — 플레이어가 머리 위를 뛰어넘는 동안은 겹친 것이 아니다 */
			const FVector EnemyLocation = Transforms[EntityIt].GetTransform().GetLocation();
			const float EnemyHalfHeight = Heights[EntityIt].Height * 0.5f;
			const float EnemyCenterZ = EnemyLocation.Z + EnemyHalfHeight;
			const float HeightDifference = FMath::Abs(PlayerLocation.Z - EnemyCenterZ);
			const bool bHeightOverlapping = HeightDifference <= PlayerHalfHeight + EnemyHalfHeight;

			/* 몸이 딱 닿는 거리까지만 민다 — 더 멀리 밀면 캡슐에 닿기만 한 플레이어가 적을 밀고 다닌다 (UJY_MassBlockerSubsystem) */
			const FVector FromPlayer(EnemyLocation.X - PlayerLocation.X, EnemyLocation.Y - PlayerLocation.Y, 0.0f);
			const float Distance = FromPlayer.Size();
			const float Overlap = PlayerRadius + Radii[EntityIt].Radius - Distance;
			const FVector& DesiredVelocity = DesiredMovements[EntityIt].DesiredVelocity;
			if (bHeightOverlapping == false || Overlap <= 0.0f)
			{
				/* 밀려나던 속도가 남은 채 겹침이 풀리면 그 자리에 세운다 — 서 있는 적은 원래 속도가 0이고, 엔진 서기 조향의 브레이크는 최대 가속도에서 잘려 한참 미끄러진다 (MassSteeringProcessors.cpp:344) */
				const bool bStanding = EnemyState == EJY_EnemyState::Wait || EnemyState == EJY_EnemyState::Hit;
				if (bStanding == true && DesiredVelocity.IsNearlyZero() == false)
				{
					Forces[EntityIt].Value -= FVector(DesiredVelocity.X, DesiredVelocity.Y, 0.0f) * VelocityGain;
				}
				continue;
			}

			/* 정확히 같은 자리면 방향이 없다 — 엔진 회피와 같은 기본 방향 (MassAvoidanceProcessors.cpp:948) */
			const FVector PushDirection = Distance > UE_KINDA_SMALL_NUMBER ? FromPlayer / Distance : FVector::ForwardVector;

			/* 깊이 겹쳤을 때는 최고 빠르기, 다 빠져나올수록 느리게 */
			const float SpeedRatio = FMath::Min(1.0f, Overlap / SlowdownDistance);
			const float TargetSpeed = PushSpeed * SpeedRatio;

			/* 미는 방향 속도만 맞춘다 (모자라면 밀고 넘치면 줄임) — 옆 방향은 조향·회피가 넣은 힘 그대로 */
			const float CurrentSpeed = FVector::DotProduct(DesiredVelocity, PushDirection);
			Forces[EntityIt].Value += PushDirection * ((TargetSpeed - CurrentSpeed) * VelocityGain);
		}
	};
	EntityQuery.ForEachEntityChunk(Context, PushOverlappingEnemies);
}
