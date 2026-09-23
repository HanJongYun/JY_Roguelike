#pragma once

#include "CoreMinimal.h"

class AActor;

/** 프로젝트 공용 로그 채널 */
DECLARE_LOG_CATEGORY_EXTERN(LogJY, Log, All);

/** 로그 맨 앞에 붙일 네트워크 역할 이름 (Standalone / Server / OwnClient / SimulatedProxy) */
const TCHAR* JY_GetNetRoleTag(const AActor* Actor);
