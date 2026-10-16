#include "Player/JY_CheatManager.h"
#include "Player/JY_PlayerState.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "AbilitySystem/Effects/JY_DebugGameplayEffects.h"
#include "Interfaces/JY_HitReactable.h"
#include "Interfaces/JY_InterfaceUtils.h"
#include "Room/JY_RoomFlowSubsystem.h"
#include "Skill/JY_SkillDefinition.h"
#include "Skill/JY_SkillLoadoutComponent.h"
#include "System/JY_AssetManager.h"
#include "System/JY_GameData.h"
#include "System/JY_GameMode.h"
#include "System/JY_GameplayTags.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_CheatManager)

/*
 * BlueprintAuthorityOnly 명령은 클라에서 치면 엔진이 ServerExec로 서버에 보내고(CheatManager.cpp:125) 서버 PC의 CheatManager가 실행한다(Player.cpp:145).
 * 서버 PC의 CheatManager는 GameMode AllowCheats일 때만 생긴다(PlayerController.cpp:1162, PIE는 허용) — 개발 빌드 전용 서버에서 개조 클라가 무한 회복하는 것을 막는 관문.
 */

// [Server]
void UJY_CheatManager::JYDamage(float Amount)
{
	/* 방향은 명령 이름이 정하고, 값은 크기만 받는다 */
	if (Amount <= 0.0f)
		return;

	ApplyHealthDelta(-Amount);
}

// [Server]
void UJY_CheatManager::JYHeal(float Amount)
{
	if (Amount <= 0.0f)
		return;

	ApplyHealthDelta(Amount);
}

