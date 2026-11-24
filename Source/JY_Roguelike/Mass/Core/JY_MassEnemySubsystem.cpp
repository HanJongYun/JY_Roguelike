#include "Mass/Core/JY_MassEnemySubsystem.h"

#include "Mass/Core/JY_EnemyDefinition.h"
#include "Combat/JY_CombatQuerySubsystem.h"
#include "Combat/JY_CombatTypes.h"
#include "MassCommonFragments.h"
#include "MassNavigationSubsystem.h"
#include "MassExecutionContext.h"
#include "Mass/JY_MassStateTreeWake.h"
#include "Mass/JY_MassTypes.h"
#include "MassSignalSubsystem.h"
#include "Mass/Net/JY_MassNetWriteProcessor.h"
#include "Mass/Net/JY_RoomEnemyNetActor.h"
#include "Mass/EntityFragments.h"
#include "MassEntitySubsystem.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "MassEntityConfigAsset.h"
#include "MassRepresentationFragments.h"
#include "MassRepresentationSubsystem.h"
#include "MassVisualizationComponent.h"
#include "MassVisualizationTrait.h"
#include "Animation/AnimSequenceTransformProviderData.h"
#include "Components/InstancedSkinnedMeshComponent.h"
#include "MassSpawnerSubsystem.h"
#include "MassSpawnerTypes.h"
#include "MassSpawnLocationProcessor.h"
#include "MassSimulationSubsystem.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Room/JY_RoomFlowSubsystem.h"
#include "System/JY_LogChannels.h"
#include "StructUtils/InstancedStruct.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassEnemySubsystem)

namespace JY_MassEnemy
{
	/* 서버 Trait(ValidTargetConfig = Server)는 Standalone 월드에서 빠져 Fragment 없는 엔티티가 생긴다 (MassEntityTemplateRegistry.cpp:124) — 기준 실행이 Play As Client라 Standalone은 적을 만들지 않는다 */
	bool IsEnemyServer(const UWorld& World)
	{
		const ENetMode NetMode = World.GetNetMode();
		return NetMode == NM_DedicatedServer || NetMode == NM_ListenServer;
	}

	/* 보간 구간 하나의 최대 길이 = 전송 간격의 이 배수 — 오래 서 있다 움직일 때 느리게 출발하지 않게 */
	constexpr float MaxInterpSendIntervals = 2.0f;

	/* 상태마다 틀 모션 칸 — 상태를 늘리면 여기에 한 줄 */
	EJY_EnemyAnimSlot GetAnimSlot(EJY_EnemyState State)
	{
		switch (State)
		{
		case EJY_EnemyState::Spawning:
		case EJY_EnemyState::Wait:
			return EJY_EnemyAnimSlot::Idle;
		case EJY_EnemyState::Chase:
			return EJY_EnemyAnimSlot::Run;
		case EJY_EnemyState::Hit:
			return EJY_EnemyAnimSlot::Hit;
		case EJY_EnemyState::Dying:
			return EJY_EnemyAnimSlot::Death;
		}

		checkNoEntry();
		return EJY_EnemyAnimSlot::Idle;
	}

	/* 죽음은 끝 자세에서 멈춘다. 피격은 한 번만 — 공중에 뜬 채 맞으면 착지할 때까지 되풀이한다. 나머지는 되풀이 */
	EAnimSequenceTrackLoopMode GetAnimLoopMode(EJY_EnemyAnimSlot AnimSlot, bool bAirborne)
	{
		const bool bGroundHit = AnimSlot == EJY_EnemyAnimSlot::Hit && bAirborne == false;
		const bool bPlaysOnce = AnimSlot == EJY_EnemyAnimSlot::Death || bGroundHit == true;
		return bPlaysOnce == true ? EAnimSequenceTrackLoopMode::Clamp : EAnimSequenceTrackLoopMode::Loop;
	}

	/*
	 * 모션 목록(TP_JY_*)에서 그 칸 애니의 길이를 읽는다 — 그리기 Trait는 클라 전용이지만 설계도 에셋에는 서버에도 들어 있다.
	 * Mass는 목록의 재생 속도가 아니라 FMassRepresentationAnimationFragment의 속도(1배)로 틀어서 길이가 곧 재생 시간이다 (MassVisualizationComponent.cpp:768)
	 */
	float GetAnimSlotSeconds(const UMassEntityConfigAsset& EntityConfig, EJY_EnemyAnimSlot AnimSlot)
	{
		const UMassEntityTraitBase* Trait = EntityConfig.FindTrait(UMassVisualizationTrait::StaticClass());
		const UMassVisualizationTrait* VisualizationTrait = Cast<UMassVisualizationTrait>(Trait);
		if (VisualizationTrait == nullptr)
			return 0.0f;

		const TArray<FMassSkinnedMeshInstanceVisualizationMeshDesc>& Meshes = VisualizationTrait->SkinnedMeshInstanceDesc.Meshes;
		if (Meshes.Num() == 0)
			return 0.0f;

		const UAnimSequenceTransformProviderData* MotionList = Cast<UAnimSequenceTransformProviderData>(Meshes[0].TransformProvider);
		if (MotionList == nullptr)
			return 0.0f;

		return MotionList->GetSequencePlayLength(static_cast<int32>(AnimSlot));
	}
}

