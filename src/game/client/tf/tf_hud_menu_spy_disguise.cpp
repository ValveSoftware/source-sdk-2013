//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "c_baseobject.h"
#include "inputsystem/iinputsystem.h"

#ifdef SIXENSE
#include "sixense/in_sixense.h"
#endif

#include "tf_hud_menu_spy_disguise.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar tf_simple_disguise_menu( "tf_simple_disguise_menu", NULL, FCVAR_ARCHIVE, "Use a more concise disguise selection menu." );

//======================================

DECLARE_HUDELEMENT( CHudMenuSpyDisguise );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMenuSpyDisguise::CHudMenuSpyDisguise( const char *pElementName ) 
	: CHudBaseBuildMenu( pElementName, "HudMenuSpyDisguise" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	for ( int i=0; i<9; ++i )
	{
		char buf[32];
		Q_snprintf( buf, sizeof(buf), "class_item_red_%d", i+1 );
		m_pClassItems_Red[i] = new EditablePanel( this, buf );

		m_pKeyIcons_Red[i] = new CIconPanel( m_pClassItems_Red[i], "NumberBg" );
		m_pKeyLabels_Red[i] = new CExLabel( m_pClassItems_Red[i], "NumberLabel", "" );
		m_pKeyLabelsNew_Red[i] = new CExLabel( m_pClassItems_Red[i], "NewNumberLabel", "" );

		Q_snprintf( buf, sizeof(buf), "class_item_blue_%d", i+1 );
		m_pClassItems_Blue[i] = new EditablePanel( this, buf );

		m_pKeyIcons_Blue[i] = new CIconPanel( m_pClassItems_Blue[i], "NumberBg" );
		m_pKeyLabels_Blue[i] = new CExLabel( m_pClassItems_Blue[i], "NumberLabel", "" );
		m_pKeyLabelsNew_Blue[i] = new CExLabel( m_pClassItems_Blue[i], "NewNumberLabel", "" );
	}

	m_pKeyIcons_Category[0] = new CIconPanel( this, "NumberBg1" );
	m_pKeyIcons_Category[1] = new CIconPanel( this, "NumberBg2" );
	m_pKeyIcons_Category[2] = new CIconPanel( this, "NumberBg3" );
	m_pKeyLabels_Category[0] = new CExLabel( this, "NumberLabel1", "" );
	m_pKeyLabels_Category[1] = new CExLabel( this, "NumberLabel2", "" );
	m_pKeyLabels_Category[2] = new CExLabel( this, "NumberLabel3", "" );

	m_iShowingTeam = TF_TEAM_RED;

	ListenForGameEvent( "spy_pda_reset" );
	ListenForGameEvent( "gameui_hidden" );

	m_iSelectedItem = -1;
	m_iGroupSelection = -1;

	m_pActiveSelection = NULL;

	InvalidateLayout( false, true );

	m_bInConsoleMode = false;

	RegisterForRenderGroup( "mid" );
}

