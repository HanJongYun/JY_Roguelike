#include "AbilitySystem/Abilities/JY_GameplayAbility_DashStrike.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_DashStrike)

// [Server] [OwnClient]
void UJY_GameplayAbility_DashStrike::OnSkillStarted() //override
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	StartLocation = AvatarActor->GetActorLocation();
}

// [Server] [OwnClient]
/* 돌진이 지나간 자리 전체를 벤다 — 원점을 시작 위치로 옮기고 경로 방향으로 뻗은 상자. 벽에 막혀 거의 안 움직였으면 몸 앞 */
FJY_HitShape UJY_GameplayAbility_DashStrike::MakeHitShapeForEvent(int32 EventIndex, FTransform& InOutOrigin) const //override
{
	const FVector CurrentLocation = InOutOrigin.GetLocation();
	const FVector Path = CurrentLocation - StartLocation;
	const FVector Path2D(Path.X, Path.Y, 0.0f);

	FJY_HitShape PathShape;
	PathShape.Type = EJY_HitShapeType::Box;
	PathShape.Width = PathWidth;
	PathShape.Length = Path2D.Size();

	if (Path2D.IsNearlyZero() == false)
	{
		const FRotator PathRotation = Path2D.Rotation();
		InOutOrigin = FTransform(PathRotation, StartLocation);
	}
	return PathShape;
}
