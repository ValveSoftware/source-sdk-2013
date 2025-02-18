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
#include "tf_hud_training.h"
#include "tf_hud_objectivestatus.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudTrainingMsg : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudTrainingMsg, EditablePanel );

public:
	CHudTrainingMsg( const char *pElementName );

	virtual void	Init( void );
	virtual bool	ShouldDraw( void );

	void			MsgFunc_TrainingMsg( bf_read &msg );
	void			MsgFunc_TrainingObjective( bf_read &msg );

private:
};

DECLARE_HUDELEMENT( CHudTrainingMsg );
DECLARE_HUD_MESSAGE( CHudTrainingMsg, TrainingMsg );
DECLARE_HUD_MESSAGE( CHudTrainingMsg, TrainingObjective );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTrainingMsg::CHudTrainingMsg( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudTrainingMsg" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTrainingMsg::Init()
{
	HOOK_HUD_MESSAGE( CHudTrainingMsg, TrainingMsg );
	HOOK_HUD_MESSAGE( CHudTrainingMsg, TrainingObjective );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudTrainingMsg::ShouldDraw( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Activates the hint display
//-----------------------------------------------------------------------------
void CHudTrainingMsg::MsgFunc_TrainingMsg( bf_read &msg )
{
	if ( engine->IsPlayingDemo() )
		return;

	char szString[MAX_TRAINING_MSG_LENGTH];
	msg.ReadString( szString, MAX_TRAINING_MSG_LENGTH );

	CTFHudObjectiveStatus *pStatus = GET_HUDELEMENT( CTFHudObjectiveStatus );
	if ( pStatus )
	{
		pStatus->SetTrainingText(szString);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Activates the hint display
//-----------------------------------------------------------------------------
void CHudTrainingMsg::MsgFunc_TrainingObjective( bf_read &msg )
{
	if ( engine->IsPlayingDemo() )
		return;

	char szString[MAX_TRAINING_MSG_LENGTH];
	msg.ReadString( szString, MAX_TRAINING_MSG_LENGTH );

	CTFHudObjectiveStatus *pStatus = GET_HUDELEMENT( CTFHudObjectiveStatus );
	if ( pStatus )
	{
		pStatus->SetTrainingObjective(szString);
	}
}

