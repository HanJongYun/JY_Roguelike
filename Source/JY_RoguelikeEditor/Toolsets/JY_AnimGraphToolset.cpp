#include "Toolsets/JY_AnimGraphToolset.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationStateMachineSchema.h"
#include "AnimStateNode.h"
#include "AnimStateEntryNode.h"
#include "AnimGraphNode_Base.h"
#include "AnimStateTransitionNode.h"
#include "AnimationModifier.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "Factories/AnimMontageFactory.h"
#include "Factories/AnimBlueprintFactory.h"
#include "Factories/Factory.h"
#include "AnimationBlueprintLibrary.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_AnimGraphToolset)

namespace JY_AnimGraphToolset
{
	/* BlueprintGraphEditor::ListAvailableNodes(BlueprintGraphEditor.cpp:1788)와 같은 type_id 형식: "카테고리|메뉴이름" 공백 제거 */
	FString MakeTypeId(const FBlueprintActionUiSpec& UiSpec)
	{
		const FString Category = UiSpec.Category.ToString();
		const FString MenuName = UiSpec.MenuName.ToString();
		return FString::Printf(TEXT("%s|%s"), *Category.Replace(TEXT(" "), TEXT("")), *MenuName.Replace(TEXT(" "), TEXT("")));
	}

	/*
	 * 그래프에 쓸 수 있는 노드 생성기를 하나씩 넘긴다. Visitor가 false를 돌려주면 멈춘다.
	 * 블루프린트는 그래프 바깥 사슬을 따라 찾는다(FindBlueprintForGraph) — 엔진 Python 툴셋은 바로 위 Outer만 봐서 중첩 그래프에서 실패한다
	 */
	void ForEachAvailableAction(UEdGraph* Graph, TFunctionRef<bool(const FString& TypeId, const UBlueprintNodeSpawner* Spawner, const FBlueprintActionInfo& ActionInfo)> Visitor)
	{
		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
		FBlueprintActionFilter Filter;
		Filter.Context.Graphs.Add(Graph);
		Filter.Context.Blueprints.Add(Blueprint);

		FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
		const FBlueprintActionDatabase::FActionRegistry& ActionRegistry = ActionDatabase.GetAllActions();
		for (const TPair<FObjectKey, FBlueprintActionDatabase::FActionList>& Entry : ActionRegistry)
		{
			UObject* ActionObject = Entry.Key.ResolveObjectPtr();
			if (ActionObject == nullptr)
				continue;

			for (const UBlueprintNodeSpawner* Spawner : Entry.Value)
			{
				if (Spawner == nullptr)
					continue;

				FBlueprintActionInfo ActionInfo(ActionObject, Spawner);
				if (Filter.IsFiltered(ActionInfo) == true)
					continue;

				const FBlueprintActionUiSpec UiSpec = Spawner->GetUiSpec(Filter.Context, ActionInfo.GetBindings());
				const FString TypeId = MakeTypeId(UiSpec);
				if (Visitor(TypeId, Spawner, ActionInfo) == false)
					return;
			}
		}
	}

	void MarkModified(UEdGraph* Graph)
	{
		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
		if (Blueprint != nullptr)
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		}
	}
}

TArray<UEdGraph*> UJY_AnimGraphToolset::GetSubGraphs(UEdGraphNode* Node)
{
	if (Node == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Node is null."));
		return {};
	}

	return Node->GetSubGraphs();
}

TArray<UEdGraphNode*> UJY_AnimGraphToolset::ListGraphNodes(UEdGraph* Graph)
{
	if (Graph == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Graph is null."));
		return {};
	}

	TArray<UEdGraphNode*> Result;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		Result.Add(Node);
	}
	return Result;
}

/* 에디터의 '상태 추가' 메뉴와 같은 경로(FEdGraphSchemaAction_NewStateNode) — 상태 그래프·결과 노드까지 함께 생긴다 */
UEdGraphNode* UJY_AnimGraphToolset::AddState(UEdGraph* StateMachineGraph, const FString& StateName, FIntPoint Position)
{
	if (Cast<UAnimationStateMachineGraph>(StateMachineGraph) == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("StateMachineGraph is not a state machine graph. Use GetSubGraphs on the state machine node."));
		return nullptr;
	}

	UAnimStateNode* Template = NewObject<UAnimStateNode>();
	const FVector2f Location(Position.X, Position.Y);
	UAnimStateNode* StateNode = FEdGraphSchemaAction_NewStateNode::SpawnNodeFromTemplate<UAnimStateNode>(StateMachineGraph, Template, Location, false);
	if (StateNode == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Failed to create state node."));
		return nullptr;
	}

	/* 상태 이름 = 상태 그래프 이름. UEdGraphNode 포인터로 불러 가상 함수로 간다 (UAnimStateNodeBase는 MinimalAPI) */
	UEdGraphNode* NodeBase = StateNode;
	NodeBase->OnRenameNode(StateName);
	JY_AnimGraphToolset::MarkModified(StateMachineGraph);
	return StateNode;
}

