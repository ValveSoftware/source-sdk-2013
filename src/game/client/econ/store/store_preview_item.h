//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef STORE_PREVIEW_ITEM_H
#define STORE_PREVIEW_ITEM_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include "econ_controls.h"
#include "store_page.h"

enum preview_state_t
{
	PS_ITEM,
	PS_PLAYER,
	PS_DETAILS,
};

//-----------------------------------------------------------------------------
// Purpose: Button that handles the rotation of the preview model.
//-----------------------------------------------------------------------------
class CPreviewRotButton : public CExButton
{
	DECLARE_CLASS_SIMPLE( CPreviewRotButton, CExButton );
public:
	CPreviewRotButton( vgui::Panel *parent, const char *name, const char *text, vgui::Panel *pActionSignalTarget = NULL, const char *cmd = NULL ) :
		CExButton( parent, name, text, pActionSignalTarget, cmd )
	{
	}
	CPreviewRotButton( vgui::Panel *parent, const char *name, const wchar_t *wszText, vgui::Panel *pActionSignalTarget = NULL, const char *cmd = NULL ) :
		CExButton( parent, name, wszText, pActionSignalTarget, cmd )
	{
	}

	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseReleased(vgui::MouseCode code);

	// Our fire action signal does nothing, because it's all done in mouse pressed/released
	virtual void FireActionSignal( void ) { return; }
};



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStorePreviewItemPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CStorePreviewItemPanel, vgui::EditablePanel );
public:
	CStorePreviewItemPanel( vgui::Panel *pParent, const char *pResFile, const char *pPanelName, CStorePage *pOwner );
	virtual ~CStorePreviewItemPanel();

	CStorePage *GetOwningStorePage()	{ return m_pOwner; }

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );
	virtual void PerformLayout( void );
	virtual void OnTick( void );

	virtual void PreviewItem( int iClass, CEconItemView *pItem, const econ_store_entry_t* pEntry=NULL );
	virtual void SetState( preview_state_t iState );

	// Subclass interface.
	virtual int	 GetPreviewTeam() const { return 0; }

	MESSAGE_FUNC_PARAMS( OnRotButtonDown, "RotButtonDown", data );
	MESSAGE_FUNC( OnRotButtonUp, "RotButtonUp" );

	MESSAGE_FUNC_PARAMS( OnItemIconSelected, "ItemIconSelected", data );

protected:
	virtual void UpdateIcons( void );

protected:
	const char					*m_pResFile;
	CUtlVector<CStorePreviewItemIcon*>	m_pItemIcons;

	int							m_iCurrentIconPosition;

	CEconItemDetailsRichText	*m_pDataTextRichText;
	CItemModelPanel				*m_pItemFullImage;

	CEconItemView				m_item;
	preview_state_t				m_iState;

	int							m_iCurrentRotation;
	CExButton					*m_pIconsMoveLeftButton;
	CExButton					*m_pIconsMoveRightButton;

	CStorePage					*m_pOwner;
};

#endif // STORE_PREVIEW_ITEM_H
