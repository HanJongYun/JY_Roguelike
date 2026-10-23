#include "Object/Actions/JY_BreakAction.h"
#include "Object/JY_Object.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Field/FieldSystemObjects.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_BreakAction)

namespace JY_Break
{
	/* 파편 충돌 — 바닥·벽(WorldStatic)과 파편끼리만 (DefaultEngine.ini) */
	static const FName DebrisCollisionProfileName(TEXT("JY_Debris"));

	/* 맞은 점 높이 — 몸체 중심에서 아래로 반높이의 이 비율만큼 */
	static constexpr float ImpactHeightRatio = 0.5f;
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

	/* 터지는 점에서 바깥으로 — 모든 조각이 그 점 반대편으로 흩어진다 */
	const FVector& PushVelocity = Object->GetActivationImpulse();
	const FVector BurstCenter = GetBurstCenter(Body, PushVelocity);
	URadialVector* BurstVelocity = NewObject<URadialVector>(this);
	BurstVelocity->SetRadialVector(DebrisSpeed, BurstCenter);

#if !UE_BUILD_SHIPPING
	/* 범위 보기(jy.Debug.ShowRanges, L 키)를 켜면 터지는 점(빨강)과 미는 방향(화살표)을 3초 그린다 */
	const IConsoleVariable* ShowRangesVariable = IConsoleManager::Get().FindConsoleVariable(TEXT("jy.Debug.ShowRanges"));
	if (ShowRangesVariable != nullptr && ShowRangesVariable->GetInt() != 0)
	{
		DrawDebugSphere(GetWorld(), BurstCenter, 8.0f, 8, FColor::Red, false, 3.0f);
		DrawDebugDirectionalArrow(GetWorld(), BurstCenter, BurstCenter + PushVelocity.GetSafeNormal() * 150.0f, 30.0f, FColor::Red, false, 3.0f, 0, 3.0f);
	}
#endif

	/* [임시 JY_SWINGDBG] */
	UE_LOG(LogTemp, Warning, TEXT("[JY_SWINGDBG][Client] Break %s Push=%s Center=%s BodyOrigin=%s Extent=%s"), *GetNameSafe(Object), *PushVelocity.ToString(), *BurstCenter.ToString(), *Body->Bounds.Origin.ToString(), *Body->Bounds.BoxExtent.ToString());
	DebrisComponent->ApplyPhysicsField(true, EGeometryCollectionPhysicsTypeEnum::Chaos_LinearVelocity, nullptr, BurstVelocity);

	/* 맞은 방향으로 한 번 더 — 속도 필드는 덮어쓰지 않고 더한다 (FieldSystemProxyHelper.h:613) */
	if (PushVelocity.IsNearlyZero() == false)
	{
		UUniformVector* PushField = NewObject<UUniformVector>(this);
		PushField->SetUniformVector(PushVelocity.Size(), PushVelocity.GetSafeNormal());
		DebrisComponent->ApplyPhysicsField(true, EGeometryCollectionPhysicsTypeEnum::Chaos_LinearVelocity, nullptr, PushField);
	}

	GetWorld()->GetTimerManager().SetTimer(DebrisTimerHandle, this, &ThisClass::DestroyDebris, DebrisMaxLifeSeconds, false);
}

/*
 * 맞은 방향이 있으면 맞은 쪽 표면(칼이 들어온 쪽)이 터지는 점 — 조각이 칼 방향 반구로 날아간다.
 * 높이는 몸체 아래쪽이라 조각이 바닥에 박히지 않고 위로 뜨며 날아간다. 방향이 없으면 바닥 중심(사방)
 */
FVector UJY_BreakAction::GetBurstCenter(const UPrimitiveComponent* Body, const FVector& PushVelocity) const
{
	const FBoxSphereBounds& BodyBounds = Body->Bounds;
	const FVector BottomCenter = BodyBounds.Origin - FVector::UpVector * BodyBounds.BoxExtent.Z;
	const FVector PushDirection = PushVelocity.GetSafeNormal2D();
	if (PushDirection.IsZero() == true)
		return BottomCenter;

	const float HorizontalExtent = FMath::Max(BodyBounds.BoxExtent.X, BodyBounds.BoxExtent.Y);
	const FVector LowerCenter = BodyBounds.Origin - FVector::UpVector * BodyBounds.BoxExtent.Z * JY_Break::ImpactHeightRatio;
	return LowerCenter - PushDirection * HorizontalExtent;
}

// [Client]
void UJY_BreakAction::DestroyDebris()
{
	if (DebrisComponent == nullptr)
		return;

	DebrisComponent->DestroyComponent();
	DebrisComponent = nullptr;
}
