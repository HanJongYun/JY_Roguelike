#include "Room/JY_RoomMarker.h"
#include "Room/JY_RoomFlowSubsystem.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_RoomMarker)

/* 엔진 ATargetPoint와 같은 구성(TargetPoint.cpp:12) — ATargetPoint는 MinimalAPI라 다른 모듈에서 상속하지 않음 */
AJY_RoomMarker::AJY_RoomMarker()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));

	/* 커맨들릿(쿠킹 등)은 에디터 아이콘 에셋을 못 찾을 수 있어 건너뜀 — 엔진 TargetPoint.cpp:23과 같음 */
	if (IsRunningCommandlet() == false)
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> TargetIcon(TEXT("/Engine/EditorResources/S_TargetPoint"));

		if (SpriteComponent != nullptr)
		{
			SpriteComponent->Sprite = TargetIcon.Get();
			SpriteComponent->bIsScreenSizeScaled = true;
			SpriteComponent->SetupAttachment(RootComponent);
		}

		if (ArrowComponent != nullptr)
		{
			ArrowComponent->ArrowColor = FColor(150, 200, 255);
			ArrowComponent->bTreatAsASprite = true;
			ArrowComponent->bIsScreenSizeScaled = true;
			ArrowComponent->SetupAttachment(RootComponent);
		}
	}
#endif

	SetHidden(true);
	SetCanBeDamaged(false);
}

/*
 * 문은 레벨에 놓인 액터라 처음엔 잠긴 상태로 서버·클라 양쪽에 같이 로드된다.
 * 열림 상태만 복제 — 처음부터 휴면(DORM_Initial), 바꿀 때만 FlushNetDormancy (AJY_BreakableProp과 같음).
 */
AJY_RoomExitDoor::AJY_RoomExitDoor()
{
	bReplicates = true;
	NetDormancy = DORM_Initial;

	/* 문 위 (SM_DoorWay 높이 600 안쪽) */
	RewardIconAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("RewardIconAnchor"));
	RewardIconAnchor->SetupAttachment(RootComponent);
	RewardIconAnchor->SetRelativeLocation(FVector(0.0f, 0.0f, 450.0f));

	/* 문틀 안쪽(폭 약 500·높이 400)을 덮음 */
	PassTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("PassTrigger"));
	PassTrigger->SetupAttachment(RootComponent);
	PassTrigger->SetBoxExtent(FVector(80.0f, 250.0f, 200.0f));
	PassTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f));
	PassTrigger->SetCollisionProfileName(TEXT("Trigger"));
	PassTrigger->SetCanEverAffectNavigation(false);

	/* 통과 감지보다 얇게 문틀 가운데를 막음 — 높이는 문틀(600)까지 */
	Barrier = CreateDefaultSubobject<UBoxComponent>(TEXT("Barrier"));
	Barrier->SetupAttachment(RootComponent);
	Barrier->SetBoxExtent(FVector(20.0f, 260.0f, 300.0f));
	Barrier->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));
	Barrier->SetCollisionProfileName(TEXT("InvisibleWall"));
	Barrier->SetCanEverAffectNavigation(false);

	/* 막는 건 Barrier가 한다 — 창살은 보이기만 */
	GateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh"));
	GateMesh->SetupAttachment(RootComponent);
	GateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GateMesh->SetCanEverAffectNavigation(false);

	OpenLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("OpenLight"));
	OpenLight->SetupAttachment(RootComponent);
	OpenLight->SetRelativeLocation(FVector(0.0f, 0.0f, 250.0f));
	OpenLight->SetCastShadows(false);
	OpenLight->SetVisibility(false);

	/* 다른 마커와 달리 창살·불빛이 게임에서 보여야 한다 */
	SetHidden(false);
}

// [Server] [SimulatedProxy]
void AJY_RoomExitDoor::BeginPlay() //override
{
	Super::BeginPlay();

	ApplyOpenState();

	if (HasAuthority() == false)
		return;

	PassTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandlePassBeginOverlap);
}

// [Server]
void AJY_RoomExitDoor::HandlePassBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	/* 적·소환물이 문에 닿아도 넘어가지 않게 */
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn == nullptr || Pawn->IsPlayerControlled() == false)
		return;

	UJY_RoomFlowSubsystem* RoomFlow = GetWorld()->GetSubsystem<UJY_RoomFlowSubsystem>();
	RoomFlow->RequestNextRoom();
}

void AJY_RoomExitDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AJY_RoomExitDoor, bOpen);
}

// [Server]
void AJY_RoomExitDoor::SetOpen(bool bNewOpen)
{
	if (bOpen == bNewOpen)
		return;

	/* 휴면 액터는 깨워야 바뀐 값이 나간다 */
	FlushNetDormancy();
	bOpen = bNewOpen;

	/* 서버는 OnRep이 안 불리므로 직접 */
	ApplyOpenState();
}

// [SimulatedProxy]
void AJY_RoomExitDoor::OnRep_Open()
{
	ApplyOpenState();
}

/*
 * 통과 감지는 열렸을 때만 켠다 — 잠긴 문에 붙어 서 있다가 열리면, 켜는 순간 겹침을 다시 계산해 이미 안에 있는 캐릭터도 "들어옴"으로 잡힌다
 * (SetCollisionEnabled → OnComponentCollisionSettingsChanged → UpdateOverlaps, PrimitiveComponentPhysics.cpp:1385·1446).
 * 계속 켜 두면 잠겼을 때 받은 "들어옴"이 무시된 뒤 다시 오지 않아, 열린 뒤 그대로 문 밖 허공으로 걸어 나간다.
 */
// [Server] [SimulatedProxy]
void AJY_RoomExitDoor::ApplyOpenState()
{
	const ECollisionEnabled::Type PassCollision = bOpen == true ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision;
	PassTrigger->SetCollisionEnabled(PassCollision);

	const ECollisionEnabled::Type BarrierCollision = bOpen == true ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics;
	Barrier->SetCollisionEnabled(BarrierCollision);
	GateMesh->SetVisibility(bOpen == false);
	OpenLight->SetVisibility(bOpen == true);
}
