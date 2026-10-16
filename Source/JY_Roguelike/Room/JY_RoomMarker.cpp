#include "Room/JY_RoomMarker.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_RoomMarker)

/* 엔진 ATargetPoint와 같은 구성(TargetPoint.cpp:12) — ATargetPoint는 MinimalAPI라 다른 모듈에서 상속하지 않음 */
AJY_RoomMarker::AJY_RoomMarker()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));

	/* 커맨들릿(쿠킹 등)은 에디터 아이콘 에셋을 못 찾을 수 있어 건너뜀 — 엔진 TargetPoint.cpp:23과 같음 */
	if (IsRunningCommandlet() == false)
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> TargetIcon(TEXT("/Engine/EditorResources/S_TargetPoint"));

		if (SpriteComponent != nullptr)
		{
			SpriteComponent->Sprite = TargetIcon.Get();
			SpriteComponent->bIsScreenSizeScaled = true;
			SpriteComponent->SetupAttachment(RootComponent);
		}

		if (ArrowComponent != nullptr)
		{
			ArrowComponent->ArrowColor = FColor(150, 200, 255);
			ArrowComponent->bTreatAsASprite = true;
			ArrowComponent->bIsScreenSizeScaled = true;
			ArrowComponent->SetupAttachment(RootComponent);
		}
	}
#endif

	SetHidden(true);
	SetCanBeDamaged(false);
}
