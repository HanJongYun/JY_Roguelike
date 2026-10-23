#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimExecutionContext.h"
#include "Animation/AnimNodeReference.h"
#include "JY_AnimInstance.generated.h"

class ACharacter;
class UAnimSequence;

/* 주인공 애님. 이동·점프 값 계산과 출발·멈춤 거리 맞추기 */
UCLASS()
class UJY_AnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

private:
	void UpdateHitAdditive(const ACharacter* Character);

protected:
	/* 출발 애니를 처음부터 */
	UFUNCTION(BlueprintCallable, Category = "JY|Locomotion", meta = (BlueprintThreadSafe))
	void SetupStartAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	/* 출발 애니를 이동 거리만큼 진행 */
	UFUNCTION(BlueprintCallable, Category = "JY|Locomotion", meta = (BlueprintThreadSafe))
	void UpdateStartAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	/* 달리기 루프 재생 속도를 실제 속도에 맞춤 */
	UFUNCTION(BlueprintCallable, Category = "JY|Locomotion", meta = (BlueprintThreadSafe))
	void UpdateCycleAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	/* 멈춤 애니를 처음부터 */
	UFUNCTION(BlueprintCallable, Category = "JY|Locomotion", meta = (BlueprintThreadSafe))
	void SetupStopAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	/* 멈춤 애니를 남은 정지 거리에 맞춤 */
	UFUNCTION(BlueprintCallable, Category = "JY|Locomotion", meta = (BlueprintThreadSafe))
	void UpdateStopAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

protected:
	/* ABP_JY_HeroLoco 상태 전환·노드가 읽는 값 */

	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	float Speed2D = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	bool bHasAcceleration = false;

	/* Idle·Stop에서 Start로 넘어갈 조건 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	bool bShouldMove = false;

	/* Idle에서 Start를 건너뛰고 Cycle로 갈 조건 (착지 직후처럼 이미 빠르게 움직이는 중) */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	bool bShouldSkipStart = false;

	/* Start·Cycle에서 Stop으로 넘어갈 조건 (이동 입력이 끊김) */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	bool bShouldStop = false;

	/* 아직 미끄러지는 중이라 멈춤 애니를 남은 거리에 맞출 때 (속도는 있고 입력은 없음) */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	bool bShouldDistanceMatchStop = false;

	/* 지금 속도로 브레이크를 걸면 멈출 때까지 남은 거리 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	float StopDistance = 0.0f;

	/* 지난 애님 업데이트 이후 실제로 이동한 거리 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	float DisplacementSinceLastUpdate = 0.0f;

	/* 실제 이동 속도 (이동 거리 / 업데이트 간격). 보폭 조절 노드의 LocomotionSpeed */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	float DisplacementSpeed = 0.0f;

	/* Start 상태 보폭 조절 노드의 Alpha — 출발 애니 앞부분은 0, 섞어 넣는 구간 동안 1까지 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	float StrideWarpingStartAlpha = 0.0f;

	/* Cycle 상태 보폭 조절 노드의 Alpha */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Locomotion")
	float StrideWarpingCycleAlpha = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "JY|Jump")
	bool bIsOnGround = true;

	/* 공중에서 올라가는 중 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Jump")
	bool bIsJumping = false;

	/* 공중에서 내려가는 중 */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Jump")
	bool bIsFalling = false;

	/* 이번 업데이트에 공중에서 다시 뛰어오름 (2단 점프). 한 업데이트만 true */
	UPROPERTY(BlueprintReadOnly, Category = "JY|Jump")
	bool bDoubleJumped = false;

	/* 맞아서 상체에 더하는 움찔 애니 — Sequence Evaluator의 Sequence */
	UPROPERTY(BlueprintReadOnly, Category = "JY|HitReaction")
	TObjectPtr<UAnimSequence> HitAdditiveSequence;

	/* 움찔 애니 재생 위치 — Sequence Evaluator의 Explicit Time */
	UPROPERTY(BlueprintReadOnly, Category = "JY|HitReaction")
	float HitAdditiveTime = 0.0f;

	/* 움찔 애니를 더하는 중 — Apply Additive의 Alpha */
	UPROPERTY(BlueprintReadOnly, Category = "JY|HitReaction")
	float HitAdditiveAlpha = 0.0f;

protected:
	/* 설정값 — ABP_JY_HeroLoco의 Class Defaults에서 수정 */

	/* 출발·멈춤 애니에 넣은 거리 커브 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Locomotion")
	FName DistanceCurveName = TEXT("Distance");

	/* 출발 애니 거리 맞추기 재생 속도 한계 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Locomotion")
	FVector2D StartPlayRateClamp = FVector2D(0.6f, 5.0f);

	/* 달리기 루프 재생 속도 한계 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Locomotion")
	FVector2D CyclePlayRateClamp = FVector2D(0.8f, 1.2f);

	/* 출발 애니 이 시간(초)부터 보폭 조절을 섞기 시작 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Locomotion")
	float StrideWarpingBlendInStartOffset = 0.15f;

	/* 보폭 조절을 다 섞는 데 걸리는 출발 애니 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Locomotion")
	float StrideWarpingBlendInDuration = 0.2f;

	/* Cycle 보폭 조절 Alpha가 1로 따라가는 보간 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Locomotion")
	float StrideWarpingCycleAlphaInterpSpeed = 10.0f;

	/* 이 속도 이상으로 움직이며 Idle에 들어오면 출발 애니 없이 바로 달리기 (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Locomotion")
	float StartSkipSpeed = 300.0f;

	/* 시뮬 프록시용: 공중에서 한 업데이트 사이 위쪽 속도가 이만큼 늘면 2단 점프로 본다 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Jump")
	float DoubleJumpSpeedGain = 200.0f;

private:
	/* 게임 스레드에서 복사해 두는 원본 값 (워커 스레드는 이것만 읽는다) */
	FVector WorldLocation = FVector::ZeroVector;
	FVector WorldVelocity = FVector::ZeroVector;
	FVector LastUpdateVelocity = FVector::ZeroVector;
	FVector WorldAcceleration = FVector::ZeroVector;
	bool bOnGroundMode = true;
	bool bUseSeparateBrakingFriction = false;
	float BrakingFriction = 0.0f;
	float GroundFriction = 0.0f;
	float BrakingFrictionFactor = 0.0f;
	float BrakingDecelerationWalking = 0.0f;
	int32 JumpCount = 0;
	bool bIsSimulatedProxy = false;

	/* 지난 업데이트 값 (이동 거리·2단 점프 감지) */
	FVector PreviousWorldLocation = FVector::ZeroVector;
	float PreviousVelocityZ = 0.0f;
	int32 PreviousJumpCount = 0;
	bool bWasOnGround = true;
	bool bIsFirstUpdate = true;

};
