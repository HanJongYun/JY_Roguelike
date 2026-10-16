#include "Object/Actions/JY_BreakAction.h"
#include "Object/JY_Object.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Field/FieldSystemObjects.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_BreakAction)

namespace JY_Break
{
	/* 파편 충돌 — 바닥·벽(WorldStatic)과 파편끼리만 (DefaultEngine.ini) */
	static const FName DebrisCollisionProfileName(TEXT("JY_Debris"));
}

// [Server]
void UJY_BreakAction::OnServerEnter(AJY_Object* Object) //override
{
	HideBody(Object);
}

/* 늦게 받았으면(bCatchUp) 이미 부서져 있던 것 — 숨기기만 하고 파편·먼지는 없다 */
// [Client]
void UJY_BreakAction::OnClientEnter(AJY_Object* Object, bool bCatchUp) //override
{
	HideBody(Object);

	if (bCatchUp == true)
		return;

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(Object, BreakFX, Object->GetActorLocation());
	SpawnDebris(Object);
}

// [Server] [Client]
void UJY_BreakAction::HideBody(AJY_Object* Object) const
{
	UPrimitiveComponent* Body = FindPart<UPrimitiveComponent>(Object, BodyPartTag);
	if (Body == nullptr)
		return;

	Body->SetVisibility(false, true);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

/*
 * 파편은 복제하지 않는 클라 연출 — 각 클라가 만든다.
 * 등록 → 묶음 풀기 → 속도 필드를 한 프레임에 해야 풀린 조각마다 속도가 붙는다 (PBDRigidsSolver.cpp:2153)
 */
// [Client]
void UJY_BreakAction::SpawnDebris(AJY_Object* Object)
{
	if (BreakGeometry == nullptr)
		return;

	UPrimitiveComponent* Body = FindPart<UPrimitiveComponent>(Object, BodyPartTag);
	if (Body == nullptr)
		return;

	DebrisComponent = NewObject<UGeometryCollectionComponent>(Object, TEXT("Debris"));
	DebrisComponent->SetRestCollection(BreakGeometry);

	/* 캐릭터가 밟고 걸리거나 전투 판정·카메라 가림 검사에 잡히지 않게 */
	DebrisComponent->SetCollisionProfileName(JY_Break::DebrisCollisionProfileName);

	/* 몸체 위치·크기 그대로 (BP 메시 배율 포함) */
	DebrisComponent->SetupAttachment(Body);
	DebrisComponent->RegisterComponent();
	DebrisComponent->CrumbleActiveClusters();

	/* 몸체 바닥 중심에서 바깥으로 — 아래 조각은 옆으로, 위 조각은 위로 흩어짐 */
	const FBoxSphereBounds& BodyBounds = Body->Bounds;
	const FVector BurstCenter = BodyBounds.Origin - FVector::UpVector * BodyBounds.BoxExtent.Z;
	URadialVector* BurstVelocity = NewObject<URadialVector>(this);
	BurstVelocity->SetRadialVector(DebrisSpeed, BurstCenter);
	DebrisComponent->ApplyPhysicsField(true, EGeometryCollectionPhysicsTypeEnum::Chaos_LinearVelocity, nullptr, BurstVelocity);

	/* 맞은 방향으로 한 번 더 — 속도 필드는 덮어쓰지 않고 더한다 (FieldSystemProxyHelper.h:613) */
	const FVector& PushVelocity = Object->GetActivationImpulse();
	if (PushVelocity.IsNearlyZero() == false)
	{
		UUniformVector* PushField = NewObject<UUniformVector>(this);
		PushField->SetUniformVector(PushVelocity.Size(), PushVelocity.GetSafeNormal());
		DebrisComponent->ApplyPhysicsField(true, EGeometryCollectionPhysicsTypeEnum::Chaos_LinearVelocity, nullptr, PushField);
	}

	GetWorld()->GetTimerManager().SetTimer(DebrisTimerHandle, this, &ThisClass::DestroyDebris, DebrisMaxLifeSeconds, false);
}

// [Client]
void UJY_BreakAction::DestroyDebris()
{
	if (DebrisComponent == nullptr)
		return;

	DebrisComponent->DestroyComponent();
	DebrisComponent = nullptr;
}
