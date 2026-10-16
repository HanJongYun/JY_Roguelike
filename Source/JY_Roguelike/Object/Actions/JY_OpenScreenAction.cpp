#include "Object/Actions/JY_OpenScreenAction.h"
#include "Object/JY_Object.h"
#include "System/JY_GameplayTags.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_OpenScreenAction)

/*
 * E를 누를 때 처음 로드하면 그 프레임이 멈춘다 → 맵 시작 때 비동기로 미리 로드해 두고, E에서는 LoadSynchronous가 바로 돌려준다.
 * 데디 서버는 화면을 열지 않으므로 로드하지 않는다
 */
// [Server] [Client]
void UJY_OpenScreenAction::Initialize(AJY_Object* Object) //override
{
	if (ScreenClass.IsNull() == true || Object->IsNetMode(NM_DedicatedServer) == true)
		return;

	ScreenPreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ScreenClass.ToSoftObjectPath());
}

// [OwnClient]
void UJY_OpenScreenAction::OnLocalUserActivate(AJY_Object* Object, APawn* User) //override
{
	/* 기획 실수(화면 미지정)를 조용히 넘기지 않는다 — 에디터에 어느 물체인지 띄우고 아무것도 안 함 */
	if (ensureMsgf(ScreenClass.IsNull() == false, TEXT("%s: Open Screen Action의 ScreenClass가 비어 있음"), *Object->GetName()) == false)
		return;

	const APlayerController* PC = User->GetController<APlayerController>();
	UJY_UIManagerSubsystem* UIManager = PC != nullptr ? ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(PC->GetLocalPlayer()) : nullptr;
	if (UIManager == nullptr)
		return;

	TSubclassOf<UJY_ScreenWidget> LoadedScreenClass = ScreenClass.LoadSynchronous();

	/* 화면이 켜지기 전에 "누가 열었나"를 넣는다 — 켜질 때(NativeOnActivated) 화면이 바로 쓸 수 있게 */
	auto SetSourceBeforeActivate = [Object](UJY_ScreenWidget& Screen) { Screen.SetSourceActor(Object); };
	UIManager->PushScreen(JYGameplayTags::UI_Layer_GameMenu, LoadedScreenClass, SetSourceBeforeActivate);
}
