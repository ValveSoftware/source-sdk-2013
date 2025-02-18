//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef RENAME_TOOL_UI_H
#define RENAME_TOOL_UI_H
#ifdef _WIN32
#pragma once
#endif

#include "tool_items.h"

class CNameToolUsageDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CNameToolUsageDialog, CBaseToolUsageDialog );

public:
	CNameToolUsageDialog( vgui::Panel *pParent, const char* pszName, CEconItemView *pTool, CEconItemView *pToolSubject, bool bDescription );
	virtual int GetMaxLength();
	virtual int GetMaxDBSize();

protected:
	bool m_bDescription;
};

//-----------------------------------------------------------------------------
// Purpose: A dialog used to input a Tool's name payload
//-----------------------------------------------------------------------------
class CRequestNameDialog : public CNameToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CRequestNameDialog, CNameToolUsageDialog );

public:
	CRequestNameDialog( vgui::Panel *pParent, const char* pszName, CEconItemView *pTool, CEconItemView *pToolSubject, bool bDescription );

	virtual void	MoveToFront();
	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	Apply( void );

	MESSAGE_FUNC_PTR( OnItemPanelEntered, "ItemPanelEntered", panel );

private:
	vgui::TextEntry *m_pCustomNameEntry;
	vgui::Label		*m_pOldNameLabel;
	vgui::Label		*m_pOldName;
	vgui::Label		*m_pNewNameLabel;
};


//-----------------------------------------------------------------------------
// Purpose: Confirm name and commit or reject
//-----------------------------------------------------------------------------
class CConfirmNameDialog : public CNameToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmNameDialog, CNameToolUsageDialog );

public:
	CConfirmNameDialog( vgui::Panel *pParent, const char* pszName, CEconItemView *pTool, CEconItemView *pToolSubject, const wchar_t *name, bool bDescription );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Apply( void );
	virtual void	OnCommand( const char *command );

private:
	wchar_t m_name[ MAX_ITEM_CUSTOM_DESC_LENGTH+1 ];

	bool IsNameValid( void ) const;
};


#endif // RENAME_TOOL_UI_H
