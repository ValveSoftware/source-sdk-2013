//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tf_survey_questions.h"
#include "ienginevgui.h"
#include "tf_match_description.h"

#ifdef CLIENT_DLL
	#include "tf_gc_client.h"
	#include "vgui_controls/RadioButton.h"
	#include "iclientmode.h"
	#include <vgui_controls/AnimationController.h>
	#include "game/client/iviewport.h"
	#include "tf_hud_mainmenuoverride.h"
	#include "tf_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Criteria function for competitive inquiry survey 
//			Only include casual matches 
//-----------------------------------------------------------------------------
bool CompetitiveInquiry( const CMsgGC_Match_Result& msgMatchResult, uint32 nPlayerIndex )
{
	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( (ETFMatchGroup) msgMatchResult.match_group() );
	
	if ( pMatchDesc )
	{
		// Only show this in Casual 12v12
		return pMatchDesc->m_eMatchGroup == k_eTFMatchGroup_Casual_12v12;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Criteria function for casual inquiry survey 
//			Only include competitive matches 
//-----------------------------------------------------------------------------
bool CasualInquiry( const CMsgGC_Match_Result& msgMatchResult, uint32 nPlayerIndex )
{
	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( ( ETFMatchGroup )msgMatchResult.match_group() );

	if ( pMatchDesc )
	{
		// Only show this in Competitive 6v6
		return pMatchDesc->m_eMatchGroup == k_eTFMatchGroup_Ladder_6v6;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: The actual definition of the survey questions
//-----------------------------------------------------------------------------
																		//	  Survey Enum				Help Text String	   Weight	Criteria Function		Active
const SurveyQuestion_t g_SurveyQuestions[SurveyQuestionType_ARRAYSIZE] =  { { QUESTION_MATCH_QUALITY,	"MatchQuality",			1.f,	NULL,					true },
																			{ QUESTION_MAP_QUALITY,		"MapQuality",			1.f,	NULL,					true },
																			{ QUESTION_COMP_INQUIRY,	"CompetitiveInquiry",	1.f,	&CompetitiveInquiry,	true },
																			{ QUESTION_CASUAL_INQUIRY,	"CasualInquiry",		1.f,	&CasualInquiry,			true }, 
																			{ QUESTION_RANDOM_CRIT,		"RandomCritInquiry",	1.f,	NULL,					true } };

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Figure out which survey panel to make
//-----------------------------------------------------------------------------
CSurveyQuestionPanel* CreateSurveyQuestionPanel( Panel* pParent, const CMsgGCSurveyRequest& msgSurveyQuestion )
{
	CSurveyQuestionPanel* pSurveyPanel = NULL;

	Assert( msgSurveyQuestion.has_question_type() );
	switch( msgSurveyQuestion.question_type() )
	{
	case QUESTION_MATCH_QUALITY:
		pSurveyPanel = new CMatchQualitySurvey( pParent, msgSurveyQuestion );
		break;
	case QUESTION_MAP_QUALITY:
		pSurveyPanel = new CMapQualitySurvey( pParent, msgSurveyQuestion );
		break;
	case QUESTION_COMP_INQUIRY:
		pSurveyPanel = new CCompInquirySurvey( pParent, msgSurveyQuestion );
		break;
	case QUESTION_CASUAL_INQUIRY:
		pSurveyPanel = new CCasualInquirySurvey( pParent, msgSurveyQuestion );
		break;
	case QUESTION_RANDOM_CRIT:
		pSurveyPanel = new CRandomCritSurvey( pParent, msgSurveyQuestion );
		break;
	default:
		Assert( !"Unhandled survey question type!" );
	}

	return pSurveyPanel;
}

CSurveyQuestionPanel::CSurveyQuestionPanel( Panel* pParent, CMsgGCSurveyRequest msgSurveyQuestion )
	: EditablePanel( pParent, "Survey" )
	, m_msgRequest( msgSurveyQuestion )
	, m_bResponded( false )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	ListenForGameEvent( "server_spawn" );
	ListenForGameEvent( "client_disconnect" );
	ListenForGameEvent( "client_beginconnect" );
}

CSurveyQuestionPanel::~CSurveyQuestionPanel()
{
	if ( !m_bResponded )
	{
		GTFGCClientSystem()->SendSurveyResponse( SEEN_BUT_UNANSWERED_SURVEY_QUESTION );
		m_bResponded = true;
	}

	GTFGCClientSystem()->ClearSurveyRequest();
}

void CSurveyQuestionPanel::OnCommand( const char *command )
{
	if ( FStrEq( command, "close" ) ) 
	{
		if ( !m_bResponded )
		{
			GTFGCClientSystem()->SendSurveyResponse( SEEN_AND_DISMISSED_SURVEY_QUESTION );
			m_bResponded = true;
		}

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( GetParent(), "SurveyHideSequence", false );
		return;
	}
	else if ( FStrEq( command, "submit" ) )
	{
		if ( !m_bResponded )
		{
			Submit();
			m_bResponded = true;
		}
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( GetParent(), "SurveySubmitSequence", false );

		return;
	}
	else if ( FStrEq( command, "delete" ) ) 
	{
		MarkForDeletion();
		return;
	}

	BaseClass::OnCommand( command );
}

void CSurveyQuestionPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetProportional( true );
	LoadControlSettings( GetResFile() );
	InvalidateLayout( true );
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( GetParent(), "SurveyShowSequence", false );
}

void CSurveyQuestionPanel::FireGameEvent( IGameEvent *event ) 
{
	const char *pEventName = event->GetName();
	
	// They left/changed servers.  Consider the survey abandoned
	if ( !Q_stricmp( pEventName, "client_disconnect" ) ||
		 !Q_stricmp( pEventName, "client_beginconnect" ) ||
		 !Q_stricmp( pEventName, "server_spawn" ) )
	{
		MarkForDeletion();
	}
}

CMultipleChoiceSurveyQuestionPanel::CMultipleChoiceSurveyQuestionPanel( Panel* pParent, CMsgGCSurveyRequest msgSurveyQuestion, uint16 nSurveyResponses )
	: CSurveyQuestionPanel( pParent, msgSurveyQuestion )
{
	m_nSurveyResponses = nSurveyResponses;
}

//-----------------------------------------------------------------------------
// Purpose: Get the labels under the radio buttons to highlight
//-----------------------------------------------------------------------------
void CMultipleChoiceSurveyQuestionPanel::Think()
{
	// We need to be on top of absolutely everything.  Clicking on another
	// popup will move it on top of us, and that cannot be!
	MoveToFront();

	bool bAnySelected = false;
	// Radio buttons aren't cool and can ONLY have their labels to the right.
	// This panel has extra labels under the radio labels that we want to highlight
	// so set the label's FG colors to the radio buttons FG colors.
	for( int i = 0; i < m_nSurveyResponses; ++i )
	{
		RadioButton* pRadioButton = FindControl< RadioButton >( CFmtStr( "Radio%d", i ), true );
		Panel* pRadioLabel = FindChildByName( CFmtStr( "Radio%dLabel", i ), true );

		if ( pRadioButton && pRadioLabel )
		{
			pRadioLabel->SetFgColor( pRadioButton->GetFgColor() );
			bAnySelected = bAnySelected || pRadioButton->IsSelected();
		}
	}

	Panel* pSubmitButton = FindChildByName( "SubmitButton", true );
	if ( pSubmitButton )
	{
		pSubmitButton->SetEnabled( bAnySelected );
	}
}

void CMultipleChoiceSurveyQuestionPanel::Submit()
{
	// Figure out which radio button is checked, and use that as our response
	for( int i = 0; i < m_nSurveyResponses; ++i )
	{
		RadioButton* pRadioButton = FindControl< RadioButton >( CFmtStr( "Radio%d", i ), true );

		if ( pRadioButton && pRadioButton->IsSelected() )
		{
			GTFGCClientSystem()->SendSurveyResponse( i );
			return;
		}
	}
}

void CMapQualitySurvey::PerformLayout()
{
	const MapDef_t* pMapDef = NULL;

	if ( TFGameRules() )
	{
		pMapDef = GetItemSchema()->GetMasterMapDefByName ( TFGameRules()->MapName() );
	}
	else
	{
		pMapDef = GetItemSchema()->GetMasterMapDefByIndex( RandomInt( 1, GetItemSchema()->GetMapCount() - 1 ) );
	}

	Assert( pMapDef );
	if ( !pMapDef )
		return;

	EditablePanel* pMapChoice = FindControl< EditablePanel >( "QuestionContainer", true );
	if ( pMapChoice )
	{
		// Label text
		pMapChoice->SetDialogVariable( "mapname", g_pVGuiLocalize->Find( pMapDef->pszMapNameLocKey ) );
	}
}


#endif