/* 에디터에서 엔트리 핀을 상태로 끄는 것과 같다 — 스키마 연결이라 기존 시작 상태 연결은 끊긴다 */
void UJY_AnimGraphToolset::SetEntryState(UEdGraph* StateMachineGraph, UEdGraphNode* State)
{
	UAnimationStateMachineGraph* MachineGraph = Cast<UAnimationStateMachineGraph>(StateMachineGraph);
	UAnimStateNodeBase* StateNode = Cast<UAnimStateNodeBase>(State);
	if (MachineGraph == nullptr || StateNode == nullptr || MachineGraph->EntryNode == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("StateMachineGraph must be a state machine graph and State a state node."));
		return;
	}

	UEdGraphPin* EntryPin = MachineGraph->EntryNode->GetOutputPin();
	UEdGraphPin* StatePin = StateNode->GetInputPin();
	const UEdGraphSchema* Schema = MachineGraph->GetSchema();
	if (Schema->TryCreateConnection(EntryPin, StatePin) == false)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Failed to connect entry to state."));
	}
}

void UJY_AnimGraphToolset::RenameNode(UEdGraphNode* Node, const FString& NewName)
{
	if (Node == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Node is null."));
		return;
	}

	Node->OnRenameNode(NewName);
	JY_AnimGraphToolset::MarkModified(Node->GetGraph());
}

/* 에디터에서 상태끼리 선을 끌 때와 같은 생성 방식 (UAnimationStateMachineSchema::CreateAutomaticConversionNodeAndConnections) */
UEdGraphNode* UJY_AnimGraphToolset::AddTransition(UEdGraphNode* FromState, UEdGraphNode* ToState)
{
	UAnimStateNodeBase* FromNode = Cast<UAnimStateNodeBase>(FromState);
	UAnimStateNodeBase* ToNode = Cast<UAnimStateNodeBase>(ToState);
	if (FromNode == nullptr || ToNode == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("FromState and ToState must be state nodes."));
		return nullptr;
	}

	UEdGraph* Graph = FromNode->GetGraph();
	const FVector2f FromLocation(FromNode->NodePosX, FromNode->NodePosY);
	const FVector2f ToLocation(ToNode->NodePosX, ToNode->NodePosY);
	const FVector2f Location = (FromLocation + ToLocation) * 0.5f;
	UAnimStateTransitionNode* Template = NewObject<UAnimStateTransitionNode>();
	UAnimStateTransitionNode* TransitionNode = FEdGraphSchemaAction_NewStateNode::SpawnNodeFromTemplate<UAnimStateTransitionNode>(Graph, Template, Location, false);
	if (TransitionNode == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Failed to create transition node."));
		return nullptr;
	}

	TransitionNode->CreateConnections(FromNode, ToNode);
	JY_AnimGraphToolset::MarkModified(Graph);
	return TransitionNode;
}

TArray<FString> UJY_AnimGraphToolset::FindNodeTypes(UEdGraph* Graph, const FString& TypeIdFilter)
{
	if (Graph == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Graph is null."));
		return {};
	}

	TArray<FString> Result;
	auto CollectMatch = [&Result, &TypeIdFilter](const FString& TypeId, const UBlueprintNodeSpawner* Spawner, const FBlueprintActionInfo& ActionInfo)
	{
		if (TypeId.Contains(TypeIdFilter, ESearchCase::IgnoreCase) == true)
		{
			Result.AddUnique(TypeId);
		}
		return true;
	};
	JY_AnimGraphToolset::ForEachAvailableAction(Graph, CollectMatch);
	return Result;
}

UEdGraphNode* UJY_AnimGraphToolset::CreateNode(UEdGraph* Graph, const FString& TypeId, FIntPoint Position)
{
	if (Graph == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Graph is null."));
		return nullptr;
	}

	UEdGraphNode* SpawnedNode = nullptr;
	const FVector2D Location(Position.X, Position.Y);
	auto SpawnIfMatch = [&SpawnedNode, &TypeId, Graph, Location](const FString& CandidateTypeId, const UBlueprintNodeSpawner* Spawner, const FBlueprintActionInfo& ActionInfo)
	{
		if (CandidateTypeId.Equals(TypeId, ESearchCase::CaseSensitive) == false)
			return true;

		SpawnedNode = Spawner->Invoke(Graph, ActionInfo.GetBindings(), Location);
		return SpawnedNode == nullptr;
	};
	JY_AnimGraphToolset::ForEachAvailableAction(Graph, SpawnIfMatch);

	if (SpawnedNode == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Could not create node '%s'. Use FindNodeTypes for exact type ids."), *TypeId));
		return nullptr;
	}

	JY_AnimGraphToolset::MarkModified(Graph);
	return SpawnedNode;
}