/* 복제 Actor는 퍼시스턴트 레벨에 두어 방이 바뀌어도 남는다 */
// [Server]
void UJY_MassEnemySubsystem::OnWorldBeginPlay(UWorld& InWorld) //override
{
	Super::OnWorldBeginPlay(InWorld);

	if (JY_MassEnemy::IsEnemyServer(InWorld) == false)
		return;

	RoomEnemyNetActor = InWorld.SpawnActor<AJY_RoomEnemyNetActor>();

	/* 엔진 LOD 서브시스템이 뷰어 위치를 맞추는 것과 같은 자리 (MassLODSubsystem.cpp:130) */
	UMassSimulationSubsystem* MassSimulationSubsystem = InWorld.GetSubsystem<UMassSimulationSubsystem>();
	MassSimulationSubsystem->GetOnProcessingPhaseStarted(EMassProcessingPhase::PrePhysics).AddUObject(this, &ThisClass::HandlePrePhysicsStarted);
}

/* Processor는 작업 스레드에서 돌 수 있어 Pawn(UObject)을 직접 읽지 않고 여기서 복사한 값만 읽는다 (MassProcessingTypes.h:13) */
// [Server]
void UJY_MassEnemySubsystem::HandlePrePhysicsStarted(float DeltaSeconds)
{
	const UWorld* World = GetWorld();
	GravityZ = World->GetGravityZ();

	/* 데디 서버의 PC는 접속한 플레이어 것뿐 — 플레이어는 항상 한 명 */
	const APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController == nullptr)
		return;

	const APawn* PlayerPawn = PlayerController->GetPawn();
	if (PlayerPawn == nullptr)
		return;

	PlayerLocation = PlayerPawn->GetActorLocation();
	PlayerPawn->GetSimpleCollisionCylinder(OUT PlayerRadius, OUT PlayerHalfHeight);

	/* 길 표를 읽을 적이 없으면 갱신하지 않는다 — 표는 남겨 둔다(다음 웨이브 때 플레이어가 NavMesh 밖이어도 마지막 표로 쫓아오게) */
	if (EntityByReplicationID.Num() == 0)
		return;

	const ARecastNavMesh* NavMesh = GetNavMesh();
	if (NavMesh == nullptr)
		return;

	/* 갈 곳은 바닥 점이어야 한다 — 엔진은 갈 곳이 바닥 위라고 보고 남은 거리를 잰다(캡슐 중심을 주면 높이 차만큼 못 닿음). NavMesh 밖에 서면 표는 그대로 두고 아래에서 좌우만 따라간다 */
	FNavLocation PlayerNavPoint;
	if (NavMesh->ProjectPoint(PlayerLocation, OUT PlayerNavPoint, NavMesh->GetDefaultQueryExtent()) == false)
	{
		/* 발밑 NavMesh를 못 찾아도 좌우로는 플레이어를 따라가게 — 높이만 마지막 바닥 높이 */
		PlayerNavLocation = FVector(PlayerLocation.X, PlayerLocation.Y, PlayerNavLocation.Z);
		return;
	}

	PlayerNavLocation = PlayerNavPoint.Location;

	/* 같은 다각형 안에서 움직이는 동안은 표가 그대로 맞다 — 다각형을 옮겼을 때만 다시 만든다 */
	if (PlayerNavPoint.NodeRef == FlowField.GetGoalPoly())
		return;

	FlowField.Rebuild(*NavMesh, PlayerNavPoint.NodeRef, PlayerNavLocation);
}

// [Server]
void UJY_MassEnemySubsystem::InitializeNavPolys(TConstArrayView<FMassEntityHandle> Entities)
{
	const ARecastNavMesh* NavMesh = GetNavMesh();
	if (NavMesh == nullptr)
		return;

	FMassEntityManager& EntityManager = GetEntityManager();
	for (const FMassEntityHandle& Entity : Entities)
	{
		FTransformFragment& TransformFragment = EntityManager.GetFragmentDataChecked<FTransformFragment>(Entity);
		FTransform& Transform = TransformFragment.GetMutableTransform();

		/* 모든 스폰을 NavMesh 위에서 시작시킨다 — 이동(UJY_MassApplyMovementProcessor)이 이 위치·다각형에서 출발한다 */
		FNavLocation NavLocation;
		if (NavMesh->ProjectPoint(Transform.GetLocation(), OUT NavLocation, NavMesh->GetDefaultQueryExtent()) == false)
		{
			/* 실패한 적은 PolyRef가 비어 길·이동이 건너뛰어 그 자리에 선다 — 스폰 포인트 배치 실수를 찾으려고 남긴다 */
			UE_LOG(LogJY, Warning, TEXT("[Server] JYMassSpawn NavMesh 투영 실패 — 스폰 위치 %s 가 NavMesh 밖, 적이 움직이지 않음"), *Transform.GetLocation().ToString());
			continue;
		}

		Transform.SetLocation(NavLocation.Location);
		FJY_NavPolyFragment& NavPoly = EntityManager.GetFragmentDataChecked<FJY_NavPolyFragment>(Entity);
		NavPoly.PolyRef = NavLocation.NodeRef;
	}
}

