#include "UI/ViewModels/JY_ViewModelResolver.h"
#include "UI/JY_UIManagerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_ViewModelResolver)

/*
 * 엔진이 위젯 생성 시 부른다 (MVVMViewClass.cpp:150). ExpectedType = WBP 뷰모델 목록에서 고른 클래스.
 * 위젯은 누가 만들든 같은 인스턴스를 받는다 — 위젯이 UIManager를 몰라도 된다.
 * DestroyInstance는 두지 않는다 — 부모 기본은 BP 이벤트 호출뿐이고(MVVMViewModelContextResolver.h:33), ViewModel은 UIManager가 계속 보관한다.
 */
// [OwnClient]
UObject* UJY_ViewModelResolver::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const //override
{
	const ULocalPlayer* LocalPlayer = UserWidget != nullptr ? UserWidget->GetOwningLocalPlayer() : nullptr;
	UJY_UIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UJY_UIManagerSubsystem>(LocalPlayer);
	if (UIManager == nullptr)
		return nullptr;

	return UIManager->GetOrCreateViewModel(const_cast<UClass*>(ExpectedType));
}
