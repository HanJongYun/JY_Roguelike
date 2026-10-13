#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "JY_CursorSubsystem.generated.h"

class FJY_CursorInputProcessor;

/* 마우스 왼쪽 버튼을 누르고 있는 동안 커서를 바꾼다 (강철 칼 → 금빛 칼) — 게임·메뉴 어디서든 */
UCLASS(Config = Game)
class UJY_CursorSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	/* 누름 커서로 바꾸거나 원래 커서(UserInterfaceSettings의 HardwareCursors)로 되돌림 */
	void ShowPressedCursor(bool bPressed);

private:
	/* 설정값 — Config/DefaultGame.ini [/Script/JY_Roguelike.JY_CursorSubsystem] */

	/* 누르고 있는 동안의 커서 (Content 기준 경로, 확장자 없이. 예: Slate/Cursors/SwordGold) */
	UPROPERTY(Config)
	FName PressedCursorPath;

private:
	/* Slate 입력 전처리기 — 모든 위젯보다 먼저 마우스 버튼을 본다 */
	TSharedPtr<FJY_CursorInputProcessor> InputProcessor;

	/* 지금 누름 커서를 보여 주는 중인지 (같은 상태로 두 번 바꾸지 않게) */
	bool bShowingPressedCursor = false;
};
