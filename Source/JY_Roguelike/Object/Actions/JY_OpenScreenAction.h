#pragma once

#include "CoreMinimal.h"
#include "Object/JY_Action.h"
#include "JY_OpenScreenAction.generated.h"

class UJY_ScreenWidget;
struct FStreamableHandle;

/* E키를 쓴 플레이어 화면에만 창을 연다 (기록판·제단) */
UCLASS(meta = (DisplayName = "Open Screen"))
class UJY_OpenScreenAction : public UJY_Action
{
	GENERATED_BODY()

public:
	virtual void Initialize(AJY_Object* Object) override;
	virtual void OnLocalUserActivate(AJY_Object* Object, APawn* User) override;

private:
	/* 설정값 */

	/* GameMenu 층에 여는 화면 (예: WBP_JY_RecordBoard) */
	UPROPERTY(EditAnywhere, Category = "JY|OpenScreen")
	TSoftClassPtr<UJY_ScreenWidget> ScreenClass;

private:
	/* 화면 클래스 미리 로드 — 핸들을 쥐고 있어야 로드된 클래스가 메모리에 남는다 */
	TSharedPtr<FStreamableHandle> ScreenPreloadHandle;
};
