//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_hud_item_progress_tracker.h"
#include "iclientmode.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include "gc_clientsystem.h"
#include "engine/IEngineSound.h"
#include "quest_log_panel.h"
#include "econ_controls.h"
#include "tf_item_inventory.h"
#include "c_tf_player.h"
#include "quest_objective_manager.h"
#include "tf_spectatorgui.h"
#include "econ_quests.h"
#include "inputsystem/iinputsystem.h"
#include "tf_quest_map_node.h"
#include "tf_controls.h"
#include <vgui_controls/AnimationController.h>
#include "clientmode_tf.h"
#include "tf_quest_map_controller.h"
#include "tf_gc_client.h"
#include "iinput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

const float ATTRIB_TRACK_GLOW_HOLD_TIME = 2.f;
const float ATTRIB_TRACK_GLOW_DECAY_TIME = 0.25f;
const float ATTRIB_TRACK_BAR_GROW_RATE = 0.3f;
const float ATTRIB_TRACK_COMPLETE_PULSE_RATE = 2.f;
const float ATTRIB_TRACK_COMPLETE_PULSE_DIM_HOLD = 0.3f;
const float ATTRIB_TRACK_COMPLETE_PULSE_GLOW_HOLD = 0.9f;

enum EContractHUDVisibility
{
	CONTRACT_HUD_SHOW_NONE = 0,
	CONTRACT_HUD_SHOW_EVERYTHING,
	CONTRACT_HUD_SHOW_ACTIVE,
};

