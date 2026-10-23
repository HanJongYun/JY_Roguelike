#include "Combat/JY_HitReactionComponent.h"
#include "Character/JY_HealthComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/RootMotionSource.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_HitReactionComponent)

UJY_HitReactionComponent::UJY_HitReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	/* 조종 클라에 밀림을 보내는 Client RPC용 */
	SetIsReplicatedByDefault(true);
}

// [Server]
/* 수평 LaunchCharacter는 바로 착지해 마찰로 0.1초 안에 멈춘다 (CharacterMovementComponent.cpp:1237) */
/* 루트모션 소스는 정한 시간 동안 속도를 덮어써 마찰과 무관하게 거리가 나오고, 시뮬 프록시에도 복제된다 */
void UJY_HitReactionComponent::ReceiveKnockback(const FVector2f& Impulse, float Height)
{
	const float KnockbackScale = 1.0f - KnockbackResistance;
	const FVector KnockbackVelocity(Impulse.X * KnockbackScale, Impulse.Y * KnockbackScale, 0.0f);
	const float KnockbackHeight = Height * KnockbackScale;

	/* 저항 1(안 밀림) */
	if (KnockbackVelocity.IsNearlyZero() == true && KnockbackHeight <= 0.0f)
		return;

	ApplyKnockback(KnockbackVelocity, KnockbackHeight);

	/* 서버에만 건 루트모션 소스는 조종 클라가 버리고 위치 보정만 받아 덜컹인다(CharacterMovementComponent.cpp:11620) — 양쪽에 따로 걸면 엔진이 짝지어 맞춘다(12229) */
	const APawn* Pawn = GetOwner<APawn>();
	if (Pawn->IsPlayerControlled() == true && Pawn->IsLocallyControlled() == false)
	{
		Client_ApplyKnockback(KnockbackVelocity, KnockbackHeight);
	}
}

// [OwnClient]
void UJY_HitReactionComponent::Client_ApplyKnockback_Implementation(FVector_NetQuantize10 Velocity, float Height)
{
	ApplyKnockback(Velocity, Height);
}

// [Server] [OwnClient]
void UJY_HitReactionComponent::ApplyKnockback(const FVector& Velocity, float Height)
{
	ACharacter* Character = GetOwner<ACharacter>();
	UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();

	/* 연타로 다시 밀리면 이전 밀림을 새 방향으로 바꾼다 */
	MovementComponent->RemoveRootMotionSourceByID(KnockbackRootMotionSourceID);

	if (Height > 0.0f)
	{
		/* 엔진 점프 포물선 — 끝나면 속도 0으로 내려놓아 미끄러지지 않게 */
		TSharedPtr<FRootMotionSource_JumpForce> LaunchForce = MakeShared<FRootMotionSource_JumpForce>();
		LaunchForce->InstanceName = TEXT("Knockback");
		LaunchForce->AccumulateMode = ERootMotionAccumulateMode::Override;
		LaunchForce->Priority = 5;
		LaunchForce->Rotation = Velocity.Rotation();
		LaunchForce->Distance = Velocity.Size() * AirKnockbackDuration;
		LaunchForce->Height = Height;
		LaunchForce->Duration = AirKnockbackDuration;
		LaunchForce->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
		LaunchForce->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
		KnockbackRootMotionSourceID = MovementComponent->ApplyRootMotionSource(LaunchForce);
		return;
	}

	TSharedPtr<FRootMotionSource_ConstantForce> KnockbackForce = MakeShared<FRootMotionSource_ConstantForce>();
	KnockbackForce->InstanceName = TEXT("Knockback");
	KnockbackForce->AccumulateMode = ERootMotionAccumulateMode::Override;
	KnockbackForce->Priority = 5;
	KnockbackForce->Force = Velocity;
	KnockbackForce->Duration = KnockbackDuration;

	/* 수직은 덮지 않는다 — 낭떠러지에선 중력대로 떨어진다 */
	KnockbackForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);

	KnockbackRootMotionSourceID = MovementComponent->ApplyRootMotionSource(KnockbackForce);
}

