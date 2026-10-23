#include "Animation/JY_AnimInstance.h"
#include "AnimCharacterMovementLibrary.h"
#include "AnimDistanceMatchingLibrary.h"
#include "SequenceEvaluatorLibrary.h"
#include "SequencePlayerLibrary.h"
#include "AnimExecutionContextLibrary.h"
#include "Animation/AnimSequence.h"
#include "Combat/JY_HitReactionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AnimInstance)

void UJY_AnimInstance::NativeInitializeAnimation() //override
{
	Super::NativeInitializeAnimation();

	bIsFirstUpdate = true;
}

// [Server] [OwnClient] [SimulatedProxy] 각자 받은 이동 상태로 계산 (애님은 복제 안 함)
/* 게임 스레드: 캐릭터·CMC 값을 복사만 한다. 계산은 NativeThreadSafeUpdateAnimation에서 */
void UJY_AnimInstance::NativeUpdateAnimation(float DeltaSeconds) //override
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	/* 애님 에디터 미리보기에는 캐릭터가 없다 */
	const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
	if (Character == nullptr)
		return;

	const UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	WorldLocation = Character->GetActorLocation();
	WorldVelocity = Character->GetVelocity();
	LastUpdateVelocity = MovementComp->GetLastUpdateVelocity();
	WorldAcceleration = MovementComp->GetCurrentAcceleration();
	/* 막 생겨 아직 빙의 전이면 이동 모드가 None이라 IsMovingOnGround가 false — 떨어지는 중일 때만 공중으로 본다 (Character.cpp:1388 Restart에서 걷기로) */
	bOnGroundMode = MovementComp->IsFalling() == false;
	bUseSeparateBrakingFriction = MovementComp->bUseSeparateBrakingFriction;
	BrakingFriction = MovementComp->BrakingFriction;
	GroundFriction = MovementComp->GroundFriction;
	BrakingFrictionFactor = MovementComp->BrakingFrictionFactor;
	BrakingDecelerationWalking = MovementComp->BrakingDecelerationWalking;
	JumpCount = Character->JumpCurrentCount;
	bIsSimulatedProxy = Character->GetLocalRole() == ROLE_SimulatedProxy;

	UpdateHitAdditive(Character);
}

/* 움찔은 피격 큐(클라)가 컴포넌트에 기록한다 — 애님은 재생 위치만 읽는다 */
void UJY_AnimInstance::UpdateHitAdditive(const ACharacter* Character)
{
	const UJY_HitReactionComponent* HitReactionComponent = Character->FindComponentByClass<UJY_HitReactionComponent>();
	if (HitReactionComponent == nullptr)
		return;

	UAnimSequence* Sequence = nullptr;
	float Time = 0.0f;
	if (HitReactionComponent->GetHitAdditivePose(OUT Sequence, OUT Time) == false)
	{
		HitAdditiveAlpha = 0.0f;
		return;
	}

	HitAdditiveSequence = Sequence;
	HitAdditiveTime = Time;
	HitAdditiveAlpha = 1.0f;
}

void UJY_AnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds) //override
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	/* 첫 업데이트는 이전 값이 없어 이동 거리·속도 변화가 순간이동처럼 커진다 → 이번 값으로 채움 */
	if (bIsFirstUpdate == true)
	{
		PreviousWorldLocation = WorldLocation;
		PreviousVelocityZ = WorldVelocity.Z;
		PreviousJumpCount = JumpCount;
		bWasOnGround = bOnGroundMode;
		bIsFirstUpdate = false;
	}

	DisplacementSinceLastUpdate = FVector::Dist2D(WorldLocation, PreviousWorldLocation);
	DisplacementSpeed = DeltaSeconds > 0.0f ? DisplacementSinceLastUpdate / DeltaSeconds : 0.0f;
	PreviousWorldLocation = WorldLocation;

	const FVector Velocity2D(WorldVelocity.X, WorldVelocity.Y, 0.0f);
	const FVector Acceleration2D(WorldAcceleration.X, WorldAcceleration.Y, 0.0f);
	Speed2D = Velocity2D.Size();
	bHasAcceleration = Acceleration2D.IsNearlyZero() == false;

	bIsOnGround = bOnGroundMode;
	bIsJumping = bIsOnGround == false && WorldVelocity.Z > 0.0f;
	bIsFalling = bIsOnGround == false && WorldVelocity.Z <= 0.0f;
	bShouldMove = bHasAcceleration == true && bIsOnGround == true;
	bShouldSkipStart = bShouldMove == true && Speed2D >= StartSkipSpeed;
	bShouldStop = bHasAcceleration == false;
	bShouldDistanceMatchStop = Velocity2D.IsNearlyZero() == false && bHasAcceleration == false;

	/* 속도로는 상승 초반 2단 점프를 놓쳐 점프 횟수로 본다. 시뮬 프록시는 점프 횟수가 복제되지 않아 속도 증가로 본다 */
	const bool bStayedInAir = bWasOnGround == false && bIsOnGround == false;
	const bool bJumpCountIncreased = JumpCount > PreviousJumpCount;
	const float VelocityZGain = WorldVelocity.Z - PreviousVelocityZ;
	const bool bAirJumpDetected = bIsSimulatedProxy == true ? VelocityZGain > DoubleJumpSpeedGain : bJumpCountIncreased;
	bDoubleJumped = bStayedInAir == true && bAirJumpDetected == true;
	PreviousVelocityZ = WorldVelocity.Z;
	PreviousJumpCount = JumpCount;
	bWasOnGround = bIsOnGround;

	/* 보간된 표시 속도가 아니라 실제 이동에 쓴 CMC 마지막 속도로 예측 */
	const FVector LastUpdateVelocity2D(LastUpdateVelocity.X, LastUpdateVelocity.Y, 0.0f);
	const FVector StopLocation = UAnimCharacterMovementLibrary::PredictGroundMovementStopLocation(LastUpdateVelocity2D, bUseSeparateBrakingFriction, BrakingFriction, GroundFriction, BrakingFrictionFactor, BrakingDecelerationWalking);
	StopDistance = StopLocation.Size2D();
}

