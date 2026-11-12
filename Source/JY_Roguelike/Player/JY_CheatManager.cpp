#include "Player/JY_CheatManager.h"
#include "Player/JY_PlayerState.h"
#include "AbilitySystem/JY_AbilitySystemComponent.h"
#include "Combat/JY_CombatQuerySubsystem.h"
#include "Interfaces/JY_HitReactable.h"
#include "Interfaces/JY_InterfaceUtils.h"
#include "Mass/Core/JY_EnemyDefinition.h"
#include "Mass/Core/JY_MassEnemySubsystem.h"
#include "Mass/JY_MassTypes.h"
#include "Player/JY_PlayerController.h"
#include "MassCommonFragments.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassNavigationFragments.h"
#include "NavMesh/RecastNavMesh.h"
#include "TimerManager.h"
#include "Room/JY_RoomFlowSubsystem.h"
#include "Room/JY_RoomMarker.h"
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
	HitPayload.BaseDamage = 1.0f;
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

/* 지점 둘레 원 위에 같은 간격으로 자리를 나눈다 — 웨이브 디렉터가 격자 칸 검사로 자리를 고르기 전까지 쓰는 시험용 배치 */
// [Server]
void UJY_CheatManager::JYMassSpawn(int32 CountPerPoint)
{
	if (CountPerPoint <= 0)
		return;

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	if (ensureMsgf(GameData->EnemyDefinitions.Num() > 0, TEXT("DA_JY_GameData에 EnemyDefinitions가 없음")) == false)
		return;

	UJY_RoomFlowSubsystem* RoomFlow = GetWorld()->GetSubsystem<UJY_RoomFlowSubsystem>();
	const ULevel* RoomLevel = RoomFlow->GetCurrentRoomLevel();
	if (RoomLevel == nullptr)
		return;

	/* 한 마리면 지점 위, 여러 마리면 서로 겹치지 않게 둘레로 */
	const float RingRadius = CountPerPoint == 1 ? 0.0f : 150.0f;
	TArray<FVector> SpawnLocations;
	for (const AActor* Actor : RoomLevel->Actors)
	{
		const AJY_RoomEnemySpawnPoint* SpawnPoint = Cast<AJY_RoomEnemySpawnPoint>(Actor);
		if (SpawnPoint == nullptr)
			continue;

		const FVector PointLocation = SpawnPoint->GetActorLocation();
		for (int32 Index = 0; Index < CountPerPoint; ++Index)
		{
			const float AngleRadians = UE_TWO_PI * Index / CountPerPoint;
			const FVector RingOffset(FMath::Cos(AngleRadians) * RingRadius, FMath::Sin(AngleRadians) * RingRadius, 0.0f);
			SpawnLocations.Add(PointLocation + RingOffset);
		}
	}

	UJY_MassEnemySubsystem* MassEnemySubsystem = GetWorld()->GetSubsystem<UJY_MassEnemySubsystem>();
	const UJY_EnemyDefinition* EnemyDefinition = GameData->EnemyDefinitions[0];
	/* 시험용이라 예고 없이 바로 Chase */
	MassEnemySubsystem->SpawnWave(*EnemyDefinition, SpawnLocations, 0.0f);
}

/* [임시코드] 한 마리만 보려는 시험용 — 2.5에서 삭제 */
// [Server]
void UJY_CheatManager::JYMassSpawnFront(int32 Count)
{
	const APawn* Pawn = GetOuterAPlayerController()->GetPawn();
	if (Pawn == nullptr || Count <= 0)
		return;

	const UJY_GameData* GameData = UJY_AssetManager::Get().GetGameData();
	if (ensureMsgf(GameData->EnemyDefinitions.Num() > 0, TEXT("DA_JY_GameData에 EnemyDefinitions가 없음")) == false)
		return;

	/* SpawnWave는 지금 방의 기준점이 필요하다 — 첫 방이 붙기 전이면 안 만든다 (JY_RoomFlowSubsystem.cpp:171) */
	const UJY_RoomFlowSubsystem* RoomFlow = GetWorld()->GetSubsystem<UJY_RoomFlowSubsystem>();
	if (RoomFlow->GetCurrentRoomLevel() == nullptr)
		return;

	/* 메시 기준점이 발이라 캡슐 가운데에서 반 높이만큼 내린다 */
	const FVector FrontCenter = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 700.0f - FVector(0.0f, 0.0f, Pawn->GetSimpleCollisionHalfHeight());
	const float RingRadius = Count == 1 ? 0.0f : 150.0f;
	TArray<FVector> SpawnLocations;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float AngleRadians = UE_TWO_PI * Index / Count;
		SpawnLocations.Add(FrontCenter + FVector(FMath::Cos(AngleRadians) * RingRadius, FMath::Sin(AngleRadians) * RingRadius, 0.0f));
	}

	UJY_MassEnemySubsystem* MassEnemySubsystem = GetWorld()->GetSubsystem<UJY_MassEnemySubsystem>();
	MassEnemySubsystem->DestroyAllEnemies();
	MassEnemySubsystem->SpawnWave(*GameData->EnemyDefinitions[0], SpawnLocations, 0.0f);
}