// [OwnClient] [SimulatedProxy]
/* 번쩍임은 맞을 때마다, 몽타주·애니 멈춤·화면 흔들림은 ReactCooldownSeconds에 한 번 — 지속 피해에 매번 움찔하지 않게 */
void UJY_HitReactionComponent::PlayHitReaction(const FJY_HitReactionParams& Params)
{
	/* 마지막 타의 피격 큐가 죽음 애니를 덮지 않게 */
	const UJY_HealthComponent* HealthComponent = GetOwner()->FindComponentByClass<UJY_HealthComponent>();
	if (HealthComponent != nullptr && HealthComponent->IsDeadOrDying() == true)
		return;

	StartHitFlash();
	StartHitAdditive(Params);

	const double Now = GetWorld()->GetTimeSeconds();
	if (LastReactSeconds >= 0.0 && Now - LastReactSeconds < ReactCooldownSeconds)
		return;

	LastReactSeconds = Now;
	PlayHitReactMontage(Params);
	StartHitStop(Params.HitStopSeconds);
	StartCameraShake(Params.CameraShake);
}

bool UJY_HitReactionComponent::GetHitAdditivePose(UAnimSequence*& OutSequence, float& OutTime) const
{
	if (HitAdditiveSequence == nullptr)
		return false;

	const double Now = GetWorld()->GetTimeSeconds();
	const float Time = static_cast<float>(Now - HitAdditiveStartSeconds) * AdditivePlayRate;
	if (Time >= HitAdditiveSequence->GetPlayLength())
		return false;

	OutSequence = HitAdditiveSequence;
	OutTime = Time;
	return true;
}

FGameplayTag UJY_HitReactionComponent::ResolveReactionTag(const FJY_HitReactionParams& Params) const
{
	if (HeavyHitDamage > 0.0f && Params.Damage >= HeavyHitDamage)
		return HeavyReactionTag;

	return Params.ReactionTag;
}

template <typename T>
T* UJY_HitReactionComponent::SelectByHitDirection(const FVector& HitDirection, T* Front, T* Back, T* Left, T* Right) const
{
	/* 때린 쪽을 향하는 방향 기준 */
	const AActor* Owner = GetOwner();
	const FVector AttackerDirection = -HitDirection.GetSafeNormal2D();
	if (AttackerDirection.IsZero() == true)
		return Front;

	const FVector ForwardVector = Owner->GetActorForwardVector();
	const FVector RightVector = Owner->GetActorRightVector();
	const float ForwardDot = FVector::DotProduct(ForwardVector, AttackerDirection);
	const float RightDot = FVector::DotProduct(RightVector, AttackerDirection);
	if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
		return ForwardDot >= 0.0f ? Front : Back;

	return RightDot >= 0.0f ? Right : Left;
}

UAnimMontage* UJY_HitReactionComponent::FindHitReactMontage(const FJY_HitReactionParams& Params) const
{
	const FJY_HitReactMontages* Montages = ReactionMontages.Find(ResolveReactionTag(Params));
	if (Montages == nullptr)
	{
		Montages = ReactionMontages.Find(DefaultReactionTag);
	}

	/* 반응을 비워 둔 대상(보스 등)은 버틴다 */
	if (Montages == nullptr)
		return nullptr;

	return SelectByHitDirection<UAnimMontage>(Params.HitDirection, Montages->Front, Montages->Back, Montages->Left, Montages->Right);
}

UAnimSequence* UJY_HitReactionComponent::FindHitReactAdditive(const FJY_HitReactionParams& Params) const
{
	const FJY_HitReactAdditives* Additives = ReactionAdditives.Find(ResolveReactionTag(Params));
	if (Additives == nullptr)
	{
		Additives = ReactionAdditives.Find(DefaultReactionTag);
	}

	if (Additives == nullptr)
		return nullptr;

	return SelectByHitDirection<UAnimSequence>(Params.HitDirection, Additives->Front, Additives->Back, Additives->Left, Additives->Right);
}

// [OwnClient] [SimulatedProxy]
/* 하던 동작을 끊지 않고 위에 더하므로 공격·대시 중에도, 연타면 맞을 때마다 처음부터 다시 튄다 */
void UJY_HitReactionComponent::StartHitAdditive(const FJY_HitReactionParams& Params)
{
	UAnimSequence* Additive = FindHitReactAdditive(Params);
	if (Additive == nullptr)
		return;

	HitAdditiveSequence = Additive;
	HitAdditiveStartSeconds = GetWorld()->GetTimeSeconds();
}

bool UJY_HitReactionComponent::IsHitReactMontage(const UAnimMontage* Montage) const
{
	for (const TPair<FGameplayTag, FJY_HitReactMontages>& Pair : ReactionMontages)
	{
		const FJY_HitReactMontages& Montages = Pair.Value;
		if (Montage == Montages.Front || Montage == Montages.Back || Montage == Montages.Left || Montage == Montages.Right)
			return true;
	}
	return false;
}