/*
 * 디테일 패널과 같은 순서(PreEditChange → 값 → PostEditChangeProperty).
 * UAnimGraphNode_Base::PostEditChangeProperty(AnimGraphNode_Base.cpp:103)가 런타임 노드 함수 연결·확장 요청(NodeRelevancy)·노드 재구성을 한다
 */
void UJY_AnimGraphToolset::SetNodeFunction(UEdGraphNode* Node, EJY_AnimNodeFunctionSlot Slot, FName FunctionName)
{
	UAnimGraphNode_Base* AnimNode = Cast<UAnimGraphNode_Base>(Node);
	if (AnimNode == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Node must be an anim graph node."));
		return;
	}

	FName PropertyName = NAME_None;
	switch (Slot)
	{
	case EJY_AnimNodeFunctionSlot::InitialUpdate:
		PropertyName = TEXT("InitialUpdateFunction");
		break;
	case EJY_AnimNodeFunctionSlot::BecomeRelevant:
		PropertyName = TEXT("BecomeRelevantFunction");
		break;
	case EJY_AnimNodeFunctionSlot::Update:
		PropertyName = TEXT("UpdateFunction");
		break;
	}

	FProperty* Property = FindFProperty<FProperty>(UAnimGraphNode_Base::StaticClass(), PropertyName);
	FMemberReference* FunctionReference = Property->ContainerPtrToValuePtr<FMemberReference>(AnimNode);

	AnimNode->PreEditChange(Property);
	FunctionReference->SetSelfMember(FunctionName);
	FPropertyChangedEvent ChangedEvent(Property);
	AnimNode->PostEditChangeProperty(ChangedEvent);
}

/* 디테일 패널과 같은 편집 이벤트 순서 — 후처리(애니 재압축, 애님 노드 확장 요청 등)가 원래대로 돈다 */
void UJY_AnimGraphToolset::SetPropertyAsText(UObject* Object, FName PropertyName, const FString& ValueText)
{
	if (Object == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Object is null."));
		return;
	}

	UClass* ObjectClass = Object->GetClass();
	FProperty* Property = FindFProperty<FProperty>(ObjectClass, PropertyName);
	if (Property == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Property '%s' not found on %s."), *PropertyName.ToString(), *ObjectClass->GetName()));
		return;
	}

	/*
	 * 인라인 하위 오브젝트(Instanced·EditInline, 예: 커브 압축 Codec)는 텍스트로 클래스 경로를 준다 — 디테일 패널 클래스 선택처럼 새 인스턴스를 만든다.
	 * 오브젝트 속성인데 값이 그 속성 타입의 하위 클래스면 인라인 생성으로 본다 (에셋 참조는 클래스가 아니라 이 분기에 안 걸린다)
	 */
	FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property);
	UClass* InstanceClass = nullptr;
	if (ObjectProperty != nullptr && ObjectProperty->PropertyClass != UClass::StaticClass())
	{
		UClass* ValueClass = FindObject<UClass>(nullptr, *ValueText);
		if (ValueClass != nullptr && ValueClass->IsChildOf(ObjectProperty->PropertyClass) == true)
		{
			InstanceClass = ValueClass;
		}
	}
	const bool bIsInstancedObject = InstanceClass != nullptr;

	Object->Modify();
	Object->PreEditChange(Property);
	const TCHAR* ImportResult = TEXT("");
	if (bIsInstancedObject == true)
	{
		UObject* Instance = NewObject<UObject>(Object, InstanceClass, NAME_None, RF_Transactional);
		ObjectProperty->SetObjectPropertyValue_InContainer(Object, Instance);
	}
	else
	{
		void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
		ImportResult = Property->ImportText_Direct(*ValueText, ValuePtr, Object, PPF_None);
	}
	FPropertyChangedEvent ChangedEvent(Property, EPropertyChangeType::ValueSet);
	Object->PostEditChangeProperty(ChangedEvent);

	if (ImportResult == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Could not parse '%s' for property '%s'."), *ValueText, *PropertyName.ToString()));
		return;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(Object);
	UEdGraphNode* GraphNode = Cast<UEdGraphNode>(Object);
	if (Blueprint == nullptr && GraphNode != nullptr)
	{
		Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(GraphNode);
	}
	if (Blueprint != nullptr)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}
}

