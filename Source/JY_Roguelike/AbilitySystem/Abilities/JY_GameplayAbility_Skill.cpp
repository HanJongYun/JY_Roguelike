#include "AbilitySystem/Abilities/JY_GameplayAbility_Skill.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Effects/JY_SkillGameplayEffects.h"
#include "AbilitySystem/Tasks/JY_AbilityTask_WaitMoveInput.h"
#include "Animation/JY_AnimNotifyState_CancelWindow.h"
#include "Player/JY_PlayerState.h"
#include "Skill/JY_SkillDefinition.h"
#include "Skill/JY_SkillLoadoutComponent.h"
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
	 * 스킬은 기본 공격을 언제든 끊고, 다른 스킬은 그 스킬의 끊기 구간 안에서만 끊는다(CanActivateAbility).
	 * 실행 중 점프는 막는다 (막기 = 상대 어빌리티의 AssetTags로 판정, GameplayAbility.cpp:432)
	 */
	CancelAbilitiesWithTag.AddTag(JYGameplayTags::Ability_Type_Attack);
	CancelAbilitiesWithTag.AddTag(JYGameplayTags::Ability_Type_Skill);
	BlockAbilitiesWithTag.AddTag(JYGameplayTags::Ability_Type_Jump);

	ActivationBlockedTags.AddTag(JYGameplayTags::State_Dashing);

	/* 끊기 구간에서 같은 스킬을 다시 누르면 지금 실행을 끝내고 새로 시작 (InputPressed) */
	bRetriggerInstancedAbility = true;
}

// [Server] [OwnClient] 서버는 발동 RPC를 클라가 보낸 시점과 같은 몽타주 위치에서 받는다(서버 몽타주도 발동 RPC 도착 지연만큼 늦게 시작) → 같은 판정
bool UJY_GameplayAbility_Skill::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const //override
{
	if (Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) == false)
		return false;

	return IsSkillInProgress(ActorInfo) == false;
}

// [Server] [OwnClient]
/* 막기 태그(BlockAbilitiesWithTag)는 실행 내내 고정이라 "구간 안에서만 허용"을 표현할 수 없다 → 실행 중 스킬에 직접 묻는다 */
bool UJY_GameplayAbility_Skill::IsSkillInProgress(const FGameplayAbilityActorInfo* ActorInfo)
{
	const UAbilitySystemComponent* AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();
	for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (AbilitySpec.IsActive() == false)
			continue;

		const UJY_GameplayAbility_Skill* ActiveSkill = Cast<UJY_GameplayAbility_Skill>(AbilitySpec.GetPrimaryInstance());
		if (ActiveSkill == nullptr)
			continue;

		if (ActiveSkill->IsInCancelWindow() == false)
			return true;
	}
	return false;
}

/* 엔진 CheckCooldown이 이 태그로 막는다(GameplayAbility.cpp:1071~1078) — 쿨타임 GE에 붙는 태그와 같은 컨테이너 */
const FGameplayTagContainer* UJY_GameplayAbility_Skill::GetCooldownTags() const //override
{
	return &CooldownTags;
}

// [Server] [OwnClient] CommitAbility가 예측 창 안에서 부름 → 조종 클라도 즉시 막힘
/*
 * 기본 구현은 고정 GE만 적용한다(GameplayAbility.cpp:1106). 스킬마다 GE 에셋을 만들지 않고
 * 쿨타임 GE 1개에 길이(SetByCaller)·태그(DynamicGrantedTags, GameplayEffect.cpp:4664)를 스펙에서 넣는다
 */
void UJY_GameplayAbility_Skill::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const //override
{
	const FJY_SkillStats Stats = GetSkillStats();
	const int32 AbilityLevel = GetAbilityLevel(Handle, ActorInfo);
	const FGameplayEffectSpecHandle CooldownSpecHandle = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, UJY_GameplayEffect_SkillCooldown::StaticClass(), AbilityLevel);

	FGameplayEffectSpec* CooldownSpec = CooldownSpecHandle.Data.Get();
	CooldownSpec->DynamicGrantedTags.AppendTags(CooldownTags);
	CooldownSpec->SetSetByCallerMagnitude(JYGameplayTags::SetByCaller_Cooldown, Stats.Cooldown);

	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpecHandle);
}

// [OwnClient] 실행 중에 같은 입력이 오면 엔진은 발동 대신 이 함수를 부른다 (UJY_AbilitySystemComponent::ProcessAbilityInput)
/*
 * 다시 발동은 엔진 재발동 경로 — InternalTryActivateAbility가 CanActivateAbility(쿨타임·끊기 구간) 통과 후 지금 실행을 끝내고 새로 시작한다
 * (AbilitySystemComponent_Abilities.cpp:1817·1836). 끝냄 RPC가 발동 RPC보다 먼저 서버에 간다 (같은 ASC Reliable)
 */
void UJY_GameplayAbility_Skill::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) //override
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	if (IsInCancelWindow() == false)
		return;

	UJY_AbilitySystemComponent* JYASC = GetJYAbilitySystemComponentFromActorInfo();
	JYASC->TryActivateAbility(Handle);
}

UJY_SkillDefinition* UJY_GameplayAbility_Skill::GetSkillDefinition() const
{
	UObject* SourceObject = GetCurrentSourceObject();
	return CastChecked<UJY_SkillDefinition>(SourceObject);
}

// [Server] [OwnClient] 로드아웃은 소유 클라에 복제 → 양쪽이 같은 수치
FJY_SkillStats UJY_GameplayAbility_Skill::GetSkillStats() const
{
	const AActor* OwnerActor = GetOwningActorFromActorInfo();
	const AJY_PlayerState* JYPlayerState = CastChecked<AJY_PlayerState>(OwnerActor);
	const UJY_SkillLoadoutComponent* SkillLoadout = JYPlayerState->GetSkillLoadoutComponent();
	const UJY_SkillDefinition* SkillDefinition = GetSkillDefinition();
	return SkillLoadout->GetSkillStats(SkillDefinition);
}

FJY_HitShape UJY_GameplayAbility_Skill::ScaleHitShape(const FJY_HitShape& Shape, float RadiusScale)
{
	FJY_HitShape ScaledShape = Shape;
	ScaledShape.Radius *= RadiusScale;
	ScaledShape.Width *= RadiusScale;
	return ScaledShape;
}

FTransform UJY_GameplayAbility_Skill::GetAvatarHitOrigin() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const FRotator AvatarRotation = AvatarActor->GetActorRotation();
	const FRotator YawRotation(0.0f, AvatarRotation.Yaw, 0.0f);
	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	return FTransform(YawRotation, AvatarLocation);
}

// [OwnClient] 끊는 판단은 조종 클라 — EndAbility가 서버로 복제된다 (기본 공격 이동 끊기와 같음)
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
 * 몽타주에 걸린 구간 데이터와 재생 위치로 직접 판단 — 섹션 이름 문자열·ASC 태그에 기대지 않는다.
 * 태그로 하면 끊긴 몽타주가 블렌드 아웃하는 동안에도 노티파이가 돌아 태그가 남고, 그사이 시작한 다음 스킬이 바로 끊긴다
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
