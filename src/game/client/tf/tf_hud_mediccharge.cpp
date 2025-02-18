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
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "tf_weapon_medigun.h"
#include <vgui_controls/AnimationController.h>
#include "tf_imagepanel.h"
#include "vgui_controls/Label.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern ConVar weapon_medigun_resist_num_chunks;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudMedicChargeMeter : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudMedicChargeMeter, EditablePanel );

public:
	CHudMedicChargeMeter( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );

private:

	void UpdateKnownChargeType( bool bForce );
	void UpdateControlVisibility();

	vgui::ContinuousProgressBar*	m_pChargeMeter;
	CTFImagePanel*					m_pResistPanel;
	CUtlVector<vgui::ContinuousProgressBar*>	m_vecpChargeMeters;
	Label*							m_pUberchargeLabel;
	Label*							m_pUberchargeCountLabel;

	bool m_bCharged;
	float m_flLastChargeValue;

	int m_nLastActiveResist;
	medigun_weapontypes_t m_eLastKnownMedigunType;
};

DECLARE_HUDELEMENT( CHudMedicChargeMeter );

struct ResistIcons_t
{
	const char* m_pzsRed;
	const char* m_pzsBlue;
};

static ResistIcons_t g_ResistIcons[MEDIGUN_NUM_RESISTS] = 
{	
	{ "../HUD/defense_buff_bullet_blue",	"../HUD/defense_buff_bullet_red"  },
	{ "../HUD/defense_buff_explosion_blue",	"../HUD/defense_buff_explosion_red" },
	{ "../HUD/defense_buff_fire_blue",		"../HUD/defense_buff_fire_red" },
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMedicChargeMeter::CHudMedicChargeMeter( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMedicCharge" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pChargeMeter = new ContinuousProgressBar( this, "ChargeMeter" );

	for( int i=0; i<weapon_medigun_resist_num_chunks.GetInt(); ++i )
	{
		m_vecpChargeMeters.AddToTail( new ContinuousProgressBar( this,  CFmtStr( "ChargeMeter%d", i+1 ) ) );
	}

	m_pResistPanel = new CTFImagePanel( this, "ResistIcon" );
	m_pResistPanel->SetVisible( false );


	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_bCharged = false;
	m_flLastChargeValue = -1;
	m_eLastKnownMedigunType = MEDIGUN_STANDARD;
	m_nLastActiveResist = MEDIGUN_BULLET_RESIST;

	SetDialogVariable( "charge", 0 );
	SetDialogVariable( "charge_count", 0 );

	RegisterForRenderGroup( "inspect_panel" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMedicChargeMeter::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudMedicCharge.res" );

	m_pUberchargeLabel = dynamic_cast<Label*>( FindChildByName("ChargeLabel") );
	Assert( m_pUberchargeLabel );

	m_pUberchargeCountLabel = dynamic_cast<Label*>( FindChildByName("IndividualChargesLabel") );
	Assert( m_pUberchargeCountLabel );
	if( m_pUberchargeCountLabel )
	{
		m_pUberchargeCountLabel->SetVisible( false );
	}

	for( int i=0; i<weapon_medigun_resist_num_chunks.GetInt(); ++i )
	{
		m_vecpChargeMeters[i]->SetVisible( false );
	}

	m_pResistPanel->SetVisible( false );
	
	// Figure out which controls to show
	UpdateKnownChargeType( true );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMedicChargeMeter::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) || !pPlayer->IsAlive() )
	{
		return false;
	}

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

	if ( !pWpn )
	{
		return false;
	}

	if ( pWpn->GetWeaponID() != TF_WEAPON_MEDIGUN && pWpn->GetWeaponID() != TF_WEAPON_BONESAW && 
		 !( pWpn->GetWeaponID() == TF_WEAPON_SYRINGEGUN_MEDIC && pWpn->UberChargeAmmoPerShot() > 0.0f ) )
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMedicChargeMeter::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

	// Might have switched medigun types
	UpdateKnownChargeType( false);

	if ( !pWpn )
		return;

	if ( pWpn->GetWeaponID() == TF_WEAPON_BONESAW || 
		 ( pWpn->GetWeaponID() == TF_WEAPON_SYRINGEGUN_MEDIC && pWpn->UberChargeAmmoPerShot() > 0.0f ) )
	{
		pWpn = pPlayer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
	}

	if ( !pWpn || ( pWpn->GetWeaponID() != TF_WEAPON_MEDIGUN ) )
		return;

	CWeaponMedigun *pMedigun = assert_cast< CWeaponMedigun *>( pWpn );

	// We need to update the resist that we show here.  Only do so if the medigun is actually
	// the gun that's eruipped
	if( pMedigun->GetMedigunType() == MEDIGUN_RESIST &&  pPlayer->GetActiveTFWeapon() == pMedigun )
	{
		int nCurrentActiveResist = pMedigun->GetResistType();
		CBaseEntity* pOwner = pMedigun->GetOwner();

		// Figure out which image to show based on team
		const char* pzsImage = g_ResistIcons[nCurrentActiveResist].m_pzsBlue;
		if( pOwner && pOwner->GetTeamNumber() == TF_TEAM_BLUE )
		{
			pzsImage = g_ResistIcons[nCurrentActiveResist].m_pzsRed;
		}

		// Resist Medigun is equipped.  Update visibulity
		m_pResistPanel->SetVisible( true );
		m_pResistPanel->MoveToFront();
		m_pResistPanel->SetPos( 0, 0 );
		m_pResistPanel->SetImage( pzsImage );
	}
	else
	{
		m_pResistPanel->SetVisible( false );
	}

	float flCharge = pMedigun->GetChargeLevel();


	if ( flCharge != m_flLastChargeValue )
	{
		if( pMedigun->GetMedigunType() == MEDIGUN_RESIST )
		{
			float flChunkSize = 1.f / weapon_medigun_resist_num_chunks.GetFloat();
			for( int i=0; i<weapon_medigun_resist_num_chunks.GetInt(); ++i )
			{
				float flChunkCharge = flCharge - (flChunkSize * i);
				float flProgress = MIN(flChunkCharge, flChunkSize) / flChunkSize;

				// Not-full bars are dimmed a bit
				if( flProgress < 1.f && !pMedigun->IsReleasingCharge() )
				{
					m_vecpChargeMeters[i]->SetFgColor( Color( 190, 190, 190, 255 ) );
				}
				else	// Full bars are white
				{
					m_vecpChargeMeters[i]->SetFgColor( Color( 255, 255, 255, 255 ) );
				}

				m_vecpChargeMeters[i]->SetProgress( flProgress );
			}

			// We want to count full bars
			SetDialogVariable( "charge_count", int(floor(flCharge / flChunkSize)) );
		}
		else if ( m_pChargeMeter )	// Regular uber
		{
			m_pChargeMeter->SetProgress( flCharge );
			SetDialogVariable( "charge", (int)( flCharge * 100 ) );
		}

		if ( !m_bCharged )
		{
			if ( flCharge >= 1.0 )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMedicCharged" );
				m_bCharged = true;
			}
		}
		else
		{
			// we've got invuln charge or we're using our invuln
			if ( !pMedigun->IsReleasingCharge() )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMedicChargedStop" );
				m_bCharged = false;
			}
		}

	}	

	m_flLastChargeValue = flCharge;
}

