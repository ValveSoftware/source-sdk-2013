//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Flame entity to be attached to target entity. Serves two purposes:
//
//			1) An entity that can be placed by a level designer and triggered
//			   to ignite a target entity.
//
//			2) An entity that can be created at runtime to ignite a target entity.
//
//=============================================================================//

#include "cbase.h"
#include "EntityFlame.h"
#include "ai_basenpc.h"
#include "fire.h"
#include "shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CEntityFlame )

	DEFINE_KEYFIELD( m_flLifetime, FIELD_FLOAT, "lifetime" ),

	DEFINE_FIELD( m_flSize, FIELD_FLOAT ),
	DEFINE_FIELD( m_hEntAttached, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bUseHitboxes, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iNumHitboxFires, FIELD_INTEGER ),
	DEFINE_FIELD( m_flHitboxFireScale, FIELD_FLOAT ),
	// DEFINE_FIELD( m_bPlayingSound, FIELD_BOOLEAN ),
	
	DEFINE_FUNCTION( FlameThink ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Ignite", InputIgnite ),

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST( CEntityFlame, DT_EntityFlame )
	SendPropEHandle( SENDINFO( m_hEntAttached ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( entityflame, CEntityFlame );
LINK_ENTITY_TO_CLASS( env_entity_igniter, CEntityFlame );
PRECACHE_REGISTER(entityflame);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEntityFlame::CEntityFlame( void )
{
	m_flSize			= 0.0f;
	m_iNumHitboxFires	= 10;
	m_flHitboxFireScale	= 1.0f;
	m_flLifetime		= 0.0f;
	m_bPlayingSound		= false;
}

void CEntityFlame::UpdateOnRemove()
{
	// Sometimes the entity I'm burning gets destroyed by other means,
	// which kills me. Make sure to stop the burning sound.
	if ( m_bPlayingSound )
	{
		EmitSound( "General.StopBurning" );
		m_bPlayingSound = false;
	}

	BaseClass::UpdateOnRemove();
}

void CEntityFlame::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "General.StopBurning" );
	PrecacheScriptSound( "General.BurningFlesh" );
	PrecacheScriptSound( "General.BurningObject" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : inputdata - 
//-----------------------------------------------------------------------------
void CEntityFlame::InputIgnite( inputdata_t &inputdata )
{
	if (m_target != NULL_STRING)
	{
		CBaseEntity *pTarget = NULL;
		while ((pTarget = gEntList.FindEntityGeneric(pTarget, STRING(m_target), this, inputdata.pActivator)) != NULL)
		{
			// Combat characters know how to catch themselves on fire.
			CBaseCombatCharacter *pBCC = pTarget->MyCombatCharacterPointer();
			if (pBCC)
			{
				// DVS TODO: consider promoting Ignite to CBaseEntity and doing everything here
				pBCC->Ignite(m_flLifetime);
			}
			// Everything else, we handle here.
			else
			{
				CEntityFlame *pFlame = CEntityFlame::Create(pTarget);
				if (pFlame)
				{
					pFlame->SetLifetime(m_flLifetime);
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Creates a flame and attaches it to a target entity.
// Input  : pTarget - 
//-----------------------------------------------------------------------------
CEntityFlame *CEntityFlame::Create( CBaseEntity *pTarget, bool useHitboxes )
{
	CEntityFlame *pFlame = (CEntityFlame *) CreateEntityByName( "entityflame" );

	if ( pFlame == NULL )
		return NULL;

	float xSize = pTarget->CollisionProp()->OBBMaxs().x - pTarget->CollisionProp()->OBBMins().x;
	float ySize = pTarget->CollisionProp()->OBBMaxs().y - pTarget->CollisionProp()->OBBMins().y;

	float size = ( xSize + ySize ) * 0.5f;
	
	if ( size < 16.0f )
	{
		size = 16.0f;
	}

	UTIL_SetOrigin( pFlame, pTarget->GetAbsOrigin() );

	pFlame->m_flSize = size;
	pFlame->SetThink( &CEntityFlame::FlameThink );
	pFlame->SetNextThink( gpGlobals->curtime + 0.1f );

	pFlame->AttachToEntity( pTarget );
	pFlame->SetLifetime( 2.0f );

	//Send to the client even though we don't have a model
	pFlame->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	pFlame->SetUseHitboxes( useHitboxes );

	return pFlame;
}


//-----------------------------------------------------------------------------
// Purpose: Attaches the flame to an entity and moves with it
// Input  : pTarget - target entity to attach to
//-----------------------------------------------------------------------------
void CEntityFlame::AttachToEntity( CBaseEntity *pTarget )
{
	// For networking to the client.
	m_hEntAttached = pTarget;

	if( pTarget->IsNPC() )
	{
		EmitSound( "General.BurningFlesh" );
	}
	else
	{
		EmitSound( "General.BurningObject" );
	}

	m_bPlayingSound = true;

	// So our heat emitter follows the entity around on the server.
	SetParent( pTarget );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : lifetime - 
//-----------------------------------------------------------------------------
void CEntityFlame::SetLifetime( float lifetime )
{
	m_flLifetime = gpGlobals->curtime + lifetime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : use - 
//-----------------------------------------------------------------------------
void CEntityFlame::SetUseHitboxes( bool use )
{
	m_bUseHitboxes = use;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iNumHitBoxFires - 
//-----------------------------------------------------------------------------
void CEntityFlame::SetNumHitboxFires( int iNumHitboxFires )
{
	m_iNumHitboxFires = iNumHitboxFires;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flHitboxFireScale - 
//-----------------------------------------------------------------------------
void CEntityFlame::SetHitboxFireScale( float flHitboxFireScale )
{
	m_flHitboxFireScale = flHitboxFireScale;
}

float CEntityFlame::GetRemainingLife( void )
{
	return m_flLifetime - gpGlobals->curtime;
}

int CEntityFlame::GetNumHitboxFires( void )
{
	return m_iNumHitboxFires;
}

float CEntityFlame::GetHitboxFireScale( void )
{
	return m_flHitboxFireScale;
}

//-----------------------------------------------------------------------------
// Purpose: Burn targets around us
//-----------------------------------------------------------------------------
void CEntityFlame::FlameThink( void )
{
	// Assure that this function will be ticked again even if we early-out in the if below.
	SetNextThink( gpGlobals->curtime + FLAME_DAMAGE_INTERVAL );

	if ( m_hEntAttached )
	{
		if ( m_hEntAttached->GetFlags() & FL_TRANSRAGDOLL )
		{
			SetRenderColorA( 0 );
			return;
		}
	
		CAI_BaseNPC *pNPC = m_hEntAttached->MyNPCPointer();
		if ( pNPC && !pNPC->IsAlive() )
		{
			UTIL_Remove( this );
			// Notify the NPC that it's no longer burning!
			pNPC->Extinguish();
			return;
		}

		if( m_hEntAttached->GetWaterLevel() > 0 )
		{
			Vector mins, maxs;

			mins = m_hEntAttached->WorldSpaceCenter();
			maxs = mins;

			maxs.z = m_hEntAttached->WorldSpaceCenter().z;
			maxs.x += 32;
			maxs.y += 32;
			
			mins.z -= 32;
			mins.x -= 32;
			mins.y -= 32;

			UTIL_Bubbles( mins, maxs, 12 );
		}
	}
	else
	{
		UTIL_Remove( this );
		return;
	}

	// See if we're done burning, or our attached ent has vanished
	if ( m_flLifetime < gpGlobals->curtime || m_hEntAttached == NULL )
	{
		EmitSound( "General.StopBurning" );
		m_bPlayingSound = false;
		SetThink( &CEntityFlame::SUB_Remove );
		SetNextThink( gpGlobals->curtime + 0.5f );

		// Notify anything we're attached to
		if ( m_hEntAttached )
		{
			CBaseCombatCharacter *pAttachedCC = m_hEntAttached->MyCombatCharacterPointer();

			if( pAttachedCC )
			{
				// Notify the NPC that it's no longer burning!
				pAttachedCC->Extinguish();
			}
		}

		return;
	}

	if ( m_hEntAttached )
	{
		// Do radius damage ignoring the entity I'm attached to. This will harm things around me.
		RadiusDamage( CTakeDamageInfo( this, this, 4.0f, DMG_BURN ), GetAbsOrigin(), m_flSize/2, CLASS_NONE, m_hEntAttached );

		// Directly harm the entity I'm attached to. This is so we can precisely control how much damage the entity
		// that is on fire takes without worrying about the flame's position relative to the bodytarget (which is the
		// distance that the radius damage code uses to determine how much damage to inflict)
		m_hEntAttached->TakeDamage( CTakeDamageInfo( this, this, FLAME_DIRECT_DAMAGE, DMG_BURN | DMG_DIRECT ) );

		if( !m_hEntAttached->IsNPC() && hl2_episodic.GetBool() )
		{
			const float ENTITYFLAME_MOVE_AWAY_DIST = 24.0f;
			// Make a sound near my origin, and up a little higher (in case I'm on the ground, so NPC's still hear it)
			CSoundEnt::InsertSound( SOUND_MOVE_AWAY, GetAbsOrigin(), ENTITYFLAME_MOVE_AWAY_DIST, 0.1f, this, SOUNDENT_CHANNEL_REPEATED_DANGER );
			CSoundEnt::InsertSound( SOUND_MOVE_AWAY, GetAbsOrigin() + Vector( 0, 0, 48.0f ), ENTITYFLAME_MOVE_AWAY_DIST, 0.1f, this, SOUNDENT_CHANNEL_REPEATING );
		}
	}
	else
	{
		RadiusDamage( CTakeDamageInfo( this, this, FLAME_RADIUS_DAMAGE, DMG_BURN ), GetAbsOrigin(), m_flSize/2, CLASS_NONE, NULL );
	}

	FireSystem_AddHeatInRadius( GetAbsOrigin(), m_flSize/2, 2.0f );

}  


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pEnt -	
//-----------------------------------------------------------------------------
void CreateEntityFlame(CBaseEntity *pEnt)
{
	CEntityFlame::Create( pEnt );
}