UObject* UJY_AnimGraphToolset::CreateAssetWithFactory(UClass* AssetClass, UClass* FactoryClass, const FString& FolderPath, const FString& AssetName)
{
	if (AssetClass == nullptr || FactoryClass == nullptr || FactoryClass->IsChildOf(UFactory::StaticClass()) == false)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("AssetClass and a UFactory FactoryClass are required."));
		return nullptr;
	}

	UFactory* Factory = NewObject<UFactory>(GetTransientPackage(), FactoryClass);
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& AssetTools = AssetToolsModule.Get();
	UObject* NewAsset = AssetTools.CreateAsset(AssetName, FolderPath, AssetClass, Factory);
	if (NewAsset == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Failed to create asset (name may already exist)."));
	}
	return NewAsset;
}

TArray<FString> UJY_AnimGraphToolset::SampleCurve(UAnimSequence* Sequence, FName CurveName, int32 SampleCount)
{
	if (Sequence == nullptr || SampleCount < 2)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Sequence is required and SampleCount must be at least 2."));
		return {};
	}

	TArray<FString> Result;
	const float Length = Sequence->GetPlayLength();
	for (int32 Index = 0; Index < SampleCount; ++Index)
	{
		const float Time = Length * Index / (SampleCount - 1);
		const float Value = UAnimationBlueprintLibrary::GetFloatValueAtTime(Sequence, CurveName, Time);
		Result.Add(FString::Printf(TEXT("%.3f %.2f"), Time, Value));
	}
	return Result;
}

TArray<FString> UJY_AnimGraphToolset::SampleBoneLocation(UAnimSequence* Sequence, FName BoneName, int32 SampleCount)
{
	if (Sequence == nullptr || SampleCount < 2)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Sequence is required and SampleCount must be at least 2."));
		return {};
	}

	/* 원본 애니 데이터(압축 전)에서 본 트랙을 읽는다 — GetBonePoseForTime은 5.8에서 폐기 예정 */
	IAnimationDataModel* DataModel = Sequence->GetDataModel();
	const FFrameRate FrameRate = DataModel->GetFrameRate();

	TArray<FString> Result;
	const float Length = Sequence->GetPlayLength();
	for (int32 Index = 0; Index < SampleCount; ++Index)
	{
		const float Time = Length * Index / (SampleCount - 1);
		const FFrameTime FrameTime = FrameRate.AsFrameTime(Time);
		const FTransform Pose = DataModel->EvaluateBoneTrackTransform(BoneName, FrameTime, EAnimInterpolationType::Linear);
		const FVector Location = Pose.GetLocation();
		Result.Add(FString::Printf(TEXT("%.3f %.1f %.1f %.1f"), Time, Location.X, Location.Y, Location.Z));
	}
	return Result;
}

/* 루트까지 부모 사슬을 따라 로컬 변환을 곱한다. 트랙이 없는 본은 레퍼런스 포즈 값 */
TArray<FString> UJY_AnimGraphToolset::SampleBoneComponentSpace(UAnimSequence* Sequence, FName BoneName, int32 SampleCount)
{
	if (Sequence == nullptr || SampleCount < 2)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Sequence is required and SampleCount must be at least 2."));
		return {};
	}

	const USkeleton* Skeleton = Sequence->GetSkeleton();
	if (Skeleton == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Sequence has no skeleton."));
		return {};
	}

	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	const int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
	if (BoneIndex == INDEX_NONE)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Bone %s not found."), *BoneName.ToString()));
		return {};
	}

	/* 대상 본 → 루트 순서 */
	TArray<int32> Chain;
	for (int32 Index = BoneIndex; Index != INDEX_NONE; Index = RefSkeleton.GetParentIndex(Index))
	{
		Chain.Add(Index);
	}

	IAnimationDataModel* DataModel = Sequence->GetDataModel();
	const FFrameRate FrameRate = DataModel->GetFrameRate();
	const TArray<FTransform>& RefPose = RefSkeleton.GetRefBonePose();

	TArray<FString> Result;
	const float Length = Sequence->GetPlayLength();
	for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
	{
		const float Time = Length * SampleIndex / (SampleCount - 1);
		const FFrameTime FrameTime = FrameRate.AsFrameTime(Time);

		FTransform ComponentTransform = FTransform::Identity;
		for (const int32 ChainIndex : Chain)
		{
			const FName ChainBoneName = RefSkeleton.GetBoneName(ChainIndex);
			FTransform Local = RefPose[ChainIndex];
			if (DataModel->IsValidBoneTrackName(ChainBoneName) == true)
			{
				Local = DataModel->EvaluateBoneTrackTransform(ChainBoneName, FrameTime, EAnimInterpolationType::Linear);
			}
			ComponentTransform = ComponentTransform * Local;
		}

		const FVector Location = ComponentTransform.GetLocation();
		const FRotator Rotation = ComponentTransform.Rotator();
		Result.Add(FString::Printf(TEXT("%.3f %.1f %.1f %.1f %.1f %.1f"), Time, Location.X, Location.Y, Location.Z, Rotation.Yaw, Rotation.Pitch));
	}
	return Result;
}

