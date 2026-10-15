#include "AbilitySystem/Cues/JY_GameplayCueNotify_Hit.h"
#include "Enemy/JY_EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayCueNotify_Hit)

// [OwnClient] [SimulatedProxy] — 데디 서버에선 큐가 돌지 않는다(GameplayCueManager.cpp:178~183)
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

	/* 맞은 쪽이 Actor 적이면 반응 몽타주 (플레이어가 맞는 큐는 나중에 같은 클래스) */
	AJY_EnemyCharacter* Enemy = Cast<AJY_EnemyCharacter>(MyTarget);
	if (Enemy != nullptr)
	{
		const AActor* InstigatorActor = Parameters.GetInstigator();
		const FVector InstigatorLocation = InstigatorActor->GetActorLocation();
		Enemy->PlayHitReact(InstigatorLocation, Parameters.RawMagnitude);
	}
	return true;
}
