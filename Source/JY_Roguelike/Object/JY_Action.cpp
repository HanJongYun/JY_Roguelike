#include "Object/JY_Action.h"
#include "Object/JY_Object.h"
#include "Components/SceneComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_Action)

/* 바깥(Outer) = 물체 액터. CDO·BP 편집 중 사본은 월드가 없다 */
UWorld* UJY_Action::GetWorld() const //override
{
	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) == true)
		return nullptr;

	const UObject* Outer = GetOuter();
	return Outer != nullptr ? Outer->GetWorld() : nullptr;
}

/* 부품은 BP가 붙이므로 태그로 찾는다 */
UActorComponent* UJY_Action::FindPart(const AJY_Object* Object, FName PartTag, TSubclassOf<UActorComponent> PartClass)
{
	if (PartTag.IsNone() == true)
	{
		USceneComponent* Root = Object->GetRootComponent();
		return Root != nullptr && Root->IsA(PartClass) == true ? Root : nullptr;
	}

	TArray<UActorComponent*> Parts = Object->GetComponentsByTag(PartClass, PartTag);
	ensureMsgf(Parts.Num() > 0, TEXT("%s: 태그 %s인 %s 부품이 없음"), *Object->GetName(), *PartTag.ToString(), *PartClass->GetName());
	return Parts.Num() > 0 ? Parts[0] : nullptr;
}
