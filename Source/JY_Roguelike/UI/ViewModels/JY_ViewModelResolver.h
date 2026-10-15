#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "JY_ViewModelResolver.generated.h"

/* 위젯이 만들어질 때 그 위젯 주인(로컬 플레이어)의 UIManager에서 ViewModel을 꺼내 줌 (WBP 뷰모델 Creation Type = Resolver) */
UCLASS(meta = (DisplayName = "JY ViewModel Resolver"))
class UJY_ViewModelResolver : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
