#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "JY_UIManagerSubsystem.generated.h"

class APlayerController;
class UJY_AbilitySystemComponent;
class UJY_HealthViewModel;
class UJY_ScreenWidget;

/* 로컬 플레이어의 화면과 ViewModel을 관리 */
UCLASS()
class UJY_UIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	/* 체력 ViewModel (없으면 생성) */
	UFUNCTION(BlueprintCallable, Category = "JY|UI")
	UJY_HealthViewModel* GetHealthViewModel();

	/* HUD를 만들어 화면에 띄움 (이미 있으면 무시) */
	void ShowHUD(APlayerController* PC, TSubclassOf<UJY_ScreenWidget> HUDClass);

	/* ASC를 ViewModel에 연결 */
	void BindAbilitySystem(UJY_AbilitySystemComponent* InASC);

private:
	UPROPERTY(Transient)
	TObjectPtr<UJY_HealthViewModel> HealthViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UJY_ScreenWidget> HUDWidget;
};
