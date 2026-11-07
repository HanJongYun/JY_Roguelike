#include "AbilitySystem/Abilities/JY_GameplayAbility_Skill.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Tasks/JY_AbilityTask_WaitMoveInput.h"
#include "Animation/JY_AnimNotifyState_CancelWindow.h"
#include "Skill/JY_SkillData.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "System/JY_GameplayTags.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_GameplayAbility_Skill)

UJY_GameplayAbility_Skill::UJY_GameplayAbility_Skill(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer SkillTags;
	SkillTags.AddTag(JYGameplayTags::Ability_Type_Skill);
	SetAssetTags(SkillTags);

	/*
	 * 기본 공격은 언제든 끊고, 다른 스킬은 끊기 구간 안에서만 끊는다(CanActivateAbility)
	 * 막기는 상대 어빌리티의 AssetTags로 판정한다(GameplayAbility.cpp:432)
	 */
	CancelAbilitiesWithTag.AddTag(JYGameplayTags::Ability_Type_Attack);
	CancelAbilitiesWithTag.AddTag(JYGameplayTags::Ability_Type_Skill);
	BlockAbilitiesWithTag.AddTag(JYGameplayTags::Ability_Type_Jump);

	/* 끊기 구간에서 같은 스킬을 다시 누르면 지금 실행을 끝내고 새로 시작 (InputPressed) */
	bRetriggerInstancedAbility = true;
}

// [Server] [OwnClient]
/* 끊기 구간 밖이면 다른 스킬·기본 공격이 시작되지 않는다 */
bool UJY_GameplayAbility_Skill::IsBlockingOtherAttacks() const //override
{
	return IsInCancelWindow() == false;
}

// [Server] [OwnClient]
/*
 * 엔진 CheckCooldown이 이 태그로 막는다(GameplayAbility.cpp:1071) — 쿨타임 GE에 붙는 태그와 같은 컨테이너.
 * 인자 없이 불리므로 인스턴스의 현재 스펙으로 찾는다 — 지급 때 스펙 핸들이 들어감(GameplayAbility.cpp:2011)
 */
const FGameplayTagContainer* UJY_GameplayAbility_Skill::GetCooldownTags() const //override
{
	const FJY_SkillEntry* SkillEntry = FindSkillEntry(CurrentSpecHandle, CurrentActorInfo);
	if (SkillEntry == nullptr)
		return nullptr;

	return &SkillEntry->CooldownTags;
}

// [Server] [OwnClient] CommitAbility가 예측 창 안에서 부름 → 조종 클라도 즉시 막힘
/* 스킬마다 GE 에셋을 만들지 않고 쿨타임 GE 1개에 길이(SetByCaller)·태그(DynamicGrantedTags)를 넣는다 */
void UJY_GameplayAbility_Skill::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const //override
{
	const FJY_SkillEntry* SkillEntry = FindSkillEntry(Handle, ActorInfo);
	if (SkillEntry == nullptr)
		return;

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const TSubclassOf<UGameplayEffect> CooldownEffectClass = GameData->SkillData->CooldownGameplayEffect;
	if (ensureMsgf(CooldownEffectClass != nullptr, TEXT("DA_JY_SkillData에 CooldownGameplayEffect가 없음")) == false)
		return;

	const int32 AbilityLevel = GetAbilityLevel(Handle, ActorInfo);
	const FGameplayEffectSpecHandle CooldownSpecHandle = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownEffectClass, AbilityLevel);

	FGameplayEffectSpec* CooldownSpec = CooldownSpecHandle.Data.Get();
	CooldownSpec->DynamicGrantedTags.AppendTags(SkillEntry->CooldownTags);
	CooldownSpec->SetSetByCallerMagnitude(JYGameplayTags::SetByCaller_Cooldown, SkillEntry->Cooldown);

	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpecHandle);
}

// [OwnClient] 실행 중에 같은 입력이 오면 엔진은 발동 대신 이 함수를 부른다 (UJY_AbilitySystemComponent::ProcessAbilityInput)
/* 엔진 재발동 경로 — CanActivateAbility 통과 후 지금 실행을 끝내고 새로 시작한다(AbilitySystemComponent_Abilities.cpp:1817) */
void UJY_GameplayAbility_Skill::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) //override
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	if (IsInCancelWindow() == false)
		return;

	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	JYASC->TryActivateAbility(Handle);
}

