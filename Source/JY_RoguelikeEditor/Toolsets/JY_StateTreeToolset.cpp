#include "Toolsets/JY_StateTreeToolset.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Logging/TokenizedMessage.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityTraitBase.h"
#include "MassStateTreeSchema.h"
#include "Misc/PackageName.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeEditorNode.h"
#include "StateTreeFactory.h"
#include "StateTreeState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_StateTreeToolset)

namespace JY_StateTreeToolset
{
	UStateTreeState* FindStateIn(const TArray<TObjectPtr<UStateTreeState>>& States, FName StateName)
	{
		for (UStateTreeState* State : States)
		{
			if (State == nullptr)
				continue;

			if (State->Name == StateName)
				return State;

			UStateTreeState* ChildState = FindStateIn(State->Children, StateName);
			if (ChildState != nullptr)
				return ChildState;
		}
		return nullptr;
	}

	UStateTreeState* FindState(UStateTree* StateTree, const FString& StateName)
	{
		if (StateTree == nullptr)
			return nullptr;

		UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
		if (EditorData == nullptr)
			return nullptr;

		return FindStateIn(EditorData->SubTrees, FName(*StateName));
	}

	/* 노드(Task·조건) 하나를 구조체 경로로 만들고 속성 텍스트를 넣는다 */
	FString InitializeNode(FStateTreeEditorNode& EditorNode, UStateTreeState& OuterState, const FString& NodeStructPath, const FString& NodePropertiesText)
	{
		UScriptStruct* NodeStruct = LoadObject<UScriptStruct>(nullptr, *NodeStructPath);
		if (NodeStruct == nullptr)
			return FString::Printf(TEXT("구조체를 찾지 못함: %s"), *NodeStructPath);

		EditorNode.InitializeAs(&OuterState, NodeStruct);

		if (NodePropertiesText.IsEmpty() == false)
		{
			const TCHAR* ImportResult = NodeStruct->ImportText(*NodePropertiesText, EditorNode.Node.GetMutableMemory(), &OuterState, PPF_None, GLog, NodeStruct->GetName());
			if (ImportResult == nullptr)
				return FString::Printf(TEXT("속성 텍스트를 읽지 못함: %s"), *NodePropertiesText);
		}
		return FString();
	}

	bool ParseTrigger(const FString& Trigger, EStateTreeTransitionTrigger& OutTrigger)
	{
		if (Trigger == TEXT("OnStateCompleted"))
		{
			OutTrigger = EStateTreeTransitionTrigger::OnStateCompleted;
			return true;
		}
		if (Trigger == TEXT("OnStateSucceeded"))
		{
			OutTrigger = EStateTreeTransitionTrigger::OnStateSucceeded;
			return true;
		}
		if (Trigger == TEXT("OnStateFailed"))
		{
			OutTrigger = EStateTreeTransitionTrigger::OnStateFailed;
			return true;
		}
		if (Trigger == TEXT("OnTick"))
		{
			OutTrigger = EStateTreeTransitionTrigger::OnTick;
			return true;
		}
		return false;
	}
}

UStateTree* UJY_StateTreeToolset::CreateMassStateTree(const FString& AssetPath)
{
	const FString PackagePath = FPackageName::GetLongPackagePath(AssetPath);
	const FString AssetName = FPackageName::GetLongPackageAssetName(AssetPath);
	if (FPackageName::DoesPackageExist(AssetPath) == true)
		return nullptr;

	/* 에디터에서 새 StateTree를 만들 때와 같은 팩토리 — 스키마·에디터 데이터·Root 상태까지 채워 준다 (StateTreeFactory.cpp:130) */
	UStateTreeFactory* Factory = NewObject<UStateTreeFactory>();
	Factory->SetSchemaClass(UMassStateTreeSchema::StaticClass());

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	IAssetTools& AssetTools = AssetToolsModule.Get();
	UObject* CreatedAsset = AssetTools.CreateAsset(AssetName, PackagePath, UStateTree::StaticClass(), Factory);
	return Cast<UStateTree>(CreatedAsset);
}

FString UJY_StateTreeToolset::AddState(UStateTree* StateTree, const FString& ParentStateName, const FString& StateName)
{
	UStateTreeState* ParentState = JY_StateTreeToolset::FindState(StateTree, ParentStateName);
	if (ParentState == nullptr)
		return FString::Printf(TEXT("부모 상태를 찾지 못함: %s"), *ParentStateName);

	if (JY_StateTreeToolset::FindState(StateTree, StateName) != nullptr)
		return FString::Printf(TEXT("같은 이름의 상태가 이미 있음: %s"), *StateName);

	ParentState->Modify();
	ParentState->AddChildState(FName(*StateName));
	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	StateTree->MarkPackageDirty();
	return FString();
}

FString UJY_StateTreeToolset::AddTask(UStateTree* StateTree, const FString& StateName, const FString& TaskStructPath, const FString& TaskPropertiesText)
{
	UStateTreeState* State = JY_StateTreeToolset::FindState(StateTree, StateName);
	if (State == nullptr)
		return FString::Printf(TEXT("상태를 찾지 못함: %s"), *StateName);

	State->Modify();
	FStateTreeEditorNode& TaskNode = State->Tasks.AddDefaulted_GetRef();
	const FString Error = JY_StateTreeToolset::InitializeNode(TaskNode, *State, TaskStructPath, TaskPropertiesText);
	if (Error.IsEmpty() == false)
	{
		State->Tasks.Pop();
		return Error;
	}

	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	StateTree->MarkPackageDirty();
	return FString();
}