/* 노티파이 클래스마다 헤더를 넣지 않고 리플렉션으로 오브젝트·이름 속성만 꺼낸다 (팩 전용 노티파이도 읽힘) */
TArray<FString> UJY_AnimGraphToolset::ListAnimNotifies(UAnimSequenceBase* Animation)
{
	if (Animation == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Animation is required."));
		return {};
	}

	auto DescribeObject = [](const UObject* Object) -> FString
	{
		if (Object == nullptr)
			return TEXT("");

		FString Text = Object->GetClass()->GetName();
		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			const FProperty* Property = *It;
			const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
			if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
			{
				const UObject* Value = ObjectProperty->GetObjectPropertyValue(ValuePtr);
				if (Value != nullptr)
				{
					Text += FString::Printf(TEXT(" %s=%s"), *Property->GetName(), *Value->GetPathName());
				}
			}
			else if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
			{
				const FName Value = NameProperty->GetPropertyValue(ValuePtr);
				if (Value.IsNone() == false)
				{
					Text += FString::Printf(TEXT(" %s=%s"), *Property->GetName(), *Value.ToString());
				}
			}
		}
		return Text;
	};

	TArray<FString> Result;
	for (const FAnimNotifyEvent& Event : Animation->Notifies)
	{
		const UObject* NotifyObject = Event.Notify.Get();
		if (NotifyObject == nullptr)
		{
			NotifyObject = Event.NotifyStateClass.Get();
		}
		const FString Description = DescribeObject(NotifyObject);
		Result.Add(FString::Printf(TEXT("%.3f %.3f %s %s"), Event.GetTriggerTime(), Event.GetDuration(), *Event.NotifyName.ToString(), *Description));
	}

	if (const UAnimMontage* Montage = Cast<UAnimMontage>(Animation))
	{
		for (const FCompositeSection& Section : Montage->CompositeSections)
		{
			Result.Add(FString::Printf(TEXT("Section %s %.3f Next=%s"), *Section.SectionName.ToString(), Section.GetTime(), *Section.NextSectionName.ToString()));
		}
		for (const FSlotAnimationTrack& SlotTrack : Montage->SlotAnimTracks)
		{
			for (const FAnimSegment& Segment : SlotTrack.AnimTrack.AnimSegments)
			{
				const UAnimSequenceBase* Reference = Segment.GetAnimReference();
				const FString ReferencePath = Reference != nullptr ? Reference->GetPathName() : TEXT("None");
				const FString SlotName = SlotTrack.SlotName.ToString();
				Result.Add(FString::Printf(TEXT("Segment %s %s %.3f..%.3f"), *SlotName, *ReferencePath, Segment.AnimStartTime, Segment.AnimEndTime));
			}
		}
	}
	return Result;
}

/* 콘텐츠 브라우저 '애니메이션 블루프린트 생성'과 같은 팩토리 — 대상 스켈레톤은 생성 때만 정할 수 있다 */
UAnimBlueprint* UJY_AnimGraphToolset::CreateAnimBlueprint(USkeleton* Skeleton, UClass* ParentClass, USkeletalMesh* PreviewMesh, const FString& FolderPath, const FString& AssetName)
{
	if (Skeleton == nullptr || ParentClass == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Skeleton and ParentClass are required."));
		return nullptr;
	}

	if (ParentClass->IsChildOf(UAnimInstance::StaticClass()) == false)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("ParentClass must derive from UAnimInstance."));
		return nullptr;
	}

	UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
	Factory->TargetSkeleton = Skeleton;
	Factory->ParentClass = ParentClass;
	Factory->PreviewSkeletalMesh = PreviewMesh;

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& AssetTools = AssetToolsModule.Get();
	UObject* NewAsset = AssetTools.CreateAsset(AssetName, FolderPath, UAnimBlueprint::StaticClass(), Factory);
	UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(NewAsset);
	if (AnimBlueprint == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Failed to create anim blueprint (name may already exist)."));
		return nullptr;
	}
	return AnimBlueprint;
}

