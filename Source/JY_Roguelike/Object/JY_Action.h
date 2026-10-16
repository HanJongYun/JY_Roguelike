#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JY_Action.generated.h"

class AJY_Object;
class APawn;
class UActorComponent;

/*
 * 물체가 상태에 들어갈 때 하는 일 하나 (범위 피해·부서짐·화면 열기·이동·부품 이동·이펙트).
 * 물체에 ASC가 없어 GA 대신 쓰는 모듈 — 종류를 늘릴 때 이 클래스의 자식 하나를 추가한다.
 * 실행 위치는 자식이 재정의한 함수로 정해진다: 서버 판정 / 모든 화면 연출 / 쓴 플레이어 화면.
 * 물체 BP의 상태별 Actions 목록에 Instanced로 들어가 물체마다 자기 사본을 가진다
 */
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

	/*
	 * [Client] (리슨 서버 포함, 데디 서버 제외) 상태에 들어감 — 화면 연출.
	 * bCatchUp = 늦게 받은 상태(늦은 접속 등)라 결과만 맞춘다: 한 번 터지는 연출은 건너뛰고, 움직임은 바로 끝 위치로
	 */
	virtual void OnClientEnter(AJY_Object* Object, bool bCatchUp) {}

	/* [Client] 상태에서 나감 — 켜 둔 연출 끄기 */
	virtual void OnClientExit(AJY_Object* Object) {}

	/* [OwnClient] E키를 쓴 플레이어 화면에서만 (Active 상태 Action) */
	virtual void OnLocalUserActivate(AJY_Object* Object, APawn* User) {}

	/* [Client] AJY_Object::AddTickingAction으로 등록한 동안 매 프레임. false를 돌려주면 등록 해제 */
	virtual bool TickClient(AJY_Object* Object, float DeltaSeconds) { return false; }

protected:
	/* BP에서 이 태그(Component Tags)를 단 부품 — 저장하지 않고 쓸 때마다 찾는다. 태그가 비면 루트 */
	static UActorComponent* FindPart(const AJY_Object* Object, FName PartTag, TSubclassOf<UActorComponent> PartClass);

	template <typename T>
	static T* FindPart(const AJY_Object* Object, FName PartTag)
	{
		return Cast<T>(FindPart(Object, PartTag, T::StaticClass()));
	}
};
