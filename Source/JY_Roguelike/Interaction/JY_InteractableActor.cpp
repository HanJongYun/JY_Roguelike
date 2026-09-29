#include "Interaction/JY_InteractableActor.h"
#include "Interaction/JY_InteractionComponent.h"
#include "UI/Common/JY_InteractPrompt.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_InteractableActor)

/*
 * 쿼터뷰라 라인트레이스(Lyra 1인칭 방식) 대신 범위 겹침으로 대상을 잡는다.
 * 레벨에 배치된 액터라 복제하지 않아도 서버·클라가 같은 이름으로 가리킬 수 있다 → Server RPC 인자로 그대로 넘긴다.
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

	PromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptWidget"));
	PromptWidget->SetupAttachment(Mesh);
	PromptWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	PromptWidget->SetWidgetSpace(EWidgetSpace::Screen);
	PromptWidget->SetDrawAtDesiredSize(true);
	PromptWidget->SetVisibility(false);
}

// [Server] [OwnClient]
void AJY_InteractableActor::BeginPlay() //override
{
	Super::BeginPlay();

	InteractionRange->OnComponentBeginOverlap.AddDynamic(this, &AJY_InteractableActor::HandleRangeBeginOverlap);
	InteractionRange->OnComponentEndOverlap.AddDynamic(this, &AJY_InteractableActor::HandleRangeEndOverlap);
}

/* 위젯은 처음 보일 때 한 번만 글자를 채우고, 이후엔 켜고 끄기만 한다 */
// [OwnClient]
void AJY_InteractableActor::SetPromptVisible(bool bVisible)
{
	UJY_InteractPrompt* Prompt = Cast<UJY_InteractPrompt>(PromptWidget->GetUserWidgetObject());
	if (Prompt != nullptr && bVisible == true)
	{
		Prompt->SetPromptText(DisplayName);
	}

	PromptWidget->SetVisibility(bVisible);
}

// [Server]
bool AJY_InteractableActor::IsWithinInteractRange(const AActor* User) const
{
	if (User == nullptr)
		return false;

	const float MaxDistance = InteractionRange->GetScaledSphereRadius() + ServerRangeTolerance;
	return FVector::DistSquared(User->GetActorLocation(), InteractionRange->GetComponentLocation()) <= FMath::Square(MaxDistance);
}

/* 겹침은 서버·클라 모두에서 일어난다 — 내가 조종하는 폰일 때만 반응 (탐지·안내는 로컬 일) */
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

// [Server] [OwnClient]
void AJY_InteractableActor::HandleRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn == nullptr || Pawn->IsLocallyControlled() == false)
		return;

	UJY_InteractionComponent* InteractionComponent = Pawn->FindComponentByClass<UJY_InteractionComponent>();
	if (InteractionComponent == nullptr)
		return;

	InteractionComponent->RemoveCandidate(this);
}
