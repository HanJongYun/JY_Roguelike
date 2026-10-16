#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

/*
 * 인터페이스는 액터나 컴포넌트가 구현한다 (Lyra IInteractableTarget과 같음) — 액터 Cast 먼저, 없으면 컴포넌트에서 찾는다.
 * 컴포넌트는 BP에서 붙이므로 저장해 두지 않고 쓸 때마다 찾는다 (Actor.h:3852 FindComponentByInterface<T>)
 */
template <typename TInterface>
TInterface* JY_FindInterface(AActor* Actor)
{
	if (Actor == nullptr)
		return nullptr;

	if (TInterface* ActorInterface = Cast<TInterface>(Actor))
		return ActorInterface;

	return Actor->FindComponentByInterface<TInterface>();
}
