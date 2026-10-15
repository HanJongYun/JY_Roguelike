#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Skill/JY_SkillTypes.h"
#include "JY_SkillLoadoutComponent.generated.h"

class UJY_AbilitySystemComponent;
class UJY_SkillDefinition;
class UJY_SkillUpgradeCard;

DECLARE_MULTICAST_DELEGATE(FJY_SkillLoadoutChanged);

/* 이번 런의 스킬 슬롯 — 획득·레벨·카드·어빌리티 지급(서버), 목록은 소유 클라에 복제 */
UCLASS(ClassGroup = (JY))
class UJY_SkillLoadoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_SkillLoadoutComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	/* [Server] 스킬 획득 — 이미 있으면 레벨 +1·카드 뽑기, 없으면 SlotIndex 칸에 장착(차 있으면 교체) */
	void AcquireSkill(UJY_SkillDefinition* Skill, int32 SlotIndex);

	/* [Server] 뽑아 둔 카드 중 하나 고르기 */
	void ChooseUpgradeCard(int32 ChoiceIndex);

	/* [Server] 슬롯 1칸 늘리기 (최대 = 슬롯 입력 태그 수) */
	void UnlockSlot();

	/* [Server] [OwnClient] 레벨·카드 반영 최종 수치 */
	FJY_SkillStats GetSkillStats(const UJY_SkillDefinition* Skill) const;

	const TArray<FJY_SkillSlotEntry>& GetSlots() const { return Slots; }

	const TArray<TObjectPtr<const UJY_SkillUpgradeCard>>& GetPendingCardChoices() const { return PendingCardChoices; }

private:
	/* [Server] 이 슬롯 스킬의 카드 후보를 섞어 선택지 만들기 */
	void DrawUpgradeCards(int32 SlotIndex);

	/* [Server] 슬롯 GA 지급 (입력 태그 = 슬롯 태그) */
	void GrantSlotAbility(int32 SlotIndex);

	/* [Server] 스킬 GA 지급 (입력 태그 심기, SourceObject = 스킬 정의). 실패하면 빈 핸들 */
	FGameplayAbilitySpecHandle GrantSkillAbility(UJY_SkillDefinition* Skill, FGameplayTag InputTag);

	/* [Server] 슬롯 GA 회수 (실행 중이면 엔진이 끝냄) */
	void RemoveSlotAbility(int32 SlotIndex);

	UJY_AbilitySystemComponent* GetAbilitySystemComponent() const;

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_PendingCardChoices();

public:
	/* 델리게이트 — 서버: 바꾼 직후 / 소유 클라: 복제 도착. HUD 슬롯 ViewModel이 구독 예정 */

	FJY_SkillLoadoutChanged OnSlotsChanged;

	FJY_SkillLoadoutChanged OnCardChoicesChanged;

private:
	/* 복제 값 — 소유자만 (COND_OwnerOnly) */

	/* 열린 슬롯 (개수 = 해금된 칸 수). ReplicatedUsing = OnRep_Slots */
	UPROPERTY(ReplicatedUsing = OnRep_Slots)
	TArray<FJY_SkillSlotEntry> Slots;

	/* 고를 강화 카드 (비면 고를 것 없음). ReplicatedUsing = OnRep_PendingCardChoices */
	UPROPERTY(ReplicatedUsing = OnRep_PendingCardChoices)
	TArray<TObjectPtr<const UJY_SkillUpgradeCard>> PendingCardChoices;

private:
	/* 카드를 고를 슬롯 (서버만) */
	int32 PendingCardSlotIndex = INDEX_NONE;
};
