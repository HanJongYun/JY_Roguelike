#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JY_Object.generated.h"

class APawn;
class UJY_Action;

/* 물체의 상태 — 서버가 정하고 복제. 차례: Idle → (PreActive) → Active → (InActive) → Idle */
UENUM()
enum class EJY_ObjectState : uint8
{
	/* 발동을 기다림 */
	Idle,

	/* 발동 직전 (가시 경고·화염판 예열·폭발 통 점화) — ActivationDelay가 0이면 건너뜀 */
	PreActive,

	/* 발동 중 */
	Active,

	/* 쿨타임, 또는 한 번 쓰고 끝난 상태 (bTriggerOnce) — CooldownSeconds가 0이고 여러 번 쓰는 물체면 건너뜀 */
	InActive
};

DECLARE_MULTICAST_DELEGATE_OneParam(FJY_OnObjectStateChanged, EJY_ObjectState /*NewState*/);

/*
 * 월드 물체의 유일한 C++ 클래스 (함정·부서지는 물체·제단·포털·출구 문) — 클래스 조건 2: 복제·휴면 설정.
 * 어떤 컴포넌트(발동 방식·모양·연출 부품)를 가질지는 BP 클래스가 정하고, 상태마다 할 일은 Action으로 조합한다
 */
UCLASS()
class AJY_Object : public AActor
{
	GENERATED_BODY()

public:
	AJY_Object();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	/* 지금 발동 요청을 받나 — Idle일 때만 */
	bool CanActivate() const { return ObjectState == EJY_ObjectState::Idle; }

	/* [Server] 발동 요청 (발동 컴포넌트가 부름). Impulse = 발동 방향·세기 (맞은 방향 등, 파편이 튀는 쪽) */
	void RequestActivate(AActor* InInstigator, const FVector& Impulse = FVector::ZeroVector);

	/* [OwnClient] E키를 쓴 플레이어 화면에서만 하는 일 — Active 상태 Action의 OnLocalUserActivate */
	void ExecuteLocalActions(APawn* User);

	EJY_ObjectState GetState() const { return ObjectState; }

	/* [Server] 발동시킨 쪽 (부순 플레이어 등) — 피해 원인으로 남긴다 */
	AActor* GetActivationInstigator() const { return ActivationInstigator.Get(); }

	const FVector& GetActivationImpulse() const { return ActivationImpulse; }

	/* [Client] 부드러운 움직임 등 매 프레임 그려야 하는 Action — 등록한 동안만 Tick이 켜진다 */
	void AddTickingAction(UJY_Action* Action);
	void RemoveTickingAction(UJY_Action* Action);

	/* 상태가 바뀜 (서버·클라 모두, Action 실행 뒤) */
	FJY_OnObjectStateChanged OnStateChanged;

private:
	// [Server]
	void SetState(EJY_ObjectState NewState);

	/* [Server] 지금 상태의 시간이 끝남 — 다음 상태로 */
	void AdvanceState();

	/* 이 물체 설정에 따른 다음 상태 (서버 진행·클라 따라잡기 공통) */
	EJY_ObjectState GetNextState(EJY_ObjectState FromState) const;

	/* 이 상태에 머무는 시간 (초). 0이면 Action 실행 직후 다음 상태, 음수면 머묾 (Idle·한 번 쓰고 끝난 InActive) */
	float GetStateSeconds(EJY_ObjectState InState) const;

	/* 이 기기에서 Action을 마지막으로 실행한 상태부터 TargetState까지 차례로 나가고 들어감 */
	void ApplyStateLocally(EJY_ObjectState TargetState, bool bCatchUp);

	void EnterState(EJY_ObjectState InState, bool bCatchUp);
	void ExitState(EJY_ObjectState InState);

	const TArray<TObjectPtr<UJY_Action>>& GetActions(EJY_ObjectState InState) const;

	UFUNCTION()
	void OnRep_State();

private:
	/*
	 * 복제 값 — ReplicatedUsing=OnRep_State: 클라가 상태 Action(연출)을 각자 실행.
	 * Transient: 실행 중 상태라 맵에 저장하지 않는다. 단 CDO는 Transient도 저장된다(Property.cpp:1064 IsSerializingDefaults).
	 * 이름이 State가 아닌 이유: BP 부모를 옛 클래스에서 바꿀 때 같은 이름 값(옛 EJY_BreakableState)이 CDO에 복사돼 잘못된 값으로 저장됐다
	 */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_State)
	EJY_ObjectState ObjectState = EJY_ObjectState::Idle;

	/* 복제 값 — State가 바뀐 서버 시각. 늦게 받은 상태(늦은 접속·관련성 회복)면 한 번 터지는 연출을 건너뛴다 */
	UPROPERTY(Transient, Replicated)
	float StateServerSeconds = 0.0f;

	/* 복제 값 — 발동 방향·세기 (OnRep 없음: State와 같은 묶음으로 와서 OnRep_State에서 읽음) */
	UPROPERTY(Transient, Replicated)
	FVector_NetQuantize10 ActivationImpulse;

	/* 설정값 — BP_JY_*의 Class Defaults에서 수정 */

	/* 한 번 발동하면 InActive로 끝 (항아리) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Object")
	bool bTriggerOnce = false;

	/* PreActive 시간 (초) — 0이면 바로 Active */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Object", Meta = (ClampMin = "0.0"))
	float ActivationDelay = 0.0f;

	/* Active 시간 (초) — 0이면 Active Action 실행 직후 다음 상태 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Object", Meta = (ClampMin = "0.0"))
	float ActiveSeconds = 0.0f;

	/* InActive(쿨타임) 시간 (초) — 0이면 바로 Idle. bTriggerOnce면 무시 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Object", Meta = (ClampMin = "0.0", EditCondition = "bTriggerOnce == false"))
	float CooldownSeconds = 0.0f;

	/* 상태에 들어갈 때 실행하는 Action — 나갈 때 멈춘다 */

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "JY|Object|Actions")
	TArray<TObjectPtr<UJY_Action>> IdleActions;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "JY|Object|Actions")
	TArray<TObjectPtr<UJY_Action>> PreActiveActions;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "JY|Object|Actions")
	TArray<TObjectPtr<UJY_Action>> ActiveActions;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "JY|Object|Actions")
	TArray<TObjectPtr<UJY_Action>> InActiveActions;

private:
	/* 이 기기에서 Action을 마지막으로 실행한 상태 — 클라가 여러 상태를 한 번에 받으면 사이 상태도 차례로 실행 */
	EJY_ObjectState AppliedState = EJY_ObjectState::Idle;

	TWeakObjectPtr<AActor> ActivationInstigator;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UJY_Action>> TickingActions;

	FTimerHandle StateTimerHandle;
};