/* 콘텐츠 브라우저 '애님 몽타주 생성'과 같은 팩토리 — 슬롯·구간·길이를 원본 애니에서 채운다 (AnimMontageFactory.cpp FactoryCreateNew) */
UAnimMontage* UJY_AnimGraphToolset::CreateMontage(UAnimSequence* Sequence, const FString& FolderPath, const FString& AssetName)
{
	if (Sequence == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Sequence is null."));
		return nullptr;
	}

	UAnimMontageFactory* Factory = NewObject<UAnimMontageFactory>();
	Factory->SourceAnimation = Sequence;

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& AssetTools = AssetToolsModule.Get();
	UObject* NewAsset = AssetTools.CreateAsset(AssetName, FolderPath, UAnimMontage::StaticClass(), Factory);
	UAnimMontage* Montage = Cast<UAnimMontage>(NewAsset);
	if (Montage == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Failed to create montage (name may already exist)."));
		return nullptr;
	}
	return Montage;
}

/* Animation Data Modifiers 창의 Apply와 같은 함수(UAnimationModifier::ApplyToAnimationSequence) — 적용 기록도 애니 에셋에 남는다 */
void UJY_AnimGraphToolset::ApplyAnimationModifier(UAnimSequence* Sequence, UClass* ModifierClass, const TMap<FName, FString>& PropertyValues)
{
	if (Sequence == nullptr || ModifierClass == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Sequence and ModifierClass are required."));
		return;
	}

	if (ModifierClass->IsChildOf(UAnimationModifier::StaticClass()) == false)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("ModifierClass must derive from UAnimationModifier."));
		return;
	}

	UAnimationModifier* Modifier = NewObject<UAnimationModifier>(GetTransientPackage(), ModifierClass);

	/* 모디파이어 창에서 값을 바꾼 것과 같다 — 적용 기록에 이 값이 복사돼 남는다 */
	for (const TPair<FName, FString>& PropertyValue : PropertyValues)
	{
		FProperty* Property = FindFProperty<FProperty>(ModifierClass, PropertyValue.Key);
		if (Property == nullptr)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Modifier property '%s' not found."), *PropertyValue.Key.ToString()));
			return;
		}

		void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Modifier);
		if (Property->ImportText_Direct(*PropertyValue.Value, ValuePtr, Modifier, PPF_None) == nullptr)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Could not parse '%s' for modifier property '%s'."), *PropertyValue.Value, *PropertyValue.Key.ToString()));
			return;
		}
	}

	Modifier->ApplyToAnimationSequence(Sequence);
}

/* Animation Data Modifiers 창의 Revert와 같은 함수 — 그 클래스로 적용된 기록을 찾아 되돌린다 (UAnimationModifier::RevertFromAnimationSequence) */
void UJY_AnimGraphToolset::RevertAnimationModifier(UAnimSequence* Sequence, UClass* ModifierClass)
{
	if (Sequence == nullptr || ModifierClass == nullptr || ModifierClass->IsChildOf(UAnimationModifier::StaticClass()) == false)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Sequence and a UAnimationModifier ModifierClass are required."));
		return;
	}

	UAnimationModifier* Modifier = NewObject<UAnimationModifier>(GetTransientPackage(), ModifierClass);
	Modifier->RevertFromAnimationSequence(Sequence);
}

TArray<FString> UJY_AnimGraphToolset::ImportFiles(const TArray<FString>& Filenames, const FString& DestinationPath)
{
	UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
	ImportData->Filenames = Filenames;
	ImportData->DestinationPath = DestinationPath;
	ImportData->bReplaceExisting = true;

	IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
	const TArray<UObject*> Imported = AssetTools.ImportAssetsAutomated(ImportData);

	TArray<FString> Paths;
	for (UObject* Asset : Imported)
		Paths.Add(Asset->GetPathName());
	return Paths;
}

namespace JY_AnimGraphToolset
{
	/* 노티파이를 넣을 트랙 이름 */
	const FName NotifyTrackName(TEXT("JY"));

	/* 몽타주 편집기 SortAndUpdateMontage와 같은 순서 (SAnimMontagePanel.cpp:354) — 링크 갱신 → 길이 → 섹션 정렬 → 노티파이 트리거 오프셋 → 저장 표시 */
	void RefreshMontage(UAnimMontage* Montage)
	{
		Montage->UpdateLinkableElements();

		const float SequenceLength = Montage->CalculateSequenceLength();
		Montage->SetCompositeLength(SequenceLength);

		auto IsSectionEarlier = [](const FCompositeSection& A, const FCompositeSection& B)
		{
			return A.GetTime() < B.GetTime();
		};
		Montage->CompositeSections.Sort(IsSectionEarlier);

		for (FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			const float NotifyTime = NotifyEvent.GetTime();
			const EAnimEventTriggerOffsets::Type TriggerOffset = Montage->CalculateOffsetForNotify(NotifyTime);
			NotifyEvent.RefreshTriggerOffset(TriggerOffset);
		}

		Montage->RefreshCacheData();
		Montage->PostEditChange();
		Montage->MarkPackageDirty();
	}

