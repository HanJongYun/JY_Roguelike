#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "JY_Character.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UJY_HealthComponent;
struct FInputActionValue;

/** 쿼터뷰 카메라로 움직이는 플레이어 캐릭터 */
UCLASS()
class AJY_Character : public ACharacter
{
	GENERATED_BODY()

public:
	AJY_Character();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }
	UJY_HealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** 이동 입력 처리 */
	void Move(const FInputActionValue& Value);

	/** PlayerState의 ASC에 이 캐릭터를 아바타로 연결 */
	void InitializeAbilitySystem();

	/** 죽기 시작하면 이동·충돌 끄기 */
	UFUNCTION()
	void OnDeathStarted(AActor* OwningActor);

	/** 이동 입력·이동·캡슐 충돌 정지 */
	void DisableMovementAndCollision();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JY|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JY|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;

	/** 체력 0 → 죽음 상태 복제 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JY|Health", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UJY_HealthComponent> HealthComponent;

	/** 카메라 붐의 월드 기준 회전 (캐릭터가 돌아도 고정) */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Camera")
	FRotator CameraBoomRotation = FRotator(-55.0f, 0.0f, 0.0f);

	/** 캐릭터와 카메라 사이 거리 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|Camera")
	float CameraBoomLength = 1400.0f;

	/** 이동 입력 (Vector2D: X=오른쪽, Y=앞) */
	UPROPERTY(EditAnywhere, Category = "JY|Input")
	TObjectPtr<UInputAction> MoveAction;
};
