#include "Skill/JY_SkillLoadoutComponent.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility_Skill.h"
#include "Skill/JY_SkillData.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_SkillLoadoutComponent)

UJY_SkillLoadoutComponent::UJY_SkillLoadoutComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/* 빙의·교체 때만 동작 */
	PrimaryComponentTick.bCanEverTick = false;
}

// [Server]
void UJY_SkillLoadoutComponent::GiveStartSkills(UJY_AbilitySystemComponent* AbilitySystemComponent)
{
	const UJY_SkillData* SkillData = GetSkillData();
	Slots.SetNum(SkillData->SlotInputTags.Num());

	const int32 StartSkillCount = FMath::Min(SkillData->StartSlotSkills.Num(), Slots.Num());
	for (int32 SlotIndex = 0; SlotIndex < StartSkillCount; ++SlotIndex)
	{
		SetSlotSkill(AbilitySystemComponent, SlotIndex, SkillData->StartSlotSkills[SlotIndex]);
	}

	for (const FJY_FixedSkill& FixedSkill : SkillData->FixedSkills)
	{
		const FGameplayAbilitySpecHandle Handle = GiveSkillAbility(AbilitySystemComponent, FixedSkill.SkillTag, FixedSkill.InputTag);
		FixedSkillHandles.Add(Handle);
	}
}

// [Server]
/* 실행 중인 스킬은 엔진이 끝낸다 (ClearAbility) */
void UJY_SkillLoadoutComponent::RemoveAllSkills(UJY_AbilitySystemComponent* AbilitySystemComponent)
{
	for (FJY_SkillSlotEntry& Slot : Slots)
	{
		AbilitySystemComponent->ClearAbility(Slot.GrantedHandle);
	}
	Slots.Reset();

	for (const FGameplayAbilitySpecHandle& Handle : FixedSkillHandles)
	{
		AbilitySystemComponent->ClearAbility(Handle);
	}
	FixedSkillHandles.Reset();
}

// [Server]
void UJY_SkillLoadoutComponent::SetSlotSkill(UJY_AbilitySystemComponent* AbilitySystemComponent, int32 SlotIndex, const FGameplayTag& SkillTag)
{
	if (ensureMsgf(Slots.IsValidIndex(SlotIndex) == true, TEXT("슬롯 %d 없음 (칸 %d)"), SlotIndex, Slots.Num()) == false)
		return;

	FJY_SkillSlotEntry& Slot = Slots[SlotIndex];
	AbilitySystemComponent->ClearAbility(Slot.GrantedHandle);

	const UJY_SkillData* SkillData = GetSkillData();
	const FGameplayTag& InputTag = SkillData->SlotInputTags[SlotIndex];
	Slot.SkillTag = SkillTag;
	Slot.GrantedHandle = GiveSkillAbility(AbilitySystemComponent, SkillTag, InputTag);
}

// [Server]
/* 기본 어빌리티 세트(UJY_AbilitySet::GiveToAbilitySystem)와 같은 입력 태그 방식 — 스킬 태그도 같은 자리에 심는다 */
FGameplayAbilitySpecHandle UJY_SkillLoadoutComponent::GiveSkillAbility(UJY_AbilitySystemComponent* AbilitySystemComponent, const FGameplayTag& SkillTag, const FGameplayTag& InputTag) const
{
	const UJY_SkillData* SkillData = GetSkillData();
	const FJY_SkillEntry* SkillEntry = SkillData->FindSkill(SkillTag);
	if (ensureMsgf(SkillEntry != nullptr && SkillEntry->AbilityClass != nullptr, TEXT("DA_JY_SkillData에 %s 칸 또는 AbilityClass가 없음"), *SkillTag.ToString()) == false)
		return FGameplayAbilitySpecHandle();

	FGameplayAbilitySpec AbilitySpec(SkillEntry->AbilityClass, 1, INDEX_NONE, GetOwner());
	AbilitySpec.GetDynamicSpecSourceTags().AddTag(SkillTag);
	if (InputTag.IsValid() == true)
	{
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(InputTag);
	}

	return AbilitySystemComponent->GiveAbility(AbilitySpec);
}

const UJY_SkillData* UJY_SkillLoadoutComponent::GetSkillData() const
{
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	check(GameData->SkillData != nullptr);
	return GameData->SkillData;
}
