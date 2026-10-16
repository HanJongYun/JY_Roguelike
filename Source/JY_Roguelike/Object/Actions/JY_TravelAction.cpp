#include "Object/Actions/JY_TravelAction.h"
#include "Object/JY_Object.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_TravelAction)

/* 솔로 게임이라 누가 썼든 서버 전체가 이동 — 심리스라 접속은 유지되고 PC·PlayerState는 새로 생긴다 */
// [Server]
void UJY_TravelAction::OnServerEnter(AJY_Object* Object) //override
{
	if (DestinationMap.IsNull() == true)
		return;

	Object->GetWorld()->ServerTravel(DestinationMap.GetLongPackageName());
}
