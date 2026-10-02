#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/JY_ScreenWidget.h"
#include "JY_HUDLayout.generated.h"

class UProgressBar;
class UCommonTextBlock;
class UJY_AbilitySystemComponent;
class AJY_PlayerState;
struct FOnAttributeChangeData;

/* 게임 중 항상 떠 있는 HUD 화면 — 틀의 Game 층에 올라간다 */
UCLASS(Abstract, Blueprintable)
class UJY_HUDLayout : public UJY_ScreenWidget
{
	GENERATED_BODY()

public:
	UJY_HUDLayout(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeDestruct() override;

public:
	/* 체력을 표시할 ASC를 연결 — 체력이 바뀔 때마다 자동 갱신. 다시 불러도 안전 */
	void BindAbilitySystem(UJY_AbilitySystemComponent* InAbilitySystemComponent);

	/* 영혼석을 표시할 PlayerState를 연결 — 진행 값이 바뀔 때마다 자동 갱신. 다시 불러도 안전 */
	void BindProgress(AJY_PlayerState* InPlayerState);

private:
	/* 연결했던 ASC의 체력 알림 구독을 끊음 */
	void UnbindAbilitySystem();

	/* 체력·최대 체력이 바뀌었다는 ASC 알림을 받음 */
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);

	/* ASC의 현재 체력·최대 체력을 읽어 체력바와 글자에 표시 */
	void RefreshHealth();

	/* 연결했던 PlayerState의 진행 값 알림 구독을 끊음 */
	void UnbindProgress();

	/* PlayerState의 영혼석 수를 읽어 글자에 표시 */
	void RefreshSoulStones();

protected:
	/* WBP_JY_HUD 안의 같은 이름 위젯과 연결 (BindWidget) */

	/* 좌하단 체력바 묶음 칸 — SizeBox 380×44 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> CombatPanel;

	/* 체력 막대 — Percent = 현재 체력 / 최대 체력 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	/* 체력 글자 — "현재 / 최대" */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> HealthText;

	/* 우하단 영혼석 수 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> SoulText;

private:
	/* 체력을 읽어 오는 ASC — PlayerState 소유라 약한 참조 */
	TWeakObjectPtr<UJY_AbilitySystemComponent> AbilitySystemComponent;

	/* 구독 해제용 핸들 */
	FDelegateHandle HealthChangedHandle;

	FDelegateHandle MaxHealthChangedHandle;

	/* 영혼석을 읽어 오는 PlayerState — 맵 이동 때 바뀔 수 있어 약한 참조 */
	TWeakObjectPtr<AJY_PlayerState> ProgressPlayerState;

	FDelegateHandle ProgressChangedHandle;
};
