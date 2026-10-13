#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Framework/Application/IInputProcessor.h"
#include "JY_CursorSubsystem.generated.h"

class UJY_CursorSubsystem;

/* 위젯·게임 입력보다 먼저 마우스 왼쪽 버튼을 보고 커서 서브시스템에 알림 (이벤트는 먹지 않음) */
class FJY_CursorInputProcessor : public IInputProcessor
{
public:
	explicit FJY_CursorInputProcessor(UJY_CursorSubsystem& InOwner);

public:
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;

private:
	TWeakObjectPtr<UJY_CursorSubsystem> Owner;
};

/* 커서를 칼 그림으로 바꾸고, 마우스 왼쪽 버튼을 누르고 있는 동안 금빛 칼로 바꾼다 — 게임·메뉴 어디서든 */
UCLASS()
class UJY_CursorSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

public:
	/* 누름 커서로 바꾸거나 평소 커서로 되돌림 (그림은 DA_JY_GameData의 Cursor 항목) */
	void ShowPressedCursor(bool bPressed);

private:
	/* 커서 모양마다 평소(false)·누름(true) 그림을 씌움 */
	void ApplyCursor(bool bPressed);

private:
	/* Slate 입력 전처리기 — 모든 위젯보다 먼저 마우스 버튼을 본다 */
	TSharedPtr<FJY_CursorInputProcessor> InputProcessor;

	/* 지금 누름 커서를 보여 주는 중인지 (같은 상태로 두 번 바꾸지 않게) */
	bool bShowingPressedCursor = false;
};
