#include "Combat/JY_CombatQuerySubsystem.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "Combat/JY_HitReactionComponent.h"
#include "Interfaces/JY_HitReactable.h"
#include "Interfaces/JY_InterfaceUtils.h"
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

	/* 높이 허용치 안이고 2D 모양에 걸치는지 — 대상 크기는 단순 충돌 원기둥 */
	bool IsActorInsideShape(const FJY_HitShape& Shape, const FVector& Center, const FVector& Forward, const AActor* Actor)
	{
		float TargetRadius = 0.0f;
		float TargetHalfHeight = 0.0f;
		Actor->GetSimpleCollisionCylinder(OUT TargetRadius, OUT TargetHalfHeight);

		const FVector TargetLocation = Actor->GetActorLocation();
		const float HeightDifference = FMath::Abs(TargetLocation.Z - Center.Z);
		if (HeightDifference > Shape.HalfHeight + TargetHalfHeight)
			return false;

		return IsInsideShape2D(Shape, Center, Forward, TargetLocation, TargetRadius);
	}
}

bool UJY_CombatQuerySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const //override
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// [Server] [OwnClient]
/*
 * 넓은 구 겹침으로 후보를 모으고 높이·수평 2D 모양으로 거른다 — 칼 궤적 대신 모양이라 Mass 적에도 같은 판정을 쓴다.
 * 함정·폭발·공격이 모두 여기를 거쳐, Mass 적 후보를 더할 곳이 여기 하나다
 */
void UJY_CombatQuerySubsystem::FindActorsInShape(const FJY_HitShape& Shape, const FTransform& Origin, const AActor* IgnoreActor, const FCollisionObjectQueryParams& ObjectQueryParams, TArray<AActor*>& OutActors) const
{
	const UWorld* World = GetWorld();
	const FVector LocalOffset(Shape.Offset.X, Shape.Offset.Y, 0.0f);
	const FVector Center = Origin.TransformPosition(LocalOffset);
	const FVector Forward = Origin.GetUnitAxis(EAxis::X);

	/* 상자는 중심에서 앞으로 뻗으므로 중심에서 가장 먼 모서리까지 */
	float QueryRadius = Shape.Radius;
	if (Shape.Type == EJY_HitShapeType::Box)
	{
		const FVector2D FarCorner(Shape.Length, Shape.Width * 0.5f);
		QueryRadius = FarCorner.Size();
	}

	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(JY_FindActorsInShape), false, IgnoreActor);
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

		if (JY_CombatQuery::IsActorInsideShape(Shape, Center, Forward, Actor) == false)
			continue;

		OutActors.Add(Actor);
	}
}

