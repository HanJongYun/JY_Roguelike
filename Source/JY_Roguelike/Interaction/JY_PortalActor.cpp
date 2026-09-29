#include "Interaction/JY_PortalActor.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_PortalActor)

/*
 * 서버가 맵을 바꾸면 클라들이 따라온다. 새 맵에서는 PC가 새로 생겨 UI 틀이 다시 만들어지고(UJY_UIManagerSubsystem::PlayerControllerChanged),
 * ViewModel은 LocalPlayer에 남아 있다가 새 PlayerState·ASC에 다시 연결된다.
 * 솔로 게임이라 누가 눌렀는지와 무관하게 서버 전체가 이동한다.
 * hard travel 전제(seamless 끔) — seamless를 켜면 클라 PC가 유지돼 UI 틀 재생성 계기를 따로 추가해야 한다 (UJY_UIManagerSubsystem::PlayerControllerChanged 주석).
 * 허브↔던전만 맵 이동. 던전 안 방 이동은 같은 맵의 서브레벨 스트리밍이다.
 */
// [Server]
void AJY_PortalActor::OnServerInteract(APlayerController* User) //override
{
	if (DestinationMap.IsNull() == true)
		return;

	GetWorld()->ServerTravel(DestinationMap.GetLongPackageName());
}
