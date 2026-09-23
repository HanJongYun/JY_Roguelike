#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "JY_HealthViewModel.generated.h"

class UJY_AbilitySystemComponent;
struct FOnAttributeChangeData;

/* 체력바가 보여줄 값 (ASC 체력 변화를 받아 화면용으로 가공) */
UCLASS(BlueprintType, Blueprintable)
class UJY_HealthViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UJY_HealthViewModel();

public:
	virtual void PostInitProperties() override;

protected:
	virtual void BeginDestroy() override;

public:
	/* ASC 체력 변화 구독 + 현재 값으로 한 번 갱신 */
	void BindAbilitySystem(UJY_AbilitySystemComponent* InASC);

	/* 구독 해제 */
	void UnbindAbilitySystem();

	float GetCurrentHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetHealthPercent() const { return HealthPercent; }
	FText GetHealthText() const { return HealthText; }
	bool IsLowHealth() const { return bLowHealth; }
	FLinearColor GetHealthBarColor() const { return HealthBarColor; }

private:
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);

	/* 체력·최대 체력으로 나머지 표시 값 다시 계산 */
	void RefreshDisplayValues(float NewCurrentHealth, float NewMaxHealth);

private:
	/* 델리게이트 */

	FDelegateHandle HealthChangedHandle;

	FDelegateHandle MaxHealthChangedHandle;

private:
	/*
	 * WBP_JY_HUD가 바인딩하는 값 (FieldNotify — 값이 바뀌면 위젯이 자동 갱신)
	 * HealthPercent  → HealthBar(ProgressBar).Percent
	 * HealthText     → HealthText(TextBlock).Text
	 * HealthBarColor → HealthBar(ProgressBar).FillColorAndOpacity
	 * CurrentHealth·MaxHealth·bLowHealth는 아직 바인딩한 위젯 없음 (다른 화면용으로 열어둠)
	 */

	/* 현재 체력 */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Health", meta = (AllowPrivateAccess))
	float CurrentHealth = 0.0f;

	/* 최대 체력 */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Health", meta = (AllowPrivateAccess))
	float MaxHealth = 0.0f;

	/* 체력 비율 0~1 (체력바 Percent) */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Health", meta = (AllowPrivateAccess))
	float HealthPercent = 0.0f;

	/* "90 / 100" 형태 글자 */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Health", meta = (AllowPrivateAccess))
	FText HealthText;

	/* 체력이 기준 비율 이하인지 (색 바꾸기용) */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter = "IsLowHealth", Category = "JY|Health", meta = (AllowPrivateAccess))
	bool bLowHealth = false;

	/* 체력바 채움 색 (낮은 체력이면 LowHealthColor) */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Getter, Category = "JY|Health", meta = (AllowPrivateAccess))
	FLinearColor HealthBarColor = FLinearColor(ForceInit);

private:
	/*
	 * 설정값 — JY_HealthViewModel을 부모로 한 BP 자식의 Class Defaults에서 수정
	 * 그 BP를 프로젝트 설정 > Game > JY UI > HealthViewModelClass에 지정해야 적용된다 (비어 있으면 아래 C++ 기본값)
	 */

	/* 낮은 체력 기준 비율 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Health", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthRatio = 0.3f;

	/* 평소 체력바 색 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Health")
	FLinearColor NormalHealthColor = FLinearColor(0.1f, 0.8f, 0.2f, 1.0f);

	/* 낮은 체력일 때 체력바 색 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Health")
	FLinearColor LowHealthColor = FLinearColor(1.0f, 0.1f, 0.1f, 1.0f);

private:
	TWeakObjectPtr<UJY_AbilitySystemComponent> AbilitySystemComponent;
};
