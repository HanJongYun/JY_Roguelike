#include "Toolsets/JY_FractureToolset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/StaticMesh.h"
#include "FractureEngineFracturing.h"
#include "FractureEngineMaterials.h"
#include "GeometryCollection/GeometryCollection.h"
#include "GeometryCollection/GeometryCollectionClusteringUtility.h"
#include "GeometryCollection/GeometryCollectionEngineConversion.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "GeometryCollection/GeometryCollectionUtility.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Math/RandomStream.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace JY_Fracture
{
	/* GC 사용 표시가 없는 재질은 게임에서 회색으로 바뀐다 (Material.cpp:1884) */
	/* 팩 원본 대신 재질 사슬을 "_GC"로 복사해 표시를 켠다 */
	static UMaterialInterface* GetGeometryCollectionMaterial(UMaterialInterface* SourceMaterial, const FString& MaterialFolder, TArray<UPackage*>& OutChangedPackages)
	{
		if (SourceMaterial == nullptr)
			return nullptr;

		const UMaterial* BaseMaterial = SourceMaterial->GetMaterial();
		if (BaseMaterial == nullptr || BaseMaterial->bUsedWithGeometryCollections == true)
			return SourceMaterial;

		const FString CopyName = SourceMaterial->GetName() + TEXT("_GC");
		const FString CopyPackageName = MaterialFolder / CopyName;
		if (FPackageName::DoesPackageExist(CopyPackageName) == true)
		{
			const FString CopyObjectPath = CopyPackageName + TEXT(".") + CopyName;
			return LoadObject<UMaterialInterface>(nullptr, *CopyObjectPath);
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		UObject* Copy = AssetTools.DuplicateAsset(CopyName, MaterialFolder, SourceMaterial);
		if (UMaterial* MaterialCopy = Cast<UMaterial>(Copy))
		{
			/* 에디터(PIE 밖)에서 부르면 표시를 켜고 셰이더를 다시 컴파일한다 (Material.cpp:1893) */
			MaterialCopy->SetMaterialUsage(MATUSAGE_GeometryCollections);
		}
		else if (UMaterialInstanceConstant* InstanceCopy = Cast<UMaterialInstanceConstant>(Copy))
		{
			UMaterialInterface* ParentCopy = GetGeometryCollectionMaterial(InstanceCopy->Parent, MaterialFolder, OutChangedPackages);
			InstanceCopy->SetParentEditorOnly(ParentCopy);
			InstanceCopy->PostEditChange();
		}

		UMaterialInterface* MaterialCopyInterface = Cast<UMaterialInterface>(Copy);
		if (MaterialCopyInterface == nullptr)
			return SourceMaterial;

		OutChangedPackages.Add(Copy->GetPackage());
		return MaterialCopyInterface;
	}

	static bool SavePackage(UPackage* Package, UObject* Asset)
	{
		const FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);
	}
}