	void EnsureNotifyTrack(UAnimSequenceBase* Animation)
	{
		if (UAnimationBlueprintLibrary::IsValidAnimNotifyTrackName(Animation, NotifyTrackName) == true)
			return;

		UAnimationBlueprintLibrary::AddAnimationNotifyTrack(Animation, NotifyTrackName);
	}
}

/* SAnimSegmentsPanel::AddAnimSegment와 같은 구간 생성 (SetAnimReference(…, true)가 시작·끝·재생 속도를 애니 길이로 채움) */
float UJY_AnimGraphToolset::AppendMontageSegment(UAnimMontage* Montage, UAnimSequenceBase* Animation)
{
	if (Montage == nullptr || Animation == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Montage and Animation are required."));
		return -1.0f;
	}

	Montage->Modify();

	FSlotAnimationTrack& SlotTrack = Montage->SlotAnimTracks[0];
	FAnimTrack& AnimTrack = SlotTrack.AnimTrack;
	const float StartTime = AnimTrack.GetLength();

	FAnimSegment NewSegment;
	NewSegment.SetAnimReference(Animation, true);
	NewSegment.StartPos = StartTime;
	AnimTrack.AnimSegments.Add(NewSegment);

	JY_AnimGraphToolset::RefreshMontage(Montage);
	return StartTime;
}

bool UJY_AnimGraphToolset::SetMontageSections(UAnimMontage* Montage, const TArray<FString>& Sections)
{
	if (Montage == nullptr || Sections.Num() == 0)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Montage and at least one section are required."));
		return false;
	}

	Montage->Modify();
	Montage->CompositeSections.Reset();

	TArray<FName> NextSectionNames;
	for (const FString& SectionText : Sections)
	{
		TArray<FString> Parts;
		SectionText.ParseIntoArrayWS(OUT Parts);
		if (Parts.Num() != 3)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Bad section '%s' (expected 'Name Start Next')."), *SectionText));
			return false;
		}

		const FName SectionName(*Parts[0]);
		const float StartTime = FCString::Atof(*Parts[1]);
		Montage->AddAnimCompositeSection(SectionName, StartTime);

		const FName NextSectionName = Parts[2] == TEXT("-") ? NAME_None : FName(*Parts[2]);
		NextSectionNames.Add(NextSectionName);
	}

	/* 섹션 이름으로 다음 섹션을 찾으므로 전부 만든 뒤에 연결 */
	for (int32 SectionIndex = 0; SectionIndex < Sections.Num(); ++SectionIndex)
	{
		const FString& SectionText = Sections[SectionIndex];
		TArray<FString> Parts;
		SectionText.ParseIntoArrayWS(OUT Parts);
		const FName SectionName(*Parts[0]);
		const int32 MontageSectionIndex = Montage->GetSectionIndex(SectionName);
		FCompositeSection& Section = Montage->GetAnimCompositeSection(MontageSectionIndex);
		Section.NextSectionName = NextSectionNames[SectionIndex];
	}

	JY_AnimGraphToolset::RefreshMontage(Montage);
	return true;
}

UAnimNotify* UJY_AnimGraphToolset::AddMontageNotify(UAnimMontage* Montage, float Time, UClass* NotifyClass)
{
	if (Montage == nullptr || NotifyClass == nullptr || NotifyClass->IsChildOf(UAnimNotify::StaticClass()) == false)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Montage and a UAnimNotify class are required."));
		return nullptr;
	}

	Montage->Modify();
	JY_AnimGraphToolset::EnsureNotifyTrack(Montage);
	UAnimNotify* Notify = UAnimationBlueprintLibrary::AddAnimationNotifyEvent(Montage, JY_AnimGraphToolset::NotifyTrackName, Time, NotifyClass);
	JY_AnimGraphToolset::RefreshMontage(Montage);
	return Notify;
}

UAnimNotifyState* UJY_AnimGraphToolset::AddMontageNotifyState(UAnimMontage* Montage, float Time, float Duration, UClass* NotifyStateClass)
{
	if (Montage == nullptr || NotifyStateClass == nullptr || NotifyStateClass->IsChildOf(UAnimNotifyState::StaticClass()) == false)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Montage and a UAnimNotifyState class are required."));
		return nullptr;
	}

	Montage->Modify();
	JY_AnimGraphToolset::EnsureNotifyTrack(Montage);
	UAnimNotifyState* NotifyState = UAnimationBlueprintLibrary::AddAnimationNotifyStateEvent(Montage, JY_AnimGraphToolset::NotifyTrackName, Time, Duration, NotifyStateClass);
	JY_AnimGraphToolset::RefreshMontage(Montage);
	return NotifyState;
}

