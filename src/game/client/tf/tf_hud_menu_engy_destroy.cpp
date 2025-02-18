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
#include "tf_gamerules.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "c_baseobject.h"

#include "tf_hud_menu_engy_destroy.h"
#include "tf_hud_menu_engy_build.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern const EngyConstructBuilding_t g_kEngyBuildings[];

//======================================

DECLARE_BUILD_FACTORY( CEngyDestroyMenuItem );


//======================================

DECLARE_HUDELEMENT_DEPTH( CHudMenuEngyDestroy, 40 );	// in front of engy building status

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMenuEngyDestroy::CHudMenuEngyDestroy( const char *pElementName ) 
	: CHudBaseBuildMenu( pElementName, "HudMenuEngyDestroy" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_iCurrentDestroyMenuLayout = DESTROYMENU_DEFAULT;

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	for ( int i=0; i<NUM_ENGY_BUILDINGS; i++ )
	{
		char buf[32];

		Q_snprintf( buf, sizeof(buf), "active_item_%d", i+1 );
		m_pActiveItems[i] = new CEngyDestroyMenuItem( this, buf );

		Q_snprintf( buf, sizeof(buf), "inactive_item_%d", i+1 );
		m_pInactiveItems[i] = new CEngyDestroyMenuItem( this, buf );

		Q_snprintf( buf, sizeof(buf), "unavailable_item_%d", i+1 );
		m_pUnavailableItems[i] = new CEngyDestroyMenuItem( this, buf );
	}

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	RegisterForRenderGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: called whenever a new level is starting
//-----------------------------------------------------------------------------
void CHudMenuEngyDestroy::LevelInit( void )
{
	//RecalculateBuildingItemState( ALL_BUILDINGS );

	CHudElement::LevelInit();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuEngyDestroy::ApplySchemeSettings( IScheme *pScheme )
{
	const char *pszCustomDir = NULL;
	switch( m_iCurrentDestroyMenuLayout )
	{
	case DESTROYMENU_PIPBOY:
		pszCustomDir = "resource/UI/destroy_menu/pipboy";
		break;

	default:
	case DESTROYMENU_DEFAULT:
		pszCustomDir = "resource/UI/destroy_menu";
		break;
	}

	// load control settings...
	LoadControlSettings( VarArgs("%s/HudMenuEngyDestroy.res",pszCustomDir) );

	for ( int i=0; i<NUM_ENGY_BUILDINGS; ++i )
	{
		CExLabel *pNumberLabel = NULL;
		if ( m_Buildings[i].m_pszDestroyActiveObjectRes )
		{
			m_pActiveItems[i]->LoadControlSettings( VarArgs( "%s/%s", pszCustomDir, m_Buildings[i].m_pszDestroyActiveObjectRes ) );
		}

		if ( m_Buildings[i].m_pszDestroyInactiveObjectRes )
		{
			m_pInactiveItems[i]->LoadControlSettings( VarArgs( "%s/%s", pszCustomDir, m_Buildings[i].m_pszDestroyInactiveObjectRes ) );
		}

		if ( m_Buildings[i].m_pszDestroyUnavailableObjectRes )
		{
			m_pUnavailableItems[i]->LoadControlSettings( VarArgs( "%s/%s", pszCustomDir, m_Buildings[i].m_pszDestroyUnavailableObjectRes ) );
		}

		// Set the Numerical Number
		pNumberLabel = dynamic_cast<CExLabel *>( m_pActiveItems[i]->FindChildByName( "NumberLabel" ) );
		if ( pNumberLabel )
		{
			pNumberLabel->SetText( VarArgs( "%d", i + 1 ) );
		}
		pNumberLabel = dynamic_cast<CExLabel *>( m_pInactiveItems[i]->FindChildByName( "NumberLabel" ) );
		if ( pNumberLabel )
		{
			pNumberLabel->SetText( VarArgs( "%d", i + 1 ) );
		}
		pNumberLabel = dynamic_cast<CExLabel *>( m_pUnavailableItems[i]->FindChildByName( "NumberLabel" ) );
		if ( pNumberLabel )
		{
			pNumberLabel->SetText( VarArgs( "%d", i + 1 ) );
		}
	}

	for ( int i = 0; i < NUM_ENGY_BUILDINGS; ++i )
	{
		vgui::Panel* pNotBuiltLabel = m_pUnavailableItems[i]->FindChildByName( "NotBuiltLabel" );
		vgui::Panel* pUnavailableLabel = m_pUnavailableItems[i]->FindChildByName( "UnavailableLabel" );
		if ( pNotBuiltLabel )
		{
			pNotBuiltLabel->SetVisible( false );
		}
		if ( pUnavailableLabel )
		{
			pUnavailableLabel->SetVisible( true );
		}
	}

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: Keyboard input hook. Return 0 if handled
//-----------------------------------------------------------------------------
int	CHudMenuEngyDestroy::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !ShouldDraw() )
	{
		return 1;
	}

	if ( !down )
	{
		return 1;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return 1;

	bool bCanDestroyBuildings = true;
	if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		ConVarRef training_can_destroy_buildings("training_can_destroy_buildings");
		bCanDestroyBuildings = training_can_destroy_buildings.GetBool();
	}
	if ( bCanDestroyBuildings == false )
	{
		return 1;
	}

	bool bHandled = false;

	int iSlot = -1;

	// convert slot1, slot2 etc to 1,2,3,4
	if( pszCurrentBinding && !Q_strncmp( pszCurrentBinding, "slot", NUM_ENGY_BUILDINGS ) && Q_strlen(pszCurrentBinding) > NUM_ENGY_BUILDINGS )
	{
		const char *pszNum = pszCurrentBinding+NUM_ENGY_BUILDINGS;
		iSlot = atoi(pszNum);

		// slot10 cancels
		if ( iSlot == 10 )
		{
			engine->ExecuteClientCmd( "lastinv" );
			return 0;
		}

		iSlot -= 1;	// adjust to be 0 based

		// allow slot1 - slot4 
		if ( iSlot < 0 || iSlot > 3 )
			return 1;
	}
	else
	{
		switch( keynum )
		{
		case KEY_1:
		case KEY_XBUTTON_UP:
			iSlot = 0;
			bHandled = true;
			break;
		case KEY_2:
		case KEY_XBUTTON_RIGHT:
			iSlot = 1;
			bHandled = true;
			break;
		case KEY_3:
		case KEY_XBUTTON_DOWN:
			iSlot = 2;
			bHandled = true;
			break;
		case KEY_4:
		case KEY_XBUTTON_LEFT:
			iSlot = 3;
			bHandled = true;
			break;

		case KEY_5:
		case KEY_6:
		case KEY_7:
		case KEY_8:
		case KEY_9:
			// Eat these keys
			bHandled = true;
			break;

		case KEY_0:
		case KEY_XBUTTON_B:
			engine->ExecuteClientCmd( "lastinv" );
			bHandled = true;
			break;

		default:
			break;
		}
	}

	if ( iSlot >= 0 )
	{
		int iBuildingID, iMode;
		CHudMenuEngyBuild::GetBuildingIDAndModeFromSlot( iSlot+1, iBuildingID, iMode, m_Buildings );

		C_BaseObject *pObj = pLocalPlayer->GetObjectOfType( iBuildingID, iMode );

		if ( pObj && !pObj->HasSapper() && !pObj->IsPlasmaDisabled() )
		{
			char szCmd[128];
			Q_snprintf( szCmd, sizeof(szCmd), "destroy %d %d; lastinv", iBuildingID, iMode );
			engine->ExecuteClientCmd( szCmd );
		}
		else
		{
			ErrorSound();
		}
	}

	// return 0 if we ate the key
	return ( bHandled == false );
}

void CHudMenuEngyDestroy::ErrorSound( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pLocalPlayer )
	{
		pLocalPlayer->EmitSound( "Player.DenyWeaponSelection" );
	}
}


