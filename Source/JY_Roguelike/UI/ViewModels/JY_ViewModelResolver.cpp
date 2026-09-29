#include "UI/ViewModels/JY_ViewModelResolver.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "MVVMViewModelBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_ViewModelResolver)

// [OwnClient]
/*
 * 엔진이 위젯 생성 시 부른다 (MVVMViewClass.cpp:150 FMVVMViewClass_Source::CreateInstance).
 * ExpectedType = WBP 뷰모델 목록에서 고른 클래스. 위젯은 누가 만들든 같은 인스턴스를 받는다 — 위젯이 UIManager를 몰라도 된다.
 */
UObject* UJY_ViewModelResolver::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const //override
{
	const ULocalPlayer* LocalPlayer = UserWidget != nullptr ? UserWidget->GetOwningLocalPlayer() : nullptr;
	UJY_UIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(LocalPlayer);
	if (UIManager == nullptr)
		return nullptr;

	return UIManager->GetOrCreateViewModel(const_cast<UClass*>(ExpectedType));
}

/* 위젯이 사라질 때 엔진이 부른다. ViewModel은 UIManager가 플레이어 수명 동안 보관·공유하므로 여기서 지우지 않는다 */
void UJY_ViewModelResolver::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const //override
{
}