/* Fracture 모드 New → Uniform Voronoi → 마무리와 같은 순서로 엔진 함수를 직접 부른다 (FractureToolGenerators.cpp:857) */
FString UJY_FractureToolset::CreateFracturedGeometryCollection(const FString& StaticMeshPath, const FString& AssetPath, int32 PieceCount, int32 RandomSeed)
{
	if (PieceCount < 2)
		return TEXT("PieceCount는 2 이상");

	UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *StaticMeshPath);
	if (StaticMesh == nullptr)
		return FString::Printf(TEXT("메시를 못 찾음: %s"), *StaticMeshPath);

	/* 이미 있으면 같은 에셋을 비우고 다시 쪼갠다 — BP 등 참조가 그대로 유지됨 (UGeometryCollection::Reset) */
	const FString AssetName = FPackageName::GetLongPackageAssetName(AssetPath);
	UGeometryCollection* GeometryCollectionAsset = nullptr;
	UPackage* Package = nullptr;
	if (FPackageName::DoesPackageExist(AssetPath) == true)
	{
		const FString ObjectPath = AssetPath + TEXT(".") + AssetName;
		GeometryCollectionAsset = LoadObject<UGeometryCollection>(nullptr, *ObjectPath);
		if (GeometryCollectionAsset == nullptr)
			return FString::Printf(TEXT("같은 경로에 지오메트리 컬렉션이 아닌 에셋이 있음: %s"), *AssetPath);

		GeometryCollectionAsset->Reset();
		GeometryCollectionAsset->GeometrySource.Empty();
		Package = GeometryCollectionAsset->GetPackage();
	}
	else
	{
		Package = CreatePackage(*AssetPath);
		GeometryCollectionAsset = NewObject<UGeometryCollection>(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(GeometryCollectionAsset);
	}

	/* 원본을 기록해 둬야 Fracture 모드에서 Reset·다시 쪼개기 가능 */
	const FString MaterialFolder = FPackageName::GetLongPackagePath(AssetPath) / TEXT("Materials");
	TArray<UPackage*> ChangedMaterialPackages;
	TArray<UMaterialInterface*> Materials;
	for (const FStaticMaterial& StaticMaterial : StaticMesh->GetStaticMaterials())
	{
		UMaterialInterface* GeometryCollectionMaterial = JY_Fracture::GetGeometryCollectionMaterial(StaticMaterial.MaterialInterface, MaterialFolder, ChangedMaterialPackages);
		Materials.Add(GeometryCollectionMaterial);
	}

	for (UPackage* MaterialPackage : ChangedMaterialPackages)
	{
		JY_Fracture::SavePackage(MaterialPackage, MaterialPackage->FindAssetInPackage());
	}

	const FSoftObjectPath SourcePath(StaticMesh);
	const decltype(FGeometryCollectionSource::SourceMaterial) SourceMaterials(Materials);
	GeometryCollectionAsset->GeometrySource.Emplace(SourcePath, FTransform::Identity, SourceMaterials, false, false);
	FGeometryCollectionEngineConversion::AppendStaticMesh(StaticMesh, Materials, FTransform::Identity, GeometryCollectionAsset, false, false, false, false);
	GeometryCollectionAsset->InitializeMaterials(false);

	/* 메시 범위 안에 고르게 흩뿌린 점으로 보로노이 — 같은 시드면 같은 조각 */
	const FBox MeshBounds = StaticMesh->GetBoundingBox();
	const FVector MeshExtent = MeshBounds.Max - MeshBounds.Min;
	FRandomStream RandomStream(RandomSeed);
	TArray<FVector> Sites;
	for (int32 SiteIndex = 0; SiteIndex < PieceCount; ++SiteIndex)
	{
		const FVector RandomFraction(RandomStream.FRand(), RandomStream.FRand(), RandomStream.FRand());
		Sites.Add(MeshBounds.Min + RandomFraction * MeshExtent);
	}

	FGeometryCollection* Collection = GeometryCollectionAsset->GetGeometryCollection().Get();
	FDataflowTransformSelection Selection;
	Selection.InitializeFromCollection(*Collection, false);
	Selection.SetSelected(0);

	/* 노이즈·여백 없음 — 값은 Fracture 모드 커터 기본값(UFractureCutterSettings)과 같음 */
	constexpr float ChanceToFracture = 1.0f;
	constexpr float Grout = 0.0f;
	constexpr float NoiseAmplitude = 0.0f;
	constexpr float NoiseFrequency = 0.1f;
	constexpr float NoisePersistence = 0.5f;
	constexpr float NoiseLacunarity = 2.0f;
	constexpr int32 NoiseOctaveNumber = 4;
	constexpr float PointSpacing = 10.0f;
	constexpr float CollisionSampleSpacing = 50.0f;
	const int32 FirstNewGeometryIndex = FFractureEngineFracturing::VoronoiFracture(*Collection, Selection, Sites, FTransform::Identity, RandomSeed, ChanceToFracture, FIslandSplitSettings(), Grout, NoiseAmplitude, NoiseFrequency, NoisePersistence, NoiseLacunarity, NoiseOctaveNumber, PointSpacing, false, CollisionSampleSpacing);
	if (FirstNewGeometryIndex <= INDEX_NONE)
		return TEXT("쪼개기 실패 — 메시가 닫혀 있지 않거나 점이 메시 밖에만 있음");

	/* 잘린 안쪽 면은 메시 첫 재질 */
	FFractureEngineMaterials::SetMaterialOnGeometryAfter(*Collection, FirstNewGeometryIndex, FFractureEngineMaterials::ETargetFaces::InternalFaces, 0);
	Collection->ReindexMaterials();
	FGeometryCollectionClusteringUtility::UpdateHierarchyLevelOfChildren(Collection, -1);
	::GeometryCollection::GenerateTemporaryGuids(Collection, 0, true);

	/* 연출 전용 파편 — 멈추면(느리게 굴러도) 1~2초 뒤 1초 남짓 줄어들며 사라짐 */
	GeometryCollectionAsset->EnableNanite = false;
	GeometryCollectionAsset->bRemoveOnMaxSleep = true;
	GeometryCollectionAsset->MaximumSleepTime = FVector2D(1.0, 2.0);
	GeometryCollectionAsset->RemovalDuration = FVector2D(1.0, 1.5);
	GeometryCollectionAsset->bSlowMovingAsSleeping = true;

	/* 충돌 모양 계산 — 없으면 조각이 공중에 멈춘 채 움직이지 않는다 */
	GeometryCollectionAsset->UpdateGeometryDependentProperties();
	GeometryCollectionAsset->InvalidateCollection();
	GeometryCollectionAsset->RebuildRenderData();

	Package->MarkPackageDirty();

	const bool bSaved = JY_Fracture::SavePackage(Package, GeometryCollectionAsset);
	if (bSaved == false)
		return FString::Printf(TEXT("저장 실패: %s"), *AssetPath);

	const int32 PieceTotal = Collection->NumElements(FGeometryCollection::GeometryGroup);
	return FString::Printf(TEXT("%s 조각 %d"), *AssetPath, PieceTotal);
}
