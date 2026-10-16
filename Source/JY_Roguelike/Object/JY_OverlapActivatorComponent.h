#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_OverlapActivatorComponent.generated.h"

enum class EJY_ObjectState : uint8;

/*
 * 들어오면 발동 — 누구든(플레이어·적) 물체에 닿으면 발동시킨다 (화염판).
 * 감지 모양은 BP에서 붙인 엔진 충돌체(Box·Sphere, 겹침 프로필)가 정한다 — 모양과 무관하게 액터 단위 겹침을 받는다 (Actor.h:1368 OnActorBeginOverlap)
 */
UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_OverlapActivatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_OverlapActivatorComponent();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleOwnerBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

	/* 다시 Idle이 됐을 때 이미 올라와 있는 폰이 있으면 다시 발동 — 쿨타임 중에 올라와 계속 서 있던 경우 */
	void HandleStateChanged(EJY_ObjectState NewState);
};
