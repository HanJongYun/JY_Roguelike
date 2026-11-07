#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "JY_SkillLoadoutComponent.generated.h"

class UJY_AbilitySystemComponent;
class UJY_SkillData;

/* 슬롯 한 칸 (서버만) */
USTRUCT()
struct FJY_SkillSlotEntry
{
	GENERATED_BODY()

	/* 비면 빈 슬롯 */
	UPROPERTY()
	FGameplayTag SkillTag;

	/* 지급한 어빌리티 */
	FGameplayAbilitySpecHandle GrantedHandle;
};

/* 캐릭터의 스킬 슬롯 — 빙의 때 스킬 데이터대로 배치·지급, 해제 때 회수, 슬롯 교체 (서버) */
UCLASS(ClassGroup = (JY))
class UJY_SkillLoadoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_SkillLoadoutComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	/* [Server] 시작 배치대로 슬롯 스킬과 고정 스킬 지급 */
	void GiveStartSkills(UJY_AbilitySystemComponent* AbilitySystemComponent);

	/* [Server] 지급한 스킬 전부 회수 */
	void RemoveAllSkills(UJY_AbilitySystemComponent* AbilitySystemComponent);

	/* [Server] SlotIndex 칸에 스킬 넣기 (차 있으면 교체) */
	void SetSlotSkill(UJY_AbilitySystemComponent* AbilitySystemComponent, int32 SlotIndex, const FGameplayTag& SkillTag);

private:
	/* [Server] 스킬 GA 지급 (스펙에 스킬 태그·입력 태그를 심음). 실패하면 빈 핸들 */
	FGameplayAbilitySpecHandle GiveSkillAbility(UJY_AbilitySystemComponent* AbilitySystemComponent, const FGameplayTag& SkillTag, const FGameplayTag& InputTag) const;

	const UJY_SkillData* GetSkillData() const;

private:
	/* 슬롯 (개수 = 슬롯 입력 태그 수) */
	TArray<FJY_SkillSlotEntry> Slots;

	/* 고정 스킬 지급 핸들 */
	TArray<FGameplayAbilitySpecHandle> FixedSkillHandles;
};
