#include "Interaction/JY_InteractableActor.h"
#include "Interaction/JY_InteractionComponent.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_InteractableActor)

/*
 * 쿼터뷰라 라인트레이스(Lyra 1인칭 방식) 대신 범위 겹침으로 대상을 잡는다.
 * 레벨에 배치된 액터라 복제하지 않아도 서버·클라가 같은 이름으로 가리킬 수 있다 → Server RPC 인자로 그대로 넘긴다.
 * 안내는 하데스처럼 화면 아래 가운데 HUD가 보여 준다 — 액터는 이름만 제공 (월드 위젯 없음 → 틱·서버 로드 비용 없음).
 */
AJY_InteractableActor::AJY_InteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
	InteractionRange->SetupAttachment(Mesh);
	InteractionRange->SetSphereRadius(250.0f);
	InteractionRange->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

/* 화면형이면 클라에서 그 화면을 미리 비동기 로드 — 첫 E 때 끊기지 않게. 데디 서버는 UI를 올리지 않는다 */
// [Server] [OwnClient]
void AJY_InteractableActor::BeginPlay() //override
{
	Super::BeginPlay();

	InteractionRange->OnComponentBeginOverlap.AddDynamic(this, &AJY_InteractableActor::HandleRangeBeginOverlap);
	InteractionRange->OnComponentEndOverlap.AddDynamic(this, &AJY_InteractableActor::HandleRangeEndOverlap);

	if (OpensScreen() == true && IsNetMode(NM_DedicatedServer) == false)
	{
		ScreenPreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ScreenClass.ToSoftObjectPath());
	}
}

// [Server]
bool AJY_InteractableActor::IsWithinInteractRange(const AActor* User) const
{
	if (User == nullptr)
		return false;

	const float MaxDistance = InteractionRange->GetScaledSphereRadius() + ServerRangeTolerance;
	return FVector::DistSquared(User->GetActorLocation(), InteractionRange->GetComponentLocation()) <= FMath::Square(MaxDistance);
}

/* 겹침은 서버·클라 모두에서 일어난다 — 내가 조종하는 폰일 때만 후보에 넣는다 (탐지·안내는 로컬 일) */
// [Server] [OwnClient]
void AJY_InteractableActor::HandleRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn == nullptr || Pawn->IsLocallyControlled() == false)
		return;

	UJY_InteractionComponent* InteractionComponent = Pawn->FindComponentByClass<UJY_InteractionComponent>();
	if (InteractionComponent == nullptr)
		return;

	InteractionComponent->AddCandidate(this);
}

/* 나갈 때는 조종 여부를 묻지 않는다 — 폰이 죽거나 빙의가 풀린 뒤 나가도 안내가 남지 않게 (후보에 없으면 아무 일 없음) */
// [Server] [OwnClient]
void AJY_InteractableActor::HandleRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UJY_InteractionComponent* InteractionComponent = OtherActor != nullptr ? OtherActor->FindComponentByClass<UJY_InteractionComponent>() : nullptr;
	if (InteractionComponent == nullptr)
		return;

	InteractionComponent->RemoveCandidate(this);
}
