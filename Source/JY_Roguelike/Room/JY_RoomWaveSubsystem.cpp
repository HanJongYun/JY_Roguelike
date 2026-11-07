#include "Room/JY_RoomWaveSubsystem.h"

#include "Mass/Core/JY_EnemyDefinition.h"
#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "Room/JY_RoomData.h"
#include "Room/JY_RoomFlowSubsystem.h"
#include "Room/JY_RoomMarker.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_RoomWaveSubsystem)

namespace JY_RoomWave
{
	/* 한 틱에 만드는 최대 마릿수 — 엔티티 생성과 복제 원소 추가가 한 프레임에 몰리지 않게. ⚠️ 측정해서 다시 정한다 */
	constexpr int32 MaxSpawnsPerTick = 10;

	/* 스폰 자리 하나를 찾을 때 무작위로 뽑아 볼 횟수 — 넘으면 지점 위에 세운다 */
	constexpr int32 MaxSpawnLocationTries = 4;

	TArray<const AJY_RoomEnemySpawnPoint*> FindSpawnPoints(const ULevel& RoomLevel)
	{
		TArray<const AJY_RoomEnemySpawnPoint*> SpawnPoints;
		for (const AActor* Actor : RoomLevel.Actors)
		{
			const AJY_RoomEnemySpawnPoint* SpawnPoint = Cast<AJY_RoomEnemySpawnPoint>(Actor);
			if (SpawnPoint != nullptr)
			{
				SpawnPoints.Add(SpawnPoint);
			}
		}
		return SpawnPoints;
	}
}

bool UJY_RoomWaveSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const //override
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

/* 구독할 방 흐름이 먼저 만들어져 있게 한다 — OnWorldBeginPlay 순서까지 정해 주지는 않는다(SubsystemCollection.cpp:524) */
void UJY_RoomWaveSubsystem::Initialize(FSubsystemCollectionBase& Collection) //override
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UJY_RoomFlowSubsystem>();
}

/*
 * 방 흐름의 입장 알림이 서버에서만 오므로 클라는 구독하지 않는다.
 * BeginPlay 순서와 상관없이 놓치지 않는다 — 첫 입장은 레벨 로딩 콜백(Latent Action) 뒤라 다음 틱 이후다 (LevelStreaming.cpp:250)
 */
// [Server]
void UJY_RoomWaveSubsystem::OnWorldBeginPlay(UWorld& InWorld) //override
{
	Super::OnWorldBeginPlay(InWorld);

	if (InWorld.GetNetMode() == NM_Client)
		return;

	UJY_RoomFlowSubsystem* RoomFlow = InWorld.GetSubsystem<UJY_RoomFlowSubsystem>();
	RoomFlow->OnRoomEntered.AddUObject(this, &ThisClass::HandleRoomEntered);
}

// [Server]
void UJY_RoomWaveSubsystem::HandleRoomEntered(const ULevel* RoomLevel, const UJY_RoomData* RoomData)
{
	UWorld* World = GetWorld();
	FTimerManager& TimerManager = World->GetTimerManager();
	TimerManager.ClearTimer(FirstWaveTimerHandle);
	TimerManager.ClearTimer(SpawnTimerHandle);
	QueuedEnemySpawns.Reset();

	/* 지난 방에 남은 적 — 화면이 다 어두워진 뒤 불려 사라지는 게 안 보인다 (UJY_RoomFlowSubsystem::TryEnterNextRoom) */
	UJY_MassEnemySubsystem* MassEnemySubsystem = World->GetSubsystem<UJY_MassEnemySubsystem>();
	MassEnemySubsystem->DestroyAllEnemies();

	CurrentRoomLevel = RoomLevel;
	CurrentRoomData = RoomData;
	if (RoomData->IsCombatRoom() == false)
		return;

	/* 웨이브 수(DA)와 지점 번호(맵)가 두 곳에 있다 — 어긋나면 그 지점은 영영 안 나오므로 드러낸다 */
	const int32 WaveCount = RoomData->GetWaveCount();
	ensureMsgf(WaveCount > 0, TEXT("%s는 전투 방인데 WaveCount가 0"), *RoomData->GetName());
	for (const AJY_RoomEnemySpawnPoint* SpawnPoint : JY_RoomWave::FindSpawnPoints(*RoomLevel))
	{
		ensureMsgf(SpawnPoint->GetWaveNumber() <= WaveCount, TEXT("%s의 WaveNumber %d가 %s WaveCount %d보다 큼"), *SpawnPoint->GetName(), SpawnPoint->GetWaveNumber(), *RoomData->GetName(), WaveCount);
	}

	/* 시간 0으로 SetTimer를 부르면 타이머가 지워지기만 한다 (TimerManager.cpp:673) */
	const float FirstWaveDelaySeconds = RoomData->GetFirstWaveDelaySeconds();
	if (FirstWaveDelaySeconds <= 0.0f)
	{
		StartWave(1);
		return;
	}

	const FTimerDelegate StartFirstWave = FTimerDelegate::CreateUObject(this, &ThisClass::StartWave, 1);
	TimerManager.SetTimer(FirstWaveTimerHandle, StartFirstWave, FirstWaveDelaySeconds, false);
}