// [Server] [OwnClient] 스펙(동적 태그 포함)은 소유 클라에 복제 → 양쪽이 같은 칸을 찾는다
const FJY_SkillEntry* UJY_GameplayAbility_Skill::FindSkillEntry(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (ActorInfo == nullptr)
		return nullptr;

	const UAbilitySystemComponent* AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();
	if (AbilitySystemComponent == nullptr)
		return nullptr;

	const FGameplayAbilitySpec* AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
	if (AbilitySpec == nullptr)
		return nullptr;

	const FGameplayTagContainer SkillRootTags(JYGameplayTags::Skill);
	const FGameplayTagContainer SkillTags = AbilitySpec->GetDynamicSpecSourceTags().Filter(SkillRootTags);
	const FGameplayTag SkillTag = SkillTags.First();

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const FJY_SkillEntry* SkillEntry = GameData->SkillData->FindSkill(SkillTag);
	ensureMsgf(SkillEntry != nullptr, TEXT("%s: 스펙의 스킬 태그(%s) 칸이 DA_JY_SkillData에 없음"), *GetNameSafe(this), *SkillTag.ToString());
	return SkillEntry;
}

const FJY_SkillEntry& UJY_GameplayAbility_Skill::GetCurrentSkillEntry() const
{
	const FJY_SkillEntry* SkillEntry = FindSkillEntry(CurrentSpecHandle, CurrentActorInfo);
	check(SkillEntry != nullptr);
	return *SkillEntry;
}

FJY_HitShape UJY_GameplayAbility_Skill::ScaleHitShape(const FJY_HitShape& Shape, float RadiusScale)
{
	FJY_HitShape ScaledShape = Shape;
	ScaledShape.Radius *= RadiusScale;
	ScaledShape.Width *= RadiusScale;
	return ScaledShape;
}

// [OwnClient] 끊는 판단은 조종 클라 — EndAbility가 서버로 복제된다
void UJY_GameplayAbility_Skill::StartMoveCancelListener()
{
	if (CurrentActorInfo->IsLocallyControlled() == false)
		return;

	UJY_AbilityTask_WaitMoveInput* MoveInputTask = UJY_AbilityTask_WaitMoveInput::WaitMoveInput(this);
	MoveInputTask->OnMoveInput.AddDynamic(this, &ThisClass::OnMoveInput);
	MoveInputTask->ReadyForActivation();
}

// [Server] [OwnClient]
/*
 * 태그 대신 몽타주 구간과 재생 위치로 판단 — 태그는 끊긴 몽타주의 블렌드 아웃 중에도 남아
 * 그사이 시작한 다음 스킬이 바로 끊긴다
 */
bool UJY_GameplayAbility_Skill::IsInCancelWindow() const
{
	const UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();

	/* 다른 어빌리티가 몽타주를 넘겨받았으면 이 스킬의 구간이 아니다 */
	if (JYASC->GetAnimatingAbility() != this)
		return false;

	const UAnimMontage* PlayingMontage = JYASC->GetCurrentMontage();
	if (PlayingMontage == nullptr)
		return false;

	const UAnimInstance* AnimInstance = CurrentActorInfo->GetAnimInstance();
	const float MontagePosition = AnimInstance->Montage_GetPosition(PlayingMontage);
	for (const FAnimNotifyEvent& NotifyEvent : PlayingMontage->Notifies)
	{
		const UAnimNotifyState* NotifyState = NotifyEvent.NotifyStateClass;
		if (NotifyState == nullptr || NotifyState->IsA<UJY_AnimNotifyState_CancelWindow>() == false)
			continue;

		const float WindowStart = NotifyEvent.GetTriggerTime();
		const float WindowEnd = NotifyEvent.GetEndTriggerTime();
		if (MontagePosition >= WindowStart && MontagePosition <= WindowEnd)
			return true;
	}
	return false;
}

// [OwnClient] 구간 전 이동 입력은 무시 — 누르고 있으면 구간이 열리는 순간 끊긴다
void UJY_GameplayAbility_Skill::OnMoveInput()
{
	if (IsInCancelWindow() == false)
		return;

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
