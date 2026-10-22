#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "JY_CheatManager.generated.h"

class AJY_PlayerState;

/* 시험용 콘솔 명령 (치트가 허용된 서버만) */
UCLASS(Within = PlayerController)
class UJY_CheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/* 내 체력 깎기 (예: JYDamage 10) */
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void JYDamage(float Amount);

	/* 내 체력 채우기 (예: JYHeal 10) */
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void JYHeal(float Amount);

	/* 영혼석 더하기 (예: JYAddSoul 100) */
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void JYAddSoul(int32 Amount);

	/* 스킬 얻기 (예: JYGiveSkill Plunge 1) — 이름 = Skill 태그 끝 이름, 숫자 = 슬롯 키 번호(1부터) */
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void JYGiveSkill(const FString& SkillName, int32 SlotNumber);

	/* 스킬 슬롯 1칸 늘리기 */
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void JYUnlockSkillSlot();

	/* 쿨타임 무시 켜기·끄기 (예: JYNoCooldown 1) — 내 쪽과 서버 양쪽에 켠다 */
	UFUNCTION(Exec)
	void JYNoCooldown(bool bEnable);

	/* 지금 방을 깬 것으로 처리해 출구 문 열기 */
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void JYClearRoom();

	/* 내 주변 부서지는 물체를 모두 부수기 (예: JYBreakProps 500) */
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void JYBreakProps(float Radius);

	/* 내 캐릭터가 맞은 것처럼 피격 반응 보기 (예: JYHitReact F Spike) — 방향 F·B·L·R = 때린 쪽, 이름 = GameplayCue.Hit 끝 이름(비우면 Spike) */
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void JYHitReact(const FString& Direction, const FString& CueName);

	/* 안 죽기 켜기·끄기 (예: JYUnlimitedHealth 0) — 기본 켜짐 */
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void JYUnlimitedHealth(bool bEnable);

private:
	/* 음수=데미지, 양수=회복 */
	void ApplyHealthDelta(float Delta);

	AJY_PlayerState* GetJYPlayerState() const;
};
