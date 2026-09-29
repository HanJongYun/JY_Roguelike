#include "Interaction/JY_PortalActor.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_PortalActor)

AJY_PortalActor::AJY_PortalActor()
{
	/* 포털은 서버가 처리하는 대상 */
	InteractionType = EJY_InteractionType::ServerAction;
}

/*
 * 서버가 맵을 바꾸면 클라들이 따라온다. 새 맵에서는 PC가 새로 생겨 UI 틀이 다시 만들어지고(UJY_UIManagerSubsystem::PlayerControllerChanged),
 * ViewModel은 LocalPlayer에 남아 있다가 새 PlayerState·ASC에 다시 연결된다.
 * 솔로 게임이라 누가 눌렀는지와 무관하게 서버 전체가 이동한다.
 */
// [Server]
void AJY_PortalActor::OnServerInteract(APlayerController* User) //override
{
	if (DestinationMap.IsNull() == true)
		return;

	GetWorld()->ServerTravel(DestinationMap.GetLongPackageName());
}
