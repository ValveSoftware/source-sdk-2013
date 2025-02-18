//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "func_respawnflag.h"
#include "entity_capture_flag.h"
#include "tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( func_respawnflag, CFuncRespawnFlagZone );

BEGIN_DATADESC( CFuncRespawnFlagZone )
	// Functions.
	DEFINE_FUNCTION( Touch ),
END_DATADESC();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncRespawnFlagZone::CFuncRespawnFlagZone()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::Spawn( void )
{
	AddSpawnFlags( SF_TRIGGER_ALLOW_ALL );

	BaseClass::Spawn();
	InitTrigger();

	SetTouch( &CFuncRespawnFlagZone::Touch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::Touch( CBaseEntity *pOther )
{
	if ( !m_bDisabled )
	{
		if ( pOther->IsPlayer() )
		{
			CTFPlayer *pPlayer = ToTFPlayer( pOther );
			if ( pPlayer && pPlayer->HasTheFlag() )
			{
				CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*>( pPlayer->GetItem() );
				
				pPlayer->DropFlag();

				if ( pFlag )
				{
					pFlag->ResetFlag();
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Return true if the specified entity is in a RespawnFlag zone
//-----------------------------------------------------------------------------
bool PointInRespawnFlagZone( const Vector &vecPoint )
{
	CBaseEntity *pTempEnt = NULL;
	while ( ( pTempEnt = gEntList.FindEntityByClassname( pTempEnt, "func_respawnflag" ) ) != NULL )
	{
		CFuncRespawnFlagZone *pZone = dynamic_cast<CFuncRespawnFlagZone *>(pTempEnt);

		if ( pZone && !pZone->m_bDisabled && pZone->PointIsWithin( vecPoint ) )
		{
			return true;
		}
	}

	return false;
}