// [Server]
void UJY_RoomWaveSubsystem::StartWave(int32 WaveNumber)
{
	const ULevel* RoomLevel = CurrentRoomLevel.Get();
	for (const AJY_RoomEnemySpawnPoint* SpawnPoint : JY_RoomWave::FindSpawnPoints(*RoomLevel))
	{
		if (SpawnPoint->GetWaveNumber() != WaveNumber)
			continue;

		for (const FJY_SpawnPointEnemy& Enemy : SpawnPoint->GetEnemies())
		{
			if (ensureMsgf(Enemy.EnemyDefinition != nullptr, TEXT("%s의 Enemies에 EnemyDefinition이 비어 있음"), *SpawnPoint->GetName()) == false)
				continue;

			for (int32 Index = 0; Index < Enemy.Count; ++Index)
			{
				FJY_QueuedEnemySpawn QueuedSpawn;
				QueuedSpawn.EnemyDefinition = Enemy.EnemyDefinition;
				QueuedSpawn.Location = FindSpawnLocation(*SpawnPoint);
				QueuedEnemySpawns.PushLast(QueuedSpawn);
			}
		}
	}

	SpawnQueuedEnemies();
}

/*
 * 원 안 무작위 자리 → 네비메시 위로 → 지점에서 그 자리까지 네비 레이캐스트가 경계(벽)에 안 막히면 쓴다. 2-3 길찾기 격자가 생기면 그 칸 검사로 바꾼다.
 * GetRandomReachablePointInRadius를 안 쓰는 이유: 반경에 걸친 큰 바닥 폴리곤 안에서 몇 번만 뽑아 원 안에 들면 성공이라, 반경이 폴리곤보다 작으면 자주 실패한다 (DetourNavMeshQuery.cpp findRandomPointAroundCircle)
 */
// [Server]
FVector UJY_RoomWaveSubsystem::FindSpawnLocation(const AJY_RoomEnemySpawnPoint& SpawnPoint) const
{
	const FVector PointLocation = SpawnPoint.GetActorLocation();
	const float SpawnRadius = SpawnPoint.GetSpawnRadius();
	UWorld* World = GetWorld();
	const UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

	/* 지점이 네비메시 밖이면 고를 기준이 없다 — 배치 실수라 드러내고 지점 위에 세운다 */
	FNavLocation PointNavLocation;
	const bool bPointOnNavMesh = NavigationSystem != nullptr && NavigationSystem->ProjectPointToNavigation(PointLocation, OUT PointNavLocation);
	if (ensureMsgf(bPointOnNavMesh == true, TEXT("%s가 네비메시 위에 없음"), *SpawnPoint.GetName()) == false)
		return PointLocation;

	if (SpawnRadius <= 0.0f)
		return PointNavLocation.Location;

	for (int32 TryIndex = 0; TryIndex < JY_RoomWave::MaxSpawnLocationTries; ++TryIndex)
	{
		const FVector2D RandomOffset = FMath::RandPointInCircle(SpawnRadius);
		const FVector Candidate = PointNavLocation.Location + FVector(RandomOffset, 0.0);

		/* 벽 속·방 밖이면 네비메시가 없다 */
		FNavLocation CandidateNavLocation;
		if (NavigationSystem->ProjectPointToNavigation(Candidate, OUT CandidateNavLocation) == false)
			continue;

		/* 지점과 자리 사이에 벽이 있으면 네비메시 경계에 막힌다 (true = 막힘) */
		FVector HitLocation;
		if (UNavigationSystemV1::NavigationRaycast(World, PointNavLocation.Location, CandidateNavLocation.Location, OUT HitLocation) == true)
			continue;

		return CandidateNavLocation.Location;
	}

	/* 반경이 거의 다 벽이면 지점 위 — 겹쳐도 벽 밖보다 낫다 */
	return PointNavLocation.Location;
}
/* 타이머에서 불려 Mass 처리 바깥이다 — SpawnWave를 바로 불러도 된다 */
// [Server]
void UJY_RoomWaveSubsystem::SpawnQueuedEnemies()
{
	/* 같은 종류끼리 모아 한 번에 만든다 — 엔티티 생성은 종류(설계도)마다 한 번 */
	TMap<const UJY_EnemyDefinition*, TArray<FVector>> LocationsByDefinition;
	for (int32 SpawnCount = 0; SpawnCount < JY_RoomWave::MaxSpawnsPerTick && QueuedEnemySpawns.IsEmpty() == false; ++SpawnCount)
	{
		const FJY_QueuedEnemySpawn& QueuedSpawn = QueuedEnemySpawns.First();
		LocationsByDefinition.FindOrAdd(QueuedSpawn.EnemyDefinition).Add(QueuedSpawn.Location);
		QueuedEnemySpawns.PopFirst();
	}

	UWorld* World = GetWorld();
	UJY_MassEnemySubsystem* MassEnemySubsystem = World->GetSubsystem<UJY_MassEnemySubsystem>();
	const float SpawnWarningSeconds = CurrentRoomData->GetSpawnWarningSeconds();
	for (const TPair<const UJY_EnemyDefinition*, TArray<FVector>>& Pair : LocationsByDefinition)
	{
		MassEnemySubsystem->SpawnWave(*Pair.Key, Pair.Value, SpawnWarningSeconds);
	}

	if (QueuedEnemySpawns.IsEmpty() == true)
		return;

	FTimerManager& TimerManager = World->GetTimerManager();
	SpawnTimerHandle = TimerManager.SetTimerForNextTick(this, &ThisClass::SpawnQueuedEnemies);
}
