#include "Player/JY_PlayerState.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/JY_HealthSet.h"
#include "AbilitySystem/Attributes/JY_DashSet.h"
#include "Progress/JY_MetaUpgradeData.h"
#include "System/JY_GameplayTags.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_PlayerState)

/*
 * ASC를 PlayerState에 두는 이유 (Lyra LyraPlayerState.cpp:34~44 답습)
 * - 캐릭터가 죽거나 바뀌어도 PlayerState는 남아 속성·능력이 유지된다.
 * - UI도 폰 없이 PlayerState만 있으면 연결 가능.
 */
AJY_PlayerState::AJY_PlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UJY_AbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	/* Mixed — GE는 소유 클라에만 전체 복제, 남에겐 태그·큐만 (플레이어용 권장 모드) */
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	/* 서브오브젝트로 만들어 두면 ASC InitializeComponent가 자동 등록. 참조를 들고 있어 GC 방지 */
	HealthSet = CreateDefaultSubobject<UJY_HealthSet>(TEXT("HealthSet"));
	DashSet = CreateDefaultSubobject<UJY_DashSet>(TEXT("DashSet"));

	/* PlayerState 기본 업데이트 빈도는 낮아 체력 복제가 늦게 보임 → Lyra처럼 100 */
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
 * 단계가 바뀔 때만 불린다(접속·구매·환불 — 영혼석만 바뀔 때는 안 부름). 효과는 PlayerState의 ASC에 걸려 폰이 바뀌어도 유지된다.
 * 크기 = 단계 × 단계당 값 → SetByCaller.MetaValue.
 * 이미 걸린 종류는 지웠다 다시 걸지 않고 크기만 바꾼다(UpdateActiveGameplayEffectSetByCallerMagnitude) —
 * 최대 체력이 140 → 160으로 한 번만 바뀌어 증가분(+20)만큼 정확히 반영된다.
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
	 * 메타로 최대 체력이 늘면 늘어난 만큼 현재 체력도 채운다 — 예: 140/140 → 강인함 +20 → 160/160, 맵 이동 뒤 재적용 100/100 → 160/160.
	 * 게임 전체 규칙(HealthSet)이 아니라 여기서만 하는 이유: 전역으로 두면 임시 버프가 켜졌다 꺼질 때마다 회복돼 무한 회복이 된다.
	 * 줄어드는 쪽(환불)은 HealthSet이 현재 체력을 새 최대치로 자른다.
	 * 주의: 저장된 체력을 복원하는 기능이 생기면 이 함수 뒤에 복원해야 한다 (먼저 복원하면 여기서 증가분이 더해진다).
	 */
	const float NewMaxHealth = AbilitySystemComponent->GetNumericAttribute(UJY_HealthSet::GetMaxHealthAttribute());
	if (NewMaxHealth > OldMaxHealth)
	{
		AbilitySystemComponent->ApplyModToAttribute(UJY_HealthSet::GetHealthAttribute(), EGameplayModOp::AddBase, NewMaxHealth - OldMaxHealth);
	}
}
