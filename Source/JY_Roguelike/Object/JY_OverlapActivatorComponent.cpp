#include "Object/JY_OverlapActivatorComponent.h"
#include "Object/JY_Object.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_OverlapActivatorComponent)

/*
 * 겹침은 서버만 판정한다 — 결과(상태)는 물체가 복제.
 * 액터 단위 겹침은 두 액터가 처음 닿을 때 한 번, 모두 떨어질 때 한 번 (PrimitiveComponent.cpp:4146~4165).
 * 주의: 스트리밍으로 들어온 방에선 시작 시 겹침 이벤트가 없다(bGenerateOverlapEventsDuringLevelStreaming 기본 false, :4131) — 시작 때 위에 선 폰은 다음 겹침부터 감지
 */
UJY_OverlapActivatorComponent::UJY_OverlapActivatorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// [Server] [Client]
void UJY_OverlapActivatorComponent::BeginPlay() //override
{
	Super::BeginPlay();

	AJY_Object* Object = GetOwner<AJY_Object>();
	if (Object == nullptr || Object->HasAuthority() == false)
		return;

	Object->OnActorBeginOverlap.AddDynamic(this, &ThisClass::HandleOwnerBeginOverlap);
	Object->OnStateChanged.AddUObject(this, &ThisClass::HandleStateChanged);
}

// [Server]
void UJY_OverlapActivatorComponent::HandleOwnerBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (Cast<APawn>(OtherActor) == nullptr)
		return;

	GetOwner<AJY_Object>()->RequestActivate(OtherActor);
}

// [Server]
void UJY_OverlapActivatorComponent::HandleStateChanged(EJY_ObjectState NewState)
{
	if (NewState != EJY_ObjectState::Idle)
		return;

	AJY_Object* Object = GetOwner<AJY_Object>();
	TArray<AActor*> OverlappingPawns;
	Object->GetOverlappingActors(OUT OverlappingPawns, APawn::StaticClass());
	if (OverlappingPawns.Num() == 0)
		return;

	Object->RequestActivate(OverlappingPawns[0]);
}
