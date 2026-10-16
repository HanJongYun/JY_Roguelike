#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "JY_FractureToolset.generated.h"

/* MCP 도구: 스태틱 메시를 쪼갠 지오메트리 컬렉션(Chaos 파편) 에셋을 만든다 (함수 문서 주석 = 도구 설명) */
UCLASS(BlueprintType, Hidden)
class UJY_FractureToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * 스태틱 메시를 보로노이로 쪼갠 지오메트리 컬렉션 에셋을 만들어 저장한다 (Fracture 모드 Uniform Voronoi와 같음).
	 * 조각은 멈춘 뒤 사라진다. 같은 경로에 이미 있으면 그 에셋을 비우고 다시 쪼갠다(참조 유지).
	 * @param StaticMeshPath 원본 메시 경로 (예: /Game/MedievalDungeon/Meshes/Props/SM_Pot_A_Complete)
	 * @param AssetPath 만들 에셋 경로 (예: /Game/Han/Breakable/Fracture/GC_JY_Pot)
	 * @param PieceCount 조각 수 (보로노이 점 개수)
	 * @param RandomSeed 점 배치 시드 — 같은 값이면 같은 모양
	 * @return 만든 에셋 경로와 조각 수, 실패면 이유
	 */
	UFUNCTION(meta = (AICallable), Category = "JY|Fracture")
	static FString CreateFracturedGeometryCollection(const FString& StaticMeshPath, const FString& AssetPath, int32 PieceCount, int32 RandomSeed);
};
