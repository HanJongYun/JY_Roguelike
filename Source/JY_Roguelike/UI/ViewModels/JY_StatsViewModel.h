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
	/* ASC 체력 변화 구독 + 현재 값으로 한 번 갱신 */
	void BindAbilitySystem(UJY_AbilitySystemComponent* InAbilitySystemComponent);

	/* ASC 구독 해제 */
	void UnbindAbilitySystem();

private:
	/* ASC 체력·최대 체력이 바뀌었을 때 */
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);

	/* ASC의 현재 체력·최대 체력으로 화면용 값 5개를 다시 계산 */
	void RefreshHealthValues();

private:
	/* 델리게이트 */

	FDelegateHandle HealthChangedHandle;

	FDelegateHandle MaxHealthChangedHandle;

private:
	/*
	 * 위젯이 바인딩하는 값 (FieldNotify — 값이 바뀌면 위젯이 자동 갱신)
	 * WBP_JY_HUD 체력 구슬이 HealthPercent·HealthText 사용
	 */

	/* 현재 체력 */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Category = "JY|Health", meta = (AllowPrivateAccess))
	float CurrentHealth = 0.0f;

	/* 최대 체력 */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Category = "JY|Health", meta = (AllowPrivateAccess))
	float MaxHealth = 0.0f;

	/* 체력 비율 0~1 (체력 구슬 FillPercent) */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Category = "JY|Health", meta = (AllowPrivateAccess))
	float HealthPercent = 0.0f;

	/* "90 / 100" 형태 글자 */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Category = "JY|Health", meta = (AllowPrivateAccess))
	FText HealthText;

	/* 체력이 기준 비율 이하인지 */
	UPROPERTY(BlueprintReadOnly, Transient, FieldNotify, Category = "JY|Health", meta = (AllowPrivateAccess))
	bool bLowHealth = false;

private:
	/* 설정값 — 지금은 C++ 기본값을 그대로 쓴다 */

	/* 낮은 체력 기준 비율 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Health", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthRatio = 0.3f;

private:
	/* 구독 중인 ASC — PlayerState가 소유하므로 약한 참조 */
	TWeakObjectPtr<UJY_AbilitySystemComponent> AbilitySystemComponent;

	/* 마지막으로 글자를 만든 반올림 값 (-1 = 아직 없음) */
	int32 DisplayedCurrentHealth = INDEX_NONE;

	int32 DisplayedMaxHealth = INDEX_NONE;
};