void UJY_AnimInstance::SetupStartAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;
	const FSequenceEvaluatorReference Evaluator = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, OUT Result);
	USequenceEvaluatorLibrary::SetExplicitTime(Evaluator, 0.0f);
	StrideWarpingStartAlpha = 0.0f;
}

/* 앞부분(발 디딤) 이후 보폭 조절을 섞는다. 최소 재생 속도의 Lerp 시작값이 섞는 시간인 건 Lyra 그대로 */
void UJY_AnimInstance::UpdateStartAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;
	const FSequenceEvaluatorReference Evaluator = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, OUT Result);
	const float ExplicitTime = USequenceEvaluatorLibrary::GetAccumulatedTime(Evaluator);
	const FVector2D BlendInRange(0.0f, StrideWarpingBlendInDuration);
	const FVector2D AlphaRange(0.0f, 1.0f);
	StrideWarpingStartAlpha = FMath::GetMappedRangeValueClamped(BlendInRange, AlphaRange, ExplicitTime - StrideWarpingBlendInStartOffset);

	const float MinPlayRate = FMath::Lerp(StrideWarpingBlendInDuration, StartPlayRateClamp.X, StrideWarpingStartAlpha);
	const FVector2D PlayRateClamp(MinPlayRate, StartPlayRateClamp.Y);
	UAnimDistanceMatchingLibrary::AdvanceTimeByDistanceMatching(Context, Evaluator, DisplacementSinceLastUpdate, DistanceCurveName, PlayRateClamp);
}

/* 재생 속도를 실제 이동 속도에 맞추고, 남는 차이는 보폭 조절이 맡는다 */
void UJY_AnimInstance::UpdateCycleAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;
	const FSequencePlayerReference Player = USequencePlayerLibrary::ConvertToSequencePlayer(Node, OUT Result);
	UAnimDistanceMatchingLibrary::SetPlayrateToMatchSpeed(Player, DisplacementSpeed, CyclePlayRateClamp);

	const float DeltaTime = UAnimExecutionContextLibrary::GetDeltaTime(Context);
	StrideWarpingCycleAlpha = FMath::FInterpTo(StrideWarpingCycleAlpha, 1.0f, DeltaTime, StrideWarpingCycleAlphaInterpSpeed);
}

/* 이미 섰거나 입력이 있으면 멈춤점(거리 0)으로 바로. 미끄러지는 중이면 Update가 맞춘다 */
void UJY_AnimInstance::SetupStopAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (bShouldDistanceMatchStop == true)
		return;

	EAnimNodeReferenceConversionResult Result;
	const FSequenceEvaluatorReference Evaluator = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, OUT Result);
	UAnimDistanceMatchingLibrary::DistanceMatchToTarget(Evaluator, 0.0f, DistanceCurveName);
}

/* 섰으면 나머지(정리 동작)를 1배속으로 */
void UJY_AnimInstance::UpdateStopAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;
	const FSequenceEvaluatorReference Evaluator = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, OUT Result);
	if (bShouldDistanceMatchStop == true && StopDistance > 0.0f)
	{
		UAnimDistanceMatchingLibrary::DistanceMatchToTarget(Evaluator, StopDistance, DistanceCurveName);
		return;
	}

	USequenceEvaluatorLibrary::AdvanceTime(Context, Evaluator, 1.0f);
}