ConVar tf_disguise_menu_controller_mode( "tf_disguise_menu_controller_mode", "0", FCVAR_ARCHIVE, "Use console controller disguise menus. 1 = ON, 0 = OFF." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::ApplySchemeSettings( IScheme *pScheme )
{
	bool bSteamController = ::input->IsSteamControllerActive();
	bool b360Style = ( bSteamController || IsConsole() || tf_disguise_menu_controller_mode.GetBool() );

	if ( b360Style )
	{
		// load control settings...
		auto res_dir = bSteamController ? "resource/UI/disguise_menu_sc" : "resource/UI/disguise_menu_360";
		LoadControlSettings( VarArgs("%s/HudMenuSpyDisguise.res", res_dir ) );

		m_pClassItems_Red[0]->LoadControlSettings( VarArgs( "%s/scout_red.res", res_dir ) );
		m_pClassItems_Red[1]->LoadControlSettings( VarArgs( "%s/soldier_red.res", res_dir ) );
		m_pClassItems_Red[2]->LoadControlSettings( VarArgs( "%s/pyro_red.res", res_dir ) );
		m_pClassItems_Red[3]->LoadControlSettings( VarArgs( "%s/demoman_red.res", res_dir ) );
		m_pClassItems_Red[4]->LoadControlSettings( VarArgs( "%s/heavy_red.res", res_dir ) );
		m_pClassItems_Red[5]->LoadControlSettings( VarArgs( "%s/engineer_red.res", res_dir ) );
		m_pClassItems_Red[6]->LoadControlSettings( VarArgs( "%s/medic_red.res", res_dir ) );
		m_pClassItems_Red[7]->LoadControlSettings( VarArgs( "%s/sniper_red.res", res_dir ) );
		m_pClassItems_Red[8]->LoadControlSettings( VarArgs( "%s/spy_red.res", res_dir ) );

		m_pClassItems_Blue[0]->LoadControlSettings( VarArgs( "%s/scout_blue.res", res_dir ) );
		m_pClassItems_Blue[1]->LoadControlSettings( VarArgs( "%s/soldier_blue.res", res_dir ) );
		m_pClassItems_Blue[2]->LoadControlSettings( VarArgs( "%s/pyro_blue.res", res_dir ) );
		m_pClassItems_Blue[3]->LoadControlSettings( VarArgs( "%s/demoman_blue.res", res_dir ) );
		m_pClassItems_Blue[4]->LoadControlSettings( VarArgs( "%s/heavy_blue.res", res_dir ) );
		m_pClassItems_Blue[5]->LoadControlSettings( VarArgs( "%s/engineer_blue.res", res_dir ) );
		m_pClassItems_Blue[6]->LoadControlSettings( VarArgs( "%s/medic_blue.res", res_dir ) );
		m_pClassItems_Blue[7]->LoadControlSettings( VarArgs( "%s/sniper_blue.res", res_dir ) );
		m_pClassItems_Blue[8]->LoadControlSettings( VarArgs( "%s/spy_blue.res", res_dir ) );

		m_pActiveSelection = dynamic_cast< EditablePanel * >( FindChildByName( "active_selection_bg" ) );

		// Reposition the activeselection to the default position
		m_iSelectedItem = -1;	// force reposition
		SetSelectedItem( 5 );
	}
	else
	{
		// load control settings...
		LoadControlSettings( "resource/UI/disguise_menu/HudMenuSpyDisguise.res" );

		m_pClassItems_Red[0]->LoadControlSettings( "resource/UI/disguise_menu/scout_red.res" );
		m_pClassItems_Red[1]->LoadControlSettings( "resource/UI/disguise_menu/soldier_red.res" );
		m_pClassItems_Red[2]->LoadControlSettings( "resource/UI/disguise_menu/pyro_red.res" );
		m_pClassItems_Red[3]->LoadControlSettings( "resource/UI/disguise_menu/demoman_red.res" );
		m_pClassItems_Red[4]->LoadControlSettings( "resource/UI/disguise_menu/heavy_red.res" );
		m_pClassItems_Red[5]->LoadControlSettings( "resource/UI/disguise_menu/engineer_red.res" );
		m_pClassItems_Red[6]->LoadControlSettings( "resource/UI/disguise_menu/medic_red.res" );
		m_pClassItems_Red[7]->LoadControlSettings( "resource/UI/disguise_menu/sniper_red.res" );
		m_pClassItems_Red[8]->LoadControlSettings( "resource/UI/disguise_menu/spy_red.res" );

		m_pClassItems_Blue[0]->LoadControlSettings( "resource/UI/disguise_menu/scout_blue.res" );
		m_pClassItems_Blue[1]->LoadControlSettings( "resource/UI/disguise_menu/soldier_blue.res" );
		m_pClassItems_Blue[2]->LoadControlSettings( "resource/UI/disguise_menu/pyro_blue.res" );
		m_pClassItems_Blue[3]->LoadControlSettings( "resource/UI/disguise_menu/demoman_blue.res" );
		m_pClassItems_Blue[4]->LoadControlSettings( "resource/UI/disguise_menu/heavy_blue.res" );
		m_pClassItems_Blue[5]->LoadControlSettings( "resource/UI/disguise_menu/engineer_blue.res" );
		m_pClassItems_Blue[6]->LoadControlSettings( "resource/UI/disguise_menu/medic_blue.res" );
		m_pClassItems_Blue[7]->LoadControlSettings( "resource/UI/disguise_menu/sniper_blue.res" );
		m_pClassItems_Blue[8]->LoadControlSettings( "resource/UI/disguise_menu/spy_blue.res" );

		m_pActiveSelection = NULL;
	}

	m_iGroupSelection = -1;
	ToggleSelectionIcons( false );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMenuSpyDisguise::ShouldDraw( void )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	return CHudBaseBuildMenu::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: Keyboard input hook. Return 0 if handled
//-----------------------------------------------------------------------------
int	CHudMenuSpyDisguise::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !ShouldDraw() )
	{
		return 1;
	}

	if ( !down )
	{
		return 1;
	}

	// menu classes are not in the same order as the defines
	static int iRemapKeyToClass[9] = 
	{
		TF_CLASS_SCOUT,
		TF_CLASS_SOLDIER,
		TF_CLASS_PYRO,
		TF_CLASS_DEMOMAN,
		TF_CLASS_HEAVYWEAPONS,
		TF_CLASS_ENGINEER,
		TF_CLASS_MEDIC,
		TF_CLASS_SNIPER,
		TF_CLASS_SPY
	};

	bool bController = ( IsConsole() || ( keynum >= JOYSTICK_FIRST ) );

	if ( bController )
	{
		int iNewSelection = m_iSelectedItem;

		switch( keynum )
		{
		case KEY_XBUTTON_UP:
		case STEAMCONTROLLER_DPAD_UP:
			// jump to last
			iNewSelection = 9;
			break;

		case KEY_XBUTTON_DOWN:
		case STEAMCONTROLLER_DPAD_DOWN:
			// jump to first
			iNewSelection = 1;
			break;

		case KEY_XBUTTON_RIGHT:
		case STEAMCONTROLLER_DPAD_RIGHT:
			// move selection to the right
			iNewSelection++;
			if ( iNewSelection > 9 )
				iNewSelection = 1;
			break;

		case KEY_XBUTTON_LEFT:
		case STEAMCONTROLLER_DPAD_LEFT:
			// move selection to the right
			iNewSelection--;
			if ( iNewSelection < 1 )
				iNewSelection = 9;
			break;

		case KEY_XBUTTON_RTRIGGER:
		case KEY_XBUTTON_A:
		case STEAMCONTROLLER_A:
			{
				// select disguise
				int iClass = iRemapKeyToClass[m_iSelectedItem-1];
				int iTeam = ( m_iShowingTeam == TF_TEAM_BLUE ) ? 1 : 0;

				SelectDisguise( iClass, iTeam );
			}
			return 0;

		case KEY_XBUTTON_Y:
		case STEAMCONTROLLER_Y:
			ToggleDisguiseTeam();
			return 0;

		case KEY_XBUTTON_B:
		case STEAMCONTROLLER_B:
			// cancel, close the menu
			engine->ExecuteClientCmd( "lastinv" );
			return 0;

		default:
			return 1;	// key not handled
		}

		SetSelectedItem( iNewSelection );

		return 0;
	}
	else
	{
		int iSlot = -1;

#ifdef SIXENSE
		if ( !tf_simple_disguise_menu.GetBool() && !g_pSixenseInput->IsEnabled() )
#else
		if ( !tf_simple_disguise_menu.GetBool() )
#endif
		{
			// convert slot1, slot2 etc to 1,2,3,4
			if ( pszCurrentBinding && !Q_strncmp( pszCurrentBinding, "slot", 4 ) && Q_strlen(pszCurrentBinding) > 4 )
			{
				const char *pszNum = pszCurrentBinding+4;
				iSlot = atoi(pszNum);

				// slot10 cancels
				if ( iSlot == 10 )
				{
					engine->ExecuteClientCmd( "lastinv" );
					return 0;
				}

				iSlot -= 1;	// adjust to be 0 based

				// allow slot1 - slot4 
				if ( iSlot < 0 || iSlot > 8 )
					return 1;
			}
		}

		if ( pszCurrentBinding && ( FStrEq( pszCurrentBinding, "disguiseteam" ) || FStrEq( pszCurrentBinding, "+reload" ) ) )
		{
			ToggleDisguiseTeam();
			return 0;
		}
		else if ( pszCurrentBinding && FStrEq( pszCurrentBinding, "next_disguise" ) )
		{
			int iNewSelection = m_iSelectedItem;

			iNewSelection++;
			if ( iNewSelection > 9 )
				iNewSelection = 1;

			SetSelectedItem( iNewSelection );
			return 0;
		}
		else if ( pszCurrentBinding && FStrEq( pszCurrentBinding, "prev_disguise" ) )
		{
			int iNewSelection = m_iSelectedItem;

			iNewSelection--;
			if ( iNewSelection < 1 )
				iNewSelection = 9;

			SetSelectedItem( iNewSelection );
			return 0;
		}
		else if ( iSlot == -1 )
		{
			if ( m_iGroupSelection > -1 )
			{
				switch( keynum )
				{
				case KEY_1:
				case KEY_2:
				case KEY_3:
					{
						iSlot = m_iGroupSelection*3 + keynum - KEY_1;
					}
					break;
				case KEY_4:
				case KEY_5:
				case KEY_6:
				case KEY_7:
				case KEY_8:
				case KEY_9:
					{
#ifdef SIXENSE
						if ( !tf_simple_disguise_menu.GetBool() && !g_pSixenseInput->IsEnabled() )
#else
						if ( !tf_simple_disguise_menu.GetBool() )
#endif
						{
							iSlot = keynum - KEY_1;
						}
					}
					break;

				case KEY_0:
					engine->ExecuteClientCmd( "lastinv" );
					return 0;
				}
			}
			else
			{
#ifdef SIXENSE
				if ( !tf_simple_disguise_menu.GetBool() && !g_pSixenseInput->IsEnabled() )
#else
				if ( !tf_simple_disguise_menu.GetBool() )
#endif
				{
					switch( keynum )
					{
					case KEY_1:
					case KEY_2:
					case KEY_3:
					case KEY_4:
					case KEY_5:
					case KEY_6:
					case KEY_7:
					case KEY_8:
					case KEY_9:
						{
							iSlot = keynum - KEY_1;
						}
						break;

					case KEY_0:
						// cancel, close the menu
						engine->ExecuteClientCmd( "lastinv" );
						return 0;

					default:
						return 1;	// key not handled
					}
				}
				else
				{
					switch( keynum )
					{
					case KEY_1:
					case KEY_2:
					case KEY_3:
						{
							m_iGroupSelection = keynum - KEY_1;
							ToggleSelectionIcons( true );
							return 0;
						}
					}					
				}
			}
		}

		if ( iSlot >= 0 )
		{
			int iClass = iRemapKeyToClass[ iSlot ];
			int iTeam = ( m_iShowingTeam == TF_TEAM_BLUE ) ? 1 : 0;

			SelectDisguise( iClass, iTeam );

			m_iGroupSelection = -1;
			ToggleSelectionIcons( false );

			return 0;
		}
	}
	
	return 1;	// key not handled
}


