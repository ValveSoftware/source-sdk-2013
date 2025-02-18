//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "entityoutput.h"
#include "TemplateEntities.h"
#include "point_template.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_ENTMAKER_AUTOSPAWN				0x0001
#define SF_ENTMAKER_WAITFORDESTRUCTION		0x0002
#define SF_ENTMAKER_IGNOREFACING			0x0004
#define SF_ENTMAKER_CHECK_FOR_SPACE			0x0008
#define SF_ENTMAKER_CHECK_PLAYER_LOOKING	0x0010


//-----------------------------------------------------------------------------
// Purpose: An entity that mapmakers can use to ensure there's a required entity never runs out.
//			i.e. physics cannisters that need to be used.
//-----------------------------------------------------------------------------
class CEnvEntityMaker : public CPointEntity
{
	DECLARE_CLASS( CEnvEntityMaker, CPointEntity );
public:
	DECLARE_DATADESC();
	DECLARE_ENT_SCRIPTDESC();

	virtual void Spawn( void );
	virtual void Activate( void );

	void		 SpawnEntity( Vector vecAlternateOrigin = vec3_invalid, QAngle vecAlternateAngles = vec3_angle );
	void		 CheckSpawnThink( void );
	void		 InputForceSpawn( inputdata_t &inputdata );
	void		 InputForceSpawnAtEntityOrigin( inputdata_t &inputdata );

	void		 SpawnEntityFromScript();
	void		 SpawnEntityAtEntityOriginFromScript( HSCRIPT hEntity );
	void		 SpawnEntityAtNamedEntityOriginFromScript( const char *pszName );
	void		 SpawnEntityAtLocationFromScript( const Vector &vecAlternateOrigin, const Vector &vecAlternateAngles );

private:

	CPointTemplate *FindTemplate();

	bool HasRoomToSpawn();
	bool IsPlayerLooking();

	Vector			m_vecEntityMins;
	Vector			m_vecEntityMaxs;
	EHANDLE			m_hCurrentInstance;
	EHANDLE			m_hCurrentBlocker;	// Last entity that blocked us spawning something
	Vector			m_vecBlockerOrigin;

	// Movement after spawn
	QAngle			m_angPostSpawnDirection;
	float			m_flPostSpawnDirectionVariance;
	float			m_flPostSpawnSpeed;
	bool			m_bPostSpawnUseAngles;

	string_t		m_iszTemplate;

	COutputEvent	m_pOutputOnSpawned;
	COutputEvent	m_pOutputOnFailedSpawn;
};

BEGIN_DATADESC( CEnvEntityMaker )
	// DEFINE_FIELD( m_vecEntityMins, FIELD_VECTOR ),
	// DEFINE_FIELD( m_vecEntityMaxs, FIELD_VECTOR ),
	DEFINE_FIELD( m_hCurrentInstance, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hCurrentBlocker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecBlockerOrigin, FIELD_VECTOR ),
	DEFINE_KEYFIELD( m_iszTemplate, FIELD_STRING, "EntityTemplate" ),
	DEFINE_KEYFIELD( m_angPostSpawnDirection, FIELD_VECTOR, "PostSpawnDirection" ),
	DEFINE_KEYFIELD( m_flPostSpawnDirectionVariance, FIELD_FLOAT, "PostSpawnDirectionVariance" ),
	DEFINE_KEYFIELD( m_flPostSpawnSpeed, FIELD_FLOAT, "PostSpawnSpeed" ),
	DEFINE_KEYFIELD( m_bPostSpawnUseAngles, FIELD_BOOLEAN, "PostSpawnInheritAngles" ),

	// Outputs
	DEFINE_OUTPUT( m_pOutputOnSpawned, "OnEntitySpawned" ),
	DEFINE_OUTPUT( m_pOutputOnFailedSpawn, "OnEntityFailedSpawn" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceSpawn", InputForceSpawn ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ForceSpawnAtEntityOrigin", InputForceSpawnAtEntityOrigin ),

	// Functions
	DEFINE_THINKFUNC( CheckSpawnThink ),
END_DATADESC()

BEGIN_ENT_SCRIPTDESC( CEnvEntityMaker, CBaseEntity, "env_entity_maker" )
	DEFINE_SCRIPTFUNC_NAMED( SpawnEntityFromScript, "SpawnEntity", "Create an entity at the location of the maker" )
	DEFINE_SCRIPTFUNC_NAMED( SpawnEntityAtEntityOriginFromScript, "SpawnEntityAtEntityOrigin", "Create an entity at the location of a specified entity instance" )
	DEFINE_SCRIPTFUNC_NAMED( SpawnEntityAtNamedEntityOriginFromScript, "SpawnEntityAtNamedEntityOrigin", "Create an entity at the location of a named entity" )
	DEFINE_SCRIPTFUNC_NAMED( SpawnEntityAtLocationFromScript, "SpawnEntityAtLocation", "Create an entity at a specified location and orientaton, orientation is Euler angle in degrees (pitch, yaw, roll)" )
