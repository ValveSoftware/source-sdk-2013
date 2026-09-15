//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "matsys_controls/mdlsequencepicker.h"
#include "tier1/KeyValues.h"
#include "tier1/utldict.h"
#include "datacache/imdlcache.h"
#include "filesystem.h"
#include "studio.h"
#include "vgui/IVGui.h"
#include "vgui/Cursor.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Splitter.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/ToolWindow.h"
#include "vgui_controls/Button.h"
#include "matsys_controls/gamefiletreeview.h"
#include "matsys_controls/matsyscontrols.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

					  
using namespace vgui;


//-----------------------------------------------------------------------------
//
// MDL Sequence Picker
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMDLSequencePicker::CMDLSequencePicker( vgui::Panel *pParent ) : BaseClass(pParent, "MDLSequencePicker"), m_Images(false)
{
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_hSelectedMDL = MDLHANDLE_INVALID;

	// Horizontal splitter for mdls
	m_pMDLSplitter = new Splitter( this, "MDLSplitter", SPLITTER_MODE_VERTICAL, 1 );

	vgui::Panel *pSplitterLeftSide = m_pMDLSplitter->GetChild( 0 );
	vgui::Panel *pSplitterRightSide = m_pMDLSplitter->GetChild( 1 );

	// filter selection
	m_pFilterList = new ComboBox( pSplitterLeftSide, "FilterList", 16, true );
	m_pFilterList->AddActionSignalTarget( this );

	// file browser tree controls
	m_pFileTree = new CGameFileTreeView( pSplitterLeftSide, "FileTree", "All .MDLs", "models", "mdl" );

	// build our list of images
	m_Images.AddImage( scheme()->GetImage( "resource/icon_folder", false ) );
	m_Images.AddImage( scheme()->GetImage( "resource/icon_folder_selected", false ) );
	m_Images.AddImage( scheme()->GetImage( "resource/icon_file", false ) );
	m_pFileTree->SetImageList( &m_Images, false );
  	m_pFileTree->AddActionSignalTarget( this );

	// property sheet - revisions, changes, etc.
	m_pSequenceSplitter = new Splitter( pSplitterRightSide, "SequenceSplitter", SPLITTER_MODE_HORIZONTAL, 1 );

	vgui::Panel *pSplitterTopSide = m_pSequenceSplitter->GetChild( 0 );
	vgui::Panel *pSplitterBottomSide = m_pSequenceSplitter->GetChild( 1 );

	// MDL preview
	m_pMDLPreview = new CMDLPanel( pSplitterTopSide, "MDLPreview" );
	SetSkipChildDuringPainting( m_pMDLPreview );

	m_pViewsSheet = new vgui::PropertySheet( pSplitterBottomSide, "ViewsSheet" );
 	m_pViewsSheet->AddActionSignalTarget( this );

	// sequences
	m_pSequencesPage = new PropertyPage( m_pViewsSheet, "SequencesPage" );
	m_pViewsSheet->AddPage( m_pSequencesPage, "Sequences" );
	m_pSequencesList = new ListPanel( m_pSequencesPage, "SequencesList" );
 	m_pSequencesList->AddColumnHeader( 0, "sequence", "sequence", 52, 0 );
	m_pSequencesList->AddActionSignalTarget( this );
	m_pSequencesList->SetSelectIndividualCells( true );
 	m_pSequencesList->SetEmptyListText("No .MDL file currently selected.");
	m_pSequencesList->SetDragEnabled( true );
	m_pSequencesList->SetAutoResize( Panel::PIN_TOPLEFT, Panel::AUTORESIZE_DOWNANDRIGHT, 0, 0, 0, 0 );

	// Activities
	m_pActivitiesPage = new PropertyPage( m_pViewsSheet, "ActivitiesPage" );
	m_pViewsSheet->AddPage( m_pActivitiesPage, "Activities" );
	m_pActivitiesList = new ListPanel( m_pActivitiesPage, "ActivitiesList" );
 	m_pActivitiesList->AddColumnHeader( 0, "activity", "activity", 52, 0 );
	m_pActivitiesList->AddActionSignalTarget( this );
    m_pActivitiesList->SetSelectIndividualCells( true );
	m_pActivitiesList->SetEmptyListText( "No .MDL file currently selected." );
 	m_pActivitiesList->SetDragEnabled( true );
	m_pActivitiesList->SetAutoResize( Panel::PIN_TOPLEFT, Panel::AUTORESIZE_DOWNANDRIGHT, 0, 0, 0, 0 );

	// Load layout settings; has to happen before pinning occurs in code
	LoadControlSettingsAndUserConfig( "resource/mdlsequencepicker.res" );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMDLSequencePicker::~CMDLSequencePicker()
{
}


//-----------------------------------------------------------------------------
// Purpose: This is a bit of a hack to make sure that the ToolWindow containing this picker punches
// a hold for the rendering viewport, too
// Input : - 
//-----------------------------------------------------------------------------
void CMDLSequencePicker::OnTick()
{
	BaseClass::OnTick();
	if ( GetParent() )
	{
		ToolWindow *tw = dynamic_cast< ToolWindow * >( GetParent()->GetParent() );

		if ( tw )
		{
			tw->SetSkipChildDuringPainting( IsVisible() ? m_pMDLPreview : NULL );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: stops app on close
//-----------------------------------------------------------------------------
void CMDLSequencePicker::OnClose()
{
	BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: called to open
//-----------------------------------------------------------------------------
void CMDLSequencePicker::Activate()
{
	RefreshFileList();
	RefreshActivitiesAndSequencesList();
}


//-----------------------------------------------------------------------------
// Performs layout
//-----------------------------------------------------------------------------
void CMDLSequencePicker::PerformLayout()
{
	// NOTE: This call should cause auto-resize to occur
	// which should fix up the width of the panels
	BaseClass::PerformLayout();

	int w, h;
	GetSize( w, h );

	// Layout the mdl splitter
	m_pMDLSplitter->SetBounds( 0, 0, w, h );
}

	
//-----------------------------------------------------------------------------
// Purpose: Refreshes the active file list
//-----------------------------------------------------------------------------
void CMDLSequencePicker::RefreshFileList()
{
	m_pFileTree->RefreshFileList();
}


//-----------------------------------------------------------------------------
// Purpose: rebuilds the list of activities
//-----------------------------------------------------------------------------
void CMDLSequencePicker::RefreshActivitiesAndSequencesList()
{
	m_pActivitiesList->RemoveAll();
	m_pSequencesList->RemoveAll();
	m_pMDLPreview->SetSequence( 0 );

	if ( m_hSelectedMDL == MDLHANDLE_INVALID )
	{
		m_pActivitiesList->SetEmptyListText("No .MDL file currently selected");
		m_pSequencesList->SetEmptyListText("No .MDL file currently selected");
		return;
	}

	m_pActivitiesList->SetEmptyListText(".MDL file contains no activities");
	m_pSequencesList->SetEmptyListText(".MDL file contains no sequences");

	studiohdr_t *hdr = vgui::MDLCache()->GetStudioHdr( m_hSelectedMDL );

	CUtlDict<int, unsigned short> activityNames( true, 0, hdr->GetNumSeq() );
	    
	for (int j = 0; j < hdr->GetNumSeq(); j++)
	{
		if ( /*g_viewerSettings.showHidden ||*/ !(hdr->pSeqdesc(j).flags & STUDIO_HIDDEN))
		{
			const char *pActivityName = hdr->pSeqdesc(j).pszActivityName();
			if ( pActivityName && pActivityName[0] )
			{
				// Multiple sequences can have the same activity name; only add unique activity names
				if ( activityNames.Find( pActivityName ) == activityNames.InvalidIndex() )
				{
					KeyValues *pkv = new KeyValues("node", "activity", pActivityName );
					int nItemID = m_pActivitiesList->AddItem( pkv, 0, false, false );

					KeyValues *pDrag = new KeyValues( "drag", "text", pActivityName );
					pDrag->SetString( "texttype", "activityName" );
					pDrag->SetString( "mdl", vgui::MDLCache()->GetModelName( m_hSelectedMDL ) );
					m_pActivitiesList->SetItemDragData( nItemID, pDrag );

					activityNames.Insert( pActivityName, j );
				}
			}

			const char *pSequenceName = hdr->pSeqdesc(j).pszLabel();
			if ( pSequenceName && pSequenceName[0] )
			{
				KeyValues *pkv = new KeyValues("node", "sequence", pSequenceName);
				pkv->SetInt( "seqindex", j );

				int nItemID = m_pSequencesList->AddItem( pkv, 0, false, false );

				KeyValues *pDrag = new KeyValues( "drag", "text", pSequenceName );
				pDrag->SetString( "texttype", "sequenceName" );
				pDrag->SetString( "mdl", vgui::MDLCache()->GetModelName( m_hSelectedMDL ) );
				m_pSequencesList->SetItemDragData( nItemID, pDrag );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: refreshes dialog on text changing
//-----------------------------------------------------------------------------
void CMDLSequencePicker::OnTextChanged( vgui::Panel *pPanel, const char *pText )
{
//	m_pFileTree->SetFilter( pText );
	RefreshFileList();
}


/*
//-----------------------------------------------------------------------------
// Purpose: Selects an sequence based on an activity
//-----------------------------------------------------------------------------
int SelectWeightedSequence( studiohdr_t *pstudiohdr, int activity, int curSequence )
{
	if (! pstudiohdr)
		return 0;

	VerifySequenceIndex( pstudiohdr );

	int weighttotal = 0;
	int seq = ACTIVITY_NOT_AVAILABLE;
	int weight = 0;
	for (int i = 0; i < pstudiohdr->GetNumSeq(); i++)
	{
		int curActivity = GetSequenceActivity( pstudiohdr, i, &weight );
		if (curActivity == activity)
		{
			if ( curSequence == i && weight < 0 )
			{
				seq = i;
				break;
			}
			weighttotal += iabs(weight);
			
			int randomValue;
			if ( IsInPrediction() )
				randomValue = SharedRandomInt( "SelectWeightedSequence", 0, weighttotal - 1, i );
			else
				randomValue = RandomInt( 0, weighttotal - 1 );
			
			if (!weighttotal || randomValue < iabs(weight))
				seq = i;
		}
	}

	return seq;
}
*/

//-----------------------------------------------------------------------------
// Plays the selected activity
//-----------------------------------------------------------------------------
void CMDLSequencePicker::PlaySelectedActivity( )
{
	int nIndex = m_pActivitiesList->GetSelectedItem( 0 );
	if ( nIndex < 0 )
		return;

	KeyValues *pkv = m_pActivitiesList->GetItem( nIndex );
	const char *pActivityName = pkv->GetString( "activity", NULL );
	if ( !pActivityName )
		return;

	studiohdr_t *pstudiohdr = vgui::MDLCache()->GetStudioHdr( m_hSelectedMDL );
	for ( int i = 0; i < pstudiohdr->GetNumSeq(); i++ )
	{
		mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( i );
		if ( stricmp( seqdesc.pszActivityName(), pActivityName ) == 0 )
		{
			// FIXME: Add weighted sequence selection logic?
			m_pMDLPreview->SetSequence( i );
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Plays the selected sequence
//-----------------------------------------------------------------------------
void CMDLSequencePicker::PlaySelectedSequence( )
{
	int nIndex = m_pSequencesList->GetSelectedItem( 0 );
	if ( nIndex < 0 )
		return;

	KeyValues *pkv = m_pSequencesList->GetItem( nIndex );
	const char *pSequenceName = pkv->GetString( "sequence", NULL );
	if ( !pSequenceName )
		return;

	studiohdr_t *pstudiohdr = vgui::MDLCache()->GetStudioHdr( m_hSelectedMDL );
	for (int i = 0; i < pstudiohdr->GetNumSeq(); i++)
	{
		mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( i );
		if ( !Q_stricmp( seqdesc.pszLabel(), pSequenceName ) )
		{
			m_pMDLPreview->SetSequence( i );
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when a page is shown
//-----------------------------------------------------------------------------
void CMDLSequencePicker::OnPageChanged( )
{
	if ( m_pViewsSheet->GetActivePage() == m_pSequencesPage )
	{
		PlaySelectedSequence();
		return;
	}

	if ( m_pViewsSheet->GetActivePage() == m_pActivitiesPage )
	{
		PlaySelectedActivity();
		return;
	}
}


//-----------------------------------------------------------------------------
// Purpose: refreshes dialog on text changing
//-----------------------------------------------------------------------------
void CMDLSequencePicker::OnItemSelected( KeyValues *kv )
{
	Panel *pPanel = (Panel *)kv->GetPtr("panel", NULL);
	if ( pPanel == m_pSequencesList )
	{
		PlaySelectedSequence();
		return;
	}

	if ( pPanel == m_pActivitiesList )
	{
		PlaySelectedActivity();
		return;
	}
}


//-----------------------------------------------------------------------------
// An MDL was selected
//-----------------------------------------------------------------------------
void CMDLSequencePicker::SelectMDL( const char *pMDLName )
{
	m_hSelectedMDL = pMDLName ? vgui::MDLCache()->FindMDL( pMDLName ) : MDLHANDLE_INVALID;
	if ( vgui::MDLCache()->IsErrorModel( m_hSelectedMDL ) )
	{
		m_hSelectedMDL = MDLHANDLE_INVALID;
	}
	m_pMDLPreview->SetMDL( m_hSelectedMDL );
	m_pMDLPreview->LookAtMDL();
	RefreshActivitiesAndSequencesList();
}


//-----------------------------------------------------------------------------
// Purpose: updates revision view on a file being selected
//-----------------------------------------------------------------------------
void CMDLSequencePicker::OnFileSelected()
{
	// update list
	int iItem = m_pFileTree->GetFirstSelectedItem();
	if ( iItem < 0 )
		return;

	// Don't bother to change if a directory was selected
	KeyValues *pkv = m_pFileTree->GetItemData(iItem);
	if ( pkv->GetInt("dir") || pkv->GetInt("root") )
		return;

	surface()->SetCursor(dc_waitarrow);

	const char *pFullPathName = pkv->GetString( "path" );

	char pRelativePathName[MAX_PATH];
	g_pFullFileSystem->FullPathToRelativePath( pFullPathName, pRelativePathName, sizeof(pRelativePathName) );

	// FIXME: Check that we're not actually opening the wrong file!!
	SelectMDL( pRelativePathName );
}

char const *CMDLSequencePicker::GetModelName()
{
	if ( MDLHANDLE_INVALID == m_hSelectedMDL )
	{
		return "";
	}

	return vgui::MDLCache()->GetModelName( m_hSelectedMDL );
}

char const *CMDLSequencePicker::GetSequenceName()
{
	int nIndex = m_pSequencesList->GetSelectedItem( 0 );
	if ( nIndex < 0 )
		return "";

	KeyValues *pkv = m_pSequencesList->GetItem( nIndex );
	const char *pSequenceName = pkv->GetString( "sequence", NULL );
	if ( !pSequenceName )
		return "";

	return pSequenceName;
}

int	CMDLSequencePicker::GetSequenceNumber()
{
	int nIndex = m_pSequencesList->GetSelectedItem( 0 );
	if ( nIndex < 0 )
		return -1;
	KeyValues *pkv = m_pSequencesList->GetItem( nIndex );
	return pkv->GetInt( "seqindex", -1 );
}

//-----------------------------------------------------------------------------
// Sequence picker frame
//-----------------------------------------------------------------------------
CMDLSequencePickerFrame::CMDLSequencePickerFrame( vgui::Panel *parent, char const *title  ) : 
	BaseClass( parent, "MDLSequencePickerFrame" ) 
{
	m_pMDLSequencePicker = new CMDLSequencePicker( this );
	SetTitle( title, true );
	SetSizeable( false );
	SetCloseButtonVisible( false );
	SetMoveable( true );
	SetMinimumSize( 640, 480 );
	Activate();
	m_pMDLSequencePicker->Activate();

	m_pOK = new Button( this, "OK", "#vgui_ok", this );
	m_pOK->SetCommand( new KeyValues( "OnOK" ) );
	m_pCancel= new Button( this, "Cancel", "#vgui_cancel", this );
	m_pOK->SetCommand( new KeyValues( "OnCancel" ) );
	m_pOK->SetEnabled( false );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );
}

CMDLSequencePickerFrame::~CMDLSequencePickerFrame() 
{
}

void CMDLSequencePickerFrame::OnTick()
{
	BaseClass::OnTick();

	bool bHasModel = m_pMDLSequencePicker->GetModelName()[ 0 ] != 0 ? true : false;
	bool bHasSequence = m_pMDLSequencePicker->GetSequenceNumber() != -1 ? true : false;

	m_pOK->SetEnabled( bHasModel && bHasSequence );
}

void CMDLSequencePickerFrame::PerformLayout()
{
	BaseClass::PerformLayout();

	int x, y, w, h;
	GetClientArea( x, y, w, h );
	h -= 24;
	m_pMDLSequencePicker->SetBounds( x, y, w, h );

	h += 5;


	int bw = 120;
	int bwwithGap = 2 * bw + 10;

	x = ( w - bwwithGap ) / 2;
	m_pOK->SetBounds( x, y + h, bw, 16 );
	x += bw + 10;
	m_pCancel->SetBounds( x, y + h, bw, 16 );
}

void CMDLSequencePickerFrame::OnCancel()
{
	KeyValues *pActionKeys = new KeyValues( "AssetSelected" );
	pActionKeys->SetString( "ModelName", m_pMDLSequencePicker->GetModelName() );
	pActionKeys->SetString( "SequenceName", m_pMDLSequencePicker->GetSequenceName() );
	pActionKeys->SetInt( "SequenceNumber", m_pMDLSequencePicker->GetSequenceNumber() );

	PostActionSignal( pActionKeys );

	CloseModal();
}

void CMDLSequencePickerFrame::OnOK()
{
	CloseModal();
}


