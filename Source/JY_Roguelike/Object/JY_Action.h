#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JY_Action.generated.h"

class AJY_Object;
class APawn;
class UActorComponent;

/* 물체가 상태에 들어갈 때 하는 일 하나 (범위 피해·부서짐·화면 열기·이동·이펙트) — 물체마다 Instanced 사본 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class UJY_Action : public UObject
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override;

	/* [Server] [Client] 물체 BeginPlay — 미리 로드 등 */
	virtual void Initialize(AJY_Object* Object) {}

	/* [Server] 상태에 들어감 — 판정·피해·맵 이동 */
	virtual void OnServerEnter(AJY_Object* Object) {}

	/* [Server] 상태에서 나감 — 지속 피해 멈춤 */
	virtual void OnServerExit(AJY_Object* Object) {}

	/* [Client] (데디 서버 제외) 상태에 들어감 — bCatchUp(늦게 받은 상태)이면 연출 없이 결과만 맞춘다 */
	virtual void OnClientEnter(AJY_Object* Object, bool bCatchUp) {}

	/* [Client] 상태에서 나감 — 켜 둔 연출 끄기 */
	virtual void OnClientExit(AJY_Object* Object) {}

	/* [OwnClient] E키를 쓴 플레이어 화면에서만 (Active 상태 Action) */
	virtual void OnLocalUserActivate(AJY_Object* Object, APawn* User) {}

	/* [Client] AJY_Object::AddTickingAction으로 등록한 동안 매 프레임. false를 돌려주면 등록 해제 */
	virtual bool TickClient(AJY_Object* Object, float DeltaSeconds) { return false; }

protected:
	/* BP에서 이 태그(Component Tags)를 단 부품. 태그가 비면 루트 */
	static UActorComponent* FindPart(const AJY_Object* Object, FName PartTag, TSubclassOf<UActorComponent> PartClass);

	template <typename T>
	static T* FindPart(const AJY_Object* Object, FName PartTag)
	{
		return Cast<T>(FindPart(Object, PartTag, T::StaticClass()));
	}
};
