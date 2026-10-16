#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JY_InteractionComponent.generated.h"

class UPrimitiveComponent;
class UJY_UIManagerSubsystem;

/*
 * 플레이어 캐릭터에 붙어 가까운 상호작용 대상(IJY_InteractableTarget)을 고르고 E 입력을 처리 — 내 화면 일(안내·확인 팝업)까지.
 * 실제 사용은 상호작용 GA(UJY_GameplayAbility_Interact)가 서버와 함께 한다
 */
UCLASS(ClassGroup = (JY), meta = (BlueprintSpawnableComponent))
class UJY_InteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJY_InteractionComponent();

protected:
	virtual void BeginPlay() override;

public:
	/* [OwnClient] E 입력 — 초점 대상을 씀 (확인 문구가 있으면 팝업 먼저) */
	void Interact();

private:
	/* 폰 몸체(캡슐)가 무언가와 겹치기 시작 — 상호작용 대상의 범위면 후보에 넣음 */
	UFUNCTION()
	void HandleOwnerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/* 폰 몸체(캡슐)의 겹침이 끝남 — 상호작용 대상의 범위면 후보에서 뺌 */
	UFUNCTION()
	void HandleOwnerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/* 후보 목록에서 초점 대상을 다시 고르고, 바뀌었으면 안내를 갱신 */
	void RefreshFocus();

	/* 화면 아래 안내에 대상 이름을 넣음 (빈 글자면 숨김) */
	void SetPromptName(UJY_UIManagerSubsystem* UIManager, const FText& Name);

	/* 내 화면 관리자 — 폰 → PC → 로컬 플레이어. 서버·컨트롤러 없음이면 nullptr */
	UJY_UIManagerSubsystem* GetUIManager() const;

	/* 확인 팝업 결과 — 확인이면 사용 */
	void HandleConfirmResult(bool bConfirmed, TWeakObjectPtr<UPrimitiveComponent> Target);

	/* [OwnClient] 상호작용 GA를 이벤트로 발동 — 대상 액터를 실어 서버에도 같이 발동된다 */
	void TriggerInteraction(UPrimitiveComponent* Target) const;

private:
	/* 범위 안 대상들 (들어온 순서, 상호작용 인터페이스를 구현한 범위 컴포넌트). 초점 = 마지막 */
	TArray<TWeakObjectPtr<UPrimitiveComponent>> Candidates;

	/* 지금 초점인 대상 (E를 누르면 쓰일 대상) */
	TWeakObjectPtr<UPrimitiveComponent> FocusTarget;

	/* 안내를 띄운 화면 관리자 — 컨트롤러가 먼저 떨어진 뒤(맵 이동)에도 안내를 지울 수 있게 */
	TWeakObjectPtr<UJY_UIManagerSubsystem> PromptUIManager;
};
