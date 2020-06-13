#include "vscript/ivscript.h"

#include "tier1/tier1.h"

IScriptVM* makeSquirrelVM();

int vscript_token = 0;

class CScriptManager : public CTier1AppSystem<IScriptManager>
{
public:
	virtual IScriptVM* CreateVM(ScriptLanguage_t language) override
	{
		IScriptVM* pScriptVM = nullptr;
		if (language == SL_SQUIRREL)
		{
			pScriptVM = makeSquirrelVM();
		}
		else
		{
			return nullptr;
		}

		if (pScriptVM == nullptr)
		{
			return nullptr;
		}

		if (!pScriptVM->Init())
		{
			delete pScriptVM;
			return nullptr;
		}
		
		return pScriptVM;
	}

	virtual void DestroyVM(IScriptVM * pScriptVM) override
	{
		if (pScriptVM)
		{
			pScriptVM->Shutdown();
			delete pScriptVM;
		}
	}
};

EXPOSE_SINGLE_INTERFACE(CScriptManager, IScriptManager, VSCRIPT_INTERFACE_VERSION);