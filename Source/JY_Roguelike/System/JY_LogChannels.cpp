#include "System/JY_LogChannels.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY(LogJY);

/*
 * Lyra GetClientServerContextString(LyraLogChannels.cpp)은 Server/Client 두 가지만 구분.
 * 여기선 소유 클라와 남의 클라를 나눠야 해서 역할을 한 단계 더 쪼갠다.
 */
const TCHAR* JY_GetNetRoleTag(const AActor* Actor)
{
	if (Actor == nullptr)
		return TEXT("None");

	if (Actor->GetNetMode() == NM_Standalone)
		return TEXT("Standalone");

	if (Actor->HasAuthority() == true)
		return TEXT("Server");

	if (Actor->GetLocalRole() == ROLE_AutonomousProxy)
		return TEXT("OwnClient");

	/* PlayerState는 소유 클라에서도 SimulatedProxy. Owner(=PC)는 소유 클라에만 복제되므로 로컬 PC면 OwnClient */
	const APlayerState* PlayerState = Cast<APlayerState>(Actor);
	if (PlayerState != nullptr)
	{
		const APlayerController* PC = PlayerState->GetPlayerController();
		if (PC != nullptr && PC->IsLocalController() == true)
			return TEXT("OwnClient");
	}

	return TEXT("SimulatedProxy");
}