// [Server] [OwnClient]
/* 조종 클라는 연출(화면 흔들림)용으로만 부른다 */
void UJY_CombatQuerySubsystem::FindTargets(const FJY_HitShape& Shape, const FTransform& Origin, const AActor* Attacker, TArray<FJY_CombatTarget>& OutTargets) const
{
	/* 맞으면 발동하는 물체(IJY_HitReactable — 항아리 등)는 WorldDynamic */
	FCollisionObjectQueryParams ObjectQueryParams(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	TArray<AActor*> ActorsInShape;
	FindActorsInShape(Shape, Origin, Attacker, ObjectQueryParams, OUT ActorsInShape);

	for (AActor* Actor : ActorsInShape)
	{
		/* 맞으면 발동하는 물체는 팀·체력 없이 모양만 본다 — 적 공격도 부순다 */
		const IJY_HitReactable* HitReactable = JY_FindInterface<IJY_HitReactable>(Actor);
		if (HitReactable != nullptr)
		{
			if (HitReactable->CanReceiveHit() == true)
			{
				FJY_CombatTarget& Target = OutTargets.AddDefaulted_GetRef();
				Target.Actor = Actor;
				Target.Location = Actor->GetActorLocation();
			}
			continue;
		}

		if (FGenericTeamId::GetAttitude(Attacker, Actor) != ETeamAttitude::Hostile)
			continue;

		const UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
		if (TargetASC == nullptr)
			continue;

		/* 죽는 중인 대상은 시체 — 다시 맞지 않는다 */
		if (TargetASC->HasMatchingGameplayTag(JYGameplayTags::Status_Death) == true)
			continue;

		FJY_CombatTarget& Target = OutTargets.AddDefaulted_GetRef();
		Target.Actor = Actor;
		Target.Location = Actor->GetActorLocation();
	}

	if (JY_CombatQuery::CVarDrawHitShapes.GetValueOnGameThread() != 0)
	{
		const bool bHitAny = OutTargets.Num() > 0;
		DrawHitShape(GetWorld(), Shape, Origin, bHitAny);
	}
}

// [Server]
void UJY_CombatQuerySubsystem::ApplyHit(const FJY_CombatTarget& Target, const FJY_HitPayload& Payload) const
{
	AActor* TargetActor = Target.Actor.Get();
	/* 조종 클라의 연출용 판정이 실수로 부르는 것 차단 */
	if (TargetActor->HasAuthority() == false)
		return;

	AActor* InstigatorActor = Payload.Instigator.Get();

	/* 맞으면 발동하는 물체는 맞은 뒤 일을 스스로 정한다(항아리는 부서짐) — 밀어내기 속도가 파편이 튀는 방향·세기 */
	IJY_HitReactable* HitReactable = JY_FindInterface<IJY_HitReactable>(TargetActor);
	if (HitReactable != nullptr)
	{
		FVector HitVelocity(Payload.KnockbackImpulse.X, Payload.KnockbackImpulse.Y, 0.0f);

		/* 칼이 지나간 방향으로 날린다 — 세기는 그대로 두고 방향만 바꿈. 밀기 0인 타격은 크기 1을 넘겨 물체 최소 속도(UJY_HittableComponent::MinPushSpeed)를 쓰게 한다 */
		if (Payload.ObjectPushDirection.IsNearlyZero() == false)
		{
			HitVelocity = Payload.ObjectPushDirection.GetSafeNormal2D() * FMath::Max(HitVelocity.Size(), 1.0f);
		}
		HitReactable->ReceiveHit(InstigatorActor, Payload.Damage, HitVelocity);
		return;
	}

	/* 체력 있는 대상만 판정(FindTargets)을 통과한다 */
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InstigatorActor);
	UJY_AbilitySystemComponent* TargetJYASC = CastChecked<UJY_AbilitySystemComponent>(TargetASC);
	UJY_AbilitySystemComponent* SourceJYASC = CastChecked<UJY_AbilitySystemComponent>(SourceASC);
	const FVector InstigatorLocation = InstigatorActor->GetActorLocation();
	ApplyHitToCharacter(TargetActor, TargetJYASC, SourceJYASC, Payload, InstigatorLocation);
}

// [Server]
/*
 * 원인 ASC = 부순 쪽 — 폭발 통 처치 기록이 부순 플레이어에게 남는다
 * 부순 쪽 ASC가 없으면(함정) 맞는 쪽 ASC로 만든다
 */
bool UJY_CombatQuerySubsystem::ApplyEnvironmentDamage(AActor* TargetActor, const FJY_HitPayload& Payload, const FVector& Origin) const
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (TargetASC == nullptr)
		return false;

	UJY_AbilitySystemComponent* TargetJYASC = CastChecked<UJY_AbilitySystemComponent>(TargetASC);
	UJY_AbilitySystemComponent* SourceJYASC = TargetJYASC;
	AActor* InstigatorActor = Payload.Instigator.Get();
	UAbilitySystemComponent* InstigatorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InstigatorActor);
	if (InstigatorASC != nullptr)
	{
		SourceJYASC = CastChecked<UJY_AbilitySystemComponent>(InstigatorASC);
	}

	return ApplyHitToCharacter(TargetActor, TargetJYASC, SourceJYASC, Payload, Origin);
}

