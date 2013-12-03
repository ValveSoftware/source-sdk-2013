//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================//
#include "cbase.h"
#include "fx_impact.h"
#include "tempent.h"
#include "c_te_effect_dispatch.h"
#include "c_te_legacytempents.h"


//-----------------------------------------------------------------------------
// Purpose: Handle weapon effect callbacks
//-----------------------------------------------------------------------------
void SDK_EjectBrass( int shell, const CEffectData &data )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if( !pPlayer )
		return;

	tempents->CSEjectBrass( data.m_vOrigin, data.m_vAngles, data.m_fFlags, shell, pPlayer );
}

void SDK_FX_EjectBrass_9mm_Callback( const CEffectData &data )
{
	SDK_EjectBrass( CS_SHELL_9MM, data );
}

void SDK_FX_EjectBrass_12Gauge_Callback( const CEffectData &data )
{
	SDK_EjectBrass( CS_SHELL_12GAUGE, data );
}



DECLARE_CLIENT_EFFECT( "EjectBrass_9mm", SDK_FX_EjectBrass_9mm_Callback );
DECLARE_CLIENT_EFFECT( "EjectBrass_12Gauge",SDK_FX_EjectBrass_12Gauge_Callback );

