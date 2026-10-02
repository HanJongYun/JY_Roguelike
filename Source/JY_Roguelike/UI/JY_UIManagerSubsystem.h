#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameplayTagContainer.h"
#include "JY_UIManagerSubsystem.generated.h"

class APlayerController;
class AJY_PlayerState;
class UJY_GameData;
class UJY_HUDLayout;
class UJY_PrimaryLayout;
class UJY_ScreenWidget;

/* 로컬 플레이어의 화면을 관리 — 클라에만 있다(데디 서버엔 로컬 플레이어가 없음) */
UCLASS()
class UJY_UIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

public:
	/* 화면을 해당 층 스택 맨 위에 올림 (예: UI.Layer.Menu) */
	UJY_ScreenWidget* PushScreen(FGameplayTag LayerTag, TSubclassOf<UJY_ScreenWidget> ScreenClass);

	/* [UI 과외 임시] HUD에 체력·영혼석 원본을 연결 — 5장 ViewModel로 교체 */
	void BindHUDToPlayerState(AJY_PlayerState* PlayerState);

private:
	/* 화면 틀을 만들어 이 플레이어 화면에 붙이고 HUD까지 올림 */
	void CreatePrimaryLayout(APlayerController* PC);

	/* HUD를 Game 층에 올리고 PlayerState가 이미 있으면 바로 연결 */
	void PushHUD(APlayerController* PC, const UJY_GameData& GameData);

	/* 화면 틀을 화면에서 떼고 버림 */
	void DestroyPrimaryLayout();

private:
	UPROPERTY(Transient)
	TObjectPtr<UJY_PrimaryLayout> PrimaryLayout;

	/* 틀을 만든 PC — 맵 이동 후엔 위젯의 GetOwningPlayer가 새 PC를 돌려줘 비교에 못 쓴다 */
	TWeakObjectPtr<APlayerController> LayoutOwner;

	/* [UI 과외 임시] Game 층에 올린 HUD — 스택이 소유하므로 약한 참조 */
	TWeakObjectPtr<UJY_HUDLayout> HUDLayout;
};
