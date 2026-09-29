#include "Player/JY_PlayerState.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/JY_HealthSet.h"
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
 * 단계가 바뀔 때만 불린다(접속·구매·환불 — 영혼석만 바뀔 때는 안 부름). 효과는 PlayerState의 ASC에 걸려 폰이 죽고 다시 생겨도 유지된다.
 * 크기 = 단계 × 단계당 값 → SetByCaller.MetaValue.
 * 새 효과를 먼저 걸고 옛 효과를 나중에 지운다 — 반대로 하면 최대 체력이 잠깐 기본값으로 내려가
 * 현재 체력이 깎인다(UJY_HealthSet::PostAttributeChange가 최대치에 맞춰 자름). 예: 체력 140/140 → 구매 → 100/160이 되는 버그.
 */
// [Server]
void AJY_PlayerState::RefreshMetaEffects(const UJY_MetaUpgradeData* MetaUpgradeData)
{
	if (HasAuthority() == false)
		return;

	/* 표가 없으면 새로 걸 것은 없지만 옛 효과는 지운다 (환불 뒤 효과가 남지 않게) */
	TArray<FActiveGameplayEffectHandle> NewEffectHandles;
	const TArray<FJY_MetaUpgradeDefinition> NoDefinitions;
	for (const FJY_MetaUpgradeDefinition& Definition : MetaUpgradeData != nullptr ? MetaUpgradeData->GetUpgrades() : NoDefinitions)
	{
		const int32 Rank = Progress.GetMetaRank(Definition.UpgradeTag);
		if (Rank <= 0 || Definition.Effect == nullptr)
			continue;

		const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(Definition.Effect, 1.0f, AbilitySystemComponent->MakeEffectContext());
		if (SpecHandle.IsValid() == false)
			continue;

		SpecHandle.Data->SetSetByCallerMagnitude(JYGameplayTags::SetByCaller_MetaValue, Rank * Definition.ValuePerRank);
		NewEffectHandles.Add(AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get()));
	}

	for (const FActiveGameplayEffectHandle& Handle : MetaEffectHandles)
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
	}

	MetaEffectHandles = MoveTemp(NewEffectHandles);
}
