#include "Interaction/JY_PortalActor.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_PortalActor)

/* 서버 동작형은 OnServerInteract 재정의와 짝이라 C++에서 정한다 — BP에서 실수로 바꿀 수 없게 */
AJY_PortalActor::AJY_PortalActor()
{
	InteractionType = EJY_InteractionType::ServerAction;
}

/*
 * 서버가 맵을 바꾸면 클라들이 따라온다. 솔로 게임이라 누가 눌렀는지와 무관하게 서버 전체가 이동한다.
 * hard travel 전제(seamless 끔) — 새 맵에서는 PC·PlayerState가 새로 생긴다.
 * 허브↔던전만 맵 이동. 던전 안 방 이동은 같은 맵의 서브레벨 스트리밍이다.
 */
// [Server]
void AJY_PortalActor::OnServerInteract(APlayerController* User) //override
{
	if (DestinationMap.IsNull() == true)
		return;

	GetWorld()->ServerTravel(DestinationMap.GetLongPackageName());
}