// [OwnClient] [SimulatedProxy]
void UJY_HitReactionComponent::PlayHitReactMontage(const FJY_HitReactionParams& Params)
{
	UAnimMontage* HitReactMontage = FindHitReactMontage(Params);
	if (HitReactMontage == nullptr)
		return;

	/* 클라에서만 도는 몽타주라 루트모션이 있으면 이 기기에서만 움직여 서버 위치와 어긋난다 */
	if (ensureMsgf(HitReactMontage->HasRootMotion() == false, TEXT("%s: 피격 몽타주 %s에 루트모션이 있음"), *GetNameSafe(GetOwner()), *HitReactMontage->GetName()) == false)
		return;

	const ACharacter* Character = GetOwner<ACharacter>();
	USkeletalMeshComponent* MeshComponent = Character->GetMesh();
	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();

	/* 공격·대시 몽타주(어빌리티가 서버와 맞춰 돌림)를 클라 혼자 끊지 않는다 — 같은 그룹이면 Montage_Play가 멈춘다(AnimInstance.cpp:2770) */
	const UAnimMontage* ActiveMontage = AnimInstance->GetCurrentActiveMontage();
	if (ActiveMontage != nullptr && IsHitReactMontage(ActiveMontage) == false)
		return;

	AnimInstance->Montage_Play(HitReactMontage);
}

// [OwnClient] [SimulatedProxy]
/* 메시 애니만 멈춘다 — 액터 시간(CustomTimeDilation)은 CMC가 서버 시각 계산에 써서 클라만 바꾸면 위치가 어긋난다(CharacterMovementComponent.cpp:9453) */
void UJY_HitReactionComponent::StartHitStop(float Seconds)
{
	if (Seconds <= 0.0f)
		return;

	const ACharacter* Character = GetOwner<ACharacter>();
	USkeletalMeshComponent* MeshComponent = Character->GetMesh();

	/* 루트모션 몽타주 중에 애니를 멈추면 이 기기 이동만 멈춰 서버와 어긋난다 */
	const UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (AnimInstance->GetRootMotionMontageInstance() != nullptr)
		return;

	MeshComponent->GlobalAnimRateScale = 0.0f;
	GetWorld()->GetTimerManager().SetTimer(HitStopTimerHandle, this, &ThisClass::EndHitStop, Seconds, false);
}

// [OwnClient] [SimulatedProxy]
void UJY_HitReactionComponent::EndHitStop()
{
	const ACharacter* Character = GetOwner<ACharacter>();
	USkeletalMeshComponent* MeshComponent = Character->GetMesh();
	MeshComponent->GlobalAnimRateScale = 1.0f;
}

// [OwnClient] [SimulatedProxy]
void UJY_HitReactionComponent::StartHitFlash()
{
	if (HitFlashMaterial == nullptr)
		return;

	const ACharacter* Character = GetOwner<ACharacter>();
	USkeletalMeshComponent* MeshComponent = Character->GetMesh();

	/* 번쩍이는 중에 또 맞으면 시간만 늘린다 — 이미 덮은 번쩍임 재질을 원래 재질로 저장하지 않게 */
	if (bHitFlashActive == false)
	{
		OverlayMaterialBeforeFlash = MeshComponent->GetOverlayMaterial();
		MeshComponent->SetOverlayMaterial(HitFlashMaterial);
		bHitFlashActive = true;
	}

	GetWorld()->GetTimerManager().SetTimer(HitFlashTimerHandle, this, &ThisClass::EndHitFlash, HitFlashSeconds, false);
}

// [OwnClient] [SimulatedProxy]
void UJY_HitReactionComponent::EndHitFlash()
{
	const ACharacter* Character = GetOwner<ACharacter>();
	USkeletalMeshComponent* MeshComponent = Character->GetMesh();
	MeshComponent->SetOverlayMaterial(OverlayMaterialBeforeFlash);
	OverlayMaterialBeforeFlash = nullptr;
	bHitFlashActive = false;
}

// [OwnClient]
void UJY_HitReactionComponent::StartCameraShake(TSubclassOf<UCameraShakeBase> CameraShake) const
{
	if (CameraShake == nullptr)
		return;

	const APawn* Pawn = GetOwner<APawn>();
	if (Pawn->IsLocallyControlled() == false)
		return;

	const APlayerController* PlayerController = Pawn->GetController<APlayerController>();
	if (PlayerController == nullptr)
		return;

	APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
	CameraManager->StartCameraShake(CameraShake);
}
