
#include "cbase.h"
#include "cgstring_player.h"
#include "cgstring_globals.h"

BEGIN_DATADESC( CGstringPlayer )

	DEFINE_FIELD( m_bNightvisionActive, FIELD_BOOLEAN ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CGstringPlayer, DT_CGstringPlayer )

	SendPropBool( SENDINFO( m_bNightvisionActive ) ),

END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( player, CGstringPlayer );

CGstringPlayer::CGstringPlayer()
{
	m_bNightvisionActive = false;
}

void CGstringPlayer::Precache()
{
	PrecacheScriptSound( "nightvision.on" );
	PrecacheScriptSound( "nightvision.off" );
	PrecacheScriptSound( "nightvision.unavailable" );

	BaseClass::Precache();
}

bool CGstringPlayer::IsNightvisionActive() const
{
	return m_bNightvisionActive;
}

void CGstringPlayer::SetNightvisionActive( bool bActive )
{
	m_bNightvisionActive = bActive;

	if ( bActive )
		EmitSound( "nightvision.on" );
	else
		EmitSound( "nightvision.off" );
}

void CGstringPlayer::ToggleNightvision()
{
	if ( g_pGstringGlobals != NULL
		&& !g_pGstringGlobals->IsNightvisionEnabled() )
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
			ToggleNightvision();
			ClearImpulse();
			break;
		}
	default:
		BaseClass::ImpulseCommands();
	}
}