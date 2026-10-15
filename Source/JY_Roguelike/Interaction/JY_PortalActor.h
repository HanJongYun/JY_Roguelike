#pragma once

#include "CoreMinimal.h"
#include "Interaction/JY_InteractableActor.h"
#include "JY_PortalActor.generated.h"

class UNiagaraComponent;

/* 사용하면 서버가 모두를 다른 맵으로 옮기는 포털 */
UCLASS()
class AJY_PortalActor : public AJY_InteractableActor
{
	GENERATED_BODY()

public:
	AJY_PortalActor();

public:
	virtual void OnServerInteract(APlayerController* User) override;

private:
	/* 링 안에서 도는 소용돌이 이펙트 — 그릴 이펙트(Niagara System)는 BP_JY_Portal에서 지정 */
	UPROPERTY(VisibleAnywhere, Category = "JY|Portal")
	TObjectPtr<UNiagaraComponent> VortexEffect;

private:
	/* 설정값 — BP_JY_Portal의 Class Defaults에서 수정 */

	/* 이동할 맵 (던전이 생기기 전까지 Lvl_JY_Test) */
	UPROPERTY(EditAnywhere, Category = "JY|Portal")
	TSoftObjectPtr<UWorld> DestinationMap;
};
