#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "JY_AnimGraphToolset.generated.h"

class UEdGraph;
class UEdGraphNode;
class UAnimSequence;
class UAnimMontage;
class UAnimBlueprint;
class USkeleton;
class USkeletalMesh;

/* 애님 노드의 함수 칸 (디테일 패널 Functions 항목) */
UENUM()
enum class EJY_AnimNodeFunctionSlot : uint8
{
	InitialUpdate,
	BecomeRelevant,
	Update
};

/*
 * MCP 도구: 애님 블루프린트 상태 머신 편집 + 애니 에셋 작업.
 * 엔진 BlueprintTools는 중첩 그래프(상태·전환 안쪽)에서 실패하고 상태 노드를 못 만들어 보완한다.
 * 함수 위 문서 주석은 MCP 도구 설명으로 쓰인다(UHT 툴팁)
 */
UCLASS(BlueprintType, Hidden)
class UJY_AnimGraphToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * 노드가 가진 하위 그래프를 돌려준다 (상태 머신 노드 → 상태 머신 그래프, 상태 → 상태 그래프, 전환 → 전환 규칙 그래프).
	 * @param Node 상태 머신·상태·전환 노드
	 * @return 하위 그래프 목록. 없으면 빈 배열
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|AnimGraph")
	static TArray<UEdGraph*> GetSubGraphs(UEdGraphNode* Node);

	/**
	 * 그래프의 모든 노드를 돌려준다 (상태 머신 그래프의 엔트리·상태·전환 포함. BlueprintTools.find_nodes는 이것들을 못 찾는다).
	 * @param Graph 아무 그래프
	 * @return 노드 목록
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|AnimGraph")
	static TArray<UEdGraphNode*> ListGraphNodes(UEdGraph* Graph);

	/**
	 * 상태 머신 그래프에 상태를 추가한다.
	 * @param StateMachineGraph GetSubGraphs(상태 머신 노드)로 얻은 그래프
	 * @param StateName 상태 이름 (상태 그래프 이름도 같아진다)
	 * @param Position 노드 위치
	 * @return 만든 상태 노드
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|AnimGraph")
	static UEdGraphNode* AddState(UEdGraph* StateMachineGraph, const FString& StateName, FIntPoint Position);

	/**
	 * 상태 머신의 시작 상태를 정한다 (엔트리 노드 → 상태 연결. 엔트리는 K2 노드가 아니라 BlueprintTools.connect_pins로 못 잇는다).
	 * @param StateMachineGraph 상태 머신 그래프
	 * @param State 시작 상태 노드
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|AnimGraph")
	static void SetEntryState(UEdGraph* StateMachineGraph, UEdGraphNode* State);

	/**
	 * 노드 이름을 바꾼다 (상태 머신·상태 노드. 하위 그래프 이름도 같이 바뀐다).
	 * @param Node 대상 노드
	 * @param NewName 새 이름
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|AnimGraph")
	static void RenameNode(UEdGraphNode* Node, const FString& NewName);

	/**
	 * 두 상태 사이에 전환(화살표)을 추가한다. 규칙 그래프는 GetSubGraphs(전환 노드)로 얻는다.
	 * @param FromState 출발 상태
	 * @param ToState 도착 상태
	 * @return 만든 전환 노드
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|AnimGraph")
	static UEdGraphNode* AddTransition(UEdGraphNode* FromState, UEdGraphNode* ToState);

	/**
	 * 그래프에서 만들 수 있는 노드 종류를 찾는다 (중첩 그래프에서도 동작. BlueprintTools.find_node_types와 같은 type_id 형식).
	 * @param Graph 대상 그래프
	 * @param TypeIdFilter type_id에 포함될 문자열 (대소문자 무시)
	 * @return type_id 목록
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|AnimGraph")
	static TArray<FString> FindNodeTypes(UEdGraph* Graph, const FString& TypeIdFilter);

	/**
	 * 그래프에 노드를 만든다 (중첩 그래프에서도 동작).
	 * @param Graph 대상 그래프
	 * @param TypeId FindNodeTypes가 돌려준 type_id (정확히 일치)
	 * @param Position 노드 위치
	 * @return 만든 노드
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|AnimGraph")
	static UEdGraphNode* CreateNode(UEdGraph* Graph, const FString& TypeId, FIntPoint Position);

	/**
	 * 애님 노드에 노드 함수를 지정한다 (디테일 패널에서 고르는 것과 같은 경로 — ObjectTools.set_properties로 넣으면 노드 관련성 확장이 안 붙어 런타임 check 크래시).
	 * @param Node 애님 그래프 노드 (SequenceEvaluator 등)
	 * @param Slot 함수 칸
	 * @param FunctionName 애님 인스턴스(부모 C++ 포함)의 BlueprintThreadSafe 함수 이름
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|AnimGraph")
	static void SetNodeFunction(UEdGraphNode* Node, EJY_AnimNodeFunctionSlot Slot, FName FunctionName);

	/**
	 * 오브젝트의 최상위 속성 하나를 디테일 패널에서 바꾼 것처럼 넣는다 (Modify → PreEditChange → 값 → PostEditChangeProperty).
	 * ObjectTools.set_properties는 편집 이벤트가 안 돌아 재압축·확장 요청·노드 갱신 같은 후처리가 빠질 때 쓴다.
	 * 블루프린트 에셋을 넣으면 CDO가 아니라 에셋 자체를 고친다.
	 * @param Object 대상 오브젝트
	 * @param PropertyName 속성 이름 (예: AlwaysDynamicProperties, CurveCompressionSettings)
	 * @param ValueText 언리얼 텍스트 형식 값 (예: ("ExplicitTime"), /Game/X.X). 인스턴스 하위 오브젝트 속성이면 클래스 경로 (예: /Script/Engine.AnimCurveCompressionCodec_UniformIndexable)
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Editor")
	static void SetPropertyAsText(UObject* Object, FName PropertyName, const FString& ValueText);

	/**
	 * 팩토리로 에셋을 만든다 (콘텐츠 브라우저 '새로 만들기'와 같은 경로).
	 * @param AssetClass 만들 에셋 클래스
	 * @param FactoryClass UFactory 하위 클래스 (예: /Script/UnrealEd.AnimCurveCompressionSettingsFactory)
	 * @param FolderPath 저장 폴더
	 * @param AssetName 에셋 이름
	 * @return 만든 에셋
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Editor")
	static UObject* CreateAssetWithFactory(UClass* AssetClass, UClass* FactoryClass, const FString& FolderPath, const FString& AssetName);

	/**
	 * 애니 커브 값을 일정 간격으로 읽는다 (거리 커브 범위 확인 등).
	 * @param Sequence 대상 애니
	 * @param CurveName 커브 이름
	 * @param SampleCount 샘플 수 (처음·끝 포함)
	 * @return "시간 값" 문자열 목록
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Animation")
	static TArray<FString> SampleCurve(UAnimSequence* Sequence, FName CurveName, int32 SampleCount);

	/**
	 * 본의 로컬 위치를 일정 간격으로 읽는다 (점프 도약 시점 확인 등).
	 * @param Sequence 대상 애니
	 * @param BoneName 본 이름 (예: pelvis, root)
	 * @param SampleCount 샘플 수 (처음·끝 포함)
	 * @return "시간 X Y Z" 문자열 목록
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Animation")
	static TArray<FString> SampleBoneLocation(UAnimSequence* Sequence, FName BoneName, int32 SampleCount);

	/**
	 * 애님 블루프린트를 만든다 (콘텐츠 브라우저 '애니메이션 블루프린트 생성'과 같은 팩토리).
	 * @param Skeleton 대상 스켈레톤
	 * @param ParentClass 부모 AnimInstance 클래스
	 * @param PreviewMesh 미리보기 메시 (없으면 null)
	 * @param FolderPath 저장 폴더 (예: /Game/Han/Anims)
	 * @param AssetName 에셋 이름
	 * @return 만든 애님 블루프린트
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Animation")
	static UAnimBlueprint* CreateAnimBlueprint(USkeleton* Skeleton, UClass* ParentClass, USkeletalMesh* PreviewMesh, const FString& FolderPath, const FString& AssetName);

	/**
	 * 애니 시퀀스로 몽타주 에셋을 만든다 (콘텐츠 브라우저 '애님 몽타주 생성'과 같은 팩토리).
	 * @param Sequence 원본 애니
	 * @param FolderPath 저장 폴더 (예: /Game/Han/Anims)
	 * @param AssetName 에셋 이름
	 * @return 만든 몽타주
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Animation")
	static UAnimMontage* CreateMontage(UAnimSequence* Sequence, const FString& FolderPath, const FString& AssetName);

	/**
	 * 애니 시퀀스에 애니메이션 모디파이어를 적용한다 (Animation Data Modifiers 창의 Apply와 같은 함수).
	 * @param Sequence 대상 애니
	 * @param ModifierClass UAnimationModifier 하위 클래스 (예: /Script/JY_RoguelikeEditor.JY_DistanceCurveModifier)
	 * @param PropertyValues 모디파이어 설정값 (속성 이름 → 텍스트 값, 예: StopSpeedThreshold → 30). 빈 맵이면 기본값
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Animation")
	static void ApplyAnimationModifier(UAnimSequence* Sequence, UClass* ModifierClass, const TMap<FName, FString>& PropertyValues);

	/**
	 * 애니 시퀀스에 적용된 모디파이어를 되돌린다 (Animation Data Modifiers 창의 Revert와 같은 함수).
	 * @param Sequence 대상 애니
	 * @param ModifierClass 되돌릴 모디파이어 클래스
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Animation")
	static void RevertAnimationModifier(UAnimSequence* Sequence, UClass* ModifierClass);
};
