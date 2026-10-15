#include "Skill/JY_SkillLoadoutComponent.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/JY_GameplayAbility_Skill.h"
#include "Player/JY_PlayerState.h"
#include "Skill/JY_SkillDefinition.h"
#include "Skill/JY_SkillUpgradeCard.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_SkillLoadoutComponent)

UJY_SkillLoadoutComponent::UJY_SkillLoadoutComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/* 이벤트로만 동작 */
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UJY_SkillLoadoutComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	/* 남의 슬롯은 볼 일이 없다 */
	DOREPLIFETIME_CONDITION(UJY_SkillLoadoutComponent, Slots, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UJY_SkillLoadoutComponent, PendingCardChoices, COND_OwnerOnly);
}

// [Server]
void UJY_SkillLoadoutComponent::BeginPlay() //override
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority() == false)
		return;

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const int32 StartSlotCount = FMath::Max(GameData->StartSkillSlotCount, GameData->StartSkills.Num());
	Slots.SetNum(StartSlotCount);

	for (int32 SlotIndex = 0; SlotIndex < GameData->StartSkills.Num(); ++SlotIndex)
	{
		UJY_SkillDefinition* StartSkill = GameData->StartSkills[SlotIndex];
		if (StartSkill != nullptr)
		{
			AcquireSkill(StartSkill, SlotIndex);
		}
	}

	/* 고정 스킬은 슬롯·레벨·카드가 없어 핸들을 들고 있지 않는다 — PlayerState(ASC)와 수명이 같다 */
	for (const FJY_FixedSkill& FixedSkill : GameData->FixedSkills)
	{
		GrantSkillAbility(FixedSkill.Skill, FixedSkill.InputTag);
	}

	OnSlotsChanged.Broadcast();
}

// [Server]
void UJY_SkillLoadoutComponent::AcquireSkill(UJY_SkillDefinition* Skill, int32 SlotIndex)
{
	auto IsSameSkill = [Skill](const FJY_SkillSlotEntry& Slot)
	{
		return Slot.Skill == Skill;
	};
	const int32 OwnedSlotIndex = Slots.IndexOfByPredicate(IsSameSkill);

	/* 중복 획득 = 레벨 +1 + 카드 선택 (사용자 결정) */
	if (OwnedSlotIndex != INDEX_NONE)
	{
		FJY_SkillSlotEntry& OwnedSlot = Slots[OwnedSlotIndex];
		const int32 MaxLevel = Skill->GetMaxLevel();
		OwnedSlot.Level = FMath::Min(OwnedSlot.Level + 1, MaxLevel);
		DrawUpgradeCards(OwnedSlotIndex);
		OnSlotsChanged.Broadcast();
		return;
	}

	if (ensureMsgf(Slots.IsValidIndex(SlotIndex) == true, TEXT("슬롯 %d 없음 (열린 칸 %d)"), SlotIndex, Slots.Num()) == false)
		return;

	/* 교체되는 슬롯의 카드 선택은 무효 */
	if (PendingCardSlotIndex == SlotIndex)
	{
		PendingCardChoices.Reset();
		PendingCardSlotIndex = INDEX_NONE;
		OnCardChoicesChanged.Broadcast();
	}

	RemoveSlotAbility(SlotIndex);

	FJY_SkillSlotEntry& Slot = Slots[SlotIndex];
	Slot.Skill = Skill;
	Slot.Level = 1;
	Slot.Cards.Reset();
	GrantSlotAbility(SlotIndex);
	OnSlotsChanged.Broadcast();
}

// [Server]
/* 이미 고른 카드는 다시 나오지 않는다 */
void UJY_SkillLoadoutComponent::DrawUpgradeCards(int32 SlotIndex)
{
	const FJY_SkillSlotEntry& Slot = Slots[SlotIndex];

	TArray<TObjectPtr<const UJY_SkillUpgradeCard>> Candidates;
	for (const TObjectPtr<const UJY_SkillUpgradeCard>& Card : Slot.Skill->UpgradeCards)
	{
		if (Slot.Cards.Contains(Card) == false)
		{
			Candidates.Add(Card);
		}
	}

	/* 피셔-예이츠 섞기 */
	for (int32 CandidateIndex = Candidates.Num() - 1; CandidateIndex > 0; --CandidateIndex)
	{
		const int32 SwapIndex = FMath::RandRange(0, CandidateIndex);
		Candidates.Swap(CandidateIndex, SwapIndex);
	}

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const int32 ChoiceCount = FMath::Min(GameData->SkillCardChoiceCount, Candidates.Num());
	Candidates.SetNum(ChoiceCount);

	PendingCardChoices = MoveTemp(Candidates);
	PendingCardSlotIndex = PendingCardChoices.Num() > 0 ? SlotIndex : INDEX_NONE;
	OnCardChoicesChanged.Broadcast();
}

// [Server] 클라 요청(디버그 명령·이후 카드 UI)이 그대로 들어오므로 범위 검사
void UJY_SkillLoadoutComponent::ChooseUpgradeCard(int32 ChoiceIndex)
{
	if (PendingCardChoices.IsValidIndex(ChoiceIndex) == false)
		return;

	FJY_SkillSlotEntry& Slot = Slots[PendingCardSlotIndex];
	Slot.Cards.Add(PendingCardChoices[ChoiceIndex]);

	PendingCardChoices.Reset();
	PendingCardSlotIndex = INDEX_NONE;
	OnSlotsChanged.Broadcast();
	OnCardChoicesChanged.Broadcast();
}

