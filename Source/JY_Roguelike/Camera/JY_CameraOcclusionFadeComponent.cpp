#include "Camera/JY_CameraOcclusionFadeComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_CameraOcclusionFadeComponent)

UJY_CameraOcclusionFadeComponent::UJY_CameraOcclusionFadeComponent()
{
	/* 흐림 값을 프레임마다 보간해야 해서 Tick — 무거운 스윕은 OccluderCheckInterval마다만 */
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// [OwnClient]
void UJY_CameraOcclusionFadeComponent::BeginPlay() //override
{
	Super::BeginPlay();

	const APlayerController* PlayerController = CastChecked<APlayerController>(GetOwner());
	if (PlayerController->IsLocalController() == false)
		return;

	SetComponentTickEnabled(true);
}

// [OwnClient]
void UJY_CameraOcclusionFadeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) //override
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeUntilOccluderCheck -= DeltaTime;
	if (TimeUntilOccluderCheck <= 0.0f)
	{
		TimeUntilOccluderCheck = OccluderCheckInterval;
		UpdateOccluders();
	}

	UpdateFadeAmounts(DeltaTime);
}

// [OwnClient]
/*
 * 오브젝트 타입 멀티 스윕은 앞 벽에서 멈추지 않는다 (CollisionQueryFilterCallback.cpp:32).
 * 채널 스윕은 첫 벽에서 멈춰 겹친 벽 뒤쪽을 놓친다
 */
void UJY_CameraOcclusionFadeComponent::UpdateOccluders()
{
	Occluders.Reset();

	const APlayerController* PlayerController = CastChecked<APlayerController>(GetOwner());
	const APawn* Pawn = PlayerController->GetPawn();
	const APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;

	/* 캐릭터가 없으면(사망·이동 중) 가리는 것도 없음 → 흐린 벽이 서서히 돌아옴 */
	if (Pawn == nullptr || CameraManager == nullptr)
		return;

	const FVector CameraLocation = CameraManager->GetCameraLocation();
	const FVector PawnLocation = Pawn->GetActorLocation();
	const FCollisionObjectQueryParams ObjectQueryParams(ECC_WorldStatic);
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JYCameraOcclusionFade), false, Pawn);
	const FCollisionShape ProbeShape = FCollisionShape::MakeSphere(ProbeRadius);

	TArray<FHitResult> Hits;
	const UWorld* World = GetWorld();
	World->SweepMultiByObjectType(OUT Hits, CameraLocation, PawnLocation, FQuat::Identity, ObjectQueryParams, ProbeShape, QueryParams);

	for (const FHitResult& Hit : Hits)
	{
		UPrimitiveComponent* HitComponent = Hit.GetComponent();
		if (HitComponent == nullptr)
			continue;

		const AActor* HitActor = HitComponent->GetOwner();
		if (HitActor == nullptr || HitActor->ActorHasTag(FadeActorTag) == false)
			continue;

		Occluders.Add(HitComponent);
		FadeAmounts.FindOrAdd(HitComponent, 0.0f);
	}
}

// [OwnClient]
/* 재질 인스턴스 대신 Custom Primitive Data — 벽마다 MID를 만들지 않아 같은 벽끼리 묶어 그린다 */
void UJY_CameraOcclusionFadeComponent::UpdateFadeAmounts(float DeltaTime)
{
	for (auto It = FadeAmounts.CreateIterator(); It; ++It)
	{
		UPrimitiveComponent* FadeComponent = It.Key().Get();

		/* 방(서브레벨)이 내려가 물체가 사라짐 */
		if (FadeComponent == nullptr)
		{
			It.RemoveCurrent();
			continue;
		}

		const float TargetAmount = Occluders.Contains(It.Key()) == true ? 1.0f : 0.0f;
		const float NewAmount = FMath::FInterpConstantTo(It.Value(), TargetAmount, DeltaTime, FadeSpeed);
		It.Value() = NewAmount;
		FadeComponent->SetCustomPrimitiveDataFloat(FadeDataIndex, NewAmount);

		if (NewAmount <= 0.0f)
		{
			It.RemoveCurrent();
		}
	}
}
