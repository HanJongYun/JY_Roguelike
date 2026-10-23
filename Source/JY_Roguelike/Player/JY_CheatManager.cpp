#include "Player/JY_CheatManager.h"
#include "Player/JY_PlayerState.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "Combat/JY_CombatQuerySubsystem.h"
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
 * BlueprintAuthorityOnly 명령은 클라에서 치면 엔진이 서버로 보낸다 (CheatManager.cpp:125).
 * 서버 PC의 CheatManager는 AllowCheats일 때만 생겨 개조 클라의 치트를 막는다
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
	if (FMath::IsFinite(Delta) == false)
		return;

	AJY_PlayerState* JYPlayerState = GetJYPlayerState();
	if (JYPlayerState == nullptr)
		return;

	UJY_AbilitySystemComponent* JYASC = JYPlayerState->GetJYAbilitySystemComponent();

	const bool bIsDamage = Delta < 0.0f;
	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	const TSubclassOf<UGameplayEffect> EffectClass = bIsDamage == true ? GameData->DamageGameplayEffect : GameData->HealGameplayEffect;
	if (ensureMsgf(EffectClass != nullptr, TEXT("DA_JY_GameData에 DamageGameplayEffect 또는 HealGameplayEffect가 없음")) == false)
		return;
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
 * 엔진 치트 변수 AbilitySystem.IgnoreCooldowns를 켠다 (GameplayAbility.cpp:508).
 * 발동 검사는 조종 클라 예측에서도 돌므로 내 쪽에 켜고 서버에도 같은 명령을 보낸다
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

/* 함정과 같은 서버 진입점(환경 피해)으로 1 피해 — 무적 검사·피격 큐 복제·클라 반응까지 실제 경로를 그대로 탄다 */
// [Server]
void UJY_CheatManager::JYHitReact(const FString& Direction, const FString& CueName)
{
	APawn* Pawn = GetOuterAPlayerController()->GetPawn();
	if (Pawn == nullptr)
		return;

	/* 때린 쪽 위치 = 내 앞·뒤·왼·오 */
	const FVector ForwardVector = Pawn->GetActorForwardVector();
	const FVector RightVector = Pawn->GetActorRightVector();
	FVector AttackerOffset = ForwardVector;
	if (Direction.Equals(TEXT("B"), ESearchCase::IgnoreCase) == true)
	{
		AttackerOffset = -ForwardVector;
	}
	else if (Direction.Equals(TEXT("L"), ESearchCase::IgnoreCase) == true)
	{
		AttackerOffset = -RightVector;
	}
	else if (Direction.Equals(TEXT("R"), ESearchCase::IgnoreCase) == true)
	{
		AttackerOffset = RightVector;
	}

	const FString CueTagName = FString(TEXT("GameplayCue.Hit.")) + (CueName.IsEmpty() == true ? TEXT("Spike") : CueName);
	const FGameplayTag CueTag = FGameplayTag::RequestGameplayTag(FName(*CueTagName), false);
	if (ensureMsgf(CueTag.IsValid() == true, TEXT("JYHitReact: 없는 피격 큐 %s"), *CueTagName) == false)
		return;

	FJY_HitPayload HitPayload;
	HitPayload.Damage = 1.0f;
	HitPayload.HitCueTag = CueTag;

	const FVector Origin = Pawn->GetActorLocation() + AttackerOffset * 100.0f;
	const UJY_CombatQuerySubsystem* CombatQuery = GetWorld()->GetSubsystem<UJY_CombatQuerySubsystem>();
	CombatQuery->ApplyEnvironmentDamage(Pawn, HitPayload, Origin);
}

/* 그림은 각 기기 화면에만 보여 내 쪽(클라)에서 켠다 */
// [OwnClient]
void UJY_CheatManager::JYShowRanges()
{
	IConsoleVariable* ShowRangesVariable = IConsoleManager::Get().FindConsoleVariable(TEXT("jy.Debug.ShowRanges"));
	IConsoleVariable* DrawHitShapesVariable = IConsoleManager::Get().FindConsoleVariable(TEXT("jy.Combat.DrawHitShapes"));
	const bool bShow = ShowRangesVariable->GetInt() == 0;
	ShowRangesVariable->Set(bShow == true ? 1 : 0, ECVF_SetByConsole);
	DrawHitShapesVariable->Set(bShow == true ? 1 : 0, ECVF_SetByConsole);
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
