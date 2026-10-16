#include "Player/JY_PlayerState.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/JY_HealthSet.h"
#include "AbilitySystem/Attributes/JY_DashSet.h"
#include "Progress/JY_MetaUpgradeData.h"
#include "Skill/JY_SkillLoadoutComponent.h"
#include "System/JY_GameplayTags.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_PlayerState)

/* ASC·스킬 슬롯을 PlayerState에 둔다 — 캐릭터가 죽거나 바뀌어도 속성·능력이 유지된다 (Lyra와 같음) */
AJY_PlayerState::AJY_PlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UJY_AbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	/* Mixed — GE는 소유 클라에만 전체 복제, 남에겐 태그·큐만 */
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	/* 서브오브젝트로 만들면 ASC InitializeComponent가 자동 등록 */
	HealthSet = CreateDefaultSubobject<UJY_HealthSet>(TEXT("HealthSet"));
	DashSet = CreateDefaultSubobject<UJY_DashSet>(TEXT("DashSet"));

	SkillLoadoutComponent = CreateDefaultSubobject<UJY_SkillLoadoutComponent>(TEXT("SkillLoadoutComponent"));

	/* PlayerState 기본 업데이트 빈도는 낮아 체력 복제가 늦게 보인다 */
	SetNetUpdateFrequency(100.0f);
}

UAbilitySystemComponent* AJY_PlayerState::GetAbilitySystemComponent() const //override
{
	return AbilitySystemComponent;
}

void AJY_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AJY_PlayerState, Progress, COND_OwnerOnly);
}

/* 서버에서도 알림 — 리슨 서버 호스트는 OnRep이 오지 않는다 */
// [Server]
void AJY_PlayerState::SetProgress(const FJY_PlayerProgress& NewProgress)
{
	if (HasAuthority() == false)
		return;

	Progress = NewProgress;
	ForceNetUpdate();
	OnProgressChanged.Broadcast();
}

// [OwnClient]
void AJY_PlayerState::OnRep_Progress()
{
	OnProgressChanged.Broadcast();
}

/*
 * 이미 걸린 종류는 지웠다 다시 걸지 않고 크기만 바꾼다 —
 * 최대 체력이 한 번만 바뀌어 증가분만큼 정확히 반영된다
 */
// [Server]
void AJY_PlayerState::RefreshMetaEffects(const UJY_MetaUpgradeData* MetaUpgradeData)
{
	if (HasAuthority() == false)
		return;

	const float OldMaxHealth = AbilitySystemComponent->GetNumericAttribute(UJY_HealthSet::GetMaxHealthAttribute());

	TSet<FGameplayTag> ActiveTags;
	if (MetaUpgradeData != nullptr)
	{
		for (const FJY_MetaUpgradeDefinition& Definition : MetaUpgradeData->GetUpgrades())
		{
			const int32 Rank = Progress.GetMetaRank(Definition.UpgradeTag);
			if (Rank <= 0 || Definition.Effect == nullptr)
				continue;

			ActiveTags.Add(Definition.UpgradeTag);
			const float Magnitude = Rank * Definition.ValuePerRank;

			const FActiveGameplayEffectHandle* ExistingHandle = MetaEffectHandles.Find(Definition.UpgradeTag);
			if (ExistingHandle != nullptr && AbilitySystemComponent->GetActiveGameplayEffect(*ExistingHandle) != nullptr)
			{
				AbilitySystemComponent->UpdateActiveGameplayEffectSetByCallerMagnitude(*ExistingHandle, JYGameplayTags::SetByCaller_MetaValue, Magnitude);
				continue;
			}

			const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(Definition.Effect, 1.0f, AbilitySystemComponent->MakeEffectContext());
			if (SpecHandle.IsValid() == false)
				continue;

			SpecHandle.Data->SetSetByCallerMagnitude(JYGameplayTags::SetByCaller_MetaValue, Magnitude);
			MetaEffectHandles.Add(Definition.UpgradeTag, AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get()));
		}
	}

	/* 0단계가 됐거나(환불) 표에서 빠진 종류는 제거 */
	for (auto It = MetaEffectHandles.CreateIterator(); It; ++It)
	{
		if (ActiveTags.Contains(It->Key) == true)
			continue;

		AbilitySystemComponent->RemoveActiveGameplayEffect(It->Value);
		It.RemoveCurrent();
	}

	/*
	 * 늘어난 최대 체력만큼 현재 체력도 채운다 — HealthSet 전역 규칙이면 임시 버프마다 회복돼 무한 회복.
	 * 주의: 저장된 체력 복원이 생기면 이 함수 뒤에 해야 한다 (먼저 하면 증가분이 더해진다)
	 */
	const float NewMaxHealth = AbilitySystemComponent->GetNumericAttribute(UJY_HealthSet::GetMaxHealthAttribute());
	if (NewMaxHealth > OldMaxHealth)
	{
		AbilitySystemComponent->ApplyModToAttribute(UJY_HealthSet::GetHealthAttribute(), EGameplayModOp::AddBase, NewMaxHealth - OldMaxHealth);
	}
}