void cc_contract_progress_show_update( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	CHudItemAttributeTracker *pTrackerPanel = (CHudItemAttributeTracker *)GET_HUDELEMENT( CHudItemAttributeTracker );
	if ( pTrackerPanel )
	{
		pTrackerPanel->InvalidateLayout();
	}
}
ConVar tf_contract_progress_show( "tf_contract_progress_show", "1", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Settings for the contract HUD element: 0 show nothing, 1 show everything, 2 show only active contracts.", cc_contract_progress_show_update );
ConVar tf_contract_competitive_show( "tf_contract_competitive_show", "2", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Settings for the contract HUD element during competitive matches: 0 show nothing, 1 show everything, 2 show only active contracts.", cc_contract_progress_show_update );
ConVar tf_contract_progress_report_item_hold_time( "tf_contract_progress_report_item_hold_time", "2", FCVAR_DEVELOPMENTONLY );


EContractHUDVisibility GetContractHUDVisibility()
{
	if ( TFGameRules() && TFGameRules()->IsMatchTypeCompetitive() )
		return (EContractHUDVisibility)tf_contract_competitive_show.GetInt();

	return (EContractHUDVisibility)tf_contract_progress_show.GetInt();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestObjectiveTextPanel::CQuestObjectiveTextPanel( Panel* pParent, const char *pElementName, const QuestObjectiveInstance_t& objective, const char* pszResFileName  )
	: EditablePanel( pParent, pElementName )
	, m_strResFileName( pszResFileName )
	, m_objective( objective )
{
	Assert( !m_strResFileName.IsEmpty() );

	m_pAttribBlur = new Label( this, "AttribBlur", "" );
	m_pAttribGlow = new Label( this, "AttribGlow", "" );
	m_pAttribDesc = new Label( this, "AttribDesc", "" );

	REGISTER_COLOR_AS_OVERRIDABLE( m_enabledTextColor, "enabled_text_color_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( m_disabledTextColor, "disabled_text_color_override" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestObjectiveTextPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);

	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( m_strResFileName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestObjectiveTextPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
	//SetTall( GetContentTall() );
	//m_pAttribDesc->SetTall( GetTall() );
	m_bMapView = inResourceData->GetBool( "map_view", false );

	InvalidateLayout();
}

void CQuestObjectiveTextPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pAttribBlur->SetAlpha( 0 );
	m_pAttribGlow->SetAlpha( 0 );

	UpdateText();
}

void CQuestObjectiveTextPanel::UpdateText()
{
	const CQuestObjectiveDefinition *pObjectiveDef = m_objective.GetObjectiveDef();
	if ( pObjectiveDef && m_pQuestDef )
	{
		auto pQuest = GetQuestMapHelper().GetQuestByDefindex( m_pQuestDef->GetDefIndex() );
		const CQuestItemTracker* pItemTracker = pQuest ? QuestObjectiveManager()->GetTypedTracker< CQuestItemTracker* >( pQuest->GetID() ) : NULL;


		const char *pszDescriptionToken = pObjectiveDef->GetDescriptionToken();

		locchar_t loc_ItemDescription[MAX_ITEM_NAME_LENGTH];
		const locchar_t *pLocalizedObjectiveName = GLocalizationProvider()->Find( pszDescriptionToken );
		if ( !pLocalizedObjectiveName || !pLocalizedObjectiveName[0] )
		{
			// Couldn't localize it, just use it raw
			GLocalizationProvider()->ConvertUTF8ToLocchar( pszDescriptionToken, loc_ItemDescription, ARRAYSIZE( loc_ItemDescription ) );
		}
		else
		{
			locchar_t loc_IntermediateName[ MAX_ITEM_NAME_LENGTH ];
			locchar_t locCPValue[ MAX_ITEM_NAME_LENGTH ];
			loc_sprintf_safe( locCPValue, LOCCHAR( "%d" ), m_objective.GetPoints() );

			loc_scpy_safe( loc_IntermediateName, CConstructLocalizedString( pLocalizedObjectiveName, locCPValue ) );

			locchar_t *pszLocObjectiveFormat = GLocalizationProvider()->Find( g_QuestPointsDefs[ m_objective.GetPointsType() ].m_pszObjectiveText );

			int nPointsEarned = 0;
			if ( pItemTracker )
			{
				nPointsEarned = pItemTracker->GetEarnedPoints( GetPointsType() );
			}
			else if ( pQuest )
			{
				nPointsEarned = pQuest->GetEarnedPoints( GetPointsType() );
			}

			int nMaxPoints = m_pQuestDef->GetMaxPoints( GetPointsType() );
			int nNumEarnsRequired = ceil( (float)nMaxPoints / pObjectiveDef->GetPoints() );
			int nNumEarned = nPointsEarned / pObjectiveDef->GetPoints();

			CUtlString strBonusEarned( CFmtStr( "%d/%d", nNumEarned, nNumEarnsRequired ) );
			while ( strBonusEarned.Length() < 5 )
			{
				strBonusEarned.Append( ' ' );
			}
			CStrAutoEncode wstrRatio( strBonusEarned );
			locchar_t locBonusPrefix[ 32 ];
			const char* pszBonusRatioFormat = m_bMapView ? "#QuestPoints_BonusRatio_InMap" : "#QuestPoints_BonusRatio_InGame";
			loc_scpy_safe( locBonusPrefix, CConstructLocalizedString( GLocalizationProvider()->Find( pszBonusRatioFormat ), wstrRatio.ToWString() ) );

			loc_scpy_safe( loc_ItemDescription, CConstructLocalizedString( pszLocObjectiveFormat, loc_IntermediateName, locBonusPrefix ) );
		}

		SetDialogVariable( "attr_desc", loc_ItemDescription );
	}
}

void CQuestObjectiveTextPanel::SetDefinitions( const QuestObjectiveInstance_t& objective, const CQuestDefinition* pQuestDef )
{
	m_objective = objective;
	m_pQuestDef = pQuestDef;
	InvalidateLayout();
}

void CQuestObjectiveTextPanel::SetIsValid( bool bIsValid )
{
	if ( bIsValid )
	{
		m_pAttribDesc->SetFgColor( m_enabledTextColor );
	}
	else
	{
		m_pAttribDesc->SetFgColor( m_disabledTextColor );
	}
}

int CQuestObjectiveTextPanel::GetContentTall() const
{
	// Find the bottom of the text
	int nTextWide = 0, nTextTall = 0;
	m_pAttribDesc->GetContentSize( nTextWide, nTextTall );
	int nTextYpos = m_pAttribDesc->GetYPos();
	return nTextYpos + nTextTall;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestObjectiveTextPanel::SetProgress( Color glowColor )
{
	auto pAnim = g_pClientMode->GetViewportAnimationController();
	UpdateText();

	// Snap highlight
	pAnim->RunAnimationCommand( m_pAttribBlur, "alpha", 255, 0, 0, AnimationController::INTERPOLATOR_LINEAR, 0.f, true, false );
	pAnim->RunAnimationCommand( m_pAttribBlur, "fgcolor", glowColor, 0, 0, AnimationController::INTERPOLATOR_LINEAR, 0.f, true, false );

	pAnim->RunAnimationCommand( m_pAttribGlow, "alpha", 255, 0, 0, AnimationController::INTERPOLATOR_LINEAR, 0.f, true, false );
	pAnim->RunAnimationCommand( m_pAttribGlow, "fgcolor", glowColor, 0, 0, AnimationController::INTERPOLATOR_LINEAR, 0.f, true, false );

	pAnim->RunAnimationCommand( m_pAttribDesc, "alpha", 0, 0, 0, AnimationController::INTERPOLATOR_LINEAR, 0.f, true, false );
	
	// Lerp back
	pAnim->RunAnimationCommand( m_pAttribBlur, "alpha", 0, ATTRIB_TRACK_GLOW_HOLD_TIME, ATTRIB_TRACK_GLOW_DECAY_TIME, AnimationController::INTERPOLATOR_LINEAR, 0.f, false, false );
	pAnim->RunAnimationCommand( m_pAttribGlow, "alpha", 0, ATTRIB_TRACK_GLOW_HOLD_TIME, ATTRIB_TRACK_GLOW_DECAY_TIME, AnimationController::INTERPOLATOR_LINEAR, 0.f, false, false );
	pAnim->RunAnimationCommand( m_pAttribDesc, "alpha", 255, ATTRIB_TRACK_GLOW_HOLD_TIME, ATTRIB_TRACK_GLOW_DECAY_TIME, AnimationController::INTERPOLATOR_LINEAR, 0.f, false, false );
}

void CQuestObjectiveTextPanel::HighlightCompletion()
{
	Color colorHighlight = scheme()->GetIScheme( GetScheme() )->GetColor( "CreditsGreen", Color( 255, 255, 255, 255 ) );
	BrigthenColor( colorHighlight, 50 );
	// Highlight
	SetProgress( colorHighlight );
	// Fade to disabled since we're done
	auto pAnim = g_pClientMode->GetViewportAnimationController();
	pAnim->RunAnimationCommand( m_pAttribDesc, "alpha", 255, ATTRIB_TRACK_GLOW_HOLD_TIME, ATTRIB_TRACK_GLOW_DECAY_TIME, AnimationController::INTERPOLATOR_LINEAR, 0.f, true, false );
	pAnim->RunAnimationCommand( m_pAttribDesc, "fgcolor", colorHighlight, 0, 0, AnimationController::INTERPOLATOR_LINEAR, 0.f, true, false );
	pAnim->RunAnimationCommand( m_pAttribDesc, "fgcolor", m_disabledTextColor, ATTRIB_TRACK_GLOW_HOLD_TIME, ATTRIB_TRACK_GLOW_DECAY_TIME, AnimationController::INTERPOLATOR_LINEAR, 0.f, false, false );
}

static float m_sflEventRecievedTime = 0.f;

CQuestProgressTrackerPanel::PointsView_t::PointsView_t()
	: m_flCurrentProgress( 0 )
	, m_flTargetProgress( 0 )
	, m_nMaxPoints( 0 )
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestProgressTrackerPanel::CQuestProgressTrackerPanel( Panel* pParent,
														const char *pElementName,
														const CQuest* pQuest,
														const CQuestDefinition* pQuestDef,
														const char* pszResFile /*= "resource/ui/quests/QuestItemTrackerPanel_Base.res"*/ )
	: EditablePanel( pParent, pElementName )
	, m_pQuest( NULL )
	, m_flLastThink( 0.f )
	, m_pszSoundToPlay( NULL )
	, m_nContentTall( 0 )
	, m_bMapView( false )
	, m_pQuestDef( pQuestDef )
	, m_strResFile( pszResFile )
{
	ListenForGameEvent( "quest_objective_completed" );
	ListenForGameEvent( "player_spawn" );
	ListenForGameEvent( "inventory_updated" );
	ListenForGameEvent( "localplayer_changeclass" );
	ListenForGameEvent( "schema_updated" );
	ListenForGameEvent( "quest_turn_in_state" );

	m_pItemName = new Label( this, "ItemName", "" );
	m_pItemName->SetAutoDelete( false );


	m_PointsBars.m_pBarBG = new EditablePanel( this, CFmtStr( "ProgressBarBG" ) );
	m_PointsBars.m_pBarBG->SetAutoDelete( false );

	m_PointsBars.m_pBarCommitted = new EditablePanel( m_PointsBars.m_pBarBG, "ProgressBarStandard" );
	m_PointsBars.m_pBarCommitted ->SetAutoDelete( false );

	m_PointsBars.m_pBarUncommitted = new EditablePanel( m_PointsBars.m_pBarBG, "ProgressBarStandardHighlight" );
	m_PointsBars.m_pBarUncommitted->SetAutoDelete( false );

	m_PointsBars.m_pBarJustEarned = new EditablePanel( m_PointsBars.m_pBarBG, "ProgressBarJustEarned" );
	m_PointsBars.m_pBarJustEarned->SetAutoDelete( false );

	m_PointsBars.m_flUpdateTime = 0.f;

	m_bTurningIn = false;

	m_pPrimaryObjectiveLabel = new CExLabel( this, "PrimaryLabel", (const char*)NULL );
	m_pBonusObjectiveLabel = new CExLabel( this, "BonusLabel", (const char*)NULL );

	for( int i=0; i < ARRAYSIZE( m_arStarImages ); ++i )
	{
		m_arStarImages[ i ] = new ImagePanel( this, CFmtStr( "star%d", i ) );
	}

	// Do this AFTER all the panels are created
	SetQuest( pQuest );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestProgressTrackerPanel::~CQuestProgressTrackerPanel()
{
	m_pItemName->MarkForDeletion();
	m_PointsBars.m_pBarBG->MarkForDeletion();
	m_PointsBars.m_pBarCommitted ->MarkForDeletion();
	m_PointsBars.m_pBarUncommitted->MarkForDeletion();
	m_PointsBars.m_pBarJustEarned->MarkForDeletion();

	// Remove all expired labels
	FOR_EACH_VEC_BACK( m_vecScorerLabels, i )
	{
		m_vecScorerLabels[ i ].second->MarkForDeletion();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestProgressTrackerPanel::ApplySettings( KeyValues *inResourceData )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);

	BaseClass::ApplySettings( inResourceData );

	KeyValues* pBarKV = inResourceData->FindKey( "progressbarKV" );
	KeyValues* pBonusBarKV = inResourceData->FindKey( "bonusprogressbarKV" );

	if ( pBarKV )
	{
		m_PointsBars.m_pBarBG->ApplySettings( pBarKV );
	}

	if ( pBonusBarKV == NULL )
	{
		pBonusBarKV = pBarKV;
	}


	m_strItemAttributeResFile = inResourceData->GetString( "item_attribute_res_file" );
	Assert( !m_strItemAttributeResFile.IsEmpty() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestProgressTrackerPanel::ApplySchemeSettings( IScheme *pScheme )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);

	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( m_strResFile );

	UpdateObjectives();
	CaptureProgress();

	InvalidateLayout();
}

void CQuestProgressTrackerPanel::UpdateObjectives()
{
	if ( m_pQuestDef )
	{
		const QuestObjectiveDefVec_t& vecObjectives = m_pQuestDef->GetObjectives();

		// Reassign, or recreate objective panels
		int i = 0;
		for ( i=0; i < vecObjectives.Count(); ++i )
		{
			auto objective = vecObjectives[ i ];

			// Create a new objective panel if we need
			if ( i >= m_vecObjectivePanels.Count() )
			{
				CQuestObjectiveTextPanel* pObjectivePanel = new CQuestObjectiveTextPanel( this, "QuestObjectiveTextPanel", objective, m_strItemAttributeResFile );
				pObjectivePanel->InvalidateLayout( true );
				// This has to be here.  If this is not here, the objective text panel might not
				// be size right when our parents PerformLayout happens
				if ( !IsLayoutInvalid() )
				{
					pObjectivePanel->MakeReadyForUse();
				}
				m_vecObjectivePanels.AddToTail( pObjectivePanel );
			}
			else
			{
				// Re-use an existing objective panel if there's one available
				CQuestObjectiveTextPanel* pObjectivePanel = m_vecObjectivePanels[ i ];
				pObjectivePanel->SetVisible( true );
				pObjectivePanel->SetDefinitions( objective, m_pQuestDef );
				pObjectivePanel->InvalidateLayout( true );
			}
		}

		// Remove unnecessary ones
		while( m_vecObjectivePanels.Count() > vecObjectives.Count() )
		{
			m_vecObjectivePanels.Tail()->MarkForDeletion();
			m_vecObjectivePanels.Remove( m_vecObjectivePanels.Count() - 1 );
		}

		InvalidateLayout();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int QuestSort_PointsAscending( CQuestObjectiveTextPanel* const* p1, CQuestObjectiveTextPanel* const* p2 )
{
	if ( (*p1)->GetPointsType() != (*p2)->GetPointsType() )
	{
		// Largest type at the bottom
		return (*p1)->GetPointsType() - (*p2)->GetPointsType();
	}

	if ( (*p1)->GetPoints() != (*p2)->GetPoints() )
	{
		// Smallest point value on the bottom
		return (*p1)->GetPoints() - (*p2)->GetPoints();
	}

	return (*p1)->GetDefIndex() - (*p2)->GetDefIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestProgressTrackerPanel::PerformLayout()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);

	BaseClass::PerformLayout();	

	if ( !m_pQuestDef )
		return;

	// Set the name into dialog variables
	const wchar_t *pszLocToken = g_pVGuiLocalize->Find( m_pQuestDef->GetLocName() );
	SetDialogVariable( "itemname", pszLocToken );
	m_pItemName->SetVisible( m_bShowItemName );

	const CQuestItemTracker* pItemTracker = NULL;
	const CQuestMapNode* pNode = NULL;
	if ( m_pQuest )
	{
		pNode = GetQuestMapHelper().GetQuestMapNodeByID( m_pQuest->GetSourceNodeID() );
		pItemTracker = QuestObjectiveManager()->GetTypedTracker< CQuestItemTracker* >( m_pQuest->GetID() );
	}
	
	//
	// Layout the point bars
	//
	uint32 nCurrentPoints = 0;
	if ( m_pQuest )
	{
		nCurrentPoints = pItemTracker ? pItemTracker->GetEarnedPoints( QUEST_POINTS_NOVICE ) : m_pQuest->GetEarnedPoints( QUEST_POINTS_NOVICE );
	}
	locchar_t locValue[ MAX_ITEM_NAME_LENGTH ];

	if ( nCurrentPoints >= m_pQuestDef->GetMaxPoints( QUEST_POINTS_NOVICE ) )
	{
		if ( pNode && pNode->BIsMedalEarned( QUEST_POINTS_NOVICE ) )
		{
			loc_scpy_safe( locValue, g_pVGuiLocalize->Find( "#QuestPoints_Complete" ) );
		}
		else
		{
			loc_scpy_safe( locValue, g_pVGuiLocalize->Find( "#QuestPoints_ReadyTurnIn" ) );
		}
	}
	else
	{
		const locchar_t *pPointsToken = GLocalizationProvider()->Find( g_QuestPointsDefs[ QUEST_POINTS_NOVICE ].m_pszBarText );
		loc_scpy_safe( locValue, CConstructLocalizedString( pPointsToken, nCurrentPoints, m_PointsBars.m_nMaxPoints ) );
	}

	m_PointsBars.m_pBarBG->SetDialogVariable( "points" , locValue );
	m_PointsBars.m_pBarCommitted ->SetDialogVariable( "points" , locValue );
	m_PointsBars.m_pBarUncommitted->SetDialogVariable( "points" , locValue );
	m_PointsBars.m_pBarJustEarned->SetDialogVariable( "points", locValue );

	m_PointsBars.m_pBarJustEarned->SetVisible( !m_bMapView );


	int nWide = 0, nTall = 0;
	if ( m_pItemName->IsVisible() && !m_bMapView )
	{
		m_pItemName->GetContentSize( nWide, nTall );
	}

	int nX = m_nAttribXOffset;
	int nY = nTall + m_nAttribYStartOffset;

	bool bPendingCompletion = false;

	if ( pItemTracker && m_pQuest )
	{
		auto lambdaPendingCompletionForType = [ & ]( EQuestPoints eType )
		{
			return pItemTracker->GetEarnedPoints( eType ) > m_pQuest->GetEarnedPoints( eType ) &&
				   pItemTracker->GetEarnedPoints( eType ) >= m_pQuestDef->GetMaxPoints( eType );
		};

		bPendingCompletion = lambdaPendingCompletionForType( QUEST_POINTS_NOVICE ) ||
							 lambdaPendingCompletionForType( QUEST_POINTS_ADVANCED ) ||
							 lambdaPendingCompletionForType( QUEST_POINTS_EXPERT );
	}

	//
	// Not yet committed panel
	//
	EditablePanel* pNotYetCommittedPanel = FindControl< EditablePanel >( "NotYetCommittedContainer", true );
	if ( pNotYetCommittedPanel )
	{
		pNotYetCommittedPanel->SetVisible( bPendingCompletion );
		if ( bPendingCompletion )
		{
			pNotYetCommittedPanel->SetPos( pNotYetCommittedPanel->GetXPos(), nY );
			nY += pNotYetCommittedPanel->GetTall();
		}
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	m_vecObjectivePanels.Sort( &QuestSort_PointsAscending );

	UpdateBars();
	UpdateStars();

	//
	// Objective panels
	//
	m_vecObjectivePanels.Sort( &QuestSort_PointsAscending );
	bool bBonusLabelPlaced = false;
	FOR_EACH_VEC( m_vecObjectivePanels, i )
	{
		CQuestObjectiveTextPanel* pPanel = m_vecObjectivePanels[i];

		// Check if any of the points at or below this type are needed.  If this is an Expert
		// objective that is complete, but we still need normal points, then we should show.
		int nPointType = pPanel->GetPointsType();
		bool bObjectiveNeeded = false;
		for( nPointType; nPointType >= QUEST_POINTS_NOVICE && !bObjectiveNeeded; --nPointType )
		{
			bObjectiveNeeded = bObjectiveNeeded || !ArePointsCompleted( nPointType );
		}

		if ( i == 0 )
		{
			m_pPrimaryObjectiveLabel->SetVisible( m_bMapView );
			m_pBonusObjectiveLabel->SetVisible( false );

			if ( m_bMapView )
			{
				m_pPrimaryObjectiveLabel->SetPos( m_pPrimaryObjectiveLabel->GetXPos(), nY );
			}
			else
			{
				m_pPrimaryObjectiveLabel->SetVisible( false );
			}
		}

		if ( i > 0 && !bBonusLabelPlaced )
		{
			m_pBonusObjectiveLabel->SetVisible( m_bMapView );
			if ( m_bMapView )
			{
				bBonusLabelPlaced = true;
				m_pBonusObjectiveLabel->SetPos( m_pBonusObjectiveLabel->GetXPos(), nY );
			}
		}

		if ( i < ARRAYSIZE( m_arStarImages ) )
		{
			m_arStarImages[ i ]->SetPos( m_arStarImages[ i ]->GetXPos(), nY - YRES( 1 ) );
		}

		// Hide all objectives if everything is completed, or just hide standard objectives if standard points are completed
		if ( !m_bMapView && !bObjectiveNeeded )
		{
			pPanel->SetVisible( false );
		}
		else
		{
			pPanel->SetPos( nX, nY );
			nY += pPanel->GetContentTall();
			pPanel->SetVisible( true );

			InvalidReasonsContainer_t invalidReasons;
			if ( pItemTracker && pLocalPlayer )
			{
				// Fixup validity, which changes the color of the 
				const CBaseQuestObjectiveTracker* pObjectiveTracker = pItemTracker->FindTrackerForDefIndex( pPanel->GetDefIndex() );
				if ( pObjectiveTracker )
				{
					pObjectiveTracker->IsValidForPlayer( pLocalPlayer, invalidReasons );
				}
			}

			if ( !m_bTurningIn )
			{
				pPanel->SetIsValid( invalidReasons.IsValid() && bObjectiveNeeded );
			}

			nY += m_nAttribYStep;
		}

		// Bar under the primary
		if ( i == QUEST_POINTS_NOVICE )
		{
			nY += m_nBarGap;
			m_PointsBars.m_pBarBG->SetPos( m_PointsBars.m_pBarBG->GetXPos(), nY );
			nY += m_PointsBars.m_pBarBG->GetTall() + 2;
			nY += m_nBarGap;
		}
	}
	
	nY += YRES( 2 );

	m_nContentTall = nY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CQuestProgressTrackerPanel::GetContentTall() const
{
	return m_nContentTall;
}

static double s_flLastSoundTime = 0;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestProgressTrackerPanel::OnThink()
{
	BaseClass::OnThink();

	if ( BIsTurningIn() )
	{
		if ( m_bTurningIn )
		{
			locchar_t locValue[ MAX_ITEM_NAME_LENGTH ];

			float flPercentAnimated = (float)m_PointsBars.m_pBarCommitted->GetWide() / m_PointsBars.m_pBarBG->GetWide() * 100.f;

			CStrAutoEncode strProgress( CFmtStr( "%.0f", flPercentAnimated ) );
			loc_scpy_safe( locValue, CConstructLocalizedString( g_pVGuiLocalize->Find( "#QuestPoints_Transmitting" ), strProgress.ToWString() ) );
			m_PointsBars.m_pBarCommitted->SetBgColor( scheme()->GetIScheme( GetScheme() )->GetColor( "StoreGreen", Color( 255, 255, 255, 255 ) ) );
			m_PointsBars.m_pBarBG->SetDialogVariable( "points" , locValue );
			m_PointsBars.m_pBarCommitted ->SetDialogVariable( "points" , locValue );
			m_PointsBars.m_pBarUncommitted->SetDialogVariable( "points" , locValue );
			m_PointsBars.m_pBarJustEarned->SetDialogVariable( "points", locValue );
		}
	}

	const float flNow = Plat_FloatTime();
	
	// Prune expired scorer labels
	FOR_EACH_VEC_BACK( m_vecScorerLabels, i )
	{
		if ( m_vecScorerLabels[ i ].first < flNow )
		{
			m_vecScorerLabels[ i ].second->MarkForDeletion();
			m_vecScorerLabels.Remove( i );
		}
	}

	if ( !m_pQuest || m_bMapView )
		return;

	// Give a little time in case other messages are in flight
	if ( m_pszSoundToPlay && ( flNow - m_sflEventRecievedTime ) > 0.1f )
	{
		bool bConnectedToMatchServer = GTFGCClientSystem()->BConnectedToMatchServer( false );
		bool bRoundEnd = !BInEndOfMatch() && ( !TFGameRules() || TFGameRules()->State_Get() != GR_STATE_TEAM_WIN );

		if ( !bConnectedToMatchServer || !bRoundEnd || m_nQueuedSoundPriority < EQuestPoints_ARRAYSIZE )
			{
			// Dont play sounds very frequently
			if ( ( flNow - s_flLastSoundTime ) > 0.1f )
			{
				s_flLastSoundTime = flNow;
				CLocalPlayerFilter filter;
			
				C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, m_pszSoundToPlay );
			}
		}

		m_pszSoundToPlay = NULL;
	}

	float flGlowTime = flNow - m_PointsBars.m_flUpdateTime;

	if ( flGlowTime > ATTRIB_TRACK_GLOW_HOLD_TIME && !m_PointsBars.BIsDoneProgressing() )
	{
		// Resize/position the bars
		UpdateBars();

		// This happens when all the bars are all caught up
		if ( m_PointsBars.BIsDoneProgressing() )
		{
			InvalidateLayout();
		}
	}

	if ( m_PointsBars.BIsDoneProgressing() )
	{
		m_PointsBars.m_pBarUncommitted->SetVisible( true );
	}

	m_flLastThink = flNow;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestProgressTrackerPanel::FireGameEvent( IGameEvent *pEvent )
{
	if ( FStrEq( pEvent->GetName(), "quest_objective_completed" ) )
	{
		itemid_t nIDLow = 0x00000000FFFFFFFF & (itemid_t)pEvent->GetInt( "quest_item_id_low" );
		itemid_t nIDHi =  0xFFFFFFFF00000000 & (itemid_t)pEvent->GetInt( "quest_item_id_hi" ) << 32;
		itemid_t nID = nIDLow | nIDHi;
		int nUserID = pEvent->GetInt( "scorer_user_id", -1 );
		if ( !m_pQuest || m_pQuest->GetID() != nID )
			return;

		uint32 nObjectiveDefIndex = pEvent->GetInt( "quest_objective_id" );
		// Don't do sounds if there's no objective that made this progress
		if ( nObjectiveDefIndex == (uint32)-1 )
			return;

		const QuestObjectiveDefVec_t& vecObjectives = m_pQuest->GetDefinition()->GetObjectives();
		const QuestObjectiveInstance_t* pObjective = NULL;

		FOR_EACH_VEC( vecObjectives, i )
		{
			if ( vecObjectives[ i ].GetObjectiveDef()->GetDefIndex() == nObjectiveDefIndex )
			{
				pObjective = &vecObjectives[ i ];
				break;
			}
		}

		if ( !pObjective )
			return;

		float flProgress = m_PointsBars.m_flTargetProgress;
		// Capture whatever progress has happened
		CaptureProgress();
		flProgress = m_PointsBars.m_flTargetProgress - flProgress;

		

		const CQuestItemTracker* pItemTracker = QuestObjectiveManager()->GetTypedTracker< CQuestItemTracker* >( m_pQuest->GetID() );

		if ( pItemTracker )
		{
			CQuestObjectiveTextPanel* pObjectivePanel = NULL;
			FOR_EACH_VEC( m_vecObjectivePanels, i )
			{
				if ( m_vecObjectivePanels[i]->GetDefIndex() == nObjectiveDefIndex )
				{
					pObjectivePanel = m_vecObjectivePanels[ i ];
					break;
				}
			}

			// For sounds
			EQuestPoints ePointsType = pObjective->GetPointsType();
			const char* pszSoundToPlay = NULL;

			// Check if it's time to play a sound
			Color colorToUse = m_clrStandardHighlight;
			

			bool bPartyCompleted = false;
			C_BasePlayer* pScorerPlayer = UTIL_PlayerByUserId( nUserID );
			if ( pScorerPlayer )
			{
				CSteamID scorerSteamID;
				pScorerPlayer->GetSteamID( &scorerSteamID );

				// If we didn't do the scoring, show a different color and show a label of who did
				if ( !pScorerPlayer->IsLocalPlayer()
					 )
				{
					bPartyCompleted = true;

					colorToUse = Color( 0, 225, 50, 255 );

					// Get the friend's name
					wchar_t wszScorerNameBuf[ 128 ];
					GetPlayerNameForSteamID( wszScorerNameBuf, sizeof( wszScorerNameBuf ), scorerSteamID );

					// Create a panel to house the labels
					EditablePanel* pScorerPanel = new EditablePanel( this, "Scorer" );
					pScorerPanel->LoadControlSettings( "Resource/UI/Quests/QuestObjectiveScorer.res" );
					pScorerPanel->MakeReadyForUse();
					pScorerPanel->SetDialogVariable( "scorer", wszScorerNameBuf );
					pScorerPanel->SetAutoDelete( false );
					// Position it next to the objective that triggered
					pScorerPanel->SetPos(m_PointsBars.m_pBarBG->GetXPos() - pScorerPanel->GetWide(),
										  m_PointsBars.m_pBarBG->GetYPos() );
					// Animate the label so it drifts off to the left
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pScorerPanel, "ObjectiveCompletedByUser", false );

					// Set the color on the labels to match everything else
					Label * pScorerLabel = pScorerPanel->FindControl< Label >( "ScorerLabel" );
					if ( pScorerLabel )
						pScorerLabel->SetFgColor( colorToUse );
					pScorerLabel = pScorerPanel->FindControl< Label >( "ScorerLabelBlur" );
					if ( pScorerLabel )
						pScorerLabel->SetFgColor( colorToUse );

					// Store in a list that we'll delete when we need
					std::pair< float, Panel* > newEntry;
					newEntry.first = Plat_FloatTime() + ATTRIB_TRACK_GLOW_HOLD_TIME;
					newEntry.second = pScorerPanel;
					m_vecScorerLabels.AddToTail( newEntry );
				}
			}

			bool bCompleteSound = false;
			// Which sound to play
			if ( pItemTracker->GetEarnedPoints( ePointsType ) >= m_pQuest->GetDefinition()->GetMaxPoints( ePointsType ) )
			{
				pszSoundToPlay = g_QuestPointsDefs[ ePointsType ].m_pszPointsCompletedSound;
				bCompleteSound = true;
			}
			else
			{
				pszSoundToPlay = bPartyCompleted ? g_QuestPointsDefs[ ePointsType ].m_pszObjectiveCompletedSoundParty
												 : g_QuestPointsDefs[ ePointsType ].m_pszObjectiveCompletedSound;
			}

			// Make the objective text highlight
			if ( pObjectivePanel )
			{
				pObjectivePanel->SetProgress( colorToUse ); 
			}

			const float flHighlightHold = 1.f;
			const float flHighlightFade = 2.f;

			// Quickly turn bright
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(m_PointsBars.m_pBarJustEarned, "BgColor", colorToUse, 0.0f, 0.1f, vgui::AnimationController::INTERPOLATOR_GAIN, 0.8f, true, false );
			// Then fade away after a bit
			colorToUse.SetColor( colorToUse.r(), colorToUse.g(), colorToUse.b(), 0 );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(m_PointsBars.m_pBarJustEarned, "BgColor", colorToUse, 1.f, 2.f, vgui::AnimationController::INTERPOLATOR_GAIN, 0.8f, false, false );

			// If this scoring comes in while we're still highlighting a previous scoring, extend the
			// just-earned to include the new scoring.  We clear the last earned progress here if it's
			// NOT within the highlight time
			if ( ( Plat_FloatTime() - m_sflEventRecievedTime ) > ( flHighlightHold + flHighlightFade ) )
			{
				m_flCurrentJustEarnedProgress = 0.f;
			}

			m_flCurrentJustEarnedProgress += flProgress;

			// The just-earned bar goes at the end and is the size of the most recent score
			float flJustEarnedWide = floor( m_flCurrentJustEarnedProgress * m_PointsBars.m_pBarBG->GetWide() ) ;
			m_PointsBars.m_pBarJustEarned->SetWide( flJustEarnedWide );
			m_PointsBars.m_pBarJustEarned->SetPos( floor( m_PointsBars.m_flTargetProgress * m_PointsBars.m_pBarBG->GetWide() ) - flJustEarnedWide, m_PointsBars.m_pBarJustEarned->GetYPos() );

			//m_arPointsBars[ pObjective->GetPointsType() ].m_pProgressBarPointsHighlight->SetBgColor( colorToUse );

			// Priority will handle this for us
			if ( ePointsType > m_nQueuedSoundPriority || m_pszSoundToPlay == NULL )
			{
				m_nQueuedSoundPriority = bCompleteSound ? ePointsType + EQuestPoints_ARRAYSIZE : ePointsType;
				m_pszSoundToPlay = pszSoundToPlay;

				m_sflEventRecievedTime = Plat_FloatTime();
			}
		}
	}
	else if ( FStrEq( pEvent->GetName(), "player_spawn" ) 
		   || FStrEq( pEvent->GetName(), "inventory_updated" ) 
		   || FStrEq( pEvent->GetName(), "localplayer_changeclass" ) )
	{
		InvalidateLayout();
	}
	else if ( FStrEq( pEvent->GetName(), "schema_updated" ) )
	{
		FOR_EACH_VEC( m_vecObjectivePanels, i )
		{
			m_vecObjectivePanels[ i ]->InvalidateLayout();
		}

		InvalidateLayout();
	}
	else if ( FStrEq( pEvent->GetName(), "quest_turn_in_state" ) )
	{
		// m_bMapView is what's used in the CYOA Map, so if it's false, it's the in-game version.
		// We only want the CYOA Map to do all this
		if ( !m_bMapView )
			return;

		EQuestTurnInState eState = (EQuestTurnInState)pEvent->GetInt( "state" );

		switch( eState )
		{
			case TURN_IN_BEGIN:
			{
				m_bTurningIn = true;
				m_bSuppressStarChanges = true;

				//
				// Setup initial turn-in animation state
				//
				m_PointsBars.m_pBarCommitted->SetBgColor( scheme()->GetIScheme( GetScheme() )->GetColor( "StoreGreen", Color( 255, 255, 255, 255 ) ) );
				m_PointsBars.m_pBarCommitted->SetWide( 0 );
				m_PointsBars.m_pBarCommitted->SetPos( 0, 0 );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_PointsBars.m_pBarCommitted, "wide", m_PointsBars.m_pBarBG->GetWide(), 0.0f, k_flQuestTurnInTime, vgui::AnimationController::INTERPOLATOR_BIAS, RandomFloat( 0.1f, 0.3f ), true, false );

				// Tell ourselves to end after a delay
				PostMessage( this, new KeyValues( "EndTurnInAnimation" ), k_flQuestTurnInTime + 2.5f );

				break;
			}

			case TURN_IN_SHOW_SUCCESS:
			{
				break;
			}

			case TURN_IN_HIDE_SUCCESS:
			{
				// If this is a bar that would be getting turned in, make the the highlight bar fill in
				if ( m_bTurningIn )
				{
					// Snap to exaggerated bright green
					Color colorHighlight = scheme()->GetIScheme( GetScheme() )->GetColor( "CreditsGreen", Color( 255, 255, 255, 255 ) );
					BrigthenColor( colorHighlight, 20 );
					g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_PointsBars.m_pBarCommitted, "BgColor", colorHighlight, 0.0f, 0, vgui::AnimationController::INTERPOLATOR_BIAS, 0.f, true, false );
					// Lerp down to natural color
					Color colorNatural = scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_ActiveOrange", Color( 255, 255, 255, 255 ) );
					g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_PointsBars.m_pBarCommitted, "BgColor", colorNatural, 0.5f, 1.0f, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.f, false, false );

					// Set them to say "Complete"
					locchar_t* pwszCompleted = g_pVGuiLocalize->Find( "#QuestPoints_Complete" );
					m_PointsBars.m_pBarBG->SetDialogVariable( "points" , pwszCompleted );
					m_PointsBars.m_pBarCommitted ->SetDialogVariable( "points" , pwszCompleted );
					m_PointsBars.m_pBarUncommitted->SetDialogVariable( "points" , pwszCompleted );
					m_PointsBars.m_pBarJustEarned->SetDialogVariable( "points", pwszCompleted );
				}

				break;
			}

			case TURN_IN_SHOW_STARS_EARNED:
			{
				m_bSuppressStarChanges = false;
			
				auto& msgProgress = GetQuestMapController().GetMostRecentProgressReport();
				float flDelay = 0.f;
				auto lambdaUpdateStar = [&]( int nIndex )
				{
					PostMessage( this, new KeyValues( "UpdateStar", "index", nIndex ), flDelay );

					auto pAnim = g_pClientMode->GetViewportAnimationController();
					auto pStar = m_arStarImages[ nIndex ];
					float flScale = 1.5;
					pAnim->RunAnimationCommand( pStar, "wide", pStar->GetWide() * flScale,	flDelay + 0.0f,	0.05f, AnimationController::INTERPOLATOR_LINEAR, 0.f, true, false );
					pAnim->RunAnimationCommand( pStar, "wide", pStar->GetWide() ,			flDelay + 0.1f, 0.2f, AnimationController::INTERPOLATOR_LINEAR, 0.f, false, false );
					pAnim->RunAnimationCommand( pStar, "tall", pStar->GetTall() * flScale,	flDelay + 0.0f,	0.05f, AnimationController::INTERPOLATOR_LINEAR, 0.f, true, false );
					pAnim->RunAnimationCommand( pStar, "tall", pStar->GetTall() ,			flDelay + 0.1f, 0.2f, AnimationController::INTERPOLATOR_LINEAR, 0.f, false, false );

					int nDelta = ( ( pStar->GetWide() * flScale ) - pStar->GetWide() ) * 0.5f;
					pAnim->RunAnimationCommand( pStar, "xpos", pStar->GetXPos() - nDelta,	flDelay + 0.0f, 0.05f, AnimationController::INTERPOLATOR_LINEAR, 0.f, true, false );
					pAnim->RunAnimationCommand( pStar, "xpos", pStar->GetXPos(),			flDelay + 0.1f, 0.2f, AnimationController::INTERPOLATOR_LINEAR, 0.f, false, false );
					pAnim->RunAnimationCommand( pStar, "ypos", pStar->GetYPos() - nDelta,	flDelay + 0.0f, 0.05f, AnimationController::INTERPOLATOR_LINEAR, 0.f, true, false );
					pAnim->RunAnimationCommand( pStar, "ypos", pStar->GetYPos(),			flDelay + 0.1f, 0.2f, AnimationController::INTERPOLATOR_LINEAR, 0.f, false, false );

					if ( nIndex < m_vecObjectivePanels.Count() )
					{
						PostMessage( m_vecObjectivePanels[ nIndex ], new KeyValues( "HighlightCompletion" ), flDelay );
					}
					flDelay += 0.3f;
				};

				if ( msgProgress.star_0_earned() )
				{
					lambdaUpdateStar( 0 );
				}
				if ( msgProgress.star_1_earned() )
				{
					lambdaUpdateStar( 1 );
				}
				if ( msgProgress.star_2_earned() )
				{
					lambdaUpdateStar( 2 );
				}

				break;
			}

			case TURN_IN_SHOW_BLOOD_MONEY_EARNED:
			{
				break;
			}

			case TURN_IN_SHOW_ITEM_PICKUP_SCREEN:
			{
				break;
			}

			case TURN_IN_SHOW_FAILURE:
			{
				break;
			}

			case TURN_IN_HIDE_FAILURE:
			{
				break;
			}


			case TURN_IN_COMPLETE:
			{
				m_bTurningIn = false;
				m_bSuppressStarChanges = false;

				InvalidateLayout( true );
				break;
			}

			// Nothing to do for these
			case TURN_IN_HIDE_NODE_VIEW:
			case TURN_IN_SHOW_NODE_UNLOCKS:
				break;
		};
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestProgressTrackerPanel::UpdateBars()
{
	if ( !BIsTurningIn() )
	{
		// Everything is relative to the background bar
		int nWide = m_PointsBars.m_pBarBG->GetWide();

		// Resize standard bar
		m_PointsBars.m_pBarCommitted->SetWide( floor(m_PointsBars.m_flCurrentProgress * nWide ) );

		// Highlight bars snap to the target width
		m_PointsBars.m_pBarUncommitted->SetWide( floor(m_PointsBars.m_flTargetProgress * nWide) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestProgressTrackerPanel::CaptureProgress()
{
	if ( !m_pQuest )
	{

		m_PointsBars.m_flCurrentProgress = 0.f;
		m_PointsBars.m_flTargetProgress = 0.f;

		return;
	}

	const CQuestItemTracker* pItemTracker = QuestObjectiveManager()->GetTypedTracker< CQuestItemTracker* >( m_pQuest->GetID() );

	// Fixup bar bounds
	float flStandardProgress = m_pQuest->GetEarnedPoints( QUEST_POINTS_NOVICE ) / (float)( m_PointsBars.m_nMaxPoints );
	m_PointsBars.m_flCurrentProgress = flStandardProgress;

	if ( pItemTracker )
	{
		// We use the item trackers for quest progress since they're the most up-to-date
		flStandardProgress = (float)pItemTracker->GetEarnedPoints( QUEST_POINTS_NOVICE ) / (float)( m_PointsBars.m_nMaxPoints );
	}

	// Standard progress
	bool bChange = flStandardProgress != m_PointsBars.m_flTargetProgress;
	m_PointsBars.m_flTargetProgress = flStandardProgress;

	// We're being set for the first time, instantly be progressed
	if ( m_PointsBars.m_flUpdateTime == 0.f || m_bMapView )
	{
		//m_arPointsBars[ i ].m_flCurrentProgress = m_arPointsBars[ i ].m_flTargetProgress;

		m_PointsBars.m_flUpdateTime = Plat_FloatTime() - ATTRIB_TRACK_GLOW_HOLD_TIME;
		InvalidateLayout();
	}
	else if ( bChange ) // If this is a change, play effects
	{
		m_PointsBars.m_flUpdateTime = Plat_FloatTime();
		InvalidateLayout();
	}

	m_PointsBars.m_pBarJustEarned->SetVisible( !m_bMapView );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestProgressTrackerPanel::UpdateStar( KeyValues* pParams )
{
	int nIndex = pParams->GetInt( "index" );
	if( nIndex < EQuestPoints_ARRAYSIZE )
	{
		if ( !m_bMapView )
			m_arStarImages[ nIndex ]->SetVisible( false );

		if ( m_pQuestDef && m_pQuestDef->GetMaxPoints( nIndex ) == 0 )
		{
			m_arStarImages[ nIndex ]->SetVisible( false );
			return;
		}

		bool bStarEarned = false;
		if ( m_pQuest )
		{
			const CQuestMapNode* pNode = GetQuestMapHelper().GetQuestMapNodeByID( m_pQuest->GetSourceNodeID() );
			bStarEarned = pNode && pNode->BIsMedalEarned( (EQuestPoints)nIndex );
		}

		m_arStarImages[ nIndex ]->SetImage( bStarEarned ? "cyoa/star_on" : "cyoa/star_off" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestProgressTrackerPanel::UpdateStars()
{
	if ( m_bSuppressStarChanges )
		return;

	for( int i=0; i < EQuestPoints_ARRAYSIZE; ++i )
	{
		PostMessage( this, new KeyValues( "UpdateStar", "index", i ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CQuestProgressTrackerPanel::ArePointsCompleted( uint32 nIndex ) const
{
	if ( !m_pQuest || !m_pQuestDef )
		return false;

 	const CQuestMapNode* pNode = GetQuestMapHelper().GetQuestMapNodeByID( m_pQuest->GetSourceNodeID() );
	if ( pNode )
	{
		return pNode->BIsMedalEarned( (EQuestPoints)nIndex );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestProgressTrackerPanel::SetQuest( const CQuest* pQuest )
{
	m_pQuest = pQuest;

	if ( pQuest )
	{
		SetQuestDef( pQuest->GetDefinition() );
		CaptureProgress();
	}
}

void CQuestProgressTrackerPanel::SetQuestDef( const CQuestDefinition* pQuestDef )
{
	m_pQuestDef = pQuestDef;
	m_PointsBars.m_nMaxPoints = m_pQuestDef->GetMaxPoints( QUEST_POINTS_NOVICE );

	if ( m_bTurningIn )
		return;

	if ( !IsLayoutInvalid() )
	{
		UpdateObjectives();
	}
	else
	{
		InvalidateLayout( false, true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CQuestProgressTrackerPanel::IsValidForLocalPlayer() const
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	CSteamID steamID;
	if ( !pLocalPlayer || !pLocalPlayer->GetSteamID( &steamID ) )
		return false;

	Assert( m_pQuest );

	// Safeguard.  There's a crash in public from this.
	if ( !m_pQuest )
		return false;

	const CQuestItemTracker* pItemTracker = QuestObjectiveManager()->GetTypedTracker< CQuestItemTracker* >( m_pQuest->GetID() );
	InvalidReasonsContainer_t invalidReasons;
	if ( pItemTracker )
	{
		int nNumInvalid = pItemTracker->GetNumInactiveObjectives( pLocalPlayer, invalidReasons );
		if ( nNumInvalid < pItemTracker->GetObjectiveTrackers().Count() )
		{
			return true;
		}

		for( int i=0; i < EQuestPoints_ARRAYSIZE; ++i )
		{
			if ( pItemTracker->GetEarnedPoints( i ) > m_pQuest->GetEarnedPoints( EQuestPoints( i ) ) )
			{
				return true;
			}
		}
	}

	const CQuestMapNode* pNode = GetQuestMapHelper().GetQuestMapNodeByID( m_pQuest->GetSourceNodeID() );

	// Check for completed quests.  They are always visible.
	if ( pNode && GetQuestMapHelper().BCanNodeBeTurnedIn( pNode->GetNodeDefinition()->GetDefIndex() ) && GetContractHUDVisibility() == CONTRACT_HUD_SHOW_EVERYTHING )
	{
		return true;
	}

	return false;
}


DECLARE_HUDELEMENT( CHudItemAttributeTracker );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudItemAttributeTracker::CHudItemAttributeTracker( const char *pElementName )
	: CHudElement( pElementName )
	, EditablePanel( NULL, "ItemAttributeTracker" )
	, m_mapTrackers( DefLessFunc( itemid_t ) )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	ListenForGameEvent( "player_spawn" );
	ListenForGameEvent( "inventory_updated" );
	ListenForGameEvent( "client_disconnect" );
	ListenForGameEvent( "localplayer_changeclass" );
	ListenForGameEvent( "quest_objective_completed" );

	RegisterForRenderGroup( "weapon_selection" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemAttributeTracker::ApplySchemeSettings( IScheme *pScheme )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);

	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/HudItemAttributeTracker.res" );

	m_pStatusContainer = FindControl< EditablePanel >( "QuestsStatusContainer" );
	m_pStatusHeaderLabel = m_pStatusContainer->FindControl< Label >( "Header" );
	m_pCallToActionLabel = m_pStatusContainer->FindControl< Label >( "CallToAction" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemAttributeTracker::PerformLayout()
{
	BaseClass::PerformLayout();

	bool bActiveContractIsUnavailable = false;
	bool bSafeToChangeQuests = true;
	bool bNoActiveContract = true;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	const CUtlVector< CQuest* >& vecQuests = GetQuestMapHelper().GetAllQuests();
	for( int i=0; i < vecQuests.Count(); ++i )
	{
		const CQuest* pQuest = vecQuests[ i ];

		if ( pQuest->Obj().active() )
		{
			bNoActiveContract = false;
		}

		if ( pLocalPlayer )
		{
			const CQuestItemTracker* pItemTracker = QuestObjectiveManager()->GetTypedTracker< CQuestItemTracker* >( pQuest->GetID() );
			InvalidReasonsContainer_t invalidReasons;
			if ( pItemTracker )
			{
				if ( pQuest->Obj().active() && pItemTracker->GetNumInactiveObjectives( pLocalPlayer, invalidReasons ) == pItemTracker->GetObjectiveTrackers().Count() )
				{
					bActiveContractIsUnavailable = true;
				}

				for( int nPointsType=0; nPointsType < 3; ++nPointsType )
				{
					if ( pItemTracker->GetEarnedPoints( nPointsType ) != pQuest->GetEarnedPoints( (EQuestPoints)nPointsType ) )
					{
						bSafeToChangeQuests = false;
					}
				}
			}
		}
	}
	
	
	const char* pszHeaderString = NULL;
	const char* pszCallToActionString = NULL;
	auto* pActiveQuest = GetQuestMapHelper().GetActiveQuest();
	const CQuestMapNode* pNode = NULL;
	if ( pActiveQuest )
	{
		pNode = GetQuestMapHelper().GetQuestMapNodeByID( pActiveQuest->GetSourceNodeID() );
	}

	// Most prominent is "You don't have a contract, but you could"
	if ( pActiveQuest == NULL &&
		 GetQuestMapHelper().GetNumCurrentlyUnlockableNodes() > 0 )
	{
		pszHeaderString = "#QuestTracker_NoContract";
		pszCallToActionString = "QuestTracker_New_CallToAction";
	}
	else if ( pNode &&
			  GetQuestMapHelper().BCanNodeBeTurnedIn( pNode->GetNodeDefinition()->GetDefIndex() ) )
	{
		pszHeaderString = "#QuestTracker_ReadyForTurnIn";
		pszCallToActionString = "QuestTracker_New_CallToAction";
	}
	else if ( bSafeToChangeQuests && bActiveContractIsUnavailable )
	{
		pszHeaderString = "#QuestTracker_Inactive_Single";
		pszCallToActionString = "QuestTracker_New_CallToAction";
	}
	

	bool bAnyStatusToShow = pszHeaderString != NULL;

	bool bShowExtras = bAnyStatusToShow && ( GetContractHUDVisibility() != CONTRACT_HUD_SHOW_ACTIVE );
	if ( bShowExtras )
	{
		// Set the header
		const locchar_t *pwszLocalizedHeader = GLocalizationProvider()->Find( pszHeaderString );
		m_pStatusContainer->SetDialogVariable( "header", pwszLocalizedHeader );

		// Build the "Press [ F2 ] to view" string.
		const wchar_t *pszText = NULL;
		if ( pszCallToActionString )
		{
			pszText = g_pVGuiLocalize->Find( pszCallToActionString );
		}
		if ( pszText )
		{					
			wchar_t wzFinal[512] = L"";
			UTIL_ReplaceKeyBindings( pszText, 0, wzFinal, sizeof( wzFinal ), ::input->IsSteamControllerActive() ? GAME_ACTION_SET_FPSCONTROLS : GAME_ACTION_SET_NONE );
			m_pStatusContainer->SetDialogVariable( "call_to_action", wzFinal );
		}
	}
	
	int nY = 0;

	if ( m_pStatusContainer )
	{
		nY = m_pStatusContainer->GetYPos();

		m_pStatusContainer->SetVisible( bShowExtras );
		if ( m_pStatusContainer->IsVisible() )
		{
			nY += m_pStatusContainer->GetTall();
			int nLabelWide, nLabelTall;
			m_pStatusHeaderLabel->GetContentSize( nLabelWide, nLabelTall );
			
			m_pStatusContainer->SetWide( m_nStatusBufferWidth + nLabelWide );
			
			m_pStatusHeaderLabel->SetPos( m_pStatusContainer->GetWide() - m_pStatusHeaderLabel->GetWide(), m_pStatusHeaderLabel->GetYPos() );
			m_pCallToActionLabel->SetPos( m_pStatusContainer->GetWide() - m_pCallToActionLabel->GetWide(), m_pCallToActionLabel->GetYPos() );
			m_pStatusContainer->SetPos( m_pStatusContainer->GetParent()->GetWide() - m_pStatusContainer->GetWide() - YRES( 10 ), m_pStatusContainer->GetYPos() );
		}
	}

	FOR_EACH_MAP( m_mapTrackers, i )
	{
		CQuestProgressTrackerPanel* pPanel = m_mapTrackers[ i ];
		if ( pPanel )
		{
			if ( pPanel->IsValidForLocalPlayer() )
			{
				int nTall = pPanel->GetContentTall();
				pPanel->SetPos( GetWide() - pPanel->GetWide() - YRES( 10 ), nY );
				nY += nTall;
				pPanel->SetVisible( true );
			}
			else
			{
				pPanel->SetVisible( false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemAttributeTracker::OnThink()
{
	BaseClass::OnThink();

	// If the specator GUI is up, we need to drop down a bit so that we're
	// not on top of the death notices
	int nY = g_pSpectatorGUI && g_pSpectatorGUI->IsVisible() ? g_pSpectatorGUI->GetTopBarHeight() : 0;
	int nCurrentX, nCurrentY;
	GetPos( nCurrentX, nCurrentY );
	if ( nCurrentY != nY )
	{
		SetPos( nCurrentX, nY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudItemAttributeTracker::ShouldDraw( void )
{
	return false;

	if ( engine->IsPlayingDemo() )
		return false;

	// Don't draw in freezecam
	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( pPlayer && ( pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM ) )
		return false;

	if ( GetContractHUDVisibility() == CONTRACT_HUD_SHOW_NONE )
		return false;

	if ( GetLocalPlayerTeam() < FIRST_GAME_TEAM )
		return false;

	if ( TFGameRules() )
	{
		if ( TFGameRules()->ShowMatchSummary() )
			return false;

		if ( TFGameRules()->GetRoundRestartTime() > -1.f )
		{
			float flTime = TFGameRules()->GetRoundRestartTime() - gpGlobals->curtime;
			if ( flTime <= 10.f )
				return false;
		}
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemAttributeTracker::FireGameEvent( IGameEvent *pEvent )
{
	if ( FStrEq( pEvent->GetName(), "player_spawn" ) 
	  || FStrEq( pEvent->GetName(), "inventory_updated" ) 
	  || FStrEq( pEvent->GetName(), "client_disconnect" ) 
	  || FStrEq( pEvent->GetName(), "localplayer_changeclass" )
	  || FStrEq( pEvent->GetName(), "quest_objective_completed" ) )
	{
		InvalidateLayout();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemAttributeTracker::HandleSOEvent( const CSteamID & steamIDOwner, const CSharedObject *pObject )
{
	// We only care about quests!
	if( pObject->GetTypeID() != CQuest::k_nTypeID )
		return;

	const CQuest *pQuest = (CQuest *)pObject;

	// Make sure the node is still active too.  Maybe the it's expired now
	const CQuestMapNode* pNode = GetQuestMapHelper().GetQuestMapNodeByID( pQuest->GetSourceNodeID() );
	bool bNodeActive = pNode && pNode->GetNodeDefinition()->BIsActive();

	CQuestProgressTrackerPanel* pTracker = NULL;
	if ( pQuest->Obj().active() && bNodeActive )
	{
		FindTrackerForItem( pQuest, &pTracker, true );
		if ( pTracker )
		{
			pTracker->SetQuest( pQuest );
		}
	}
	else
	{
		FindTrackerForItem( pQuest, &pTracker, false );
		if ( pTracker )
		{
			m_mapTrackers.Remove( pQuest->GetID() );
			pTracker->MarkForDeletion();
		}
	}

	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudItemAttributeTracker::FindTrackerForItem( const CQuest* pItem, CQuestProgressTrackerPanel** ppTracker, bool bCreateIfNotFound )
{
	(*ppTracker) = NULL;
	bool bCreatedNew = false;

	auto idx = m_mapTrackers.Find( pItem->GetID() );
	if ( idx == m_mapTrackers.InvalidIndex() && bCreateIfNotFound )
	{
		(*ppTracker) = new CQuestProgressTrackerPanel( this
											, "ItemTrackerPanel"
											, pItem
											, pItem->GetDefinition()
											, "resource/UI/quests/QuestItemTrackerPanel_InGame_Base.res" );
		(*ppTracker)->InvalidateLayout( true, true );
		m_mapTrackers.Insert( pItem->GetID(), (*ppTracker) );
		bCreatedNew = true;
	}
	else if ( idx != m_mapTrackers.InvalidIndex() )
	{
		(*ppTracker) = m_mapTrackers[ idx ];
	}

	return bCreatedNew;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemAttributeTracker::LevelInit( void )
{
	// We want to listen for our player's SO cache updates
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		GCClientSystem()->GetGCClient()->AddSOCacheListener( steamID, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemAttributeTracker::LevelShutdown( void )
{
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		GCClientSystem()->GetGCClient()->RemoveSOCacheListener( steamID, this );
	}
}


