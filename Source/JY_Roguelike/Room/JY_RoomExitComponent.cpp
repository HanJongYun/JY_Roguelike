#include "Room/JY_RoomExitComponent.h"
#include "Room/JY_RoomFlowSubsystem.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_RoomExitComponent)

namespace JY_RoomExit
{
	static const FName PassTriggerTag(TEXT("PassTrigger"));
	static const FName BarrierTag(TEXT("Barrier"));
	static const FName GateTag(TEXT("Gate"));
	static const FName OpenLightTag(TEXT("OpenLight"));
}

/*
 * 문은 레벨에 놓인 물체라 처음엔 잠긴 상태로 서버·클라 양쪽에 같이 로드된다.
 * 열림 상태만 복제 — 물체(AJY_Object)가 처음부터 휴면이라 바꿀 때만 FlushNetDormancy
 */
UJY_RoomExitComponent::UJY_RoomExitComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UJY_RoomExitComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UJY_RoomExitComponent, bOpen);
}

template <typename T>
T* UJY_RoomExitComponent::FindPart(FName PartTag) const
{
	TArray<UActorComponent*> Parts = GetOwner()->GetComponentsByTag(T::StaticClass(), PartTag);
	ensureMsgf(Parts.Num() > 0, TEXT("%s: 태그 %s인 부품이 없음"), *GetOwner()->GetName(), *PartTag.ToString());
	return Parts.Num() > 0 ? Cast<T>(Parts[0]) : nullptr;
}

/* 방 레벨은 보이게 된 뒤(BeginPlay 끝) 방 흐름이 들어온다 → 알림을 먼저 구독하고, 이미 깬 방(재접속 등)이면 바로 연다 */
// [Server] [Client]
void UJY_RoomExitComponent::BeginPlay() //override
{
	Super::BeginPlay();

	ApplyOpenState();

	if (GetOwner()->HasAuthority() == false)
		return;

	UPrimitiveComponent* PassTrigger = FindPart<UPrimitiveComponent>(JY_RoomExit::PassTriggerTag);
	if (PassTrigger != nullptr)
	{
		PassTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandlePassBeginOverlap);
	}

	UJY_RoomFlowSubsystem* RoomFlow = GetWorld()->GetSubsystem<UJY_RoomFlowSubsystem>();
	if (RoomFlow == nullptr)
		return;

	RoomClearChangedHandle = RoomFlow->OnRoomClearChanged.AddUObject(this, &ThisClass::HandleRoomClearChanged);
	SetOpen(RoomFlow->IsRoomCleared(GetOwner()->GetLevel()));
}

// [Server] [Client]
void UJY_RoomExitComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) //override
{
	UJY_RoomFlowSubsystem* RoomFlow = GetWorld()->GetSubsystem<UJY_RoomFlowSubsystem>();
	if (RoomFlow != nullptr)
	{
		RoomFlow->OnRoomClearChanged.Remove(RoomClearChangedHandle);
	}

	Super::EndPlay(EndPlayReason);
}

// [Server]
void UJY_RoomExitComponent::HandleRoomClearChanged(const ULevel* RoomLevel, bool bCleared)
{
	if (RoomLevel != GetOwner()->GetLevel())
		return;

	SetOpen(bCleared);
}

// [Server]
void UJY_RoomExitComponent::SetOpen(bool bNewOpen)
{
	if (bOpen == bNewOpen)
		return;

	/* 휴면 액터는 깨워야 바뀐 값이 나간다 */
	GetOwner()->FlushNetDormancy();
	bOpen = bNewOpen;

	/* 서버는 OnRep이 안 불리므로 직접 */
	ApplyOpenState();
}

// [Server]
void UJY_RoomExitComponent::HandlePassBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	/* 적·소환물이 문에 닿아도 넘어가지 않게 */
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn == nullptr || Pawn->IsPlayerControlled() == false)
		return;

	UJY_RoomFlowSubsystem* RoomFlow = GetWorld()->GetSubsystem<UJY_RoomFlowSubsystem>();
	RoomFlow->RequestNextRoom();
}

// [Client]
void UJY_RoomExitComponent::OnRep_Open()
{
	ApplyOpenState();
}

/*
 * 통과 감지는 열렸을 때만 켠다 — 잠긴 문에 붙어 서 있다가 열리면, 켜는 순간 겹침을 다시 계산해 이미 안에 있는 캐릭터도 "들어옴"으로 잡힌다
 * (SetCollisionEnabled → OnComponentCollisionSettingsChanged → UpdateOverlaps, PrimitiveComponentPhysics.cpp:1385·1446).
 * 계속 켜 두면 잠겼을 때 받은 "들어옴"이 무시된 뒤 다시 오지 않아, 열린 뒤 그대로 문 밖 허공으로 걸어 나간다.
 */
// [Server] [Client]
void UJY_RoomExitComponent::ApplyOpenState()
{
	if (UPrimitiveComponent* PassTrigger = FindPart<UPrimitiveComponent>(JY_RoomExit::PassTriggerTag))
	{
		PassTrigger->SetCollisionEnabled(bOpen == true ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}

	if (UPrimitiveComponent* Barrier = FindPart<UPrimitiveComponent>(JY_RoomExit::BarrierTag))
	{
		Barrier->SetCollisionEnabled(bOpen == true ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}

	if (USceneComponent* Gate = FindPart<USceneComponent>(JY_RoomExit::GateTag))
	{
		Gate->SetVisibility(bOpen == false);
	}

	if (USceneComponent* OpenLight = FindPart<USceneComponent>(JY_RoomExit::OpenLightTag))
	{
		OpenLight->SetVisibility(bOpen == true);
	}
}