// [Server]
/*
 * 큐를 피해 GE에 묶지 않는다 — 스킬·함정마다 다른 피격 연출을 GE를 늘리지 않고 큐 태그로 바꾼다.
 * 넉백·반응 몽타주는 맞은 쪽(UJY_HitReactionComponent)이 정한다
 */
bool UJY_CombatQuerySubsystem::ApplyHitToCharacter(AActor* TargetActor, UJY_AbilitySystemComponent* TargetASC, UJY_AbilitySystemComponent* SourceASC, const FJY_HitPayload& Payload, const FVector& Origin) const
{
	/* 무적이면 HealthSet이 피해만 0으로 만든다(UJY_HealthSet::PreGameplayEffectExecute) — 피격 큐·밀림까지 나가지 않게 여기서 거른다 */
	if (TargetASC->HasMatchingGameplayTag(JYGameplayTags::Status_Death) == true || TargetASC->HasMatchingGameplayTag(JYGameplayTags::Gameplay_DamageImmunity) == true)
		return false;

	/* MakeEffectContext가 가해자(PlayerState)·원인(캐릭터)을 이미 넣는다 — 위치만 더한다 */
	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddOrigin(Origin);

	/* 화염 지대처럼 상태이상만 거는 범위는 즉시 피해가 0 */
	if (Payload.Damage > 0.0f)
	{
		const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
		const TSubclassOf<UGameplayEffect> DamageEffectClass = GameData->DamageGameplayEffect;
		if (ensureMsgf(DamageEffectClass != nullptr, TEXT("DA_JY_GameData에 DamageGameplayEffect가 없음")) == false)
			return false;

		const FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, EffectContext);
		FGameplayEffectSpec* DamageSpec = DamageSpecHandle.Data.Get();
		DamageSpec->SetSetByCallerMagnitude(JYGameplayTags::SetByCaller_Damage, Payload.Damage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpec, TargetASC);
	}

	/* 상태이상 GE가 기간·틱 간격·몸에 붙는 큐를 정한다 — 피해 방식이 늘면 GE 에셋만 추가 */
	if (Payload.StatusEffect != nullptr)
	{
		const FGameplayEffectSpecHandle StatusSpecHandle = SourceASC->MakeOutgoingSpec(Payload.StatusEffect, 1.0f, EffectContext);
		FGameplayEffectSpec* StatusSpec = StatusSpecHandle.Data.Get();
		StatusSpec->SetSetByCallerMagnitude(JYGameplayTags::SetByCaller_Damage, Payload.StatusTickDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*StatusSpec, TargetASC);
	}

	UJY_HitReactionComponent* HitReaction = TargetActor->FindComponentByClass<UJY_HitReactionComponent>();
	if (HitReaction != nullptr && (Payload.KnockbackImpulse.IsNearlyZero() == false || Payload.KnockbackHeight > 0.0f))
	{
		HitReaction->ReceiveKnockback(Payload.KnockbackImpulse, Payload.KnockbackHeight);
	}

	if (Payload.HitCueTag.IsValid() == true)
	{
		const FVector TargetLocation = TargetActor->GetActorLocation();
		const FVector FromOrigin = TargetLocation - Origin;

		FGameplayCueParameters CueParameters;
		CueParameters.Location = TargetLocation;
		CueParameters.Normal = FromOrigin.GetSafeNormal2D();
		CueParameters.Instigator = Payload.Instigator.Get();
		CueParameters.EffectContext = EffectContext;
		CueParameters.RawMagnitude = Payload.Damage;

		/* 서버가 부르면 모든 클라로 멀티캐스트, 데디 서버 자신은 연출 생략(GameplayCueManager.cpp:178) */
		TargetASC->ExecuteGameplayCue(Payload.HitCueTag, CueParameters);
	}
	return true;
}

void UJY_CombatQuerySubsystem::DrawHitShape(const UWorld* World, const FJY_HitShape& Shape, const FTransform& Origin, bool bHitAny, float DrawSeconds)
{
	const FColor ShapeColor = bHitAny == true ? FColor::Red : FColor::Green;
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
