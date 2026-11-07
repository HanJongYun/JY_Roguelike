#pragma once

#include "CoreMinimal.h"

class UAnimInstance;
class UAnimMontage;
class USkeletalMeshComponent;

namespace JY_MontageBoneMotion
{
	/* 몽타주의 그 재생 위치에서 뼈 위치 (메시 공간, 루트 이동 포함). 못 구하면 false */
	bool GetBoneLocationInMesh(const UAnimMontage* Montage, float MontagePosition, FName BoneName, FVector& OutLocation);

	// [Server] [OwnClient]
	/* 재생 중인 몽타주에서 이 뼈가 직전 SampleSeconds 동안 움직인 수평 방향 (월드). 못 구하면 0 */
	FVector GetMontageBoneMoveDirection(const UAnimInstance* AnimInstance, const USkeletalMeshComponent* MeshComponent, FName BoneName, float SampleSeconds);
}
