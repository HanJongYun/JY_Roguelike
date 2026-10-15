#include "Combat/JY_CombatQuerySubsystem.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Effects/JY_CombatGameplayEffects.h"
#include "Enemy/JY_EnemyCharacter.h"
#include "System/JY_GameplayTags.h"
#include "AbilitySystemGlobals.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GenericTeamAgentInterface.h"
#include "HAL/IConsoleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_CombatQuerySubsystem)

namespace JY_CombatQuery
{
	static TAutoConsoleVariable<int32> CVarDrawHitShapes(TEXT("jy.Combat.DrawHitShapes"), 0, TEXT("1이면 타격 판정 모양을 그린다 (서버·조종 클라 각자)"));

	/* 대상 캡슐이 2D 모양에 걸치는지 — 캡슐 반지름만큼 너그럽게 본다 */
	bool IsInsideShape2D(const FJY_HitShape& Shape, const FVector& Center, const FVector& Forward, const FVector& TargetLocation, float TargetRadius)
	{
		const FVector ToTarget3D = TargetLocation - Center;
		const FVector2D ToTarget(ToTarget3D);
		const float Distance = ToTarget.Size();
		const FVector2D Forward2D(Forward);

		switch (Shape.Type)
		{
		case EJY_HitShapeType::Sphere:
			return Distance <= Shape.Radius + TargetRadius;

		case EJY_HitShapeType::Arc:
		{
			if (Distance > Shape.Radius + TargetRadius)
				return false;

			/* 몸이 공격자에 겹쳐 있으면 각도 무시 */
			if (Distance <= TargetRadius)
				return true;

			const FVector2D TargetDirection = ToTarget / Distance;
			const float CosAngle = FVector2D::DotProduct(Forward2D, TargetDirection);
			const float HalfAngleRadians = FMath::DegreesToRadians(Shape.AngleDegrees * 0.5f);
			return CosAngle >= FMath::Cos(HalfAngleRadians);
		}

		case EJY_HitShapeType::Box:
		{
			/* 언리얼 좌표(Z 위)에서 앞(1,0)의 오른쪽은 (0,1) */
			const FVector2D Right2D(-Forward2D.Y, Forward2D.X);
			const float LocalX = FVector2D::DotProduct(ToTarget, Forward2D);
			const float LocalY = FVector2D::DotProduct(ToTarget, Right2D);
			const bool bInsideLength = LocalX >= 0.0f && LocalX <= Shape.Length + TargetRadius;
			const bool bInsideWidth = FMath::Abs(LocalY) <= Shape.Width * 0.5f + TargetRadius;
			return bInsideLength == true && bInsideWidth == true;
		}
		}
		return false;
	}
}

bool UJY_CombatQuerySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const //override
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// [Server] [OwnClient]
/*
 * 넓은 구 겹침으로 후보를 모으고 수평 2D 모양으로 거른다.
 * 칼 궤적 추적 대신 모양 판정 — Mass 적도 같은 모양으로 공간 격자에서 찾게 된다(Design_Mass.md §6-1).
 * 조종 클라는 연출(화면 흔들림)용으로만 부른다.
 */
void UJY_CombatQuerySubsystem::FindTargets(const FJY_HitShape& Shape, const FTransform& Origin, const AActor* Attacker, TArray<FJY_CombatTarget>& OutTargets) const
{
	const UWorld* World = GetWorld();
	const FVector LocalOffset(Shape.Offset.X, Shape.Offset.Y, 0.0f);
	const FVector Center = Origin.TransformPosition(LocalOffset);
	const FVector Forward = Origin.GetUnitAxis(EAxis::X);

	/* 상자는 공격자 앞으로 뻗으므로 중심에서 가장 먼 모서리까지 */
	float QueryRadius = Shape.Radius;
	if (Shape.Type == EJY_HitShapeType::Box)
	{
		const FVector2D FarCorner(Shape.Length, Shape.Width * 0.5f);
		QueryRadius = FarCorner.Size();
	}

	const FCollisionObjectQueryParams ObjectQueryParams(ECC_Pawn);
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JY_FindTargets), false, Attacker);
	const FCollisionShape QuerySphere = FCollisionShape::MakeSphere(QueryRadius);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(OUT Overlaps, Center, FQuat::Identity, ObjectQueryParams, QuerySphere, QueryParams);

	/* 캡슐·메시가 둘 다 Pawn 채널이라 같은 액터가 여러 번 걸린다 */
	TSet<const AActor*> CheckedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		bool bAlreadyChecked = false;
		CheckedActors.Add(Actor, &bAlreadyChecked);
		if (bAlreadyChecked == true)
			continue;

		if (FGenericTeamId::GetAttitude(Attacker, Actor) != ETeamAttitude::Hostile)
			continue;

		const UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
		if (TargetASC == nullptr)
			continue;

		/* 죽는 중인 대상은 시체 — 다시 맞지 않는다 */
		if (TargetASC->HasMatchingGameplayTag(JYGameplayTags::Status_Death) == true)
			continue;

		float TargetRadius = 0.0f;
		float TargetHalfHeight = 0.0f;
		Actor->GetSimpleCollisionCylinder(OUT TargetRadius, OUT TargetHalfHeight);

		const FVector TargetLocation = Actor->GetActorLocation();
		const float HeightDifference = FMath::Abs(TargetLocation.Z - Center.Z);
		if (HeightDifference > Shape.HalfHeight + TargetHalfHeight)
			continue;

		if (JY_CombatQuery::IsInsideShape2D(Shape, Center, Forward, TargetLocation, TargetRadius) == false)
			continue;

		FJY_CombatTarget& Target = OutTargets.AddDefaulted_GetRef();
		Target.Actor = Actor;
		Target.Location = TargetLocation;
	}

	if (JY_CombatQuery::CVarDrawHitShapes.GetValueOnGameThread() != 0)
	{
		const bool bHitAny = OutTargets.Num() > 0;
		DrawHitShape(World, Shape, Origin, bHitAny);
	}
}

