#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_OverlapActivatorComponent.generated.h"

enum class EJY_ObjectState : uint8;

/* 폰(플레이어·적)이 닿으면 물체를 발동 (화염판) — 감지 모양은 BP에서 붙인 충돌체가 정한다 */
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

	/* 다시 Idle이 됐을 때 이미 올라와 있는 폰이 있으면 다시 발동 */
	void HandleStateChanged(EJY_ObjectState NewState);
};
