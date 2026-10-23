#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "JY_FabToolset.generated.h"

/* MCP 도구: Fab 언리얼 엔진 팩을 프로젝트에 설치한다 (함수 문서 주석 = 도구 설명) */
UCLASS(BlueprintType, Hidden)
class UJY_FabToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Fab download-info JSON 파일로 팩 설치를 시작한다. 파일 형식: [{ "assetId": 리스팅 ID, "downloadInfo": download-info 응답의 downloadInfo 배열 }]
	 * 설치 위치 = 프로젝트 폴더(팩 안의 Content/... 그대로). 시작 후 파일은 지운다(서명 주소 포함).
	 * @param InfoJsonPath JSON 파일 절대 경로
	 * @return 시작한 팩 수
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Fab")
	static int32 StartFabPackInstall(const FString& InfoJsonPath);

	/**
	 * 설치 진행 상태를 돌려준다.
	 * @return 팩마다 "assetId 진행률% 완료여부 성공여부 첫파일"
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Fab")
	static TArray<FString> GetFabPackInstallStatus();
};