const ARecastNavMesh* UJY_MassEnemySubsystem::GetNavMesh() const
{
	const UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavigationSystem == nullptr)
		return nullptr;

	return Cast<ARecastNavMesh>(NavigationSystem->GetDefaultNavDataInstance());
}

/*
 * 엔진 회피 그리드에서 모양 근처 칸의 적만 후보로 받고, 몸 원기둥(반경·키)으로 Actor 캡슐 판정과 같은 식으로 거른다.
 * Mass 처리 중에 불려도 안전한 건 mass.FullyParallel=0(DefaultEngine.ini)으로 Mass가 게임 스레드에서만 돌기 때문
 */
// [Server] [OwnClient]
void UJY_MassEnemySubsystem::FindEnemiesInShape(const FJY_HitShape& Shape, const FVector& Center, const FVector& Forward, TArray<FJY_CombatTarget>& OutTargets) const
{
	const UWorld* World = GetWorld();
	const UMassNavigationSubsystem* NavigationSubsystem = World->GetSubsystem<UMassNavigationSubsystem>();
	if (NavigationSubsystem == nullptr)
		return;

	/* 그리드는 수평 칸이라 높이는 보지 않는다 — 몸 반경은 그리드가 넣을 때 이미 더해 둠 (MassNavigationProcessors.cpp:212) */
	const float ReachRadius = UJY_CombatQuerySubsystem::GetShapeReachRadius(Shape);
	const FVector ReachExtent(ReachRadius, ReachRadius, 0.0f);
	const FBox QueryBox(Center - ReachExtent, Center + ReachExtent);
	TArray<FMassNavigationObstacleItem> Candidates;
	const FNavigationObstacleHashGrid2D& ObstacleGrid = NavigationSubsystem->GetObstacleGrid();
	ObstacleGrid.Query(QueryBox, OUT Candidates);

	const FMassEntityManager& EntityManager = GetEntityManager();
	for (const FMassNavigationObstacleItem& Candidate : Candidates)
	{
		const FMassEntityHandle Entity = Candidate.Entity;
		if (EntityManager.IsEntityValid(Entity) == false)
			continue;

		/* 상태는 서버가 Behavior, 클라가 복제받은 Visual에 들고 있다 — 둘 다 없으면 적이 아니다 */
		const FJY_EnemyBehaviorFragment* Behavior = EntityManager.GetFragmentDataPtr<FJY_EnemyBehaviorFragment>(Entity);
		const FJY_EnemyVisualFragment* Visual = EntityManager.GetFragmentDataPtr<FJY_EnemyVisualFragment>(Entity);
		EJY_EnemyState EnemyState = EJY_EnemyState::Spawning;
		if (Behavior != nullptr)
		{
			EnemyState = Behavior->State;
		}
		else if (Visual != nullptr)
		{
			EnemyState = Visual->State;
		}
		else
		{
			continue;
		}

		if (CanBeHit(EnemyState) == false)
			continue;

		const FTransformFragment& TransformFragment = EntityManager.GetFragmentDataChecked<FTransformFragment>(Entity);
		const FAgentRadiusFragment& RadiusFragment = EntityManager.GetFragmentDataChecked<FAgentRadiusFragment>(Entity);
		const FAgentHeightFragment& HeightFragment = EntityManager.GetFragmentDataChecked<FAgentHeightFragment>(Entity);

		/* Transform은 발 위치 — Actor 캡슐처럼 몸 중심과 반높이로 비교한다 */
		const float BodyHalfHeight = HeightFragment.Height * 0.5f;
		const FTransform& Transform = TransformFragment.GetTransform();
		const FVector BodyCenter = Transform.GetLocation() + FVector(0.0f, 0.0f, BodyHalfHeight);
		const float HeightDifference = FMath::Abs(BodyCenter.Z - Center.Z);
		if (HeightDifference > Shape.HalfHeight + BodyHalfHeight)
			continue;

		if (UJY_CombatQuerySubsystem::IsInsideShape2D(Shape, Center, Forward, BodyCenter, RadiusFragment.Radius) == false)
			continue;

		FJY_CombatTarget& Target = OutTargets.AddDefaulted_GetRef();
		Target.Entity = Entity;
		Target.Location = BodyCenter;
	}
}

/* 예고 중인 적은 아직 싸움에 끼지 않고, 죽는 중인 적은 이미 빠졌다 — 안 맞는 상태를 늘리면 여기에 한 줄 */
// [Server] [OwnClient]
bool UJY_MassEnemySubsystem::CanBeHit(EJY_EnemyState State)
{
	return State != EJY_EnemyState::Spawning && State != EJY_EnemyState::Dying;
}

