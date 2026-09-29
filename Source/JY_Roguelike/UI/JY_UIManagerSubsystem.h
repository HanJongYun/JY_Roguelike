#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameplayTagContainer.h"
#include "JY_UIManagerSubsystem.generated.h"

class APlayerController;
class UJY_AbilitySystemComponent;
class UJY_StatsViewModel;
class UJY_PrimaryLayout;
class UJY_ScreenWidget;

/* 로컬 플레이어의 화면과 ViewModel을 관리 */
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

	/* 능력치 ViewModel (없으면 생성) */
	UFUNCTION(BlueprintCallable, Category = "JY|UI")
	UJY_StatsViewModel* GetStatsViewModel();

	/* ASC를 ViewModel에 연결 */
	void BindAbilitySystem(UJY_AbilitySystemComponent* InASC);

private:
	/* 화면 틀을 만들어 이 플레이어 화면에 붙임 */
	void CreatePrimaryLayout(APlayerController* PC);

	/* 화면 틀을 화면에서 떼고 버림 */
	void DestroyPrimaryLayout();

	/* 켜진 화면이 없을 때의 기본 입력(게임, 커서 표시)을 적용 */
	void ApplyDefaultInputConfig(APlayerController* PC);

private:
	UPROPERTY(Transient)
	TObjectPtr<UJY_PrimaryLayout> PrimaryLayout;

	UPROPERTY(Transient)
	TObjectPtr<UJY_StatsViewModel> StatsViewModel;
};
