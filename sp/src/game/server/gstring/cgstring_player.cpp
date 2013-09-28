
#include "cbase.h"
#include "cgstring_player.h"
#include "cgstring_globals.h"

BEGIN_DATADESC( CGstringPlayer )

	DEFINE_FIELD( m_bNightvisionActive, FIELD_BOOLEAN ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CGstringPlayer, DT_CGstringPlayer )

	SendPropBool( SENDINFO( m_bNightvisionActive ) ),
	SendPropBool( SENDINFO( m_bHasUseEntity ) ),

END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( player, CGstringPlayer );

CGstringPlayer::CGstringPlayer()
{
	m_bHasUseEntity = false;
	m_bNightvisionActive = false;
}

void CGstringPlayer::Precache()
{
	PrecacheScriptSound( "nightvision.on" );
	PrecacheScriptSound( "nightvision.off" );
	PrecacheScriptSound( "nightvision.unavailable" );

	PrecacheModel( "models/humans/group02/female_04.mdl" );

	BaseClass::Precache();
}

bool CGstringPlayer::IsNightvisionActive() const
{
	return m_bNightvisionActive;
}

void CGstringPlayer::SetNightvisionActive( bool bActive )
{
	if ( m_bNightvisionActive != bActive )
	{
		if ( bActive )
			EmitSound( "nightvision.on" );
		else
			EmitSound( "nightvision.off" );

		m_bNightvisionActive = bActive;
	}
}

void CGstringPlayer::ToggleNightvision()
{
	if ( g_pGstringGlobals != NULL
		&& !g_pGstringGlobals->IsNightvisionEnabled()
		|| !IsSuitEquipped() )
	{
		EmitSound( "nightvision.unavailable" );
		return;
	}

	SetNightvisionActive( !IsNightvisionActive() );
}

void CGstringPlayer::ImpulseCommands()
{
	int iImpulse = GetImpulse();

	switch ( iImpulse )
	{
	case 100:
		{
			if ( g_pGstringGlobals != NULL )
			{
				// neither flashlight nor nightvision
				if ( !g_pGstringGlobals->IsUserLightSourceEnabled() )
					break;

				if ( g_pGstringGlobals->IsNightvisionEnabled() ) // use nightvision
				{
					if ( FlashlightIsOn() )
						FlashlightTurnOff();

					ToggleNightvision();
				}
				else // use flashlight
				{
					SetNightvisionActive( false );
					BaseClass::ImpulseCommands();
				}

				break;
			}

			BaseClass::ImpulseCommands();
		}
		break;
	default:
		BaseClass::ImpulseCommands();
		return;
	}

	ClearImpulse();
}

void CGstringPlayer::PhysicsSimulate()
{
	BaseClass::PhysicsSimulate();

	m_bHasUseEntity = GetUseEntity() != NULL;
}