/* 상태는 서버가 Behavior, 클라가 복제받은 Visual에 들고 있다 */
// [Server] [OwnClient]
EJY_EnemyState UJY_MassEnemySubsystem::GetEnemyState(FMassEntityHandle Entity) const
{
	const FMassEntityManager& EntityManager = GetEntityManager();
	const FJY_EnemyBehaviorFragment* Behavior = EntityManager.GetFragmentDataPtr<FJY_EnemyBehaviorFragment>(Entity);
	if (Behavior != nullptr)
		return Behavior->State;

	const FJY_EnemyVisualFragment& Visual = EntityManager.GetFragmentDataChecked<FJY_EnemyVisualFragment>(Entity);
	return Visual.State;
}

/* 넉백 값은 서버에만 있다 — 클라는 복제받은 값을 쓴다 */
// [Server] [OwnClient]
bool UJY_MassEnemySubsystem::IsEnemyAirborne(FMassEntityHandle Entity) const
{
	const FMassEntityManager& EntityManager = GetEntityManager();
	const FJY_EnemyKnockbackFragment* Knockback = EntityManager.GetFragmentDataPtr<FJY_EnemyKnockbackFragment>(Entity);
	if (Knockback != nullptr)
		return Knockback->IsAirborne();

	const FJY_EnemyVisualFragment& Visual = EntityManager.GetFragmentDataChecked<FJY_EnemyVisualFragment>(Entity);
	return Visual.bAirborne;
}

/* Processor 실행 중에는 부르면 안 된다 — 타이머·치트처럼 Mass 처리 바깥에서만 (MassEntityManager.cpp:846) */
// [Server]
TArray<FMassEntityHandle> UJY_MassEnemySubsystem::SpawnWave(const UJY_EnemyDefinition& EnemyDefinition, TConstArrayView<FVector> Locations, float SpawnWarningSeconds)
{
	TArray<FMassEntityHandle> SpawnedEntities;
	UWorld* World = GetWorld();
	if (JY_MassEnemy::IsEnemyServer(*World) == false)
		return SpawnedEntities;

	/* 클라는 이 번호로 같은 배열에서 같은 정의를 찾는다 — 배열에 없는 정의는 클라가 만들 수 없다. 복제 칸이 uint8이라 256종까지 */
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const int32 DefinitionIndex = GameData->EnemyDefinitions.IndexOfByKey(&EnemyDefinition);
	check(DefinitionIndex != INDEX_NONE);
	check(DefinitionIndex <= MAX_uint8);

	/* 복제 위치는 값이 작을수록 비트를 덜 쓴다 (QuantizedVectorSerialization.cpp:90) — 방에 하나씩 있는 PlayerStart를 기준점으로 쓴다 */
	const UJY_RoomFlowSubsystem* RoomFlow = World->GetSubsystem<UJY_RoomFlowSubsystem>();
	const FVector RoomOrigin = RoomFlow->GetCurrentRoomStartLocation();
	RoomEnemyNetActor->SetRoomOrigin(RoomOrigin);

	SpawnedEntities = SpawnEntitiesAt(*EnemyDefinition.EntityConfig, Locations);
	InitializeNavPolys(SpawnedEntities);

	/* 움찔하는 시간·죽는 시간은 손으로 적지 않고 모션 길이를 그대로 쓴다 — 칸이 비면 움찔하지 않고, 죽으면 바로 사라진다 */
	const float HitSeconds = JY_MassEnemy::GetAnimSlotSeconds(*EnemyDefinition.EntityConfig, EJY_EnemyAnimSlot::Hit);
	const float DeathSeconds = JY_MassEnemy::GetAnimSlotSeconds(*EnemyDefinition.EntityConfig, EJY_EnemyAnimSlot::Death);
	if (HitSeconds <= 0.0f || DeathSeconds <= 0.0f)
	{
		UE_LOG(LogJY, Warning, TEXT("[Server] JYMassSpawn %s 의 모션 목록에 Hit(%.2f초)·Death(%.2f초) 칸이 비어 있음"), *EnemyDefinition.GetName(), HitSeconds, DeathSeconds);
	}

	const double SpawningEndSeconds = World->GetTimeSeconds() + SpawnWarningSeconds;
	FMassEntityManager& EntityManager = GetEntityManager();
	for (const FMassEntityHandle& Entity : SpawnedEntities)
	{
		FJY_EnemyBehaviorFragment& Behavior = EntityManager.GetFragmentDataChecked<FJY_EnemyBehaviorFragment>(Entity);
		Behavior.State = EJY_EnemyState::Spawning;
		Behavior.SpawningEndSeconds = SpawningEndSeconds;
		Behavior.HitSeconds = HitSeconds;
		Behavior.DeathSeconds = DeathSeconds;

		/* 엔티티가 실제로 선 위치로 복제 데이터를 만든다 — 위치 배열 순서와 엔티티 순서가 같다고 가정하지 않음 */
		const FTransformFragment& TransformFragment = EntityManager.GetFragmentDataChecked<FTransformFragment>(Entity);
		const FTransform& Transform = TransformFragment.GetTransform();
		const FJY_EnemyHealthFragment& Health = EntityManager.GetFragmentDataChecked<FJY_EnemyHealthFragment>(Entity);
		const uint8 NoStatusFlags = 0;
		const bool bNotAirborne = false;
		const FJY_EnemyState EnemyState = UJY_MassNetWriteProcessor::MakeEnemyState(Transform, Health, Behavior, NoStatusFlags, bNotAirborne, RoomOrigin);
		const int32 ReplicationID = RoomEnemyNetActor->AddEnemy(static_cast<uint8>(DefinitionIndex), EnemyState);

		/* 서버 엔티티 → 복제 데이터 연결. 클라는 반대 방향(EntityByReplicationID)으로 찾는다 */
		FJY_EnemyLastSentToClientFragment& LastSentToClient = EntityManager.GetFragmentDataChecked<FJY_EnemyLastSentToClientFragment>(Entity);
		LastSentToClient.ReplicationID = ReplicationID;
		LastSentToClient.LastSentState = EnemyState;

		EntityByReplicationID.Add(ReplicationID, Entity);
	}

	return SpawnedEntities;
}

