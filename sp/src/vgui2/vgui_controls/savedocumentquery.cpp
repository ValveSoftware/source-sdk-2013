//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Core Movie Maker UI API
//
//=============================================================================

#include "vgui_controls/savedocumentquery.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Frame.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "tier1/KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


//-----------------------------------------------------------------------------
// This dialog asks if you want to save your work
//-----------------------------------------------------------------------------
class CSaveDocumentQuery : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CSaveDocumentQuery, vgui::Frame );

public:
	CSaveDocumentQuery(	vgui::Panel *pParent, const char *filename, const char *pFileType, int nContext, 
		vgui::Panel *pActionSignalTarget = 0, KeyValues *pKeyValues = 0 );
	~CSaveDocumentQuery();

	// Inherited from vgui::Frame
	virtual void		OnCommand( char const *cmd );
	virtual void		ApplySchemeSettings( vgui::IScheme *pScheme );

	// Put the message box into a modal state
	void				DoModal();

private:
	// Posts commands to the action signal target
	void				PostCommand( const char *pCommand );

	vgui::Label			*m_pMessageLabel;
	vgui::Button		*m_pYesButton;
	vgui::Button		*m_pNoButton;
	vgui::Button		*m_pCancelButton;
	vgui::Panel			*m_pActionSignalTarget;

	char				m_szFileName[ 256 ];
	char				m_szFileType[ 256 ];
	int					m_nContext;
	KeyValues*			m_pPostSaveKeyValues;
};


//-----------------------------------------------------------------------------
// Show the save document query dialog
//-----------------------------------------------------------------------------
void ShowSaveDocumentQuery( vgui::Panel *pParent, const char *pFileName, const char *pFileType, int nContext, vgui::Panel *pActionSignalTarget, KeyValues *pPostSaveCommand )
{
	CSaveDocumentQuery *query = new CSaveDocumentQuery( pParent, pFileName, pFileType, nContext, pActionSignalTarget, pPostSaveCommand );
	query->SetSmallCaption( true );
	query->DoModal();
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CSaveDocumentQuery::CSaveDocumentQuery( vgui::Panel *pParent, char const *pFileName, const char *pFileType, int nContext, vgui::Panel *pActionSignalTarget, KeyValues *pPostSaveCommand ) :
	BaseClass( pParent, "SaveDocumentQuery" ),
	m_nContext( nContext ), 
	m_pActionSignalTarget( pActionSignalTarget )
{
	if ( !pFileName || !pFileName[0] )
	{
		pFileName = "<untitled>";
	}
	Q_strncpy( m_szFileName, pFileName, sizeof( m_szFileName ) );
	Q_strncpy( m_szFileType, pFileType, sizeof( m_szFileType ) );
	m_pPostSaveKeyValues = pPostSaveCommand;

	SetDeleteSelfOnClose(true);

	SetMenuButtonResponsive(false);
	SetMinimizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);

	SetTitle( "Save Changes", true );

	m_pMessageLabel = new Label( this, "FileNameLabel", "" );

	m_pYesButton = new Button( this, "Yes", "Yes", this, "yes" );
	m_pNoButton = new Button( this, "No", "No", this, "no" );
	m_pCancelButton = new Button( this, "Cancel", "Cancel", this, "cancel" );

	LoadControlSettings( "resource/ToolSaveDocumentQuery.res" );

	m_pMessageLabel->SetText( m_szFileName );
}

CSaveDocumentQuery::~CSaveDocumentQuery()
{
	if ( m_pPostSaveKeyValues )
	{
		m_pPostSaveKeyValues->deleteThis();
		m_pPostSaveKeyValues = NULL;
	}
}


//-----------------------------------------------------------------------------
// Posts commands to the action signal target
//-----------------------------------------------------------------------------
void CSaveDocumentQuery::PostCommand( const char *pCommand )
{
	KeyValues *kv = new KeyValues( pCommand );
	vgui::ivgui()->PostMessage( m_pActionSignalTarget->GetVPanel(), kv, 0 );
}


//-----------------------------------------------------------------------------
// Process commands
//-----------------------------------------------------------------------------
void CSaveDocumentQuery::OnCommand( char const *cmd )
{
	if ( !Q_stricmp( cmd, "yes" ) )
	{
		KeyValues *kv = new KeyValues( "OnSaveFile" );
		kv->SetString( "filename", m_szFileName );
		kv->SetString( "filetype", m_szFileType );
		kv->SetInt( "context", m_nContext );
		kv->SetPtr( "actionTarget", m_pActionSignalTarget );
		if ( m_pPostSaveKeyValues )
		{
			kv->AddSubKey( m_pPostSaveKeyValues->MakeCopy() );
		}
		vgui::ivgui()->PostMessage( m_pActionSignalTarget->GetVPanel(), kv, 0 );
		MarkForDeletion();
	}
	else if ( !Q_stricmp( cmd, "no" ) )
	{
		PostCommand( "OnMarkNotDirty" );
		if ( m_pPostSaveKeyValues )
		{
			vgui::ivgui()->PostMessage( m_pActionSignalTarget->GetVPanel(), m_pPostSaveKeyValues->MakeCopy(), 0 );
		}
		MarkForDeletion();
	}
	else if ( !Q_stricmp( cmd, "cancel" ) )
	{
		PostCommand( "OnCancelSaveDocument" );
		MarkForDeletion();
	}
	else
	{
		BaseClass::OnCommand( cmd );
	}
}


//-----------------------------------------------------------------------------
// Deal with scheme
//-----------------------------------------------------------------------------
void CSaveDocumentQuery::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int wide, tall;
	GetSize( wide, tall );

	int swide, stall;
	surface()->GetScreenSize(swide, stall);

	// put the dialog in the middle of the screen
	SetPos((swide - wide) / 2, (stall - tall) / 2);
}


//-----------------------------------------------------------------------------
// Put the message box into a modal state
//-----------------------------------------------------------------------------
void CSaveDocumentQuery::DoModal()
{
	SetVisible( true );
	SetEnabled( true );
	MoveToFront();

	RequestFocus();

	InvalidateLayout();
}