void CHudMenuEngyDestroy::OnTick( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	bool bCanDestroyBuildings = true;
	if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		ConVarRef training_can_destroy_buildings("training_can_destroy_buildings");
		bCanDestroyBuildings = training_can_destroy_buildings.GetBool();
	}

	int i;

	for ( i=0;i<NUM_ENGY_BUILDINGS; i++ )
	{
		int iRemappedObjectID, iMode;
		CHudMenuEngyBuild::GetBuildingIDAndModeFromSlot( i + 1, iRemappedObjectID, iMode, m_Buildings );

		// update this slot
		C_BaseObject *pObj = NULL;

		if ( pLocalPlayer )
		{
			pObj = pLocalPlayer->GetObjectOfType( iRemappedObjectID, iMode );
		}			

		// If the building is built, we can destroy it
		// unless we are in training, then we check the convar
		if( m_Buildings[i].m_bEnabled == false )
		{
			m_pActiveItems[i]->SetVisible( false );
			m_pInactiveItems[i]->SetVisible( false );
			m_pUnavailableItems[i]->SetVisible( false );
		}
		else if ( bCanDestroyBuildings == false )
		{
			m_pActiveItems[i]->SetVisible( false );
			m_pInactiveItems[i]->SetVisible( false );
			m_pUnavailableItems[i]->SetVisible( true );
		}
		else if ( pObj != NULL && !pObj->IsPlacing() )
		{
			m_pActiveItems[i]->SetVisible( true );
			m_pInactiveItems[i]->SetVisible( false );
			m_pUnavailableItems[i]->SetVisible( false );
		}
		else
		{
			m_pActiveItems[i]->SetVisible( false );
			m_pInactiveItems[i]->SetVisible( true );
			m_pUnavailableItems[i]->SetVisible( false );
		}
	}
}

void CHudMenuEngyDestroy::SetVisible( bool state )
{
	if ( state == IsVisible() )
		return;

	InitBuildings();

	if ( state == true )
	{
		// See if our layout needs to change, due to equipped items
		int iDesired = CalcCustomDestroyMenuLayout();
		if ( iDesired != m_iCurrentDestroyMenuLayout )
		{
			m_iCurrentDestroyMenuLayout = (destroymenulayouts_t)iDesired;
			InvalidateLayout( true, true );
		}

		// set the %lastinv% dialog var to our binding
		const char *key = engine->Key_LookupBinding( "lastinv" );
		if ( !key )
		{
			key = "< not bound >";
		}

		SetDialogVariable( "lastinv", key );

		//RecalculateBuildingState( ALL_BUILDINGS );

		HideLowerPriorityHudElementsInGroup( "mid" );
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( state );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CHudMenuEngyDestroy::CalcCustomDestroyMenuLayout( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return DESTROYMENU_DEFAULT;

	int iMenu = DESTROYMENU_DEFAULT;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pLocalPlayer, iMenu, set_custom_buildmenu );
	return iMenu;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuEngyDestroy::InitBuildings()
{
	for( int i=0; i<NUM_ENGY_BUILDINGS; ++i )
	{
		m_Buildings[i] = g_kEngyBuildings[i];
	}

	CHudMenuEngyBuild::ReplaceBuildings( m_Buildings );
		
	InvalidateLayout( true, true );
}