/* 타이머·방 입장 알림처럼 Mass 처리 바깥에서만 부른다 (MassEntityManager.cpp:846) */
// [Server]
void UJY_MassEnemySubsystem::DestroyAllEnemies()
{
	if (EntityByReplicationID.Num() == 0)
		return;

	TArray<FMassEntityHandle> Entities;
	EntityByReplicationID.GenerateValueArray(OUT Entities);
	FMassEntityManager& EntityManager = GetEntityManager();
	EntityManager.BatchDestroyEntities(Entities);

	EntityByReplicationID.Reset();
	RoomEnemyNetActor->RemoveAllEnemies();

	/* 적이 0마리면 피해 Processor가 아예 돌지 않아(MassProcessor.h:122) 지운 적의 피해가 남는다 */
	PendingHitsBuffer.Reset();
}

// [Server]
void UJY_MassEnemySubsystem::AddPendingHit(FMassEntityHandle Entity, float Damage, int32 StatusIndex, const FVector& KnockbackForce)
{
	FJY_PendingMassHit& PendingHit = PendingHitsBuffer.FindOrAdd(Entity);
	PendingHit.Damage += Damage;
	if (StatusIndex != INDEX_NONE)
	{
		PendingHit.StatusFlags |= static_cast<uint8>(1 << StatusIndex);
	}

	/* 같은 프레임에 힘 없는 타격이 뒤에 와도 앞 타격의 힘을 지우지 않는다 */
	if (KnockbackForce.IsNearlyZero() == false)
	{
		PendingHit.KnockbackForce = KnockbackForce;
	}
}

// [Server]
TMap<FMassEntityHandle, FJY_PendingMassHit> UJY_MassEnemySubsystem::ConsumePendingHits()
{
	return MoveTemp(PendingHitsBuffer);
}

/* 피해 Processor 안에서 불린다 — 처리 중엔 엔티티를 바로 못 지워 Defer로 단계 끝에 지운다(MassProcessingPhaseManager.cpp:663) */
// [Server]
void UJY_MassEnemySubsystem::DestroyEnemy(FMassExecutionContext& Context, FMassEntityHandle Entity, int32 ReplicationID)
{
	EntityByReplicationID.Remove(ReplicationID);
	RoomEnemyNetActor->RemoveEnemy(ReplicationID);
	Context.Defer().DestroyEntity(Entity);
}

/* 치트 시험용 — 실제 이동은 2장 이동 Processor가 한다 */
// [Server]
void UJY_MassEnemySubsystem::MoveAllEnemies(const FVector& Offset)
{
	FMassEntityManager& EntityManager = GetEntityManager();
	for (const TPair<int32, FMassEntityHandle>& Pair : EntityByReplicationID)
	{
		FTransformFragment& TransformFragment = EntityManager.GetFragmentDataChecked<FTransformFragment>(Pair.Value);
		FTransform& Transform = TransformFragment.GetMutableTransform();
		Transform.AddToTranslation(Offset);
	}
}

/* 치트 시험용 — 공격과 같은 길(피해 목록 → 피해 Processor)로 넣어 움찔·죽음도 똑같이 일어난다 */
// [Server]
void UJY_MassEnemySubsystem::DamageAllEnemies(float Damage)
{
	for (const TPair<int32, FMassEntityHandle>& Pair : EntityByReplicationID)
	{
		AddPendingHit(Pair.Value, Damage, INDEX_NONE, FVector::ZeroVector);
	}
}

/*
 * 속도만 적어 두고 상태는 StateTree가 바꾼다 — 실제로 옮기는 것은 UJY_MassApplyMovementProcessor.
 * 치트·공격처럼 Mass 처리 바깥에서만 부른다
 */