/* 체력은 서버에서 GE로만 바꾼다 — 음수=데미지 GE, 양수=회복 GE. 크기는 항상 양수로 SetByCaller에 싣는다 */
// [Server]
void UJY_CheatManager::ApplyHealthDelta(float Delta)
{
	/* NaN·무한대 차단 */
	if (FMath::IsFinite(Delta) == false)
		return;

	AJY_PlayerState* JYPlayerState = GetJYPlayerState();
	if (JYPlayerState == nullptr)
		return;

	UJY_AbilitySystemComponent* JYASC = JYPlayerState->GetJYAbilitySystemComponent();

	const bool bIsDamage = Delta < 0.0f;
	const TSubclassOf<UGameplayEffect> EffectClass = bIsDamage == true ? UJY_GameplayEffect_DebugDamage::StaticClass() : UJY_GameplayEffect_DebugHeal::StaticClass();
	const FGameplayTag MagnitudeTag = bIsDamage == true ? JYGameplayTags::SetByCaller_Damage : JYGameplayTags::SetByCaller_Heal;

	FGameplayEffectContextHandle EffectContext = JYASC->MakeEffectContext();
	EffectContext.AddSourceObject(GetOuterAPlayerController());

	const FGameplayEffectSpecHandle SpecHandle = JYASC->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
	SpecHandle.Data->SetSetByCallerMagnitude(MagnitudeTag, FMath::Abs(Delta));
	JYASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

// [Server]
void UJY_CheatManager::JYAddSoul(int32 Amount)
{
	if (Amount <= 0)
		return;

	AJY_GameMode* GameMode = GetWorld()->GetAuthGameMode<AJY_GameMode>();
	if (GameMode == nullptr)
		return;

	GameMode->AddSoulStones(GetOuterAPlayerController(), Amount);
}

/* 태그 끝 이름(Skill.Plunge → Plunge)으로 GameData 목록에서 찾는다. 슬롯 키 번호(1부터)를 슬롯 인덱스로 */
// [Server]
void UJY_CheatManager::JYGiveSkill(const FString& SkillName, int32 SlotNumber)
{
	AJY_PlayerState* JYPlayerState = GetJYPlayerState();
	if (JYPlayerState == nullptr)
		return;

	UJY_SkillLoadoutComponent* SkillLoadout = JYPlayerState->GetSkillLoadoutComponent();
	const int32 SlotIndex = SlotNumber - 1;
	const TArray<FJY_SkillSlotEntry>& Slots = SkillLoadout->GetSlots();
	if (Slots.IsValidIndex(SlotIndex) == false)
		return;

	const FString TagSuffix = FString(TEXT(".")) + SkillName;
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	auto IsNamedSkill = [&TagSuffix](const TObjectPtr<UJY_SkillDefinition>& Skill)
	{
		const FString TagString = Skill->SkillTag.ToString();
		return TagString.EndsWith(TagSuffix);
	};
	const TObjectPtr<UJY_SkillDefinition>* FoundSkill = GameData->SkillDefinitions.FindByPredicate(IsNamedSkill);
	if (FoundSkill == nullptr)
		return;

	SkillLoadout->AcquireSkill(*FoundSkill, SlotIndex);
}

// [Server]
void UJY_CheatManager::JYUnlockSkillSlot()
{
	AJY_PlayerState* JYPlayerState = GetJYPlayerState();
	if (JYPlayerState == nullptr)
		return;

	UJY_SkillLoadoutComponent* SkillLoadout = JYPlayerState->GetSkillLoadoutComponent();
	SkillLoadout->UnlockSlot();
}

/*
 * 엔진 치트 변수 AbilitySystem.IgnoreCooldowns(AbilitySystemGlobals.cpp:39)를 켠다 — 발동 검사·커밋 검사에서 쿨타임을 건너뜀(GameplayAbility.cpp:508·613·671).
 * 발동 검사는 조종 클라 예측에서도 돌므로 내 쪽에 켜고 서버에도 같은 명령을 보낸다 (PIE 한 프로세스면 같은 변수)
 */
// [Server] [OwnClient]
void UJY_CheatManager::JYNoCooldown(bool bEnable)
{
	IConsoleVariable* IgnoreCooldownsVariable = IConsoleManager::Get().FindConsoleVariable(TEXT("AbilitySystem.IgnoreCooldowns"));
	IgnoreCooldownsVariable->Set(bEnable, ECVF_SetByConsole);

	APlayerController* PlayerController = GetOuterAPlayerController();
	if (PlayerController->GetNetMode() != NM_Client)
		return;

	const FString ServerCommand = FString::Printf(TEXT("JYNoCooldown %d"), bEnable == true ? 1 : 0);
	PlayerController->ServerExec(ServerCommand);
}

// [Server]
void UJY_CheatManager::JYClearRoom()
{
	UJY_RoomFlowSubsystem* RoomFlow = GetWorld()->GetSubsystem<UJY_RoomFlowSubsystem>();
	RoomFlow->ClearCurrentRoom();
}

/* 실제 공격과 같은 서버 진입점(IJY_HitReactable::ReceiveHit)으로 부숨 — 복제·클라 연출 경로가 그대로 시험된다 */
// [Server]
void UJY_CheatManager::JYBreakProps(float Radius)
{
	APawn* Pawn = GetOuterAPlayerController()->GetPawn();
	if (Pawn == nullptr || Radius <= 0.0f)
		return;

	const FVector PawnLocation = Pawn->GetActorLocation();
	const float RadiusSquared = Radius * Radius;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == Pawn || FVector::DistSquared(Actor->GetActorLocation(), PawnLocation) > RadiusSquared)
			continue;

		IJY_HitReactable* HitReactable = JY_FindInterface<IJY_HitReactable>(Actor);
		if (HitReactable == nullptr || HitReactable->CanReceiveHit() == false)
			continue;

		/* 밀어내기 없는 타격과 같음 — 플레이어 → 물체 방향 최소 세기 */
		HitReactable->ReceiveHit(Pawn, 0.0f, FVector::ZeroVector);
	}
}

// [Server]
void UJY_CheatManager::JYUnlimitedHealth(bool bEnable)
{
	AJY_PlayerState* JYPlayerState = GetJYPlayerState();
	if (JYPlayerState == nullptr)
		return;

	JYPlayerState->GetJYAbilitySystemComponent()->SetLooseGameplayTagCount(JYGameplayTags::Cheat_UnlimitedHealth, bEnable == true ? 1 : 0);
}

/* 접속 직후엔 PlayerState가 아직 없을 수 있다 */
// [Server]
AJY_PlayerState* UJY_CheatManager::GetJYPlayerState() const
{
	return GetOuterAPlayerController()->GetPlayerState<AJY_PlayerState>();
}