void CHudMedicChargeMeter::UpdateKnownChargeType( bool bForce )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	CTFWeaponBase *pWpn = pPlayer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );

	if( !pWpn )
		return;

	CWeaponMedigun *pMedigun = static_cast< CWeaponMedigun *>( pWpn );

	if( pMedigun->GetMedigunType() != m_eLastKnownMedigunType || bForce )
	{
		m_eLastKnownMedigunType = (medigun_weapontypes_t)pMedigun->GetMedigunType();

		UpdateControlVisibility();
	}
}

void CHudMedicChargeMeter::UpdateControlVisibility()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	CTFWeaponBase *pWpn = pPlayer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );

	if ( !pWpn || ( pWpn->GetWeaponID() != TF_WEAPON_MEDIGUN ) )
		return;

	CWeaponMedigun *pMedigun = static_cast< CWeaponMedigun *>( pWpn );

	if ( !pMedigun )
		return;

	bool bResistMedigun = m_eLastKnownMedigunType == MEDIGUN_RESIST;

	// Using the resist medigun
	if( !bResistMedigun )
	{
		m_pResistPanel->SetVisible( false );
	}

	// Conditionally show the medigun chunks
	for( int i=0; i<m_vecpChargeMeters.Count(); ++i )
	{
		m_vecpChargeMeters[i]->SetVisible( bResistMedigun );
	}

	m_pChargeMeter->SetVisible( !bResistMedigun );
	if( m_pUberchargeLabel )
	{
		m_pUberchargeLabel->SetVisible( !bResistMedigun );
	}
	if( m_pUberchargeCountLabel )
	{
		m_pUberchargeCountLabel->SetVisible( bResistMedigun );
	}
}