#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "JY_UIManagerSubsystem.generated.h"

class APlayerController;
class UJY_AbilitySystemComponent;
class UJY_HealthViewModel;
class UJY_PrimaryLayout;

/* 로컬 플레이어의 화면과 ViewModel을 관리 */
UCLASS()
class UJY_UIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

public:
	/* 체력 ViewModel (없으면 생성) */
	UFUNCTION(BlueprintCallable, Category = "JY|UI")
	UJY_HealthViewModel* GetHealthViewModel();

	/* ASC를 ViewModel에 연결 */
	void BindAbilitySystem(UJY_AbilitySystemComponent* InASC);

private:
	/* 화면 틀을 만들어 이 플레이어 화면에 붙임 */
	void CreatePrimaryLayout(APlayerController* PC);

	/* 화면 틀을 화면에서 떼고 버림 */
	void DestroyPrimaryLayout();

private:
	UPROPERTY(Transient)
	TObjectPtr<UJY_PrimaryLayout> PrimaryLayout;

	UPROPERTY(Transient)
	TObjectPtr<UJY_HealthViewModel> HealthViewModel;
};
