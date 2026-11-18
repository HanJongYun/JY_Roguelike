#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "JY_StateTreeToolset.generated.h"

class UMassEntityConfigAsset;
class UMassEntityTraitBase;
class UStateTree;

/* MCP 도구: Mass StateTree 에셋 만들기·상태·Task·전환 추가·컴파일, Mass Config에 Trait 추가 (함수 문서 주석 = 도구 설명) */
UCLASS(BlueprintType, Hidden)
class UJY_StateTreeToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Mass용 StateTree 에셋(스키마 UMassStateTreeSchema)을 새로 만든다. 맨 위 상태 "Root" 하나가 들어 있다. 저장은 하지 않는다.
	 * @param AssetPath 만들 에셋 경로 (예: /Game/Han/Mass/ST_JY_MassEnemy_Melee)
	 * @return 만든 에셋, 이미 있거나 실패면 null
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|StateTree")
	static UStateTree* CreateMassStateTree(const FString& AssetPath);

	/**
	 * 상태를 부모 상태 아래 맨 끝에 추가한다. 이름은 트리 안에서 겹치지 않게 쓴다.
	 * @param StateTree 대상 에셋
	 * @param ParentStateName 부모 상태 이름 (예: Root)
	 * @param StateName 새 상태 이름
	 * @return 성공이면 빈 문자열, 아니면 실패 이유
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|StateTree")
	static FString AddState(UStateTree* StateTree, const FString& ParentStateName, const FString& StateName);

	/**
	 * 상태에 Task를 추가한다.
	 * @param StateTree 대상 에셋
	 * @param StateName Task를 넣을 상태 이름
	 * @param TaskStructPath Task 구조체 경로 (예: /Script/JY_Roguelike.JY_MassChaseTask)
	 * @param TaskPropertiesText Task 속성 값, 구조체 텍스트 형식 (예: (Duration=1.0)) — 없으면 빈 문자열
	 * @return 성공이면 빈 문자열, 아니면 실패 이유
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|StateTree")
	static FString AddTask(UStateTree* StateTree, const FString& StateName, const FString& TaskStructPath, const FString& TaskPropertiesText);

	/**
	 * 상태에 다른 상태로 가는 전환을 맨 끝에 추가한다. 전환은 위에서부터 먼저 맞는 것이 쓰인다.
	 * @param StateTree 대상 에셋
	 * @param FromStateName 전환을 달 상태 이름
	 * @param Trigger 언제 검사하나: OnStateCompleted / OnStateSucceeded / OnStateFailed / OnTick
	 * @param ToStateName 갈 상태 이름
	 * @param ConditionStructPath 조건 구조체 경로 (예: /Script/JY_Roguelike.JY_MassInsideRangeCondition) — 조건이 없으면 빈 문자열
	 * @param ConditionPropertiesText 조건 속성 값, 구조체 텍스트 형식 (예: (bInvert=True)) — 없으면 빈 문자열
	 * @return 성공이면 빈 문자열, 아니면 실패 이유
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|StateTree")
	static FString AddTransition(UStateTree* StateTree, const FString& FromStateName, const FString& Trigger, const FString& ToStateName, const FString& ConditionStructPath, const FString& ConditionPropertiesText);

	/**
	 * StateTree를 컴파일한다(에디터 Compile 버튼과 같음). 저장은 하지 않는다.
	 * @param StateTree 대상 에셋
	 * @return 성공이면 "OK", 아니면 컴파일 메시지
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|StateTree")
	static FString CompileStateTree(UStateTree* StateTree);

	/**
	 * Mass Config 에셋(DA_MassEnemy_*)의 Trait 목록 끝에 Trait를 하나 추가한다. 값은 돌려받은 Trait에 ObjectTools.set_properties로 넣는다. 저장은 하지 않는다.
	 * @param ConfigAsset 대상 Mass Config 에셋
	 * @param TraitClass 추가할 Trait 클래스 (예: /Script/MassAIBehavior.MassStateTreeTrait)
	 * @return 추가한 Trait, 실패면 null
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|StateTree")
	static UMassEntityTraitBase* AddTraitToMassConfig(UMassEntityConfigAsset* ConfigAsset, UClass* TraitClass);
};
