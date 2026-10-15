#include "UI/JY_CursorSubsystem.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "Framework/Application/SlateApplication.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_CursorSubsystem)

FJY_CursorInputProcessor::FJY_CursorInputProcessor(UJY_CursorSubsystem& InOwner)
	: Owner(&InOwner)
{
}

/*
 * 버튼 위든 HUD든 화면 어디서 눌러도 같은 반응.
 * false 반환 = 이벤트를 먹지 않음 → 클릭은 원래대로 버튼·게임에 간다.
 */
// [OwnClient]
bool FJY_CursorInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) //override
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Owner.IsValid() == true)
	{
		Owner->ShowPressedCursor(true);
	}
	return false;
}

/* 빠르게 두 번 누르면 두 번째 누름은 Down이 아니라 더블클릭으로 온다(SlateApplication.cpp:6041) — 같은 누름으로 처리 */
// [OwnClient]
bool FJY_CursorInputProcessor::HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) //override
{
	return HandleMouseButtonDownEvent(SlateApp, MouseEvent);
}

// [OwnClient]
bool FJY_CursorInputProcessor::HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) //override
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Owner.IsValid() == true)
	{
		Owner->ShowPressedCursor(false);
	}
	return false;
}

/* 로컬 플레이어가 생길 때(클라·에디터 PIE) — 데디 서버엔 로컬 플레이어가 없어 만들어지지 않는다 */
// [OwnClient]
void UJY_CursorSubsystem::Initialize(FSubsystemCollectionBase& Collection) //override
{
	Super::Initialize(Collection);

	InputProcessor = MakeShared<FJY_CursorInputProcessor>(*this);
	FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
}

// [OwnClient]
void UJY_CursorSubsystem::Deinitialize() //override
{
	FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	InputProcessor.Reset();

	Super::Deinitialize();
}

/*
 * 맵마다 새 PC가 붙을 때 불린다 — 이때는 뷰포트가 있어 커서를 바꿀 수 있다.
 * 엔진이 경로별로 한 번 읽어 캐시하므로(GameViewportClient.cpp:4785) 두 번째 맵부터는 파일을 읽지 않는다.
 */
// [OwnClient]
void UJY_CursorSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController) //override
{
	Super::PlayerControllerChanged(NewPlayerController);

	ApplyCursor(false);
}

// [OwnClient]
void UJY_CursorSubsystem::ShowPressedCursor(bool bPressed)
{
	if (bPressed == bShowingPressedCursor)
		return;

	bShowingPressedCursor = bPressed;
	ApplyCursor(bPressed);
}

// [OwnClient]
void UJY_CursorSubsystem::ApplyCursor(bool bPressed)
{
	UGameViewportClient* ViewportClient = GetLocalPlayer()->ViewportClient;
	if (ViewportClient == nullptr)
		return;

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	if (GameData == nullptr)
		return;

	const FJY_CursorData& CursorData = GameData->Cursor;
	const FName ImagePath = bPressed == true ? CursorData.PressedImagePath : CursorData.NormalImagePath;
	for (const TEnumAsByte<EMouseCursor::Type> CursorShape : CursorData.Shapes)
	{
		ViewportClient->SetHardwareCursor(CursorShape, ImagePath, CursorData.ClickPoint);
	}
}
