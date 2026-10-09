#include "Interaction/JY_InteractableActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "UI/Foundation/JY_ScreenWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_InteractableActor)

/*
 * 쿼터뷰라 라인트레이스(Lyra 1인칭 방식) 대신 범위 겹침으로 대상을 잡는다.
 * 범위(InteractionRange)는 물체마다 크기·모양이 달라 물체가 갖고, 겹침 이벤트는 플레이어의 UJY_InteractionComponent가 받는다.
 * 복제 + 처음부터 휴면(DORM_Initial):
 *   - Server RPC 인자로 넘기려면 서버·클라가 같은 액터를 가리켜야 한다. 레벨 배치 액터는 이름으로 맞춰지지만,
 *     방을 깬 뒤 서버가 스폰하는 문·보상 상자는 복제돼야 클라에 생기고 RPC 인자로 쓸 수 있다.
 *   - 휴면이라 배치 액터는 복제 비용이 0 (DORM_Initial은 배치 액터에만 적용, NetDriver.cpp:8559).
 *     스폰하는 쪽은 한 번 보낸 뒤 SetNetDormancy(DORM_DormantAll)로 재운다. 복제 상태를 추가하면 바꿀 때 FlushNetDormancy.
 * 주의: 레벨 스트리밍으로 들어온 방의 액터는 시작 시 겹침 이벤트가 없다(bGenerateOverlapEventsDuringLevelStreaming 기본 false)
 *   → 방 흐름을 만들 때 폰 위치에 스폰되는 대상의 첫 겹침을 처리해야 한다.
 */
AJY_InteractableActor::AJY_InteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	NetDormancy = DORM_Initial;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
	InteractionRange->SetupAttachment(Mesh);
	InteractionRange->SetSphereRadius(250.0f);
	InteractionRange->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

// [Server] [OwnClient]
void AJY_InteractableActor::BeginPlay() //override
{
	Super::BeginPlay();

	/*
	 * E를 누를 때 처음 로드하면 그 프레임이 멈춘다 → 맵 시작 때 비동기로 미리 로드해 두고, E에서는 LoadSynchronous가 바로 돌려준다.
	 * 데디 서버는 화면을 열지 않으므로 로드하지 않는다.
	 */
	if (InteractionType == EJY_InteractionType::OpenScreen && ScreenClass.IsNull() == false && IsNetMode(NM_DedicatedServer) == false)
	{
		ScreenPreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ScreenClass.ToSoftObjectPath());
	}
}

// [Server]
bool AJY_InteractableActor::IsWithinInteractRange(const AActor* User) const
{
	if (User == nullptr)
		return false;

	const float MaxDistance = InteractionRange->GetScaledSphereRadius() + ServerRangeTolerance;
	return FVector::DistSquared(User->GetActorLocation(), InteractionRange->GetComponentLocation()) <= FMath::Square(MaxDistance);
}
