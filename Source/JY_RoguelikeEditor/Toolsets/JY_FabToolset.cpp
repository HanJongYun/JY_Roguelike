#include "Toolsets/JY_FabToolset.h"

#include "FabDownloader.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace JY_FabToolset
{
	struct FInstall
	{
		FString AssetId;
		TSharedPtr<FFabDownloadRequest> Request;
		bool bDone = false;
		bool bSuccess = false;
		float Percent = 0.f;
		TArray<FString> Files;
	};

	/* 다운로드 요청은 끝날 때까지 살아 있어야 해서 모듈 수명 동안 보관 */
	static TArray<TSharedPtr<FInstall>> Installs;
}

/* FPackImportWorkflow::DownloadContent()와 같은 인자: "매니페스트주소,기본주소들" + 프로젝트 폴더 + BuildPatchRequest */
int32 UJY_FabToolset::StartFabPackInstall(const FString& InfoJsonPath)
{
	FString JsonText;
	if (FFileHelper::LoadFileToString(OUT JsonText, *InfoJsonPath) == false)
		return 0;

	/* 서명 주소가 든 파일이라 읽자마자 지운다 */
	IFileManager::Get().Delete(*InfoJsonPath);

	TArray<TSharedPtr<FJsonValue>> Entries;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (FJsonSerializer::Deserialize(Reader, OUT Entries) == false)
		return 0;

	const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	int32 StartedCount = 0;
	for (const TSharedPtr<FJsonValue>& Entry : Entries)
	{
		const TSharedPtr<FJsonObject> EntryObject = Entry->AsObject();
		const FString AssetId = EntryObject->GetStringField(TEXT("assetId"));
		const TArray<TSharedPtr<FJsonValue>>& InfoArray = EntryObject->GetArrayField(TEXT("downloadInfo"));
		if (InfoArray.Num() == 0)
			continue;

		const TSharedPtr<FJsonObject> Info = InfoArray[0]->AsObject();
		const FString ManifestUrl = Info->GetStringField(TEXT("downloadUrl"));
		TArray<FString> BaseUrls;
		Info->TryGetStringArrayField(TEXT("distributionPointBaseUrls"), OUT BaseUrls);
		const FString DownloadUrl = ManifestUrl + TEXT(",") + FString::Join(BaseUrls, TEXT(","));

		TSharedPtr<JY_FabToolset::FInstall> Install = MakeShared<JY_FabToolset::FInstall>();
		Install->AssetId = AssetId;
		Install->Request = MakeShared<FFabDownloadRequest>(AssetId, DownloadUrl, ProjectDir, EFabDownloadType::BuildPatchRequest);

		TWeakPtr<JY_FabToolset::FInstall> WeakInstall = Install;
		auto OnProgress = [WeakInstall](const FFabDownloadRequest*, const FFabDownloadStats& Stats)
		{
			TSharedPtr<JY_FabToolset::FInstall> Pinned = WeakInstall.Pin();
			if (Pinned != nullptr)
				Pinned->Percent = Stats.PercentComplete;
		};
		auto OnComplete = [WeakInstall](const FFabDownloadRequest*, const FFabDownloadStats& Stats)
		{
			TSharedPtr<JY_FabToolset::FInstall> Pinned = WeakInstall.Pin();
			if (Pinned == nullptr)
				return;

			Pinned->bDone = true;
			Pinned->bSuccess = Stats.bIsSuccess;
			Pinned->Percent = Stats.PercentComplete;
			Pinned->Files = Stats.DownloadedFiles;

			/* 새 파일을 콘텐츠 브라우저에 바로 보이게 (Fab 창 경로도 설치 후 스캔한다) */
			IAssetRegistry& AssetRegistry = FAssetRegistryModule::GetRegistry();
			AssetRegistry.ScanPathsSynchronous({ TEXT("/Game") }, true);
		};
		Install->Request->OnDownloadProgress().AddLambda(OnProgress);
		Install->Request->OnDownloadComplete().AddLambda(OnComplete);
		Install->Request->ExecuteRequest();

		JY_FabToolset::Installs.Add(Install);
		++StartedCount;
	}
	return StartedCount;
}

TArray<FString> UJY_FabToolset::GetFabPackInstallStatus()
{
	TArray<FString> Lines;
	for (const TSharedPtr<JY_FabToolset::FInstall>& Install : JY_FabToolset::Installs)
	{
		const FString FirstFile = Install->Files.Num() > 0 ? Install->Files[0] : FString();
		Lines.Add(FString::Printf(TEXT("%s %.0f%% done=%d ok=%d files=%d %s"), *Install->AssetId, Install->Percent, Install->bDone, Install->bSuccess, Install->Files.Num(), *FirstFile));
	}
	return Lines;
}
