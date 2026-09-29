#include "Character/JY_Character.h"
#include "Character/JY_HealthComponent.h"
#include "Interaction/JY_InteractionComponent.h"
#include "Player/JY_PlayerState.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "System/JY_LogChannels.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_Character)

AJY_Character::AJY_Character()
{
	/* 매 프레임 할 일이 없음. 이동은 CMC, 입력은 이벤트로 처리 */
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	/* 몸은 컨트롤러 회전을 따르지 않고 이동 방향으로 돈다 */
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	MovementComp->bOrientRotationToMovement = true;
	MovementComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	MovementComp->MaxWalkSpeed = 500.f;
	MovementComp->MinAnalogWalkSpeed = 20.f;
	MovementComp->BrakingDecelerationWalking = 2000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	/* 절대 회전 — 캐릭터가 돌아도 카메라는 고정 (쿼터뷰) */
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->bUsePawnControlRotation = false;

	/* 충돌 테스트 끔 — 벽 뒤로 가도 카메라가 캐릭터 쪽으로 당겨지지 않게 */
	CameraBoom->bDoCollisionTest = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	/* Lyra LyraCharacter.cpp:68~69와 같이 생성자에서 죽음 이벤트 연결 */
	HealthComponent = CreateDefaultSubobject<UJY_HealthComponent>(TEXT("HealthComponent"));
	InteractionComponent = CreateDefaultSubobject<UJY_InteractionComponent>(TEXT("InteractionComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);

	/* 외형 전용 — 판정은 나중에 전투에서 따로 한다 */
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), WeaponSocketName);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	/* 화면 연출용이라 복제 없음 — 각 클라가 자기 화면에서만 그린다. 데디 서버는 렌더링하지 않아 비용 없음 */
	CharacterLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CharacterLight"));
	CharacterLight->SetupAttachment(RootComponent);
	CharacterLight->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	CharacterLight->SetIntensityUnits(ELightUnits::Candelas);
	CharacterLight->SetIntensity(10.0f);
	CharacterLight->SetAttenuationRadius(400.0f);
	CharacterLight->SetLightColor(FLinearColor(1.0f, 0.85f, 0.7f));

	/* 그림자 끔 — 동적 그림자 비용 절감, 캐릭터 자체 그림자와 겹치지 않게 */
	CharacterLight->SetCastShadows(false);
}

/* BP에서 바꾼 카메라 수치를 붐에 적용. 절대 회전이라 SetRelativeRotation = 월드 회전 */
void AJY_Character::OnConstruction(const FTransform& Transform) //override
{
	Super::OnConstruction(Transform);

	CameraBoom->SetRelativeRotation(CameraBoomRotation);
	CameraBoom->TargetArmLength = CameraBoomLength;

	/* BP에서 소켓 이름을 바꿨을 수 있어 다시 붙인다 */
	WeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
}

// [Server]
/* 서버는 빙의 시점에 PlayerState가 확실히 있다 */
void AJY_Character::PossessedBy(AController* NewController) //override
{
	Super::PossessedBy(NewController);

	InitializeAbilitySystem();
}

// [OwnClient] [SimulatedProxy]
/* 클라는 PlayerState가 복제로 늦게 올 수 있어 도착 시점에 연결 */
void AJY_Character::OnRep_PlayerState() //override
{
	Super::OnRep_PlayerState();

	InitializeAbilitySystem();
}

/* 소유자 = PlayerState(ASC 주인), 아바타 = 이 캐릭터. 폰이 바뀌면 아바타만 갈아끼운다 */
void AJY_Character::InitializeAbilitySystem()
{
	AJY_PlayerState* JYPlayerState = GetPlayerState<AJY_PlayerState>();
	if (JYPlayerState == nullptr)
		return;

	UJY_AbilitySystemComponent* JYASC = JYPlayerState->GetJYAbilitySystemComponent();
	check(JYASC);

	JYASC->InitAbilityActorInfo(JYPlayerState, this);
	HealthComponent->InitializeWithAbilitySystem(JYASC);
}

void AJY_Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) //override
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (EnhancedInputComponent == nullptr)
	{
		UE_LOG(LogJY, Error, TEXT("[%s] %s: EnhancedInputComponent 없음"), JY_GetNetRoleTag(this), *GetNameSafe(this));
		return;
	}

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AJY_Character::Move);
	EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AJY_Character::Interact);
}

// [OwnClient] 로컬 입력 → CMC가 예측 이동, 서버가 검증
/* 카메라 붐 Yaw 기준으로 방향을 잡아 W가 항상 화면 위가 되게 한다 */
void AJY_Character::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	const FRotator YawRotation(0.0f, CameraBoom->GetComponentRotation().Yaw, 0.0f);
	const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDir, MovementVector.Y);
	AddMovementInput(RightDir, MovementVector.X);
}

// [OwnClient] 로컬 입력 → 초점 대상 사용 (메뉴가 열려 있으면 게임 입력이 막혀 오지 않음)
void AJY_Character::Interact()
{
	InteractionComponent->Interact();
}

// [Server] [OwnClient] [SimulatedProxy] — HealthComponent가 모두에서 방송
void AJY_Character::OnDeathStarted(AActor* OwningActor)
{
	DisableMovementAndCollision();
}

/* Lyra DisableMovementAndCollision과 같은 목적 — 죽은 뒤 입력·이동·충돌을 막는다 */
void AJY_Character::DisableMovementAndCollision()
{
	if (GetController() != nullptr)
	{
		GetController()->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	MovementComp->StopMovementImmediately();
	MovementComp->DisableMovement();
}
