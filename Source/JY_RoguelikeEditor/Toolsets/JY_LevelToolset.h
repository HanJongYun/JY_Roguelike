#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "JY_LevelToolset.generated.h"

/* MCP 도구: 하위 레벨 등록·네비 굽기·경로 검사·PIE 콘솔 명령 (함수 문서 주석 = 도구 설명) */
UCLASS(BlueprintType, Hidden)
class UJY_LevelToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * 열린 맵에 방 맵을 하위 레벨로 추가한다. 블루프린트 스트리밍이라 시작 때 안 불리고, 코드가 LoadStreamLevel로 켠다.
	 * 이미 들어 있는 방이면 아무것도 안 하고 빈 문자열을 돌려준다(엔진 동작).
	 * @param LevelPackageName 방 맵 경로 (예: /Game/Han/Maps/Lvl_JY_Room_01)
	 * @param Location 방을 놓을 월드 위치 (방끼리 겹치지 않게)
	 * @return 추가된 하위 레벨의 맵 경로, 실패면 빈 문자열
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Level")
	static FString AddStreamingLevel(const FString& LevelPackageName, const FVector& Location);

	/**
	 * 열린 맵에 등록된 하위 레벨 목록을 돌려준다.
	 * @return 하위 레벨마다 "맵경로 위치 스트리밍클래스"
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Level")
	static TArray<FString> GetStreamingLevels();

	/**
	 * 열린 맵의 네비메시를 다시 굽는다(에디터 '경로 빌드'와 같음). 저장은 하지 않는다.
	 * @return 구운 네비메시 타일 수 (0이면 네비 볼륨·바닥이 없음)
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Level")
	static int32 BuildNavigation();

	/**
	 * 네비메시를 아직 굽는 중인지 돌려준다. 굽기는 비동기라 BuildNavigation 직후엔 일부 타일이 빠져 있다 — false가 될 때까지 기다린 뒤 검사·저장할 것.
	 * @return 굽는 중이면 true
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Level")
	static bool IsNavigationBuilding();

	/**
	 * 시작점에서 각 도착점까지 네비 경로가 끝까지 이어지는지 검사한다.
	 * @param PIEInstance -1 = 에디터 월드, 0 이상 = 그 번호의 PIE 월드 (Play As Client면 0 = 서버) — 실행 중 하위 레벨 네비 조각이 붙었는지 볼 때
	 * @param Start 시작 위치 (보통 입장 지점)
	 * @param Ends 도착 위치들 (스폰·보상·출구 지점)
	 * @return 도착점마다 "OK 길이" 또는 "PARTIAL 길이" 또는 "NONE"
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Level")
	static TArray<FString> CheckNavPaths(int32 PIEInstance, const FVector& Start, const TArray<FVector>& Ends);

	/**
	 * PIE 월드에서 콘솔 명령을 실행한다 — 클라면 플레이어 컨트롤러로(JYClearRoom 등 치트), 데디 서버면 월드에 직접(servertravel 등).
	 * 다음 엔진 틱에 실행한다 (도구 실행 중엔 서버 RPC가 안 감).
	 * @param PIEInstance PIE 월드 번호 (Play As Client면 0 = 서버, 1 = 첫 클라)
	 * @param Command 콘솔 명령 (예: JYClearRoom)
	 * @return 예약했으면 "Scheduled", 아니면 실패 이유
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Level")
	static FString RunPIEConsoleCommand(int32 PIEInstance, const FString& Command);
};
