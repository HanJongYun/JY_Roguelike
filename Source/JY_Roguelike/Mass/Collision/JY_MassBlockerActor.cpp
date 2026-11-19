#include "Mass/Collision/JY_MassBlockerActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MassBlockerActor)

namespace JY_MassBlocker
{
	/* 탑다운이라 높이는 막는 데 의미가 없다 — 점프로 넘지 못하게 넉넉히 */
	constexpr float BlockerHalfHeight = 200.0f;
}

AJY_MassBlockerActor::AJY_MassBlockerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AJY_MassBlockerActor::BeginPlacement()
{
	PlacedCount = 0;
}

/* 순간 이동으로 옮긴다 — 쓸면서 옮기면 플레이어를 밀 수 있다 */
// [Server] [OwnClient]
void AJY_MassBlockerActor::PlaceBlocker(const FVector& Location, float Radius)
{
	if (Blockers.IsValidIndex(PlacedCount) == false)
	{
		UCapsuleComponent* NewBlocker = CreateBlocker();
		Blockers.Add(NewBlocker);
	}

	UCapsuleComponent* Blocker = Blockers[PlacedCount];
	if (FMath::IsNearlyEqual(Blocker->GetUnscaledCapsuleRadius(), Radius) == false)
	{
		Blocker->SetCapsuleSize(Radius, JY_MassBlocker::BlockerHalfHeight);
	}
	Blocker->SetWorldLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);

	/* 앞에서부터 EnabledCount개는 이미 켜져 있다 */
	if (PlacedCount >= EnabledCount)
	{
		Blocker->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	++PlacedCount;
}

void AJY_MassBlockerActor::EndPlacement()
{
	for (int32 Index = PlacedCount; Index < EnabledCount; ++Index)
	{
		Blockers[Index]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	EnabledCount = PlacedCount;
}

void AJY_MassBlockerActor::DisableAllBlockers()
{
	BeginPlacement();
	EndPlacement();
}

/* 엔진 Character 이동은 쿼리 충돌만 본다 — 모든 채널을 무시하고 Pawn만 막아, 이 채널을 막도록 켠 플레이어 캡슐과만 서로 막힌다 */
UCapsuleComponent* AJY_MassBlockerActor::CreateBlocker()
{
	UCapsuleComponent* Blocker = NewObject<UCapsuleComponent>(this);
	Blocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Blocker->SetCollisionObjectType(JY_CollisionChannel::MassBlocker);
	Blocker->SetCollisionResponseToAllChannels(ECR_Ignore);
	Blocker->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Blocker->SetGenerateOverlapEvents(false);
	Blocker->SetCanEverAffectNavigation(false);

	/* 낮은 턱처럼 밟고 올라서지 못하게 (CharacterMovementComponent.cpp:689) */
	Blocker->CanCharacterStepUpOn = ECB_No;

	Blocker->SetCapsuleSize(1.0f, JY_MassBlocker::BlockerHalfHeight);
	Blocker->SetupAttachment(RootComponent);
	Blocker->RegisterComponent();
	return Blocker;
}
