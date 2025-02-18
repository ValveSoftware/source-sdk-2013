
#include "econ_item_tools.h"

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_TFSpellbookPage : public IEconTool
{
public:
	CEconTool_TFSpellbookPage( const char *pszTypeName, item_capabilities_t unCapabilities )
		: IEconTool( pszTypeName, NULL, NULL, unCapabilities ) 
	{
		//
	}

#ifdef CLIENT_DLL
	virtual bool ShouldDisplayAsUseableOnItemsInArmory() const { return false; }

	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_TFEventEnableHalloween : public IEconTool
{
public:
	CEconTool_TFEventEnableHalloween( const char *pszTypeName, const char *pszUseString ) : IEconTool( pszTypeName, pszUseString, NULL, ITEM_CAP_NONE ) { }

#ifdef CLIENT_DLL
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
};