#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "JY_LoadingScreenSubsystem.generated.h"

class SWidget;
class APlayerController;
struct FWorldContext;

/* 접속·맵 이동 중에 화면 전체를 가리는 로딩창 — 클라에만 있다 */
UCLASS()
class UJY_LoadingScreenSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

private:
	/* 로딩 조건을 검사해 로딩창을 켜거나 끔 (바뀔 때만) */
	void UpdateLoadingScreen();

	/* 접속·맵 읽기·이동·준비 대기 중 하나라도 해당하면 true */
	bool IsLoading() const;

	/* 로딩 위젯을 만들어 뷰포트 맨 위에 붙임 */
	void ShowLoadingScreen();

	/* 로딩 위젯을 뷰포트에서 뗌 */
	void HideLoadingScreen();

	/* 하드 이동으로 맵을 읽기 시작 */
	void HandlePreLoadMap(const FWorldContext& WorldContext, const FString& MapName);

	/* 하드 이동으로 맵 읽기 끝 */
	void HandlePostLoadMap(UWorld* LoadedWorld);

	/* 심리스 이동 시작 */
	void HandleSeamlessTravelStart(UWorld* TravelWorld, const FString& MapName);

private:
	/* 뷰포트에 붙인 로딩 위젯 — 떠 있지 않으면 비어 있음 */
	TSharedPtr<SWidget> LoadingScreenWidget;

	/* 심리스 이동 직전의 로컬 PC — 이 PC가 남아 있는 동안은 서버의 새 PC를 기다리는 중 */
	TWeakObjectPtr<APlayerController> ControllerBeforeSeamlessTravel;

	/* 하드 이동으로 맵을 읽는 중 (PreLoadMap ~ PostLoadMap) */
	bool bLoadingMap = false;

	/* 심리스 이동 후 서버가 만든 새 PC가 아직 안 옴 */
	bool bWaitingForNewController = false;
};
