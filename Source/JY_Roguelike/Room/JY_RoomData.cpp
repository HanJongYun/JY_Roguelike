#include "Room/JY_RoomData.h"

#include "System/JY_GameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_RoomData)

bool UJY_RoomData::IsCombatRoom() const
{
	return RoomType.MatchesTag(JYGameplayTags::Room_Type_Combat);
}
