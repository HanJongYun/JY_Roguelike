#include "Object/JY_Object.h"
#include "Object/JY_Action.h"
#include "Object/JY_ObjectClock.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_Object)

namespace JY_Object
{
	/* 바뀐 지 이보다 오래된 상태를 받으면 늦게 받은 것 — 한 번 터지는 연출(파편·폭발)을 건너뛴다 */
	static constexpr float CatchUpSeconds = 0.5f;

	/* 차례 한 바퀴(Idle → PreActive → Active → InActive)보다 많이 돌지 않게 */
	static constexpr int32 MaxStateSteps = 4;
}

/*
 * 서버는 상태 하나(ObjectState)만 판정·복제하고, 연출은 각 클라가 OnRep에서 Action으로 재생한다.
 * 방마다 여러 개 배치되는 물체라 처음부터 휴면(DORM_Initial, 배치 액터 복제 비용 0, EngineTypes.h:3607) — 상태를 바꿀 때만 FlushNetDormancy로 한 번 보낸다.
 * Tick은 부드러운 움직임을 그리는 Action이 있는 동안만 켠다 (AddTickingAction)
 */
AJY_Object::AJY_Object()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	NetDormancy = DORM_Initial;
}

void AJY_Object::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //override
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AJY_Object, ObjectState);
	DOREPLIFETIME(AJY_Object, StateServerSeconds);
	DOREPLIFETIME(AJY_Object, ActivationImpulse);
}

/*
 * Idle Action은 시작하자마자 실행한다 (가시 숨김 위치, 도끼 피해).
 * 클라가 BeginPlay 전에 다른 상태를 받았으면(스폰 액터) 그 상태까지 결과만 맞춘다
 */
// [Server] [Client]
void AJY_Object::BeginPlay() //override
{
	Super::BeginPlay();

	const EJY_ObjectState AllStates[] = { EJY_ObjectState::Idle, EJY_ObjectState::PreActive, EJY_ObjectState::Active, EJY_ObjectState::InActive };
	for (const EJY_ObjectState EachState : AllStates)
	{
		for (UJY_Action* Action : GetActions(EachState))
		{
			if (Action != nullptr)
			{
				Action->Initialize(this);
			}
		}
	}

	AppliedState = EJY_ObjectState::Idle;
	EnterState(EJY_ObjectState::Idle, true);
	ApplyStateLocally(ObjectState, true);
}

// [Server] [Client]
void AJY_Object::EndPlay(const EEndPlayReason::Type EndPlayReason) //override
{
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	ExitState(AppliedState);
	TickingActions.Reset();

	Super::EndPlay(EndPlayReason);
}

// [Client]
void AJY_Object::Tick(float DeltaSeconds) //override
{
	Super::Tick(DeltaSeconds);

	/* Tick 중에 등록·해제될 수 있어 사본으로 돈다 */
	const TArray<TObjectPtr<UJY_Action>> CurrentActions = TickingActions;
	for (UJY_Action* Action : CurrentActions)
	{
		if (Action->TickClient(this, DeltaSeconds) == false)
		{
			RemoveTickingAction(Action);
		}
	}
}

// [Client]
void AJY_Object::AddTickingAction(UJY_Action* Action)
{
	TickingActions.AddUnique(Action);
	SetActorTickEnabled(true);
}

// [Client]
void AJY_Object::RemoveTickingAction(UJY_Action* Action)
{
	TickingActions.Remove(Action);
	if (TickingActions.Num() == 0)
	{
		SetActorTickEnabled(false);
	}
}

// [Server]
void AJY_Object::RequestActivate(AActor* InInstigator, const FVector& Impulse)
{
	if (HasAuthority() == false || CanActivate() == false)
		return;

	ActivationInstigator = InInstigator;

	/* State와 같은 깨우기에 실어 보냄 — SetState 안의 FlushNetDormancy보다 먼저 바꿔도 같은 프레임 끝에 함께 나간다 */
	ActivationImpulse = Impulse;
	SetState(GetNextState(EJY_ObjectState::Idle));
}

/* 화면 열기처럼 그 플레이어 화면만 바뀌는 일 — 상태 복제(모두의 화면)와 따로 쓴 플레이어 기기에서 바로 실행 */
// [OwnClient]
void AJY_Object::ExecuteLocalActions(APawn* User)
{
	for (UJY_Action* Action : ActiveActions)
	{
		if (Action != nullptr)
		{
			Action->OnLocalUserActivate(this, User);
		}
	}
}

// [Server]
void AJY_Object::SetState(EJY_ObjectState NewState)
{
	/* 휴면 액터는 깨워야 바뀐 값이 나간다 */
	FlushNetDormancy();
	ObjectState = NewState;
	StateServerSeconds = static_cast<float>(JY_ObjectClock::GetSyncedSeconds(GetWorld()));

	/* 서버는 OnRep이 안 불리므로 직접 */
	ApplyStateLocally(NewState, false);

	/* Action이 이 물체를 다시 발동시켰으면(연쇄) 이미 다음 상태 — 시간 처리는 그쪽 SetState가 했다 */
	if (ObjectState != NewState)
		return;

	const float Seconds = GetStateSeconds(NewState);
	if (Seconds < 0.0f)
		return;

	if (Seconds == 0.0f)
	{
		AdvanceState();
		return;
	}

	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &ThisClass::AdvanceState, Seconds, false);
}