// [Server]
void UJY_CheatManager::JYMassMove(float Distance)
{
	UJY_MassEnemySubsystem* MassEnemySubsystem = GetWorld()->GetSubsystem<UJY_MassEnemySubsystem>();
	MassEnemySubsystem->MoveAllEnemies(FVector(Distance, 0.0f, 0.0f));
}

// [Server]
void UJY_CheatManager::JYMassDamageAll(float Damage)
{
	UJY_MassEnemySubsystem* MassEnemySubsystem = GetWorld()->GetSubsystem<UJY_MassEnemySubsystem>();
	MassEnemySubsystem->DamageAllEnemies(Damage);
}

namespace JY_FlowFieldDebug
{
	/* 보내는 간격 = 선이 남는 시간 — 다음 묶음이 올 때 앞 묶음이 사라져 옛 화살표가 겹치지 않는다 */
	constexpr float SendIntervalSeconds = 0.5f;
}

/* [임시코드] (2장 B 끝나면 삭제) 플로우 필드는 서버에만 있어 선 목록을 클라로 보내 그린다 */
// [Server]
void UJY_CheatManager::JYShowFlowField(bool bEnable)
{
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	TimerManager.ClearTimer(FlowFieldDebugTimerHandle);
	if (bEnable == false)
		return;

	TimerManager.SetTimer(FlowFieldDebugTimerHandle, this, &ThisClass::SendFlowFieldDebugLines, JY_FlowFieldDebug::SendIntervalSeconds, true, 0.0f);
}

/* [임시코드] (2장 B 끝나면 삭제) */
// [Server]
void UJY_CheatManager::SendFlowFieldDebugLines()
{
	UWorld* World = GetWorld();
	const UJY_MassEnemySubsystem* MassEnemySubsystem = World->GetSubsystem<UJY_MassEnemySubsystem>();
	const ARecastNavMesh* NavMesh = MassEnemySubsystem->GetNavMesh();
	if (NavMesh == nullptr)
		return;

	TArray<FVector_NetQuantize> LineStarts;
	TArray<FVector_NetQuantize> LineEnds;
	TArray<FColor> LineColors;
	TArray<FVector_NetQuantize> OutlineStarts;
	TArray<FVector_NetQuantize> OutlineEnds;

	/* 다각형마다: 테두리 (초록) + 중심 → 목표 쪽 문 가운데 (노랑) — 플레이어와 이어진 다각형만 */
	const FVector LineLift(0.0f, 0.0f, 30.0f);
	const FJY_NavFlowField& FlowField = MassEnemySubsystem->GetFlowField();
	TArray<FVector> PolyVerts;
	for (const TPair<NavNodeRef, FJY_NavFlowStep>& Pair : FlowField.GetSteps())
	{
		PolyVerts.Reset();
		NavMesh->GetPolyVerts(Pair.Key, OUT PolyVerts);
		for (int32 VertIndex = 0; VertIndex < PolyVerts.Num(); ++VertIndex)
		{
			const int32 NextVertIndex = (VertIndex + 1) % PolyVerts.Num();
			OutlineStarts.Add(PolyVerts[VertIndex] + LineLift);
			OutlineEnds.Add(PolyVerts[NextVertIndex] + LineLift);
		}

		if (Pair.Key == FlowField.GetGoalPoly())
			continue;

		FVector PolyCenter;
		NavMesh->GetPolyCenter(Pair.Key, OUT PolyCenter);
		LineStarts.Add(PolyCenter + LineLift);
		LineEnds.Add(Pair.Value.GetPortalCenter() + LineLift);
		LineColors.Add(FColor::Yellow);
	}

	/* 적마다: 위치 → 갈 곳 (빨강) */
	UMassEntitySubsystem* MassEntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	FMassEntityManager& EntityManager = MassEntitySubsystem->GetMutableEntityManager();
	FMassExecutionContext ExecutionContext(EntityManager);
	FMassEntityQuery EnemyQuery(EntityManager.AsShared());
	EnemyQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EnemyQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadOnly);
	EnemyQuery.AddRequirement<FJY_NavPolyFragment>(EMassFragmentAccess::ReadOnly);

	auto CollectMoveTargets = [&LineStarts, &LineEnds, &LineColors, &LineLift](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassMoveTargetFragment> MoveTargets = ChunkContext.GetFragmentView<FMassMoveTargetFragment>();
		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			if (MoveTargets[EntityIt].GetCurrentAction() != EMassMovementAction::Move)
				continue;

			LineStarts.Add(Transforms[EntityIt].GetTransform().GetLocation() + LineLift);
			LineEnds.Add(MoveTargets[EntityIt].Center + LineLift);
			LineColors.Add(FColor::Red);
		}
	};
	EnemyQuery.ForEachEntityChunk(ExecutionContext, CollectMoveTargets);

	AJY_PlayerController* PlayerController = CastChecked<AJY_PlayerController>(GetOuterAPlayerController());
	PlayerController->Client_DrawFlowFieldDebug(LineStarts, LineEnds, LineColors, OutlineStarts, OutlineEnds, JY_FlowFieldDebug::SendIntervalSeconds);
}

/* 접속 직후엔 PlayerState가 아직 없을 수 있다 */
// [Server]
AJY_PlayerState* UJY_CheatManager::GetJYPlayerState() const
{
	return GetOuterAPlayerController()->GetPlayerState<AJY_PlayerState>();
}
