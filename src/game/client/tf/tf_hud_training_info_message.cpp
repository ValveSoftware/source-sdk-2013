//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/EditablePanel.h>
#include "tf_imagepanel.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"
#include "tf_controls.h"
#include "hud_basechat.h"
#include "c_team_objectiveresource.h"
#include "tf_hud_training.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudTrainingInfoMsg : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudTrainingInfoMsg, EditablePanel );

public:
	CHudTrainingInfoMsg( const char *pElementName );

	virtual void	Init( void );
	virtual void	Reset( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	void			MsgFunc_TrainingInfoMsg( bf_read &msg );
	void			LocalizeAndDisplay( const char *szRawString );

private:
	Label			*m_pGoalLabel;
	//Color			m_cRegularColor;
};

DECLARE_HUDELEMENT( CHudTrainingInfoMsg );
DECLARE_HUD_MESSAGE( CHudTrainingInfoMsg, TrainingInfoMsg );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTrainingInfoMsg::CHudTrainingInfoMsg( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudTrainingInfoMsg" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	RegisterForRenderGroup( "commentary" );
	SetVisible( false );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTrainingInfoMsg::Reset()
{
	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTrainingInfoMsg::Init()
{
	HOOK_HUD_MESSAGE( CHudTrainingInfoMsg, TrainingInfoMsg );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTrainingInfoMsg::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudTrainingInfoMsg.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pGoalLabel = dynamic_cast<Label *>( FindChildByName("GoalLabel") );
	//m_cRegularColor = pScheme->GetColor( "TanLight", Color( 0, 0, 0 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudTrainingInfoMsg::ShouldDraw( void )
{
	return IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: Activates the hint display
//-----------------------------------------------------------------------------
void CHudTrainingInfoMsg::MsgFunc_TrainingInfoMsg( bf_read &msg )
{
	// Read the string(s)
	char szString[255];
	msg.ReadString( szString, sizeof(szString) );

	if ( !szString[0] )
	{
		SetVisible( false );
		return;
	}


	//Localize and display the string.
	wchar_t outputText[MAX_TRAINING_MSG_LENGTH];
	CTFHudTraining::FormatTrainingText(szString , outputText);

	SetVisible( true );
	m_pGoalLabel->SetText( outputText );

	//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageShow" ); 
	//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageHide" ); 

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		pLocalPlayer->EmitSound( "Hud.Hint" );
	}


}