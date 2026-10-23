#include "Object/Actions/JY_MovePartAction.h"
#include "Object/JY_Object.h"
#include "Components/SceneComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_MovePartAction)

/* 늦게 받았거나 시간이 0이면 바로 목표로, 아니면 물체 Tick을 빌려 따라감 (Action엔 Tick이 없다) */
// [Client]
void UJY_MovePartAction::OnClientEnter(AJY_Object* Object, bool bCatchUp) //override
{
	USceneComponent* Part = FindPart<USceneComponent>(Object, PartTag);
	if (Part == nullptr)
		return;

	if (bCatchUp == true || MoveSeconds <= 0.0f)
	{
		Part->SetRelativeLocation(TargetLocation);
		return;
	}

	/* 이전 상태 이동이 덜 끝났으면 그 자리에서 출발 — 겹친 상태 전환에도 튀지 않는다 */
	StartLocation = Part->GetRelativeLocation();
	ElapsedSeconds = 0.0f;
	Object->AddTickingAction(this);
}

/* 다음 상태의 부품 이동이 같은 부품을 이어받는다 — 겹쳐 당기지 않게 멈춤 */
// [Client]
void UJY_MovePartAction::OnClientExit(AJY_Object* Object) //override
{
	Object->RemoveTickingAction(this);
}

// [Client]
bool UJY_MovePartAction::TickClient(AJY_Object* Object, float DeltaSeconds) //override
{
	USceneComponent* Part = FindPart<USceneComponent>(Object, PartTag);
	if (Part == nullptr)
		return false;

	ElapsedSeconds += DeltaSeconds;
	const float Alpha = FMath::Clamp(ElapsedSeconds / MoveSeconds, 0.0f, 1.0f);
	const FVector NewLocation = UKismetMathLibrary::VEase(StartLocation, TargetLocation, Alpha, EasingFunc, BlendExp);
	Part->SetRelativeLocation(NewLocation);
	return Alpha < 1.0f;
}
