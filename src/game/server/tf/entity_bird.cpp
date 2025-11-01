//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tf_gamerules.h"
#include "entity_bird.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "tf_fx.h"

LINK_ENTITY_TO_CLASS( entity_bird, CEntityBird );

#define ENTITYBIRD_MODEL	"models/props_forest/bird.mdl"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntityBird::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_BBOX );
	SetModel( ENTITYBIRD_MODEL );
	SetSize( -Vector(8,8,0), Vector(8,8,16) );
	SetSequence(0);
	m_takedamage = DAMAGE_YES;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntityBird::Precache( void )
{
	BaseClass::Precache();

	// We deliberately allow late precaches here. It'll cause a hitch, but it'll ensure
	// we don't load the model on any map that doesn't have birds, and we'll make sure
	// we don't accidentally keep loading it after the birds are supposed to go away.
	
	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );
	PrecacheModel( ENTITYBIRD_MODEL );
	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntityBird::Touch( CBaseEntity *pOther )
{
	BaseClass::Touch( pOther );

	// If we're touched by anything, we pop!
	Explode();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEntityBird::OnTakeDamage( const CTakeDamageInfo &info )
{
	Explode();
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntityBird::Explode( void )
{
	Vector vecOrigin = WorldSpaceCenter();
	CPVSFilter filter( vecOrigin );
	TE_TFExplosion( filter, 0.0f, vecOrigin, Vector(0,0,1), TF_WEAPON_NONE, 0 );

	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Spawn random birds at locations on certain maps.
//-----------------------------------------------------------------------------
void CEntityBird::SpawnRandomBirds( void )
{
	const char *pszMapName = STRING( gpGlobals->mapname );
	if ( !pszMapName || !pszMapName[0] )
		return;

	KeyValuesAD pFileKV( "birds" );
	if ( !pFileKV->LoadFromFile( g_pFullFileSystem, "scripts/birds.txt", "MOD" ) )
		return;

	// Build a list of birds in the map already, and make sure we don't spawn any on those spots again
	CUtlVector<Vector>	vecExistingBirds;
	CBaseEntity *pBird = gEntList.FindEntityByClassname( NULL, "entity_bird" );
	while( pBird ) 
	{
		vecExistingBirds.AddToTail( pBird->GetAbsOrigin() );
		pBird = gEntList.FindEntityByClassname( pBird, "entity_bird" ); 
	}

	// See if we have an entry for this map
	KeyValues *pMapKV = pFileKV->FindKey( pszMapName );
	if ( pMapKV )
	{
		CUtlVector<Vector> vecLocations;
	
		KeyValues *pkvLoc = pMapKV->GetFirstSubKey();
		while ( pkvLoc )
		{
			const char *pszLocation = pkvLoc->GetString();
			int iIdx = vecLocations.AddToTail();
			UTIL_StringToVector( vecLocations[iIdx].Base(), pszLocation );
			pkvLoc = pkvLoc->GetNextKey();
		}

		if ( vecLocations.Count() )
		{
			//int iLocation = RandomInt(0,vecLocations.Count()-1);
			for ( int i = 0; i < vecLocations.Count(); i++ )
			{
				bool bExists = false;
				// Make sure there isn't a bird here already
				FOR_EACH_VEC( vecExistingBirds, iBird )
				{
					if ( vecLocations[i].DistToSqr(vecExistingBirds[iBird]) < (32*32) )
					{
						bExists = true;
						break;
					}
				}

				if ( !bExists )
				{
					CBaseEntity::Create( "entity_bird", vecLocations[i], QAngle(0,RandomFloat(0,360),0) );
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define ENTITY_FLYING_BIRD_SPEED_MIN		200
#define ENTITY_FLYING_BIRD_SPEED_MAX		500

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SpawnClientsideFlyingBird( Vector &vecSpawn )
{
	float flyAngle = RandomFloat( -M_PI, M_PI );
	float flyAngleRate = RandomFloat( -1.5f, 1.5f );
	float accelZ = RandomFloat( 0.5f, 2.0f );
	float speed = RandomFloat( ENTITY_FLYING_BIRD_SPEED_MIN, ENTITY_FLYING_BIRD_SPEED_MAX );
	float flGlideTime = RandomFloat( 0.25f, 1.0f );

	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );
	CBaseEntity::PrecacheModel( "models/props_forest/dove.mdl" );
	CBaseEntity::SetAllowPrecache( bAllowPrecache );

	CPVSFilter filter( vecSpawn );
	UserMessageBegin( filter, "SpawnFlyingBird" );
		WRITE_VEC3COORD( vecSpawn );
		WRITE_FLOAT( flyAngle );
		WRITE_FLOAT( flyAngleRate );
		WRITE_FLOAT( accelZ );
		WRITE_FLOAT( speed );
		WRITE_FLOAT( flGlideTime );
	MessageEnd();
}


