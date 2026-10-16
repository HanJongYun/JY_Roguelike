#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "JY_InteractableTarget.generated.h"

class APawn;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UJY_InteractableTarget : public UInterface
{
	GENERATED_BODY()
};

/*
 * E키로 쓰는 대상 — 상호작용(Interaction)이 대상의 클래스를 모른 채 부른다 (Lyra IInteractableTarget 대응, IInteractableTarget.h).
 * 액터나 컴포넌트가 구현한다 — 찾을 때는 JY_FindInterface
 */
class IJY_InteractableTarget
{
	GENERATED_BODY()

public:
	/* 화면 아래 "[E] 이름" 안내에 쓰는 이름 */
	virtual FText GetInteractionName() const = 0;

	/* 비어 있지 않으면 쓰기 전에 확인 팝업을 띄움 (예: 던전으로 출발할까요?) */
	virtual FText GetConfirmText() const = 0;

	/* 이 대상이 제공하는 서비스 (제단 = Service.MetaAltar) — 서버가 구매·환불 요청을 받은 대상이 그 서비스를 하는지 확인 */
	virtual FGameplayTag GetServiceTag() const = 0;

	/* [Server] 이 거리 안이면 사용 가능 — 서버가 요청을 믿기 전에 다시 확인 */
	virtual bool IsWithinInteractRange(const AActor* User) const = 0;

	/* [OwnClient] 쓴 플레이어 화면에서만 하는 일 (화면 열기) */
	virtual void InteractLocal(APawn* User) = 0;

	/* [Server] 거리 검증을 통과한 뒤 서버에서 하는 일 (맵 이동) */
	virtual void InteractServer(APawn* User) = 0;
};
