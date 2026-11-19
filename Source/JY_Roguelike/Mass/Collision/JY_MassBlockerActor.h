#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JY_MassBlockerActor.generated.h"

class UCapsuleComponent;

namespace JY_CollisionChannel
{
	/* Mass 적 자리의 길막이 캡슐 — DefaultEngine.ini의 MassBlocker와 같은 번호. 기본 응답은 무시, 플레이어 캡슐만 막는다 */
	constexpr ECollisionChannel MassBlocker = ECC_GameTraceChannel1;
}

/* Mass 적 자리에 놓는 길막이 캡슐을 들고 있는 그릇 — 복제하지 않고 서버·클라가 각자 하나씩 만든다 */
UCLASS(NotPlaceable)
class AJY_MassBlockerActor : public AActor
{
	GENERATED_BODY()

public:
	AJY_MassBlockerActor();

public:
	/* 이번 프레임 배치를 시작한다 — 뒤이어 PlaceBlocker를 부르고 EndPlacement로 끝낸다 */
	void BeginPlacement();

	/* 캡슐 하나를 이 자리에 놓고 켠다 (모자라면 새로 만든다) */
	void PlaceBlocker(const FVector& Location, float Radius);

	/* 이번 프레임에 안 쓴 캡슐을 끈다 */
	void EndPlacement();

	/* 캡슐을 전부 끈다 */
	void DisableAllBlockers();

private:
	UCapsuleComponent* CreateBlocker();

private:
	UPROPERTY()
	TArray<TObjectPtr<UCapsuleComponent>> Blockers;

	/* 이번 프레임에 놓은 캡슐 수 */
	int32 PlacedCount = 0;

	/* 지금 켜져 있는 캡슐 수 — 앞에서부터 이만큼이 켜져 있다 */
	int32 EnabledCount = 0;
};