FString UJY_StateTreeToolset::AddTransition(UStateTree* StateTree, const FString& FromStateName, const FString& Trigger, const FString& ToStateName, const FString& ConditionStructPath, const FString& ConditionPropertiesText)
{
	UStateTreeState* FromState = JY_StateTreeToolset::FindState(StateTree, FromStateName);
	if (FromState == nullptr)
		return FString::Printf(TEXT("상태를 찾지 못함: %s"), *FromStateName);

	UStateTreeState* ToState = JY_StateTreeToolset::FindState(StateTree, ToStateName);
	if (ToState == nullptr)
		return FString::Printf(TEXT("상태를 찾지 못함: %s"), *ToStateName);

	EStateTreeTransitionTrigger TransitionTrigger = EStateTreeTransitionTrigger::None;
	if (JY_StateTreeToolset::ParseTrigger(Trigger, OUT TransitionTrigger) == false)
		return FString::Printf(TEXT("모르는 Trigger: %s"), *Trigger);

	FromState->Modify();
	FStateTreeTransition& Transition = FromState->AddTransition(TransitionTrigger, EStateTreeTransitionType::GotoState, ToState);
	if (ConditionStructPath.IsEmpty() == false)
	{
		FStateTreeEditorNode& ConditionNode = Transition.Conditions.AddDefaulted_GetRef();
		const FString Error = JY_StateTreeToolset::InitializeNode(ConditionNode, *FromState, ConditionStructPath, ConditionPropertiesText);
		if (Error.IsEmpty() == false)
		{
			FromState->Transitions.Pop();
			return Error;
		}
	}

	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	StateTree->MarkPackageDirty();
	return FString();
}

FString UJY_StateTreeToolset::SetTransitionPriority(UStateTree* StateTree, const FString& FromStateName, int32 TransitionIndex, const FString& Priority)
{
	UStateTreeState* FromState = JY_StateTreeToolset::FindState(StateTree, FromStateName);
	if (FromState == nullptr)
		return FString::Printf(TEXT("상태를 찾지 못함: %s"), *FromStateName);

	if (FromState->Transitions.IsValidIndex(TransitionIndex) == false)
		return FString::Printf(TEXT("전환 번호가 범위 밖: %d (전환 %d개)"), TransitionIndex, FromState->Transitions.Num());

	const UEnum* PriorityEnum = StaticEnum<EStateTreeTransitionPriority>();
	const int64 PriorityValue = PriorityEnum->GetValueByNameString(Priority);
	if (PriorityValue == INDEX_NONE)
		return FString::Printf(TEXT("모르는 Priority: %s"), *Priority);

	FromState->Modify();
	FromState->Transitions[TransitionIndex].Priority = static_cast<EStateTreeTransitionPriority>(PriorityValue);
	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	StateTree->MarkPackageDirty();
	return FString();
}

FString UJY_StateTreeToolset::RemoveTransition(UStateTree* StateTree, const FString& FromStateName, int32 TransitionIndex)
{
	UStateTreeState* FromState = JY_StateTreeToolset::FindState(StateTree, FromStateName);
	if (FromState == nullptr)
		return FString::Printf(TEXT("상태를 찾지 못함: %s"), *FromStateName);

	if (FromState->Transitions.IsValidIndex(TransitionIndex) == false)
		return FString::Printf(TEXT("전환 번호가 범위 밖: %d (전환 %d개)"), TransitionIndex, FromState->Transitions.Num());

	FromState->Modify();
	FromState->Transitions.RemoveAt(TransitionIndex);
	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	StateTree->MarkPackageDirty();
	return FString();
}

FString UJY_StateTreeToolset::RemoveState(UStateTree* StateTree, const FString& StateName)
{
	UStateTreeState* State = JY_StateTreeToolset::FindState(StateTree, StateName);
	if (State == nullptr)
		return FString::Printf(TEXT("상태를 찾지 못함: %s"), *StateName);

	UStateTreeState* ParentState = State->Parent;
	if (ParentState == nullptr)
		return FString::Printf(TEXT("맨 위 상태는 지울 수 없음: %s"), *StateName);

	ParentState->Modify();
	ParentState->Children.Remove(State);
	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	StateTree->MarkPackageDirty();
	return FString();
}

FString UJY_StateTreeToolset::CompileStateTree(UStateTree* StateTree)
{
	if (StateTree == nullptr)
		return TEXT("StateTree가 null");

	FStateTreeCompilerLog CompilerLog;
	const bool bCompiled = UStateTreeEditingSubsystem::CompileStateTree(StateTree, CompilerLog);
	StateTree->MarkPackageDirty();
	if (bCompiled == true)
		return TEXT("OK");

	FString Messages = TEXT("컴파일 실패");
	const TArray<TSharedRef<FTokenizedMessage>> TokenizedMessages = CompilerLog.ToTokenizedMessages();
	for (const TSharedRef<FTokenizedMessage>& TokenizedMessage : TokenizedMessages)
	{
		Messages += TEXT("\n");
		Messages += TokenizedMessage->ToText().ToString();
	}
	return Messages;
}

UMassEntityTraitBase* UJY_StateTreeToolset::AddTraitToMassConfig(UMassEntityConfigAsset* ConfigAsset, UClass* TraitClass)
{
	if (ConfigAsset == nullptr || TraitClass == nullptr)
		return nullptr;

	if (TraitClass->IsChildOf(UMassEntityTraitBase::StaticClass()) == false)
		return nullptr;

	ConfigAsset->Modify();
	UMassEntityTraitBase* AddedTrait = ConfigAsset->AddTrait(TraitClass);
	ConfigAsset->MarkPackageDirty();
	return AddedTrait;
}