// [Server]
void UJY_MassEnemySubsystem::KnockbackEnemies(const FVector& Center, float Radius, float HorizontalImpulse, float VerticalImpulse)
{
	UWorld* World = GetWorld();
	if (JY_MassEnemy::IsEnemyServer(*World) == false)
		return;

	/* 공격 판정과 같은 길로 반경 안의 적을 찾는다 — 안 맞는 상태(예고 중·죽는 중)의 적은 여기서 빠진다 */
	FJY_HitShape SearchShape;
	SearchShape.Type = EJY_HitShapeType::Sphere;
	SearchShape.Radius = Radius;
	TArray<FJY_CombatTarget> Targets;
	FindEnemiesInShape(SearchShape, Center, FVector::ForwardVector, OUT Targets);

	FMassEntityManager& EntityManager = GetEntityManager();
	TArray<FMassEntityHandle> KnockedBackEntities;
	KnockedBackEntities.Reserve(Targets.Num());
	for (const FJY_CombatTarget& Target : Targets)
	{
		/* 정확히 같은 자리면 방향이 없다 — 엔진 회피와 같은 기본 방향 (MassAvoidanceProcessors.cpp:948) */
		const FVector FromCenter(Target.Location.X - Center.X, Target.Location.Y - Center.Y, 0.0f);
		const float Distance = FromCenter.Size();
		const FVector OutwardDirection = Distance > UE_KINDA_SMALL_NUMBER ? FromCenter / Distance : FVector::ForwardVector;

		/* 날아가는 중에 또 맞으면 높이는 그대로 두고 속도만 새로 정한다 */
		FJY_EnemyKnockbackFragment& Knockback = EntityManager.GetFragmentDataChecked<FJY_EnemyKnockbackFragment>(Target.Entity);
		const float HorizontalSpeed = HorizontalImpulse / Knockback.Mass;
		const float VerticalSpeed = VerticalImpulse / Knockback.Mass;
		Knockback.Velocity = OutwardDirection * HorizontalSpeed + FVector(0.0f, 0.0f, VerticalSpeed);
		Knockback.bActive = true;

		KnockedBackEntities.Add(Target.Entity);
	}

	UMassSignalSubsystem* SignalSubsystem = World->GetSubsystem<UMassSignalSubsystem>();
	JY_MassStateTreeWake::WakeEntitiesOutsideProcessing(*SignalSubsystem, KnockedBackEntities);
}

// [OwnClient]
void UJY_MassEnemySubsystem::SpawnReplicatedEnemies(const FJY_FastArrayEnemyContainer& EnemyContainer, TConstArrayView<int32> AddedIndices)
{
	/* 한 번의 수신에 여러 종류가 섞여 올 수 있다 — 종류마다 한 번씩 만든다 */
	TMap<uint8, TArray<int32>> AddedIndicesByDefinition;
	for (const int32 EnemyIndex : AddedIndices)
	{
		const uint8 DefinitionIndex = EnemyContainer.Items[EnemyIndex].DefinitionIndex;
		AddedIndicesByDefinition.FindOrAdd(DefinitionIndex).Add(EnemyIndex);
	}

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const FVector RoomOrigin = EnemyContainer.OwnerActor->GetRoomOrigin();
	const float ServerSeconds = EnemyContainer.OwnerActor->GetLastSendServerSeconds();
	const float MaxInterpSeconds = GameData->EnemyNetSendInterval * JY_MassEnemy::MaxInterpSendIntervals;
	for (const TPair<uint8, TArray<int32>>& Pair : AddedIndicesByDefinition)
	{
		const UJY_EnemyDefinition* EnemyDefinition = GameData->EnemyDefinitions[Pair.Key];
		const TArray<int32>& DefinitionEnemyIndices = Pair.Value;

		TArray<FVector> Locations;
		Locations.Reserve(DefinitionEnemyIndices.Num());
		for (const int32 EnemyIndex : DefinitionEnemyIndices)
		{
			Locations.Add(RoomOrigin + EnemyContainer.Items[EnemyIndex].EnemyState.RelativeLocation);
		}

		TArray<FMassEntityHandle> SpawnedEntities = SpawnEntitiesAt(*EnemyDefinition->EntityConfig, Locations);
		for (int32 Index = 0; Index < SpawnedEntities.Num(); ++Index)
		{
			const FJY_FastArrayEnemyItem& EnemyItem = EnemyContainer.Items[DefinitionEnemyIndices[Index]];
			EntityByReplicationID.Add(EnemyItem.ReplicationID, SpawnedEntities[Index]);
			ApplyEnemyState(SpawnedEntities[Index], EnemyItem.EnemyState, RoomOrigin, ServerSeconds, MaxInterpSeconds, true);
		}
	}
}

// [OwnClient]
void UJY_MassEnemySubsystem::UpdateReplicatedEnemies(const FJY_FastArrayEnemyContainer& EnemyContainer, TConstArrayView<int32> ChangedIndices)
{
	const FVector RoomOrigin = EnemyContainer.OwnerActor->GetRoomOrigin();
	const float ServerSeconds = EnemyContainer.OwnerActor->GetLastSendServerSeconds();
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const float MaxInterpSeconds = GameData->EnemyNetSendInterval * JY_MassEnemy::MaxInterpSendIntervals;
	for (const int32 EnemyIndex : ChangedIndices)
	{
		const FJY_FastArrayEnemyItem& EnemyItem = EnemyContainer.Items[EnemyIndex];
		const FMassEntityHandle Entity = EntityByReplicationID.FindChecked(EnemyItem.ReplicationID);
		ApplyEnemyState(Entity, EnemyItem.EnemyState, RoomOrigin, ServerSeconds, MaxInterpSeconds, false);
	}
}

