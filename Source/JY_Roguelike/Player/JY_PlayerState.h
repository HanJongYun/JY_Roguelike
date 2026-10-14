#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "Progress/JY_ProgressTypes.h"
#include "JY_PlayerState.generated.h"

class UAbilitySystemComponent;
class UJY_AbilitySystemComponent;
class UJY_HealthSet;
class UJY_DashSet;
class UJY_MetaUpgradeData;
class UJY_SkillLoadoutComponent;

/* 플레이어의 ASC와 체력 속성을 가진 PlayerState */
UCLASS()
class AJY_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AJY_PlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	//~IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface

public:
	UFUNCTION(BlueprintCallable, Category = "JY|PlayerState")
	UJY_AbilitySystemComponent* GetJYAbilitySystemComponent() const { return AbilitySystemComponent; }

	UJY_SkillLoadoutComponent* GetSkillLoadoutComponent() const { return SkillLoadoutComponent; }

	const UJY_HealthSet* GetHealthSet() const { return HealthSet; }

	const FJY_PlayerProgress& GetProgress() const { return Progress; }

	/* 진행 값 교체 (서버가 로드·구매·환불 뒤 호출) → 소유 클라에 복제 */
	void SetProgress(const FJY_PlayerProgress& NewProgress);

	/* 메타 강화 단계에 맞게 효과를 맞춘다 (새로 산 것 적용 · 단계 바뀐 것 크기만 갱신 · 0단계 제거) */
	void RefreshMetaEffects(const UJY_MetaUpgradeData* MetaUpgradeData);

private:
	UFUNCTION()
	void OnRep_Progress();

public:
	/* 델리게이트 */

	/* 진행 값이 바뀜 (서버: SetProgress / 소유 클라: 복제 도착) — 화면(영혼석 표시 등)이 구독 */
	FSimpleMulticastDelegate OnProgressChanged;

private:
	/* 복제 값 — 소유자에게만 (영혼석·메타는 남에게 보일 필요 없음). ReplicatedUsing = OnRep_Progress */

	/* 영혼석·기록·메타 단계 */
	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	FJY_PlayerProgress Progress;

private:
	/* 플레이어 능력·속성의 주인 */
	UPROPERTY(VisibleAnywhere, Category = "JY|PlayerState")
	TObjectPtr<UJY_AbilitySystemComponent> AbilitySystemComponent;

	/* 체력 속성 세트 (ASC가 InitializeComponent에서 찾아 등록) */
	UPROPERTY()
	TObjectPtr<const UJY_HealthSet> HealthSet;

	/* 대시 횟수 속성 세트 */
	UPROPERTY()
	TObjectPtr<const UJY_DashSet> DashSet;

	/* 이번 런 스킬 슬롯 — PlayerState에 있어 캐릭터가 바뀌어도 유지 */
	UPROPERTY(VisibleAnywhere, Category = "JY|PlayerState")
	TObjectPtr<UJY_SkillLoadoutComponent> SkillLoadoutComponent;

	/* 강화 종류별로 걸려 있는 메타 효과 (서버만) */
	TMap<FGameplayTag, FActiveGameplayEffectHandle> MetaEffectHandles;
};