void IN_Impulse( const CCommand &args );
void SelectDisguise( int iClass, int iTeam )
{
	CHudMenuSpyDisguise *pMenuSpyDisguise = ( CHudMenuSpyDisguise * )GET_HUDELEMENT( CHudMenuSpyDisguise );

	if ( pMenuSpyDisguise )
	{
		pMenuSpyDisguise->SelectDisguise( iClass, iTeam );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::FindToggleBinding( void )
{
	// set the %lastinv% dialog var to our binding
	const char *key = engine->Key_LookupBinding( "lastinv" );
	if ( !key )
	{
		key = "< not bound >";
	}
	SetDialogVariable( "lastinv", key );

	key = engine->Key_LookupBinding( "disguiseteam" );
	if ( !key )
	{
		key = "< not bound >";
	}
	SetDialogVariable( "disguiseteam", key );

	key = engine->Key_LookupBinding( "reload" );
	if ( !key )
	{
		key = "< Reload >";
	}
	SetDialogVariable( "reload", key );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::ToggleSelectionIcons( bool bGroup )
{
	// in controller mode we don't want any of the key icons
	if( tf_disguise_menu_controller_mode.GetBool() || ::input->IsSteamControllerActive() )
	{
		for ( int i=0; i<9; ++i )
		{
			m_pKeyIcons_Red[i]->SetVisible( false );
			m_pKeyLabels_Red[i]->SetVisible( false );
			m_pKeyLabelsNew_Red[i]->SetVisible( false );
			m_pKeyIcons_Blue[i]->SetVisible( false );
			m_pKeyLabels_Blue[i]->SetVisible( false );
			m_pKeyLabelsNew_Blue[i]->SetVisible( false );
		}
	}
	else
#ifdef SIXENSE
	if ( tf_simple_disguise_menu.GetBool() || g_pSixenseInput->IsEnabled() )
#else
	if ( tf_simple_disguise_menu.GetBool() )
#endif
	{
		for ( int i=0; i<3; ++i )
		{
			m_pKeyIcons_Category[i]->SetVisible( !bGroup );
			m_pKeyLabels_Category[i]->SetVisible( !bGroup );
		}
		for ( int i=0; i<9; ++i )
		{
			int index = i-(m_iGroupSelection*3);
			bool bVisible = bGroup && ((m_iGroupSelection == -1) || ((index<=2)&&(index>=0)));
			m_pKeyIcons_Red[i]->SetVisible( bVisible );
			m_pKeyLabels_Red[i]->SetVisible( false );
			m_pKeyLabelsNew_Red[i]->SetVisible( bVisible );
			m_pKeyIcons_Blue[i]->SetVisible( bVisible );
			m_pKeyLabels_Blue[i]->SetVisible( false );
			m_pKeyLabelsNew_Blue[i]->SetVisible( bVisible );
		}
	}
	else
	{
		for ( int i=0; i<3; ++i )
		{
			m_pKeyIcons_Category[i]->SetVisible( false );
			m_pKeyLabels_Category[i]->SetVisible( false );
		}
		for ( int i=0; i<9; ++i )
		{
			m_pKeyIcons_Red[i]->SetVisible( true );
			m_pKeyLabels_Red[i]->SetVisible( true );
			m_pKeyLabelsNew_Red[i]->SetVisible( false );
			m_pKeyIcons_Blue[i]->SetVisible( true );
			m_pKeyLabels_Blue[i]->SetVisible( true );
			m_pKeyLabelsNew_Blue[i]->SetVisible( false );
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::SelectDisguise( int iClass, int iTeam )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		// intercepting the team value and reassigning what gets passed into Disguise()
		// because the team numbers in the client menu don't match the #define values for the teams
		char szDisguise[32];

		if ( iTeam <= LAST_SHARED_TEAM )
		{
			iTeam = ( iTeam == 1 ) ? TF_TEAM_BLUE : TF_TEAM_RED;
		}

		Q_snprintf( szDisguise, sizeof (szDisguise), "disguise 2%d%d", iTeam, iClass );

		CCommand args;
		args.Tokenize( szDisguise );

		IN_Impulse( args );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::ToggleDisguiseTeam( void )
{
	// flip the teams
	m_iShowingTeam = ( m_iShowingTeam == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;

	// show / hide the class items
	bool bShowBlue = ( m_iShowingTeam == TF_TEAM_BLUE );

	for ( int i=0; i<9; i++ )
	{
		m_pClassItems_Red[i]->SetVisible( !bShowBlue );
		m_pClassItems_Blue[i]->SetVisible( bShowBlue );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::SetSelectedItem( int iSlot )
{
	if ( m_iSelectedItem != iSlot )
	{
		m_iSelectedItem = iSlot;

		// move the selection item to the new position
		if ( m_pActiveSelection )
		{
			// move the selection background
			int x, y;
			m_pClassItems_Blue[m_iSelectedItem-1]->GetPos( x, y );
			m_pActiveSelection->SetPos( x, y );	
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "spy_pda_reset") == 0 )
	{
		CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer )
		{
			bool bShowBlue = ( pPlayer->GetTeamNumber() == TF_TEAM_RED );

			for ( int i=0; i<9; i++ )
			{
				m_pClassItems_Red[i]->SetVisible( !bShowBlue );
				m_pClassItems_Blue[i]->SetVisible( bShowBlue );
			}

			m_iShowingTeam = ( bShowBlue ) ? TF_TEAM_BLUE : TF_TEAM_RED;

			m_iGroupSelection = -1;
			ToggleSelectionIcons( false );
		}
	}
	else if ( Q_strcmp(type, "gameui_hidden") == 0 )
	{
		FindToggleBinding();
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::SetVisible( bool state )
{
	if ( state == true )
	{
		// close the weapon selection menu
		engine->ClientCmd( "cancelselect" );

		bool bConsoleMode = ( IsConsole() || tf_disguise_menu_controller_mode.GetBool() || ::input->IsSteamControllerActive() );
			
		if ( bConsoleMode != m_bInConsoleMode )
		{
			InvalidateLayout( true, true );
			m_bInConsoleMode = bConsoleMode;
		}

		FindToggleBinding();

		HideLowerPriorityHudElementsInGroup( "mid" );

		m_iGroupSelection = -1;
		ToggleSelectionIcons( false );
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( state );
}