#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

/* 액터가 구현했으면 액터, 아니면 컴포넌트에서 인터페이스를 찾는다 */
template <typename TInterface>
TInterface* JY_FindInterface(AActor* Actor)
{
	if (Actor == nullptr)
		return nullptr;

	if (TInterface* ActorInterface = Cast<TInterface>(Actor))
		return ActorInterface;

	return Actor->FindComponentByInterface<TInterface>();
}
