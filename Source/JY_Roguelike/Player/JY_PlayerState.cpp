#include "Player/JY_PlayerState.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/JY_HealthSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_PlayerState)

/**
 * ASC를 PlayerState에 두는 이유 (Lyra LyraPlayerState.cpp:34~44 답습)
 * - 캐릭터가 죽거나 바뀌어도 PlayerState는 남아 속성·능력이 유지된다.
 * - UI도 폰 없이 PlayerState만 있으면 연결 가능.
 */
AJY_PlayerState::AJY_PlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UJY_AbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	/** Mixed — GE는 소유 클라에만 전체 복제, 남에겐 태그·큐만 (플레이어용 권장 모드) */
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	/** 서브오브젝트로 만들어 두면 ASC InitializeComponent가 자동 등록. 참조를 들고 있어 GC 방지 */
	HealthSet = CreateDefaultSubobject<UJY_HealthSet>(TEXT("HealthSet"));

	/** PlayerState 기본 업데이트 빈도는 낮아 체력 복제가 늦게 보임 → Lyra처럼 100 */
	SetNetUpdateFrequency(100.0f);
}

UAbilitySystemComponent* AJY_PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
