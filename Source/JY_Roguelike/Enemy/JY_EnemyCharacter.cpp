#include "Enemy/JY_EnemyCharacter.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/JY_HealthSet.h"
#include "Character/JY_HealthComponent.h"
#include "Combat/JY_CombatTypes.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_EnemyCharacter)

AJY_EnemyCharacter::AJY_EnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/* 매 프레임 할 일이 없음. 이동은 CMC, 피격·죽음은 이벤트 */
	PrimaryActorTick.bCanEverTick = false;

	/* 컨트롤러가 없으면 서버 CMC가 물리(밀려남·중력)를 안 돈다 (CharacterMovementComponent.cpp:5662) */
	/* 기본값 PlacedInWorld는 다시 생성한 적에 빙의하지 않는다 */
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();

	AbilitySystemComponent = CreateDefaultSubobject<UJY_AbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	/* 적 GE는 남에게 보일 필요가 없다 — 태그·큐·속성만 복제 */
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	/* ASC가 InitializeComponent에서 같은 주인의 속성 세트를 찾아 등록한다 */
	HealthSet = CreateDefaultSubobject<UJY_HealthSet>(TEXT("HealthSet"));

	HealthComponent = CreateDefaultSubobject<UJY_HealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
}

// [Server] [SimulatedProxy]
void AJY_EnemyCharacter::BeginPlay() //override
{
	Super::BeginPlay();

	SpawnTransform = GetActorTransform();

	/* 주인 = 아바타 = 자기 */
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	/* 클라 전용 연출(큐)에 루트모션이 있으면 시뮬 프록시만 움직여 서버 위치와 어긋난다 */
	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (ensureMsgf(AnimInstance != nullptr, TEXT("%s 메시에 애님 블루프린트가 없음"), *GetNameSafe(this)) == true)
	{
		AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}

	HealthComponent->InitializeWithAbilitySystem(AbilitySystemComponent);

	if (HasAuthority() == true)
	{
		/* 최대 체력부터 — 체력은 최대 체력으로 잘린다(UJY_HealthSet::ClampAttribute) */
		AbilitySystemComponent->SetNumericAttributeBase(UJY_HealthSet::GetMaxHealthAttribute(), StartingHealth);
		AbilitySystemComponent->SetNumericAttributeBase(UJY_HealthSet::GetHealthAttribute(), StartingHealth);
	}
}

UAbilitySystemComponent* AJY_EnemyCharacter::GetAbilitySystemComponent() const //override
{
	return AbilitySystemComponent;
}

FGenericTeamId AJY_EnemyCharacter::GetGenericTeamId() const //override
{
	return FGenericTeamId(static_cast<uint8>(EJY_Team::Enemy));
}

// [Server]
/* 수평 LaunchCharacter는 바로 착지해 마찰로 0.1초 안에 멈춘다 (CharacterMovementComponent.cpp:1237) */
/* 루트모션 소스는 정한 시간 동안 속도를 덮어써 마찰과 무관하게 거리가 나오고, 시뮬 프록시에도 복제된다 */
void AJY_EnemyCharacter::ReceiveKnockback(const FVector2f& Impulse)
{
	const float KnockbackScale = 1.0f - KnockbackResistance;
	const FVector KnockbackVelocity(Impulse.X * KnockbackScale, Impulse.Y * KnockbackScale, 0.0f);

	/* 저항 1(안 밀림) */
	if (KnockbackVelocity.IsNearlyZero() == true)
		return;

	/* 연타로 다시 밀리면 이전 밀림을 새 방향으로 바꾼다 */
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->RemoveRootMotionSourceByID(KnockbackRootMotionSourceID);

	TSharedPtr<FRootMotionSource_ConstantForce> KnockbackForce = MakeShared<FRootMotionSource_ConstantForce>();
	KnockbackForce->InstanceName = TEXT("Knockback");
	KnockbackForce->AccumulateMode = ERootMotionAccumulateMode::Override;
	KnockbackForce->Priority = 5;
	KnockbackForce->Force = KnockbackVelocity;
	KnockbackForce->Duration = KnockbackDuration;

	/* 수직은 덮지 않는다 — 낭떠러지에선 중력대로 떨어진다 */
	KnockbackForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);

	KnockbackRootMotionSourceID = MovementComponent->ApplyRootMotionSource(KnockbackForce);
}

// [OwnClient] [SimulatedProxy]
void AJY_EnemyCharacter::PlayHitReact(const FVector& HitFromLocation, float Damage)
{
	/* 마지막 타의 피격 큐가 죽음 애니를 덮지 않게 */
	if (HealthComponent->IsDeadOrDying() == true)
		return;

	const FVector ToAttacker = HitFromLocation - GetActorLocation();
	const FVector AttackerDirection = ToAttacker.GetSafeNormal2D();
	const FVector ForwardVector = GetActorForwardVector();
	const FVector RightVector = GetActorRightVector();
	const float ForwardDot = FVector::DotProduct(ForwardVector, AttackerDirection);
	const float RightDot = FVector::DotProduct(RightVector, AttackerDirection);

	UAnimMontage* HitReactMontage = nullptr;
	if (Damage >= LargeHitDamage)
	{
		HitReactMontage = HitReactMontages.Large;
	}
	else if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
	{
		HitReactMontage = ForwardDot >= 0.0f ? HitReactMontages.Front : HitReactMontages.Back;
	}
	else
	{
		HitReactMontage = RightDot >= 0.0f ? HitReactMontages.Right : HitReactMontages.Left;
	}

	/* 반응을 비워 둔 적(보스의 Large 등)은 반응 없이 버틴다 */
	if (HitReactMontage == nullptr)
		return;

	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	AnimInstance->Montage_Play(HitReactMontage);
}

// [Server] [SimulatedProxy] — HealthComponent가 서버·클라 모두에서 방송
void AJY_EnemyCharacter::OnDeathStarted(AActor* OwningActor)
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->StopMovementImmediately();
	MovementComponent->DisableMovement();

	if (DeathMontages.Num() > 0)
	{
		const int32 DeathIndex = FMath::RandRange(0, DeathMontages.Num() - 1);
		USkeletalMeshComponent* MeshComponent = GetMesh();
		UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
		AnimInstance->Montage_Play(DeathMontages[DeathIndex]);
	}

	if (HasAuthority() == false)
		return;

	FTimerManager& TimerManager = GetWorldTimerManager();
	FTimerHandle FinishDeathTimerHandle;
	TimerManager.SetTimer(FinishDeathTimerHandle, this, &ThisClass::FinishDeath, DeathLingerSeconds, false);
}

// [Server]
/* 다시 생긴 적은 자기 BeginPlay에서 SpawnTransform을 다시 저장 — 처음 자리가 이어진다 */
void AJY_EnemyCharacter::FinishDeath()
{
	if (DeathBehavior == EJY_EnemyDeathBehavior::Respawn)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		UWorld* World = GetWorld();
		UClass* EnemyClass = GetClass();
		World->SpawnActor<AJY_EnemyCharacter>(EnemyClass, SpawnTransform, SpawnParameters);
	}

	Destroy();
}
