#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "ToolsetRegistry/UToolsetRegistry.h"
#include "Toolsets/JY_AnimGraphToolset.h"
#include "Toolsets/JY_FabToolset.h"

/* 에디터 전용 모듈. MCP(ToolsetRegistry)에 프로젝트 툴셋을 등록한다 */
class FJY_RoguelikeEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolsetRegistry::RegisterToolsetClass(UJY_AnimGraphToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UJY_FabToolset::StaticClass());
	}

	virtual void ShutdownModule() override
	{
		UToolsetRegistry::UnregisterToolsetClass(UJY_AnimGraphToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UJY_FabToolset::StaticClass());
	}
};

IMPLEMENT_MODULE(FJY_RoguelikeEditorModule, JY_RoguelikeEditor);
