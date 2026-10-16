#include "Object/Actions/JY_TravelAction.h"
#include "Object/JY_Object.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_TravelAction)

/*
 * 서버가 맵을 바꾸면 클라들이 따라온다. 솔로 게임이라 누가 썼는지와 무관하게 서버 전체가 이동한다.
 * GameMode가 심리스 이동이라 접속은 유지되고, 도착 맵에서 PC·PlayerState는 새로 생긴다.
 * 로비↔던전만 맵 이동. 던전 안 방 이동은 같은 맵의 서브레벨 스트리밍이다
 */
// [Server]
void UJY_TravelAction::OnServerEnter(AJY_Object* Object) //override
{
	if (DestinationMap.IsNull() == true)
		return;

	Object->GetWorld()->ServerTravel(DestinationMap.GetLongPackageName());
}
