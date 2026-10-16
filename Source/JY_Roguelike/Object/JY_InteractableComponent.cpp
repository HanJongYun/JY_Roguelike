#include "Object/JY_InteractableComponent.h"
#include "Object/JY_Object.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_InteractableComponent)

/*
 * 쿼터뷰라 라인트레이스(Lyra 1인칭 방식) 대신 범위 겹침으로 대상을 잡는다. 겹침 이벤트는 플레이어의 UJY_InteractionComponent가 받는다.
 * 주의: 레벨 스트리밍으로 들어온 방의 액터는 시작 시 겹침 이벤트가 없다(bGenerateOverlapEventsDuringLevelStreaming 기본 false, PrimitiveComponent.cpp:4131)
 */
UJY_InteractableComponent::UJY_InteractableComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/* 생성자에선 SetSphereRadius 금지 — BodySetup을 NewObject로 만들어 CDO 생성이 멈춘다 (ShapeComponent.cpp:234). 엔진 생성자처럼 값만 (SphereComponent.cpp) */
	SphereRadius = 250.0f;
	SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetCanEverAffectNavigation(false);
}

// [Server]
bool UJY_InteractableComponent::IsWithinInteractRange(const AActor* User) const //override
{
	if (User == nullptr)
		return false;

	const float MaxDistance = GetScaledSphereRadius() + ServerRangeTolerance;
	return FVector::DistSquared(User->GetActorLocation(), GetComponentLocation()) <= FMath::Square(MaxDistance);
}

// [OwnClient]
void UJY_InteractableComponent::InteractLocal(APawn* User) //override
{
	AJY_Object* Object = GetOwner<AJY_Object>();
	if (Object == nullptr)
		return;

	Object->ExecuteLocalActions(User);
}

// [Server]
void UJY_InteractableComponent::InteractServer(APawn* User) //override
{
	AJY_Object* Object = GetOwner<AJY_Object>();
	if (Object == nullptr)
		return;

	Object->RequestActivate(User);
}
