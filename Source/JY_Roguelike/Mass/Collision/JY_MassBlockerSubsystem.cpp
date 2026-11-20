#include "Mass/Collision/JY_MassBlockerSubsystem.h"

#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/JY_CombatTypes.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Mass/Collision/JY_MassBlockerActor.h"
#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "MassCommonFragments.h"
#include "MassEntitySubsystem.h"
#include "MassSimulationSubsystem.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "System/JY_GameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassBlockerSubsystem)

namespace JY_MassBlocker
{
	/* 이만큼보다 깊이 파묻혀야 겹친 것으로 본다 (cm) — 캡슐에 닿기만 한 것을 겹침으로 보면 닿는 순간 캡슐이 꺼져 걸어서 뚫린다 */
	constexpr float OverlapTolerance = 0.5f;
}

// [Server] [OwnClient]
void UJY_MassBlockerSubsystem::OnWorldBeginPlay(UWorld& InWorld) //override
{
	Super::OnWorldBeginPlay(InWorld);

	BlockerActor = InWorld.SpawnActor<AJY_MassBlockerActor>();

	/* 끝 알림은 그 단계의 Processor가 다 돈 뒤라 그리드가 이번 프레임 위치다 (MassProcessingPhaseManager.cpp:171) */
	UMassSimulationSubsystem* MassSimulationSubsystem = InWorld.GetSubsystem<UMassSimulationSubsystem>();
	FMassProcessingPhase::FOnPhaseEvent& PrePhysicsFinishedEvent = MassSimulationSubsystem->GetOnProcessingPhaseFinished(EMassProcessingPhase::PrePhysics);
	PrePhysicsFinishedEvent.AddUObject(this, &ThisClass::HandlePrePhysicsFinished);
}

bool UJY_MassBlockerSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const //override
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

/*
 * 꺼진 상태를 기억했다 푸는 것이 아니라 매번 처음부터 다시 정한다 — 태그가 떨어지거나 착지하면 다음 프레임에 안 겹친 적만 저절로 켜진다.
 * 한꺼번에 켜지 않는 이유: 플레이어 몸 안에서 켜진 캡슐은 엔진 이동이 플레이어를 밀어내게 만든다
 */
// [Server] [OwnClient]
void UJY_MassBlockerSubsystem::HandlePrePhysicsFinished(float DeltaSeconds)
{
	/* 데디 서버의 PC는 접속한 플레이어 것뿐, 클라는 자기 것 — 플레이어는 항상 한 명 */
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* ControlledPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
	if (ControlledPawn == nullptr)
	{
		BlockerActor->DisableAllBlockers();
		return;
	}

	BindPlayer(ControlledPawn);

	/* 땅에서 걸을 때만 막는다 — 충돌 무시 태그(대시·스킬)가 붙어 있거나 공중(점프·내려찍기)이면 적을 통과한다 */
	const UJY_AbilitySystemComponent* PlayerAbilitySystemComponent = AbilitySystemComponent.Get();
	const bool bIgnoreCollisionTagged = PlayerAbilitySystemComponent != nullptr && PlayerAbilitySystemComponent->HasMatchingGameplayTag(JYGameplayTags::Gameplay_IgnoreEnemyCollision) == true;
	const UPawnMovementComponent* PlayerMovementComponent = ControlledPawn->GetMovementComponent();
	const bool bAirborne = PlayerMovementComponent != nullptr && PlayerMovementComponent->IsFalling() == true;
	if (bIgnoreCollisionTagged == true || bAirborne == true)
	{
		BlockerActor->DisableAllBlockers();
		return;
	}

	/* 공격 판정과 같은 길로 주변 적만 꺼낸다 — 예고 중인 적은 여기서 빠진다 */
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	FJY_HitShape SearchShape;
	SearchShape.Type = EJY_HitShapeType::Sphere;
	SearchShape.Radius = GameData->EnemyBlockerSearchRadius;

	const FVector PlayerLocation = ControlledPawn->GetActorLocation();
	const UJY_MassEnemySubsystem* MassEnemySubsystem = GetWorld()->GetSubsystem<UJY_MassEnemySubsystem>();
	TArray<FJY_CombatTarget> NearbyEnemies;
	MassEnemySubsystem->FindEnemiesInShape(SearchShape, PlayerLocation, FVector::ForwardVector, OUT NearbyEnemies);

	const UMassEntitySubsystem* MassEntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	const FMassEntityManager& EntityManager = MassEntitySubsystem->GetEntityManager();
	const float PlayerRadius = ControlledPawn->GetSimpleCollisionRadius();

	BlockerActor->BeginPlacement();
	for (const FJY_CombatTarget& NearbyEnemy : NearbyEnemies)
	{
		/* 날아가는 적은 길을 막지 않는다 */
		if (MassEnemySubsystem->GetEnemyState(NearbyEnemy.Entity) == EJY_EnemyState::Knockback)
			continue;

		const FAgentRadiusFragment& RadiusFragment = EntityManager.GetFragmentDataChecked<FAgentRadiusFragment>(NearbyEnemy.Entity);

		/* 캡슐은 절대 플레이어를 밀지 않는다 — 겹친 적은 서버가 밀어낸 뒤에 놓인다 (UJY_MassPlayerPushProcessor) */
		const float HorizontalDistance = FVector::Dist2D(PlayerLocation, NearbyEnemy.Location);
		if (HorizontalDistance < PlayerRadius + RadiusFragment.Radius - JY_MassBlocker::OverlapTolerance)
			continue;

		BlockerActor->PlaceBlocker(NearbyEnemy.Location, RadiusFragment.Radius);
	}
	BlockerActor->EndPlacement();
}