// [Server]
/*
 * 피해는 GE(SetByCaller.Damage → HealthSet.Damage 메타 속성), 밀어내기는 적 LaunchCharacter, 연출은 큐 태그.
 * 큐를 피해 GE에 묶지 않는 이유: 스킬마다 다른 피격 연출을 GE를 늘리지 않고 데이터(큐 태그)로 바꾼다.
 */
void UJY_CombatQuerySubsystem::ApplyHit(const FJY_CombatTarget& Target, const FJY_HitPayload& Payload) const
{
	AActor* TargetActor = Target.Actor.Get();
	/* 조종 클라의 연출용 판정이 실수로 부르는 것 차단 */
	if (TargetActor->HasAuthority() == false)
		return;

	AActor* InstigatorActor = Payload.Instigator.Get();
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InstigatorActor);
	UJY_AbilitySystemComponent* TargetJYASC = CastChecked<UJY_AbilitySystemComponent>(TargetASC);
	UJY_AbilitySystemComponent* SourceJYASC = CastChecked<UJY_AbilitySystemComponent>(SourceASC);

	/* MakeEffectContext가 가해자(PlayerState)·원인(캐릭터)을 이미 넣는다 — 위치만 더한다 */
	FGameplayEffectContextHandle EffectContext = SourceJYASC->MakeEffectContext();
	const FVector InstigatorLocation = InstigatorActor->GetActorLocation();
	EffectContext.AddOrigin(InstigatorLocation);

	const FGameplayEffectSpecHandle DamageSpecHandle = SourceJYASC->MakeOutgoingSpec(UJY_GameplayEffect_Damage::StaticClass(), 1.0f, EffectContext);
	FGameplayEffectSpec* DamageSpec = DamageSpecHandle.Data.Get();
	DamageSpec->SetSetByCallerMagnitude(JYGameplayTags::SetByCaller_Damage, Payload.Damage);
	SourceJYASC->ApplyGameplayEffectSpecToTarget(*DamageSpec, TargetJYASC);

	if (Payload.KnockbackImpulse.IsNearlyZero() == false)
	{
		AJY_EnemyCharacter* Enemy = Cast<AJY_EnemyCharacter>(TargetActor);
		if (Enemy != nullptr)
		{
			Enemy->ReceiveKnockback(Payload.KnockbackImpulse);
		}
	}

	if (Payload.HitCueTag.IsValid() == true)
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Location = Target.Location;
		CueParameters.Instigator = InstigatorActor;
		CueParameters.EffectContext = EffectContext;
		CueParameters.RawMagnitude = Payload.Damage;

		/* 서버가 부르면 모든 클라로 멀티캐스트, 데디 서버 자신은 연출 생략(GameplayCueManager.cpp:178~183) */
		TargetJYASC->ExecuteGameplayCue(Payload.HitCueTag, CueParameters);
	}
}

void UJY_CombatQuerySubsystem::DrawHitShape(const UWorld* World, const FJY_HitShape& Shape, const FTransform& Origin, bool bHitAny)
{
	const FColor ShapeColor = bHitAny == true ? FColor::Red : FColor::Green;
	const float DrawSeconds = 1.0f;
	const FVector LocalOffset(Shape.Offset.X, Shape.Offset.Y, 0.0f);
	const FVector Center = Origin.TransformPosition(LocalOffset);
	const FVector Forward = Origin.GetUnitAxis(EAxis::X);
	const FVector HeightOffset(0.0f, 0.0f, Shape.HalfHeight);

	switch (Shape.Type)
	{
	case EJY_HitShapeType::Sphere:
	{
		const FVector Bottom = Center - HeightOffset;
		const FVector Top = Center + HeightOffset;
		DrawDebugCylinder(World, Bottom, Top, Shape.Radius, 24, ShapeColor, false, DrawSeconds);
		break;
	}

	case EJY_HitShapeType::Arc:
	{
		const int32 SegmentCount = 12;
		const float HalfAngle = Shape.AngleDegrees * 0.5f;
		FVector PreviousPoint = Center;
		for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
		{
			const float Alpha = static_cast<float>(SegmentIndex) / SegmentCount;
			const float Yaw = FMath::Lerp(-HalfAngle, HalfAngle, Alpha);
			const FVector EdgeDirection = Forward.RotateAngleAxis(Yaw, FVector::UpVector);
			const FVector EdgePoint = Center + EdgeDirection * Shape.Radius;
			DrawDebugLine(World, PreviousPoint, EdgePoint, ShapeColor, false, DrawSeconds);
			PreviousPoint = EdgePoint;
		}
		DrawDebugLine(World, PreviousPoint, Center, ShapeColor, false, DrawSeconds);
		break;
	}

	case EJY_HitShapeType::Box:
	{
		const FVector BoxCenter = Center + Forward * (Shape.Length * 0.5f);
		const FVector BoxExtent(Shape.Length * 0.5f, Shape.Width * 0.5f, Shape.HalfHeight);
		const FQuat BoxRotation = Origin.GetRotation();
		DrawDebugBox(World, BoxCenter, BoxExtent, BoxRotation, ShapeColor, false, DrawSeconds);
		break;
	}
	}
}
