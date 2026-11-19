#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "JY_MassBlockerSubsystem.generated.h"

class AJY_MassBlockerActor;
class APawn;
class UJY_AbilitySystemComponent;

/* 플레이어가 Mass 적을 걸어서 못 뚫게, 플레이어 주변 적 자리에 길막이 캡슐을 놓는다 — 서버·클라가 각자 자기 적 위치로 */
UCLASS()
class UJY_MassBlockerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/* Mass PrePhysics 처리 직후 — 플레이어 주변 적 자리로 캡슐을 옮긴다 */
	void HandlePrePhysicsFinished(float DeltaSeconds);

	/* 플레이어가 바뀌면 그 캡슐이 길막이 채널을 막게 하고, 충돌 무시 태그 알림을 다시 건다 */
	void BindPlayer(APawn* InPlayerPawn);

	/* 충돌 무시 태그가 붙는 순간 캡슐을 전부 끈다 */
	void HandleIgnoreCollisionTagChanged(const FGameplayTag Tag, int32 NewCount);

private:
	UPROPERTY()
	TObjectPtr<AJY_MassBlockerActor> BlockerActor;

	/* 지금 알림을 걸어 둔 플레이어 */
	TWeakObjectPtr<APawn> PlayerPawn;

	TWeakObjectPtr<UJY_AbilitySystemComponent> AbilitySystemComponent;

	FDelegateHandle IgnoreCollisionTagHandle;
};
