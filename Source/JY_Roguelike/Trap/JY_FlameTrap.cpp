#include "Trap/JY_FlameTrap.h"
#include "Combat/JY_CombatQuerySubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_FlameTrap)

/*
 * 밟힘은 서버만 판정하고 상태(State)만 복제 — 불·발판 연출은 클라가 OnRep에서 재생한다.
 * 처음부터 휴면(DORM_Initial, 배치 액터 복제 비용 0) — 상태를 바꿀 때만 FlushNetDormancy로 한 번 보낸다 (AJY_BreakableProp과 같음).
 * 주의: 스트리밍으로 들어온 방에선 시작 시 겹침 이벤트가 없다(bGenerateOverlapEventsDuringLevelStreaming 기본 false) — 시작 때 발판 위에 선 폰은 다음 겹침부터 감지.
 */
AJY_FlameTrap::AJY_FlameTrap()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	NetDormancy = DORM_Initial;

	Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	SetRootComponent(Frame);
	Frame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Frame->SetCanEverAffectNavigation(false);

	Plate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plate"));
	Plate->SetupAttachment(Frame);
	Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plate->SetCanEverAffectNavigation(false);

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Frame);
	Trigger->SetBoxExtent(FVector(50.0f, 55.0f, 40.0f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));

	ArmingEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ArmingEffect"));
	ArmingEffect->SetupAttachment(Frame);
	ArmingEffect->bAutoActivate = false;

	FlameEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlameEffect"));
	FlameEffect->SetupAttachment(Frame);
	FlameEffect->bAutoActivate = false;
}

// [Server] [SimulatedProxy]
void AJY_FlameTrap::BeginPlay() //override
{
	Super::BeginPlay();

	if (HasAuthority() == false)
		return;

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleTriggerBeginOverlap);
}

void AJY_FlameTrap::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AJY_FlameTrap, State);
}

// [Server]
void AJY_FlameTrap::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (State != EJY_FlameTrapState::Idle)
		return;

	if (Cast<APawn>(OtherActor) == nullptr)
		return;

	SetState(EJY_FlameTrapState::Arming);
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &ThisClass::StartBurning, ArmSeconds, false);
}

// [Server]
void AJY_FlameTrap::StartBurning()
{
	SetState(EJY_FlameTrapState::Burning);
	ApplyFlameDamage();
	GetWorldTimerManager().SetTimer(DamageTimerHandle, this, &ThisClass::ApplyFlameDamage, DamageIntervalSeconds, true);
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &ThisClass::StartCooldown, BurnSeconds, false);
}

// [Server]
void AJY_FlameTrap::ApplyFlameDamage()
{
	const UJY_CombatQuerySubsystem* CombatQuery = GetWorld()->GetSubsystem<UJY_CombatQuerySubsystem>();
	const FVector Center = GetActorLocation() + FVector(0.0f, 0.0f, DamageRadius * 0.5f);
	CombatQuery->ApplyAreaDamage(Center, DamageRadius, Damage, nullptr, this);
}

// [Server]
void AJY_FlameTrap::StartCooldown()
{
	GetWorldTimerManager().ClearTimer(DamageTimerHandle);
	SetState(EJY_FlameTrapState::Cooldown);
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &ThisClass::FinishCooldown, CooldownSeconds, false);
}

// [Server]
void AJY_FlameTrap::FinishCooldown()
{
	SetState(EJY_FlameTrapState::Idle);
	RetriggerIfOccupied();
}

// [Server]
void AJY_FlameTrap::RetriggerIfOccupied()
{
	TArray<AActor*> OverlappingPawns;
	Trigger->GetOverlappingActors(OUT OverlappingPawns, APawn::StaticClass());
	if (OverlappingPawns.Num() == 0)
		return;

	SetState(EJY_FlameTrapState::Arming);
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &ThisClass::StartBurning, ArmSeconds, false);
}

// [Server]
void AJY_FlameTrap::SetState(EJY_FlameTrapState NewState)
{
	/* 휴면 액터는 깨워야 바뀐 값이 나간다 */
	FlushNetDormancy();
	State = NewState;

	/* 서버는 OnRep이 안 불리므로 직접 */
	HandleStateChanged();
}

// [SimulatedProxy]
void AJY_FlameTrap::OnRep_State()
{
	HandleStateChanged();
}

// [Server] [SimulatedProxy]
void AJY_FlameTrap::HandleStateChanged()
{
	if (IsNetMode(NM_DedicatedServer) == true)
		return;

	const bool bPressed = State == EJY_FlameTrapState::Arming || State == EJY_FlameTrapState::Burning;
	const float PlateHeight = bPressed == true ? PressedPlateHeight : 0.0f;
	Plate->SetRelativeLocation(FVector(0.0f, 0.0f, PlateHeight));

	if (State == EJY_FlameTrapState::Arming)
	{
		ArmingEffect->Activate(true);
	}
	else
	{
		ArmingEffect->Deactivate();
	}

	if (State == EJY_FlameTrapState::Burning)
	{
		FlameEffect->Activate(true);
	}
	else
	{
		FlameEffect->Deactivate();
	}
}
