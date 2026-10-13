#include "UI/JY_CursorSubsystem.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/UserInterfaceSettings.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_CursorSubsystem)

/*
 * 위젯·게임 입력보다 먼저 마우스 버튼을 본다 — 버튼 위든 HUD든 화면 어디서 눌러도 같은 반응.
 * 이벤트는 먹지 않는다(false 반환) → 클릭은 원래대로 버튼·게임에 간다.
 */
class FJY_CursorInputProcessor : public IInputProcessor
{
public:
	explicit FJY_CursorInputProcessor(UJY_CursorSubsystem& InOwner) : Owner(&InOwner) {}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Owner.IsValid() == true)
		{
			Owner->ShowPressedCursor(true);
		}
		return false;
	}

	/* 빠르게 두 번 누르면 두 번째 누름은 Down이 아니라 더블클릭으로 온다(SlateApplication.cpp:6041) — 같은 누름으로 처리 */
	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		return HandleMouseButtonDownEvent(SlateApp, MouseEvent);
	}

	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Owner.IsValid() == true)
		{
			Owner->ShowPressedCursor(false);
		}
		return false;
	}

private:
	TWeakObjectPtr<UJY_CursorSubsystem> Owner;
};

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
 * 커서 모양(Default·Hand 등)마다 그림을 바꿔 끼운다 — 엔진이 경로별로 한 번 읽어 캐시하므로(GameViewportClient.cpp:4785) 두 번째부터는 파일을 읽지 않는다.
 * 원래 그림·클릭 지점은 UserInterfaceSettings에서 그대로 가져와 설정이 한 곳에만 있다.
 */
// [OwnClient]
void UJY_CursorSubsystem::ShowPressedCursor(bool bPressed)
{
	if (bPressed == bShowingPressedCursor)
		return;

	bShowingPressedCursor = bPressed;

	UGameViewportClient* ViewportClient = GetLocalPlayer()->ViewportClient;
	if (ViewportClient == nullptr)
		return;

	const UUserInterfaceSettings* UISettings = GetDefault<UUserInterfaceSettings>();
	for (const TPair<TEnumAsByte<EMouseCursor::Type>, FHardwareCursorReference>& Entry : UISettings->HardwareCursors)
	{
		const FName CursorPath = bPressed == true ? PressedCursorPath : Entry.Value.CursorPath;
		ViewportClient->SetHardwareCursor(Entry.Key, CursorPath, Entry.Value.HotSpot);
	}
}