/* 배열 수신 단계(엔진 네트 수신)는 Mass 처리 바깥이라 바로 지워도 된다 */
// [OwnClient]
void UJY_MassEnemySubsystem::DestroyReplicatedEnemies(const FJY_FastArrayEnemyContainer& EnemyContainer, TConstArrayView<int32> RemovedIndices)
{
	TArray<FMassEntityHandle> Entities;
	Entities.Reserve(RemovedIndices.Num());
	for (const int32 EnemyIndex : RemovedIndices)
	{
		const int32 ReplicationID = EnemyContainer.Items[EnemyIndex].ReplicationID;
		Entities.Add(EntityByReplicationID.FindAndRemoveChecked(ReplicationID));
	}

	FMassEntityManager& EntityManager = GetEntityManager();
	EntityManager.BatchDestroyEntities(Entities);
}

// [OwnClient]
void UJY_MassEnemySubsystem::ApplyEnemyState(FMassEntityHandle Entity, const FJY_EnemyState& EnemyState, const FVector& RoomOrigin, float ServerSeconds, float MaxInterpSeconds, bool bSnapToTarget)
{
	FMassEntityManager& EntityManager = GetEntityManager();

	const float Yaw = FRotator::DecompressAxisFromByte(EnemyState.Yaw);
	const FRotator Rotation(0.0, Yaw, 0.0);
	const FVector TargetLocation = RoomOrigin + EnemyState.RelativeLocation;
	const FQuat TargetRotation = Rotation.Quaternion();

	FTransformFragment& TransformFragment = EntityManager.GetFragmentDataChecked<FTransformFragment>(Entity);
	FTransform& Transform = TransformFragment.GetMutableTransform();
	FJY_EnemyNetInterpFragment& Interp = EntityManager.GetFragmentDataChecked<FJY_EnemyNetInterpFragment>(Entity);

	/* 구간 길이 = 서버가 이 적을 보낸 시각의 차이 — 받은 간격이 흔들려도 서버에서 흐른 시간대로 간다 (CharacterMovementComponent.cpp:8286) */
	const float ServerDeltaSeconds = ServerSeconds - Interp.LastServerSeconds;

	/* 오래 서 있다 움직이면 차이가 몇 초가 되어 느리게 출발한다 — 상한으로 자른다 */
	const float Duration = FMath::Clamp(ServerDeltaSeconds, 0.0f, MaxInterpSeconds);
	Interp.LastServerSeconds = ServerSeconds;

	const bool bTargetChanged = TargetLocation.Equals(Interp.TargetLocation) == false || TargetRotation.Equals(Interp.TargetRotation) == false;
	if (bSnapToTarget == true || Duration <= UE_SMALL_NUMBER)
	{
		Transform.SetLocation(TargetLocation);
		Transform.SetRotation(TargetRotation);
		Interp.StartLocation = TargetLocation;
		Interp.StartRotation = TargetRotation;
		Interp.TargetLocation = TargetLocation;
		Interp.TargetRotation = TargetRotation;
		Interp.Duration = 0.0f;
		Interp.Elapsed = 0.0f;
	}
	/* 체력·상태만 바뀐 수신은 가던 구간을 그대로 둔다 */
	else if (bTargetChanged == true)
	{
		/* 출발은 지금 그려진 자리 — 앞 구간을 다 못 갔어도 남은 거리가 새 구간에 합쳐진다 */
		Interp.StartLocation = Transform.GetLocation();
		Interp.StartRotation = Transform.GetRotation();
		Interp.TargetLocation = TargetLocation;
		Interp.TargetRotation = TargetRotation;
		Interp.Duration = Duration;
		Interp.Elapsed = 0.0f;
	}

	FJY_EnemyVisualFragment& Visual = EntityManager.GetFragmentDataChecked<FJY_EnemyVisualFragment>(Entity);

	/* 움찔하는 중에 또 맞았나 — 상태가 그대로라 모션 칸 번호가 안 바뀐다 */
	const bool bStaysInHit = Visual.State == EJY_EnemyState::Hit && EnemyState.State == EJY_EnemyState::Hit;
	const bool bHitAgain = bStaysInHit == true && EnemyState.HitSerial != Visual.HitSerial;
	Visual.HitSerial = EnemyState.HitSerial;
	Visual.bAirborne = EnemyState.bAirborne;
	Visual.State = EnemyState.State;
	Visual.HealthPct = EnemyState.HealthPct;
	Visual.ShieldPct = EnemyState.ShieldPct;
	Visual.StatusFlags = EnemyState.StatusFlags;

	/* 엔진은 모션 목록의 번호로만 모션을 고른다 (AnimSequenceTransformProviderData.h:108) */
	FMassRepresentationAnimationFragment* Animation = EntityManager.GetFragmentDataPtr<FMassRepresentationAnimationFragment>(Entity);
	if (Animation != nullptr)
	{
		const EJY_EnemyAnimSlot AnimSlot = JY_MassEnemy::GetAnimSlot(EnemyState.State);
		Animation->AnimData.SequenceIndex = static_cast<int32>(AnimSlot);
		Animation->AnimData.LoopMode = JY_MassEnemy::GetAnimLoopMode(AnimSlot, EnemyState.bAirborne);
		Animation->AnimData.BlendTime = 0.2f;

		if (bHitAgain == true)
		{
			RestartEnemyAnimation(Entity, Animation->AnimData);
		}
	}
}