// [Server]
/* 입력 태그가 없는 칸은 누를 수 없으므로 태그 수가 상한 — 슬롯 추가 = IA·IMC·태그·배열 1칸, 코드 0 */
void UJY_SkillLoadoutComponent::UnlockSlot()
{
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	if (Slots.Num() >= GameData->SkillSlotInputTags.Num())
		return;

	Slots.AddDefaulted();
	OnSlotsChanged.Broadcast();
}

// [Server] [OwnClient]
/*
 * 피해·범위 카드는 더해서 한 번에 곱한다(20% 두 장 = +40%).
 * 슬롯에 없으면 1레벨 값 — 고정 스킬(FixedSkills), 그리고 클라에 슬롯 복제가 GA 스펙보다 늦게 온 경우(서버 쿨타임 GE가 복제로 덮어쓴다)
 */
FJY_SkillStats UJY_SkillLoadoutComponent::GetSkillStats(const UJY_SkillDefinition* Skill) const
{
	auto IsSameSkill = [Skill](const FJY_SkillSlotEntry& Slot)
	{
		return Slot.Skill == Skill;
	};
	const FJY_SkillSlotEntry* Slot = Slots.FindByPredicate(IsSameSkill);
	const int32 Level = Slot != nullptr ? Slot->Level : 1;
	const FJY_SkillLevelData& LevelData = Skill->GetLevelData(Level);

	float DamagePercent = 0.0f;
	float RadiusPercent = 0.0f;
	float CooldownPercent = 0.0f;
	if (Slot != nullptr)
	{
		for (const TObjectPtr<const UJY_SkillUpgradeCard>& Card : Slot->Cards)
		{
			switch (Card->Kind)
			{
			case EJY_SkillUpgradeKind::DamagePercent:
				DamagePercent += Card->Percent;
				break;
			case EJY_SkillUpgradeKind::RadiusPercent:
				RadiusPercent += Card->Percent;
				break;
			case EJY_SkillUpgradeKind::CooldownPercent:
				CooldownPercent += Card->Percent;
				break;
			}
		}
	}

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const float CooldownScale = 1.0f - CooldownPercent / 100.0f;

	FJY_SkillStats Stats;
	Stats.DamageScale = LevelData.DamageScale * (1.0f + DamagePercent / 100.0f);
	Stats.RadiusScale = LevelData.RadiusScale * (1.0f + RadiusPercent / 100.0f);
	Stats.Cooldown = FMath::Max(LevelData.Cooldown * CooldownScale, GameData->MinSkillCooldown);
	return Stats;
}

// [Server]
void UJY_SkillLoadoutComponent::GrantSlotAbility(int32 SlotIndex)
{
	FJY_SkillSlotEntry& Slot = Slots[SlotIndex];

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	FGameplayTag InputTag;
	if (GameData->SkillSlotInputTags.IsValidIndex(SlotIndex) == true)
	{
		InputTag = GameData->SkillSlotInputTags[SlotIndex];
	}

	Slot.GrantedHandle = GrantSkillAbility(Slot.Skill, InputTag);
}

// [Server]
/* 기본 어빌리티 세트(UJY_AbilitySet::GiveToAbilitySystem)와 같은 입력 태그 방식 */
FGameplayAbilitySpecHandle UJY_SkillLoadoutComponent::GrantSkillAbility(UJY_SkillDefinition* Skill, FGameplayTag InputTag)
{
	if (ensureMsgf(Skill != nullptr && Skill->AbilityClass != nullptr, TEXT("스킬 정의 또는 AbilityClass가 없음 (%s)"), *GetNameSafe(Skill)) == false)
		return FGameplayAbilitySpecHandle();

	/* 같은 GA 클래스를 여러 스킬이 쓸 수 있어 GA는 SourceObject로 자기 스킬을 찾는다 (Lyra 장비 어빌리티와 같음) */
	FGameplayAbilitySpec AbilitySpec(Skill->AbilityClass, 1, INDEX_NONE, Skill);
	if (InputTag.IsValid() == true)
	{
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(InputTag);
	}

	UJY_AbilitySystemComponent* JYASC = GetAbilitySystemComponent();
	return JYASC->GiveAbility(AbilitySpec);
}

// [Server]
void UJY_SkillLoadoutComponent::RemoveSlotAbility(int32 SlotIndex)
{
	FJY_SkillSlotEntry& Slot = Slots[SlotIndex];
	if (Slot.GrantedHandle.IsValid() == false)
		return;

	UJY_AbilitySystemComponent* JYASC = GetAbilitySystemComponent();
	JYASC->ClearAbility(Slot.GrantedHandle);
	Slot.GrantedHandle = FGameplayAbilitySpecHandle();
}

UJY_AbilitySystemComponent* UJY_SkillLoadoutComponent::GetAbilitySystemComponent() const
{
	const AJY_PlayerState* JYPlayerState = CastChecked<AJY_PlayerState>(GetOwner());
	return JYPlayerState->GetJYAbilitySystemComponent();
}

// [OwnClient]
void UJY_SkillLoadoutComponent::OnRep_Slots()
{
	OnSlotsChanged.Broadcast();
}

// [OwnClient]
void UJY_SkillLoadoutComponent::OnRep_PendingCardChoices()
{
	OnCardChoicesChanged.Broadcast();
}
