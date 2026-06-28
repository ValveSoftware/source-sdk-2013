//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "matsys_controls/sequencepicker.h"

#include "tier1/utldict.h"
#include "tier1/KeyValues.h"
#include "studio.h"
#include "vgui/IInput.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Splitter.h"
#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/ListPanel.h"
#include "vgui_controls/Button.h"
#include "matsys_controls/matsyscontrols.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


//-----------------------------------------------------------------------------
//
// Sequence Picker
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Sort by sequence name
//-----------------------------------------------------------------------------
static int __cdecl SequenceSortFunc( vgui::ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	const char *string1 = item1.kv->GetString("sequence");
	const char *string2 = item2.kv->GetString("sequence");
	return stricmp( string1, string2 );
}

static int __cdecl ActivitySortFunc( vgui::ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	const char *string1 = item1.kv->GetString("activity");
	const char *string2 = item2.kv->GetString("activity");
	return stricmp( string1, string2 );
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSequencePicker::CSequencePicker( vgui::Panel *pParent, int nFlags ) : BaseClass( pParent, "SequencePicker" )
{
	m_hSelectedMDL = MDLHANDLE_INVALID;

	// property sheet - revisions, changes, etc.
	m_pPreviewSplitter = new Splitter( this, "PreviewSplitter", SPLITTER_MODE_HORIZONTAL, 1 );

	vgui::Panel *pSplitterTopSide = m_pPreviewSplitter->GetChild( 0 );
	vgui::Panel *pSplitterBottomSide = m_pPreviewSplitter->GetChild( 1 );

	// MDL preview
	m_pMDLPreview = new CMDLPanel( pSplitterTopSide, "MDLPreview" );
	SetSkipChildDuringPainting( m_pMDLPreview );

	m_pViewsSheet = new vgui::PropertySheet( pSplitterBottomSide, "ViewsSheet" );
 	m_pViewsSheet->AddActionSignalTarget( this );

	// sequences
	m_pSequencesPage = NULL;
	m_pSequencesList = NULL;
	if ( nFlags & PICK_SEQUENCES )
	{
		m_pSequencesPage = new PropertyPage( m_pViewsSheet, "SequencesPage" );
		m_pViewsSheet->AddPage( m_pSequencesPage, "Sequences" );
		m_pSequencesList = new ListPanel( m_pSequencesPage, "SequencesList" );
 		m_pSequencesList->AddColumnHeader( 0, "sequence", "sequence", 52, 0 );
		m_pSequencesList->AddActionSignalTarget( this );
		m_pSequencesList->SetSelectIndividualCells( true );
 		m_pSequencesList->SetEmptyListText(".MDL file contains no activities");
		m_pSequencesList->SetDragEnabled( true );
		m_pSequencesList->SetAutoResize( Panel::PIN_TOPLEFT, Panel::AUTORESIZE_DOWNANDRIGHT, 0, 0, 0, 0 );
		m_pSequencesList->SetSortFunc( 0, SequenceSortFunc );
		m_pSequencesList->SetSortColumn( 0 );
	}

	// Activities
	m_pActivitiesPage = NULL;
	m_pActivitiesList = NULL;
	if ( nFlags & PICK_ACTIVITIES )
	{
		m_pActivitiesPage = new PropertyPage( m_pViewsSheet, "ActivitiesPage" );
		m_pViewsSheet->AddPage( m_pActivitiesPage, "Activities" );
		m_pActivitiesList = new ListPanel( m_pActivitiesPage, "ActivitiesList" );
 		m_pActivitiesList->AddColumnHeader( 0, "activity", "activity", 52, 0 );
		m_pActivitiesList->AddActionSignalTarget( this );
		m_pActivitiesList->SetSelectIndividualCells( true );
		m_pActivitiesList->SetEmptyListText( ".MDL file contains no activities" );
 		m_pActivitiesList->SetDragEnabled( true );
		m_pActivitiesList->SetAutoResize( Panel::PIN_TOPLEFT, Panel::AUTORESIZE_DOWNANDRIGHT, 0, 0, 0, 0 );
		m_pActivitiesList->SetSortFunc( 0, ActivitySortFunc );
		m_pActivitiesList->SetSortColumn( 0 );
	}

	// Load layout settings; has to happen before pinning occurs in code
	LoadControlSettingsAndUserConfig( "resource/sequencepicker.res" );

	SETUP_PANEL( this );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSequencePicker::~CSequencePicker()
{
}


//-----------------------------------------------------------------------------
// Performs layout
//-----------------------------------------------------------------------------
void CSequencePicker::PerformLayout()
{
	// NOTE: This call should cause auto-resize to occur
	// which should fix up the width of the panels
	BaseClass::PerformLayout();

	int w, h;
	GetSize( w, h );

	// Layout the mdl splitter
	m_pPreviewSplitter->SetBounds( 0, 0, w, h );
}

	
//-----------------------------------------------------------------------------
// Purpose: rebuilds the list of activities	+ sequences
//-----------------------------------------------------------------------------
void CSequencePicker::RefreshActivitiesAndSequencesList()
{
	if ( m_pActivitiesList )
	{
		m_pActivitiesList->RemoveAll();
	}

	if ( m_pSequencesList )
	{
		m_pSequencesList->RemoveAll();
	}

	m_pMDLPreview->SetSequence( 0 );

	if ( m_hSelectedMDL == MDLHANDLE_INVALID )
		return;

	studiohdr_t *hdr = vgui::MDLCache()->GetStudioHdr( m_hSelectedMDL );

	CUtlDict<int, unsigned short> activityNames( true, 0, hdr->GetNumSeq() );
	    
	for (int j = 0; j < hdr->GetNumSeq(); j++)
	{
		if ( /*g_viewerSettings.showHidden ||*/ !(hdr->pSeqdesc(j).flags & STUDIO_HIDDEN))
		{
			const char *pActivityName = hdr->pSeqdesc(j).pszActivityName();
			if ( m_pActivitiesList && pActivityName && pActivityName[0] )
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
			if ( m_pSequencesList && pSequenceName && pSequenceName[0] )
			{
				KeyValues *pkv = new KeyValues("node", "sequence", pSequenceName);
				int nItemID = m_pSequencesList->AddItem( pkv, 0, false, false );

				KeyValues *pDrag = new KeyValues( "drag", "text", pSequenceName );
				pDrag->SetString( "texttype", "sequenceName" );
				pDrag->SetString( "mdl", vgui::MDLCache()->GetModelName( m_hSelectedMDL ) );
				m_pSequencesList->SetItemDragData( nItemID, pDrag );
			}
		}
	}

	if ( m_pSequencesList )
	{
		m_pSequencesList->SortList();
	}

	if ( m_pActivitiesList )
	{
		m_pActivitiesList->SortList();
	}
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
// Gets the selected activity/sequence
//-----------------------------------------------------------------------------
CSequencePicker::PickType_t CSequencePicker::GetSelectedSequenceType( )
{
	if ( m_pSequencesPage && ( m_pViewsSheet->GetActivePage() == m_pSequencesPage ) )
		return PICK_SEQUENCES;
	if ( m_pActivitiesPage && ( m_pViewsSheet->GetActivePage() == m_pActivitiesPage ) )
		return PICK_ACTIVITIES;
	return PICK_NONE;
}

const char *CSequencePicker::GetSelectedSequenceName( )
{
	if ( m_pSequencesPage && ( m_pViewsSheet->GetActivePage() == m_pSequencesPage ) )
	{
		int nIndex = m_pSequencesList->GetSelectedItem( 0 );
		if ( nIndex >= 0 )
		{
			KeyValues *pkv = m_pSequencesList->GetItem( nIndex );
			return pkv->GetString( "sequence", NULL );
		}
		return NULL;
	}

	if ( m_pActivitiesPage && ( m_pViewsSheet->GetActivePage() == m_pActivitiesPage ) )
	{
		int nIndex = m_pActivitiesList->GetSelectedItem( 0 );
		if ( nIndex >= 0 )
		{
			KeyValues *pkv = m_pActivitiesList->GetItem( nIndex );
			return pkv->GetString( "activity", NULL );
		}
		return NULL;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Plays the selected activity
//-----------------------------------------------------------------------------
void CSequencePicker::PlayActivity( const char *pActivityName )
{
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
void CSequencePicker::PlaySequence( const char *pSequenceName )
{
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
void CSequencePicker::OnPageChanged( )
{
	if ( m_pSequencesPage && ( m_pViewsSheet->GetActivePage() == m_pSequencesPage ) )
	{
		const char *pSequenceName = GetSelectedSequenceName();
		if ( pSequenceName )
		{
			PlaySequence( pSequenceName );
			PostActionSignal( new KeyValues( "SequencePreviewChanged", "sequence", pSequenceName ) );
		}
		return;
	}

	if ( m_pActivitiesPage && ( m_pViewsSheet->GetActivePage() == m_pActivitiesPage ) )
	{
		const char *pActivityName = GetSelectedSequenceName();
		if ( pActivityName )
		{
			PlayActivity( pActivityName );
			PostActionSignal( new KeyValues( "SequencePreviewChanged", "activity", pActivityName ) );
		}
		return;
	}
}


//-----------------------------------------------------------------------------
// Purpose: refreshes dialog on text changing
//-----------------------------------------------------------------------------
void CSequencePicker::OnItemSelected( KeyValues *kv )
{
	Panel *pPanel = (Panel *)kv->GetPtr("panel", NULL);
	if ( m_pSequencesList && (pPanel == m_pSequencesList ) )
	{
		const char *pSequenceName = GetSelectedSequenceName();
		if ( pSequenceName )
		{
			PlaySequence( pSequenceName );
			PostActionSignal( new KeyValues( "SequencePreviewChanged", "sequence", pSequenceName ) );
		}
		return;
	}

	if ( m_pActivitiesList && ( pPanel == m_pActivitiesList ) )
	{
		const char *pActivityName = GetSelectedSequenceName();
		if ( pActivityName )
		{
			PlayActivity( pActivityName );
			PostActionSignal( new KeyValues( "SequencePreviewChanged", "activity", pActivityName ) );
		}
		return;
	}
}


//-----------------------------------------------------------------------------
// Sets the MDL to select sequences in
//-----------------------------------------------------------------------------
void CSequencePicker::SetMDL( const char *pMDLName )
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
//
// Purpose: Modal picker frame
//
//-----------------------------------------------------------------------------
CSequencePickerFrame::CSequencePickerFrame( vgui::Panel *pParent, int nFlags ) : BaseClass( pParent, "SequencePickerFrame" )
{
	SetDeleteSelfOnClose( true );
	m_pPicker = new CSequencePicker( this, nFlags );
	m_pPicker->AddActionSignalTarget( this );
	m_pOpenButton = new Button( this, "OpenButton", "#FileOpenDialog_Open", this, "Open" );
	m_pCancelButton = new Button( this, "CancelButton", "#FileOpenDialog_Cancel", this, "Cancel" );
	SetBlockDragChaining( true );

	LoadControlSettingsAndUserConfig( "resource/sequencepickerframe.res" );

	m_pOpenButton->SetEnabled( false );
}


//-----------------------------------------------------------------------------
// Purpose: Activate the dialog
//-----------------------------------------------------------------------------
void CSequencePickerFrame::DoModal( const char *pMDLName )
{
	m_pPicker->SetMDL( pMDLName );
	BaseClass::DoModal();
}


//-----------------------------------------------------------------------------
// On mdl preview changed
//-----------------------------------------------------------------------------
void CSequencePickerFrame::OnSequencePreviewChanged( KeyValues *pKeyValues )
{
	const char *pSequence = pKeyValues->GetString( "sequence", NULL );
	const char *pActivity = pKeyValues->GetString( "activity", NULL );
	m_pOpenButton->SetEnabled( pSequence || pActivity );
}


//-----------------------------------------------------------------------------
// On command
//-----------------------------------------------------------------------------
void CSequencePickerFrame::OnCommand( const char *pCommand )
{
	if ( !Q_stricmp( pCommand, "Open" ) )
	{
		CSequencePicker::PickType_t type = m_pPicker->GetSelectedSequenceType( );
		if (( type == CSequencePicker::PICK_SEQUENCES ) || ( type == CSequencePicker::PICK_ACTIVITIES ))
		{
			const char *pSequenceName = m_pPicker->GetSelectedSequenceName();
			if ( pSequenceName )
			{
				if ( type == CSequencePicker::PICK_SEQUENCES )
				{
					PostActionSignal( new KeyValues("SequenceSelected", "sequence", pSequenceName ) );
				}
				else
				{
					PostActionSignal( new KeyValues("SequenceSelected", "activity", pSequenceName ) );
				}
				CloseModal();
				return;
			}
		}
		return;
	}

	if ( !Q_stricmp( pCommand, "Cancel" ) )
	{
		CloseModal();
		return;
	}

	BaseClass::OnCommand( pCommand );
}

	