END_SCRIPTDESC()

LINK_ENTITY_TO_CLASS( env_entity_maker, CEnvEntityMaker );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvEntityMaker::Spawn( void )
{
	m_vecEntityMins = vec3_origin;
	m_vecEntityMaxs = vec3_origin;
	m_hCurrentInstance = NULL;
	m_hCurrentBlocker = NULL;
	m_vecBlockerOrigin = vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvEntityMaker::Activate( void )
{
	BaseClass::Activate();

	// check for valid template
	if ( m_iszTemplate == NULL_STRING )
	{
		Warning( "env_entity_maker %s has no template entity!\n", GetEntityName().ToCStr() );
		UTIL_Remove( this );
		return;
	}

	// Spawn an instance
	if ( m_spawnflags & SF_ENTMAKER_AUTOSPAWN )
	{
		SpawnEntity();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPointTemplate *CEnvEntityMaker::FindTemplate()
{
	// Find our point_template
	CPointTemplate *pTemplate = dynamic_cast<CPointTemplate *>(gEntList.FindEntityByName( NULL, STRING(m_iszTemplate) ));
	if ( !pTemplate )
	{
		Warning( "env_entity_maker %s failed to find template %s.\n", GetEntityName().ToCStr(), STRING(m_iszTemplate) );
	}

	return pTemplate;
}


//-----------------------------------------------------------------------------
// Purpose: Spawn an instance of the entity
//-----------------------------------------------------------------------------
void CEnvEntityMaker::SpawnEntity( Vector vecAlternateOrigin, QAngle vecAlternateAngles )
{
	CPointTemplate *pTemplate = FindTemplate();
	if (!pTemplate)
		return;

	// Spawn our template
	Vector vecSpawnOrigin = GetAbsOrigin();
	QAngle vecSpawnAngles = GetAbsAngles();

	if( vecAlternateOrigin != vec3_invalid )
	{
		// We have a valid alternate origin and angles. Use those instead
		// of spawning the items at my own origin and angles.
		vecSpawnOrigin = vecAlternateOrigin;
		vecSpawnAngles = vecAlternateAngles;
	}

	CUtlVector<CBaseEntity*> hNewEntities;
	if ( !pTemplate->CreateInstance( vecSpawnOrigin, vecSpawnAngles, &hNewEntities ) )
		return;
	
	//Adrian: oops we couldn't spawn the entity (or entities) for some reason!
	if ( hNewEntities.Count() == 0 )
		 return;
	
	m_hCurrentInstance = hNewEntities[0];

	// Assume it'll block us
	m_hCurrentBlocker = m_hCurrentInstance;
	m_vecBlockerOrigin = m_hCurrentBlocker->GetAbsOrigin();

	// Store off the mins & maxs the first time we spawn
	if ( m_vecEntityMins == vec3_origin )
	{
		m_hCurrentInstance->CollisionProp()->WorldSpaceAABB( &m_vecEntityMins, &m_vecEntityMaxs );
		m_vecEntityMins -= m_hCurrentInstance->GetAbsOrigin();
		m_vecEntityMaxs -= m_hCurrentInstance->GetAbsOrigin();
	}

	// Fire our output
	m_pOutputOnSpawned.FireOutput( this, this );

	// Start thinking
	if ( m_spawnflags & SF_ENTMAKER_AUTOSPAWN )
	{
		SetThink( &CEnvEntityMaker::CheckSpawnThink );
		SetNextThink( gpGlobals->curtime + 0.5f );
	}

	// If we have a specified post spawn speed, apply it to all spawned entities
	if ( m_flPostSpawnSpeed )
	{
		for ( int i = 0; i < hNewEntities.Count(); i++ )
		{
			CBaseEntity *pEntity = hNewEntities[i];
			if ( pEntity->GetMoveType() == MOVETYPE_NONE )
				continue;

			// Calculate a velocity for this entity
			Vector vForward,vRight,vUp;
			QAngle angSpawnDir( m_angPostSpawnDirection );
			if ( m_bPostSpawnUseAngles )
			{
				if ( GetParent() )
				{
					angSpawnDir += GetParent()->GetAbsAngles();
				}
				else
				{
					angSpawnDir += GetAbsAngles();
				}
			}
			AngleVectors( angSpawnDir, &vForward, &vRight, &vUp );
			Vector vecShootDir = vForward;
			vecShootDir += vRight * random->RandomFloat(-1, 1) * m_flPostSpawnDirectionVariance;
			vecShootDir += vForward * random->RandomFloat(-1, 1) * m_flPostSpawnDirectionVariance;
			vecShootDir += vUp * random->RandomFloat(-1, 1) * m_flPostSpawnDirectionVariance;
			VectorNormalize( vecShootDir );
			vecShootDir *= m_flPostSpawnSpeed;

			// Apply it to the entity
			IPhysicsObject *pPhysicsObject = pEntity->VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				pPhysicsObject->AddVelocity(&vecShootDir, NULL);
			}
			else
			{
				pEntity->SetAbsVelocity( vecShootDir );
			}
		}
	}

	pTemplate->CreationComplete( hNewEntities );
}

//-----------------------------------------------------------------------------
// Purpose: Spawn an instance of the entity
//-----------------------------------------------------------------------------
void CEnvEntityMaker::SpawnEntityFromScript()
{
	SpawnEntity();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn an instance of the entity
//-----------------------------------------------------------------------------
void CEnvEntityMaker::SpawnEntityAtEntityOriginFromScript( HSCRIPT hEntity )
{
	CBaseEntity *pTargetEntity = ToEnt( hEntity );
	if ( pTargetEntity )
	{
		SpawnEntity( pTargetEntity->GetAbsOrigin(), pTargetEntity->GetAbsAngles() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Spawn an instance of the entity
//-----------------------------------------------------------------------------
void CEnvEntityMaker::SpawnEntityAtNamedEntityOriginFromScript( const char *pszName )
{
	CBaseEntity *pTargetEntity = gEntList.FindEntityByName( NULL, pszName, this, NULL, NULL );

	if( pTargetEntity )
	{
		SpawnEntity( pTargetEntity->GetAbsOrigin(), pTargetEntity->GetAbsAngles() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Spawn an instance of the entity
//-----------------------------------------------------------------------------
void CEnvEntityMaker::SpawnEntityAtLocationFromScript( const Vector &vecAlternateOrigin, const Vector &vecAlternateAngles )
{
	SpawnEntity( vecAlternateOrigin, *((QAngle *)&vecAlternateAngles) );
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether or not the template entities can fit if spawned.
// Input  : pBlocker - Returns blocker unless NULL.
//-----------------------------------------------------------------------------
bool CEnvEntityMaker::HasRoomToSpawn()
{
	// Do we have a blocker from last time?
	if ( m_hCurrentBlocker )
	{
		// If it hasn't moved, abort immediately
		if ( m_vecBlockerOrigin == m_hCurrentBlocker->GetAbsOrigin() )
		{
			return false;
		}
	}

	// Check to see if there's enough room to spawn
	trace_t tr;
	UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin(), m_vecEntityMins, m_vecEntityMaxs, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.m_pEnt || tr.startsolid )
	{
		// Store off our blocker to check later
		m_hCurrentBlocker = tr.m_pEnt;
		if ( m_hCurrentBlocker )
		{
			m_vecBlockerOrigin = m_hCurrentBlocker->GetAbsOrigin();
		}

		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the player is looking towards us.
//-----------------------------------------------------------------------------
bool CEnvEntityMaker::IsPlayerLooking()
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			// Only spawn if the player's looking away from me
			Vector vLookDir = pPlayer->EyeDirection3D();
			Vector vTargetDir = GetAbsOrigin() - pPlayer->EyePosition();
			VectorNormalize( vTargetDir );

			float fDotPr = DotProduct( vLookDir,vTargetDir );
			if ( fDotPr > 0 )
				return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Check to see if we should spawn another instance
//-----------------------------------------------------------------------------
void CEnvEntityMaker::CheckSpawnThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.5f );

	// Do we have an instance?
	if ( m_hCurrentInstance )
	{
		// If Wait-For-Destruction is set, abort immediately
		if ( m_spawnflags & SF_ENTMAKER_WAITFORDESTRUCTION )
			return;
	}

	// Check to see if there's enough room to spawn
	if ( !HasRoomToSpawn() )
		return;

	// We're clear, now check to see if the player's looking
	if ( !( HasSpawnFlags( SF_ENTMAKER_IGNOREFACING ) ) && IsPlayerLooking() )
		return;

	// Clear, no player watching, so spawn!
	SpawnEntity();
}


//-----------------------------------------------------------------------------
// Purpose: Spawns the entities, checking for space if flagged to do so.
//-----------------------------------------------------------------------------
void CEnvEntityMaker::InputForceSpawn( inputdata_t &inputdata )
{
	CPointTemplate *pTemplate = FindTemplate();
	if (!pTemplate)
		return;

	if ( HasSpawnFlags( SF_ENTMAKER_CHECK_FOR_SPACE ) && !HasRoomToSpawn() )
	{
		m_pOutputOnFailedSpawn.FireOutput( this, this );
		return;
	}

	if ( HasSpawnFlags( SF_ENTMAKER_CHECK_PLAYER_LOOKING ) && IsPlayerLooking() )
	{
		m_pOutputOnFailedSpawn.FireOutput( this, this );
		return;
	}

	SpawnEntity();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvEntityMaker::InputForceSpawnAtEntityOrigin( inputdata_t &inputdata )
{
	CBaseEntity *pTargetEntity = gEntList.FindEntityByName( NULL, inputdata.value.String(), this, inputdata.pActivator, inputdata.pCaller );
		
	if( pTargetEntity )
	{
		SpawnEntity( pTargetEntity->GetAbsOrigin(), pTargetEntity->GetAbsAngles() );
	}
}