int32 UJY_AnimGraphToolset::CopyAnimNotifies(UAnimSequenceBase* Source, UAnimMontage* Destination, float TimeOffset, const FString& ClassFilter)
{
	if (Source == nullptr || Destination == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Source and Destination are required."));
		return 0;
	}

	Destination->Modify();
	JY_AnimGraphToolset::EnsureNotifyTrack(Destination);

	int32 CopiedCount = 0;
	for (const FAnimNotifyEvent& SourceEvent : Source->Notifies)
	{
		UObject* SourceObject = SourceEvent.Notify != nullptr ? static_cast<UObject*>(SourceEvent.Notify) : static_cast<UObject*>(SourceEvent.NotifyStateClass);
		if (SourceObject == nullptr)
			continue;

		const UClass* SourceClass = SourceObject->GetClass();
		const FString ClassName = SourceClass->GetName();
		if (ClassFilter.IsEmpty() == false && ClassName.Contains(ClassFilter) == false)
			continue;

		const float Time = SourceEvent.GetTime() + TimeOffset;
		if (SourceEvent.Notify != nullptr)
		{
			UAnimNotify* NotifyCopy = DuplicateObject<UAnimNotify>(SourceEvent.Notify, Destination);
			UAnimationBlueprintLibrary::AddAnimationNotifyEventObject(Destination, Time, NotifyCopy, JY_AnimGraphToolset::NotifyTrackName);
		}
		else
		{
			UAnimNotifyState* NotifyStateCopy = DuplicateObject<UAnimNotifyState>(SourceEvent.NotifyStateClass, Destination);
			const float Duration = SourceEvent.GetDuration();
			UAnimationBlueprintLibrary::AddAnimationNotifyStateEventObject(Destination, Time, Duration, NotifyStateCopy, JY_AnimGraphToolset::NotifyTrackName);
		}
		++CopiedCount;
	}

	JY_AnimGraphToolset::RefreshMontage(Destination);
	return CopiedCount;
}

bool UJY_AnimGraphToolset::ReplaceMontageAnimations(UAnimMontage* Montage, const TArray<UAnimSequenceBase*>& FromAnimations, const TArray<UAnimSequenceBase*>& ToAnimations)
{
	if (Montage == nullptr || FromAnimations.Num() == 0 || FromAnimations.Num() != ToAnimations.Num())
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Montage and matching From/To arrays are required."));
		return false;
	}

	TMap<UAnimationAsset*, UAnimationAsset*> ReplacementMap;
	for (int32 AnimationIndex = 0; AnimationIndex < FromAnimations.Num(); ++AnimationIndex)
	{
		ReplacementMap.Add(FromAnimations[AnimationIndex], ToAnimations[AnimationIndex]);
	}

	Montage->Modify();
	Montage->ReplaceReferredAnimations(ReplacementMap);

	/*
	 * 엔진 교체는 구간 시작·끝 시각을 그대로 둔다 → 길이가 다른 애니면 구간이 어긋난다.
	 * 바꾼 구간은 몽타주 편집기 "Reset Segment Length"처럼 새 애니 길이로 다시 맞추고 뒤 구간을 이어 붙인다
	 */
	for (FSlotAnimationTrack& SlotTrack : Montage->SlotAnimTracks)
	{
		float NextStartPos = 0.0f;
		for (FAnimSegment& Segment : SlotTrack.AnimTrack.AnimSegments)
		{
			UAnimSequenceBase* SegmentAnimation = Segment.GetAnimReference();
			if (ToAnimations.Contains(SegmentAnimation) == true)
			{
				Segment.SetAnimReference(SegmentAnimation, true);
			}

			Segment.StartPos = NextStartPos;
			NextStartPos = Segment.GetEndPos();
		}
	}

	JY_AnimGraphToolset::RefreshMontage(Montage);
	return true;
}

int32 UJY_AnimGraphToolset::RemoveMontageNotifies(UAnimMontage* Montage, FName NotifyName)
{
	if (Montage == nullptr)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Montage is required."));
		return 0;
	}

	Montage->Modify();
	const int32 RemovedCount = UAnimationBlueprintLibrary::RemoveAnimationNotifyEventsByName(Montage, NotifyName);
	JY_AnimGraphToolset::RefreshMontage(Montage);
	return RemovedCount;
}
