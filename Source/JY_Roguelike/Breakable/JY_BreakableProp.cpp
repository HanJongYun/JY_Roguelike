#include "Breakable/JY_BreakableProp.h"
#include "Combat/JY_CombatQuerySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_BreakableProp)

/*
 * 서버는 상태 하나(State)만 판정·복제하고, 파편·폭발 연출은 각 클라가 OnRep에서 재생한다 (GameDesign_Final.md §10-3 혼합 방식).
 * 방마다 6~10개 배치되는 물체라 처음부터 휴면(DORM_Initial, 배치 액터 복제 비용 0) — 상태를 바꿀 때만 FlushNetDormancy로 한 번 보낸다.
 */
AJY_BreakableProp::AJY_BreakableProp()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	NetDormancy = DORM_Initial;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	/* 전투 판정(UJY_CombatQuerySubsystem::FindTargets)이 WorldDynamic 물체 중에서 찾는다 */
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);

	/* 작은 물체라 네비에 구멍을 내지 않음 — 부서진 뒤 네비를 다시 굽지 않아도 된다 */
	Mesh->SetCanEverAffectNavigation(false);

	IdleEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("IdleEffect"));
	IdleEffect->SetupAttachment(Mesh);
}

void AJY_BreakableProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AJY_BreakableProp, State);
}

// [Server]
void AJY_BreakableProp::BreakBy(AActor* InstigatorActor)
{
	if (HasAuthority() == false)
		return;

	if (State != EJY_BreakableState::Intact)
		return;

	BreakInstigator = InstigatorActor;

	if (BreakEffect == EJY_BreakEffect::Explosion)
	{
		/* 맞자마자 터지지 않고 잠깐 뒤 — 플레이어가 피하거나 적을 끌어들일 틈 */
		SetState(EJY_BreakableState::Fusing);
		GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &ThisClass::Detonate, FuseSeconds, false);
		return;
	}

	SetState(EJY_BreakableState::Broken);

	if (BreakEffect == EJY_BreakEffect::Poison)
	{
		GetWorldTimerManager().SetTimer(PoisonTickTimerHandle, this, &ThisClass::ApplyPoisonTick, PoisonTickSeconds, true);
		GetWorldTimerManager().SetTimer(PoisonEndTimerHandle, this, &ThisClass::StopPoison, PoisonDurationSeconds, false);
	}
}

// [Server]
void AJY_BreakableProp::SetState(EJY_BreakableState NewState)
{
	/* 휴면 액터는 깨워야 바뀐 값이 나간다 */
	FlushNetDormancy();
	State = NewState;

	/* 서버는 OnRep이 안 불리므로 직접 — 충돌 끄기는 서버 판정에도 필요 */
	HandleStateChanged();
}

// [SimulatedProxy]
void AJY_BreakableProp::OnRep_State()
{
	HandleStateChanged();
}

// [Server] [SimulatedProxy]
void AJY_BreakableProp::HandleStateChanged()
{
	const bool bPlayEffects = IsNetMode(NM_DedicatedServer) == false;
	const FVector Location = GetActorLocation();

	if (State == EJY_BreakableState::Fusing)
	{
		if (bPlayEffects == true)
		{
			FuseEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(FuseFX, Mesh, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
		}
		return;
	}

	if (State != EJY_BreakableState::Broken)
		return;

	Mesh->SetVisibility(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	IdleEffect->Deactivate();

	if (bPlayEffects == false)
		return;

	/* 점화 연출은 시스템이 컬링되면 안 생김 */
	if (FuseEffect != nullptr)
	{
		FuseEffect->DestroyComponent();
		FuseEffect = nullptr;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BreakFX, Location);

	if (BreakEffect == EJY_BreakEffect::Explosion)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, AreaFX, Location);
	}
	else if (BreakEffect == EJY_BreakEffect::Poison)
	{
		/* 독구름은 반복 시스템이라 지속 시간이 지나면 직접 끈다 (서버 피해 타이머와 같은 길이) */
		PoisonCloudEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, AreaFX, Location);
		GetWorldTimerManager().SetTimer(PoisonCloudTimerHandle, this, &ThisClass::StopPoisonCloudEffect, PoisonDurationSeconds, false);
	}
}

// [Server]
void AJY_BreakableProp::Detonate()
{
	SetState(EJY_BreakableState::Broken);

	UWorld* World = GetWorld();
	const FVector Center = GetActorLocation();
	AActor* InstigatorActor = BreakInstigator.Get();
	const UJY_CombatQuerySubsystem* CombatQuery = World->GetSubsystem<UJY_CombatQuerySubsystem>();
	CombatQuery->ApplyAreaDamage(Center, EffectRadius, EffectDamage, InstigatorActor, this);

	/* 범위 안 다른 부서지는 물체도 부숨 — 폭발 통끼리는 점화 시간만큼 차례로 터진다 */
	const FCollisionObjectQueryParams ObjectQueryParams(ECC_WorldDynamic);
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JY_BreakableChain), false, this);
	const FCollisionShape QuerySphere = FCollisionShape::MakeSphere(EffectRadius);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(OUT Overlaps, Center, FQuat::Identity, ObjectQueryParams, QuerySphere, QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AJY_BreakableProp* OtherProp = Cast<AJY_BreakableProp>(Overlap.GetActor());
		if (OtherProp == nullptr)
			continue;

		OtherProp->BreakBy(InstigatorActor);
	}
}

// [Server]
void AJY_BreakableProp::ApplyPoisonTick()
{
	const UJY_CombatQuerySubsystem* CombatQuery = GetWorld()->GetSubsystem<UJY_CombatQuerySubsystem>();
	AActor* InstigatorActor = BreakInstigator.Get();
	CombatQuery->ApplyAreaDamage(GetActorLocation(), EffectRadius, EffectDamage, InstigatorActor, this);
}

// [Server]
void AJY_BreakableProp::StopPoison()
{
	GetWorldTimerManager().ClearTimer(PoisonTickTimerHandle);
}

// [Server] [SimulatedProxy]
void AJY_BreakableProp::StopPoisonCloudEffect()
{
	/* 시스템이 컬링되면 안 생김 */
	if (PoisonCloudEffect == nullptr)
		return;

	PoisonCloudEffect->Deactivate();
	PoisonCloudEffect = nullptr;
}
