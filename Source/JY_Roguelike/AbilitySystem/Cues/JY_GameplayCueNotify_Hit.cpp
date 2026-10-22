#include "AbilitySystem/Cues/JY_GameplayCueNotify_Hit.h"
#include "Combat/JY_HitReactionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayCueNotify_Hit)

// [OwnClient] [SimulatedProxy] — 데디 서버에선 큐가 돌지 않는다(GameplayCueManager.cpp:178)
/* C++ 정적 큐는 큐 매니저가 경로의 BP 에셋만 찾으므로 GCN_JY_Hit_* BP 껍데기가 큐 태그를 갖는다 */
bool UJY_GameplayCueNotify_Hit::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const //override
{
	UWorld* World = MyTarget->GetWorld();
	const FVector HitLocation = Parameters.Location;

	if (HitEffect != nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, HitEffect, HitLocation);
	}

	if (HitSounds.Num() > 0)
	{
		const int32 SoundIndex = FMath::RandRange(0, HitSounds.Num() - 1);
		UGameplayStatics::PlaySoundAtLocation(World, HitSounds[SoundIndex], HitLocation);
	}

	/* 반응은 맞은 쪽이 정한다 — 함정은 때린 쪽(Instigator)이 없어 방향은 Normal로 받는다 */
	UJY_HitReactionComponent* HitReaction = MyTarget->FindComponentByClass<UJY_HitReactionComponent>();
	if (HitReaction != nullptr)
	{
		FJY_HitReactionParams ReactionParams;
		ReactionParams.HitDirection = Parameters.Normal;
		ReactionParams.ReactionTag = HitReactionTag;
		ReactionParams.Damage = Parameters.RawMagnitude;
		ReactionParams.HitStopSeconds = HitStopSeconds;
		ReactionParams.CameraShake = VictimCameraShake;
		HitReaction->PlayHitReaction(ReactionParams);
	}
	return true;
}