/*
 * Mass는 모션 칸 번호가 바뀔 때만 모션을 튼다 (MassVisualizationComponent.cpp:766) — 같은 칸을 처음부터 다시 틀려면 모션 재생기를 직접 부른다.
 * 화면에 아직 안 그려진 적은 트랙이 없어 아무 일도 없다
 */
// [OwnClient]
void UJY_MassEnemySubsystem::RestartEnemyAnimation(FMassEntityHandle Entity, const FAnimSequenceTrackAutoPlayData& AnimData)
{
	const UWorld* World = GetWorld();
	const UMassRepresentationSubsystem* RepresentationSubsystem = World->GetSubsystem<UMassRepresentationSubsystem>();
	const UMassVisualizationComponent* VisualizationComponent = RepresentationSubsystem->GetVisualizationComponent();
	if (VisualizationComponent == nullptr)
		return;

	/* Mass는 기본 레이어 하나만 쓴다 (MassVisualizationComponent.cpp:768) */
	const int32 BaseLayerIndex = 0;

	/* 메시 종류(LOD 포함)마다 묶음이 하나 — 이 적의 트랙이 있는 묶음을 전부 다시 튼다. 번호가 끝나면 nullptr (MassSkinnedMeshRepresentationTypes.h:534) */
	for (int32 SharedDataIndex = 0; ; ++SharedDataIndex)
	{
		const FMassInstancedSkinnedMeshComponentSharedData* SharedData = VisualizationComponent->GetSharedDataForDescriptionIndex(SharedDataIndex);
		if (SharedData == nullptr)
			return;

		const int32* TrackIndex = SharedData->GetEntityToTrackMap().Find(Entity);
		const UInstancedSkinnedMeshComponent* MeshComponent = SharedData->GetInstancedSkinnedMeshComponent();
		if (TrackIndex == nullptr || MeshComponent == nullptr)
			continue;

		UAnimSequenceTransformProviderDataInstance* MotionPlayer = Cast<UAnimSequenceTransformProviderDataInstance>(MeshComponent->GetTransformProvider());
		if (MotionPlayer == nullptr)
			continue;

		MotionPlayer->SetAutoPlayData(*TrackIndex, BaseLayerIndex, AnimData);
	}
}

FMassEntityManager& UJY_MassEnemySubsystem::GetEntityManager() const
{
	UWorld* World = GetWorld();
	UMassEntitySubsystem* MassEntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	return MassEntitySubsystem->GetMutableEntityManager();
}

TArray<FMassEntityHandle> UJY_MassEnemySubsystem::SpawnEntitiesAt(const UMassEntityConfigAsset& EnemyConfig, TConstArrayView<FVector> Locations)
{
	TArray<FMassEntityHandle> SpawnedEntities;
	if (Locations.Num() == 0)
		return SpawnedEntities;

	UWorld* World = GetWorld();

	/* Trait 목록 → 이 적이 가질 Fragment 묶음(템플릿). 처음 한 번 만들고 이후엔 캐시를 돌려준다 */
	const FMassEntityTemplate& EnemyTemplate = EnemyConfig.GetOrCreateEntityTemplate(*World);

	FInstancedStruct SpawnData = FInstancedStruct::Make<FMassTransformsSpawnData>();
	FMassTransformsSpawnData& TransformsSpawnData = SpawnData.GetMutable<FMassTransformsSpawnData>();

	/* 기본값은 위치를 섞어 나눠 준다 — 그러면 N번째 엔티티가 N번째 위치에 서지 않는다 (MassSpawnerTypes.h:29) */
	TransformsSpawnData.bRandomize = false;
	TransformsSpawnData.Transforms.Reserve(Locations.Num());
	for (const FVector& Location : Locations)
	{
		TransformsSpawnData.Transforms.Add(FTransform(Location));
	}

	/* 위치는 생성 직후 이 Processor가 TransformFragment에 넣는다 — 엔진 생성기와 같은 조합 (MassEntityEQSSpawnPointsGenerator.cpp:75) */
	UMassSpawnerSubsystem* SpawnerSubsystem = World->GetSubsystem<UMassSpawnerSubsystem>();
	SpawnerSubsystem->SpawnEntities(EnemyTemplate.GetTemplateID(), Locations.Num(), FConstStructView(SpawnData), UMassSpawnLocationProcessor::StaticClass(), OUT SpawnedEntities);

	return SpawnedEntities;
}