/* ASC는 PlayerState에 있어 Pawn보다 늦게 올 수 있다 — 둘 다 잡힐 때까지 매 프레임 다시 본다 */
// [Server] [OwnClient]
void UJY_MassBlockerSubsystem::BindPlayer(APawn* InPlayerPawn)
{
	if (PlayerPawn.Get() == InPlayerPawn && AbilitySystemComponent.IsValid() == true)
		return;

	UJY_AbilitySystemComponent* OldAbilitySystemComponent = AbilitySystemComponent.Get();
	if (OldAbilitySystemComponent != nullptr)
	{
		FOnGameplayEffectTagCountChanged& OldTagEvent = OldAbilitySystemComponent->RegisterGameplayTagEvent(JYGameplayTags::Gameplay_IgnoreEnemyCollision, EGameplayTagEventType::NewOrRemoved);
		OldTagEvent.Remove(IgnoreCollisionTagHandle);
	}
	AbilitySystemComponent = nullptr;
	PlayerPawn = InPlayerPawn;

	/* 길막이 채널의 기본 응답은 무시 — 플레이어 캡슐만 막게 켠다 */
	UPrimitiveComponent* PlayerCollision = Cast<UPrimitiveComponent>(InPlayerPawn->GetRootComponent());
	if (PlayerCollision != nullptr)
	{
		PlayerCollision->SetCollisionResponseToChannel(JY_CollisionChannel::MassBlocker, ECR_Block);
	}

	UAbilitySystemComponent* FoundAbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InPlayerPawn);
	UJY_AbilitySystemComponent* PlayerAbilitySystemComponent = Cast<UJY_AbilitySystemComponent>(FoundAbilitySystemComponent);
	if (PlayerAbilitySystemComponent == nullptr)
		return;

	AbilitySystemComponent = PlayerAbilitySystemComponent;
	FOnGameplayEffectTagCountChanged& TagEvent = PlayerAbilitySystemComponent->RegisterGameplayTagEvent(JYGameplayTags::Gameplay_IgnoreEnemyCollision, EGameplayTagEventType::NewOrRemoved);
	IgnoreCollisionTagHandle = TagEvent.AddUObject(this, &ThisClass::HandleIgnoreCollisionTagChanged);
}

/*
 * 매 프레임 검사만으로는 태그가 붙은 프레임의 이동이 막힐 수 있다 — Mass 처리와 플레이어 이동 사이에 순서 보장이 없고(MassProcessingPhaseManager.cpp:485),
 * 서버는 이동을 패킷이 온 자리에서 실행한다 (CharacterMovementComponent.cpp:9967)
 */
// [Server] [OwnClient]
void UJY_MassBlockerSubsystem::HandleIgnoreCollisionTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
		return;

	BlockerActor->DisableAllBlockers();
}
