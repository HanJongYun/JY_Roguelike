#include "Player/JY_TitlePlayerController.h"
#include "System/JY_TitleGameMode.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "System/JY_GameplayTags.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_TitlePlayerController)

/* 캐릭터가 없어 커서로만 조작 */
AJY_TitlePlayerController::AJY_TitlePlayerController()
{
	bShowMouseCursor = true;
}

/* 클라에선 SetPlayer(UI 틀 생성)가 BeginPlay보다 먼저라 여기선 틀이 이미 있다 (DataChannel.cpp:2921) */
// [OwnClient]
void AJY_TitlePlayerController::BeginPlay() //override
{
	Super::BeginPlay();

	if (IsLocalPlayerController() == false)
		return;

	UJY_UIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(GetLocalPlayer());
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	if (UIManager == nullptr || GameData == nullptr)
		return;

	const TSubclassOf<UJY_ScreenWidget> TitleScreenClass = GameData->TitleScreenClass.LoadSynchronous();
	if (ensureMsgf(TitleScreenClass != nullptr, TEXT("DA_JY_GameData의 TitleScreenClass가 비어 있음")) == false)
		return;

	UIManager->PushScreen(JYGameplayTags::UI_Layer_Menu, TitleScreenClass);
}

/* 이 PC는 AJY_TitleGameMode만 만든다 → 이 맵의 GameMode는 항상 타이틀 GameMode */
// [Server]
void AJY_TitlePlayerController::Server_StartGame_Implementation()
{
	AJY_TitleGameMode* TitleGameMode = CastChecked<AJY_TitleGameMode>(GetWorld()->GetAuthGameMode());
	TitleGameMode->StartGame();
}