// [Server]
void AJY_Object::AdvanceState()
{
	SetState(GetNextState(ObjectState));
}

EJY_ObjectState AJY_Object::GetNextState(EJY_ObjectState FromState) const
{
	switch (FromState)
	{
	case EJY_ObjectState::Idle:
		return ActivationDelay > 0.0f ? EJY_ObjectState::PreActive : EJY_ObjectState::Active;

	case EJY_ObjectState::PreActive:
		return EJY_ObjectState::Active;

	case EJY_ObjectState::Active:
		return bTriggerOnce == true || CooldownSeconds > 0.0f ? EJY_ObjectState::InActive : EJY_ObjectState::Idle;

	case EJY_ObjectState::InActive:
		return EJY_ObjectState::Idle;
	}
	return EJY_ObjectState::Idle;
}

float AJY_Object::GetStateSeconds(EJY_ObjectState InState) const
{
	switch (InState)
	{
	case EJY_ObjectState::Idle:
		return -1.0f;

	case EJY_ObjectState::PreActive:
		return ActivationDelay;

	case EJY_ObjectState::Active:
		return ActiveSeconds;

	case EJY_ObjectState::InActive:
		return bTriggerOnce == true ? -1.0f : CooldownSeconds;
	}
	return -1.0f;
}

/*
 * 클라는 상태를 하나씩 받는다는 보장이 없다 — 항아리는 Active → InActive가 같은 프레임이라 InActive만 온다.
 * 그래서 마지막으로 실행한 상태부터 받은 상태까지 차례(GetNextState)대로 나가고 들어가 사이 상태 연출(파편)도 실행한다
 */
// [Server] [Client]
void AJY_Object::ApplyStateLocally(EJY_ObjectState TargetState, bool bCatchUp)
{
	for (int32 Step = 0; Step < JY_Object::MaxStateSteps && AppliedState != TargetState; ++Step)
	{
		const EJY_ObjectState NextState = GetNextState(AppliedState);
		ExitState(AppliedState);
		AppliedState = NextState;
		EnterState(NextState, bCatchUp);

		/* [Server] Action·발동 컴포넌트가 들어간 김에 다시 발동시킴(쿨타임 끝에 올라와 있는 폰) — 그쪽 SetState가 이어서 진행했다 */
		if (ObjectState != TargetState)
			return;
	}

	/* 설정이 바뀌어 차례로 닿지 않는 상태 — 바로 맞춘다 */
	if (AppliedState != TargetState)
	{
		ExitState(AppliedState);
		AppliedState = TargetState;
		EnterState(TargetState, true);
	}
}

// [Server] [Client]
void AJY_Object::EnterState(EJY_ObjectState InState, bool bCatchUp)
{
	const bool bServer = HasAuthority();
	const bool bClient = IsNetMode(NM_DedicatedServer) == false;

	for (UJY_Action* Action : GetActions(InState))
	{
		if (Action == nullptr)
			continue;

		if (bServer == true)
		{
			Action->OnServerEnter(this);
		}

		if (bClient == true)
		{
			Action->OnClientEnter(this, bCatchUp);
		}
	}

	OnStateChanged.Broadcast(InState);
}

// [Server] [Client]
void AJY_Object::ExitState(EJY_ObjectState InState)
{
	const bool bServer = HasAuthority();
	const bool bClient = IsNetMode(NM_DedicatedServer) == false;

	for (UJY_Action* Action : GetActions(InState))
	{
		if (Action == nullptr)
			continue;

		if (bServer == true)
		{
			Action->OnServerExit(this);
		}

		if (bClient == true)
		{
			Action->OnClientExit(this);
		}
	}
}

const TArray<TObjectPtr<UJY_Action>>& AJY_Object::GetActions(EJY_ObjectState InState) const
{
	switch (InState)
	{
	case EJY_ObjectState::PreActive:
		return PreActiveActions;

	case EJY_ObjectState::Active:
		return ActiveActions;

	case EJY_ObjectState::InActive:
		return InActiveActions;

	default:
		return IdleActions;
	}
}

/* 같은 묶음의 복제 값(StateServerSeconds·ActivationImpulse)이 모두 들어온 뒤에 불린다 (DataReplication.cpp FObjectReplicator::PostReceivedBunch → CallRepNotifies) */
// [Client]
void AJY_Object::OnRep_State()
{
	/* BeginPlay 전에 받으면 BeginPlay가 맞춘다 */
	if (HasActorBegunPlay() == false)
		return;

	const double Now = JY_ObjectClock::GetSyncedSeconds(GetWorld());
	const bool bCatchUp = Now - StateServerSeconds > JY_Object::CatchUpSeconds;
	ApplyStateLocally(ObjectState, bCatchUp);
}
