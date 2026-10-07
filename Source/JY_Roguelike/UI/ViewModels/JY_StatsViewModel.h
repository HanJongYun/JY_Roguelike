#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "JY_StatsViewModel.generated.h"

class UJY_AbilitySystemComponent;
struct FOnAttributeChangeData;

/* ASC 하나의 능력치를 화면용 값으로 가공 (지금은 체력, 이후 공격력 등 추가) */
UCLASS(BlueprintType, Blueprintable)
class UJY_StatsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UJY_StatsViewModel();

public:
	virtual void PostInitProperties() override;

public:
	/* ASC 체력 변화 구독 + 현재 값으로 한 번 갱신 */
	void BindAbilitySystem(UJY_AbilitySystemComponent* InAbilitySystemComponent);

	/* ASC 구독 해제 */
	void UnbindAbilitySystem();

	float GetCurrentHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetHealthPercent() const { return HealthPercent; }
	FText GetHealthText() const { return HealthText; }
	bool IsLowHealth() const { return bLowHealth; }
	FLinearColor GetHealthBarColor() const { return HealthBarColor; }

private:
	/* ASC 체력·최대 체력이 바뀌었을 때 */
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);

	/* ASC의 현재 체력·최대 체력으로 화면용 값 6개를 다시 계산 (조각 4) */
	void RefreshHealthValues();

private:
	/* 델리게이트 */

	FDelegateHandle HealthChangedHandle;

	FDelegateHandle MaxHealthChangedHandle;

private:
	/*
	 * 위젯이 바인딩하는 값 (FieldNotify — 값이 바뀌면 위젯이 자동 갱신)
	 * WBP_JY_HUD 체력바가 HealthPercent·HealthBarColor·HealthText 사용 (5-5에서 연결)
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
	 * 설정값 — 지금은 아래 C++ 기본값을 그대로 쓴다
	 * 에디터에서 바꿔야 할 때가 오면 DA_JY_GameData로 빼거나, 색은 WBP가 bLowHealth로 정하게 바꾼다
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
	/* 구독 중인 ASC — PlayerState가 소유하므로 약한 참조 */
	TWeakObjectPtr<UJY_AbilitySystemComponent> AbilitySystemComponent;

	/* 마지막으로 글자를 만든 반올림 값 (-1 = 아직 없음) — 정수가 바뀔 때만 글자를 새로 만든다 */
	int32 DisplayedCurrentHealth = INDEX_NONE;

	int32 DisplayedMaxHealth = INDEX_NONE;
};
