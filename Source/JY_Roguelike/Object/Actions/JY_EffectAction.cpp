#include "Object/Actions/JY_EffectAction.h"
#include "Object/JY_Object.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_EffectAction)

// [Client]
void UJY_EffectAction::OnClientEnter(AJY_Object* Object, bool bCatchUp) //override
{
	if (EffectPartTag.IsNone() == false)
	{
		UNiagaraComponent* EffectPart = FindPart<UNiagaraComponent>(Object, EffectPartTag);
		if (EffectPart != nullptr)
		{
			EffectPart->Activate(true);
		}
	}

	if (SpawnSystem == nullptr || bCatchUp == true)
		return;

	/* 시스템이 컬링되면 nullptr */
	SpawnedEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(Object, SpawnSystem, Object->GetActorLocation());
}

// [Client]
void UJY_EffectAction::OnClientExit(AJY_Object* Object) //override
{
	if (EffectPartTag.IsNone() == false)
	{
		UNiagaraComponent* EffectPart = FindPart<UNiagaraComponent>(Object, EffectPartTag);
		if (EffectPart != nullptr)
		{
			EffectPart->Deactivate();
		}
	}

	if (bStopSpawnedOnExit == true && SpawnedEffect != nullptr)
	{
		SpawnedEffect->Deactivate();
	}
	SpawnedEffect = nullptr;
}
