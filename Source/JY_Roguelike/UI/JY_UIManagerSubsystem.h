#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"
#include "JY_UIManagerSubsystem.generated.h"

class APlayerController;
class UJY_AbilitySystemComponent;
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

	/* 요청한 ViewModel을 플레이어당 하나로 꺼냄 (없으면 생성) — Resolver와 C++이 같은 인스턴스를 받는다 */
	UMVVMViewModelBase* GetOrCreateViewModel(TSubclassOf<UMVVMViewModelBase> ViewModelClass);

	template <typename T>
	T* GetOrCreateViewModel() { return Cast<T>(GetOrCreateViewModel(T::StaticClass())); }

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

	/* 키 = 요청한 C++ ViewModel 클래스 (값은 GameData 교체 표의 BP 자식일 수 있음) */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UMVVMViewModelBase>> ViewModels;
};
