//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "smoke_trail.h"
#include "dt_send.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SMOKETRAIL_ENTITYNAME		"env_smoketrail"
#define SPORETRAIL_ENTITYNAME		"env_sporetrail"
#define SPOREEXPLOSION_ENTITYNAME	"env_sporeexplosion"
#define DUSTTRAIL_ENTITYNAME		"env_dusttrail"

//-----------------------------------------------------------------------------
//Data table
//-----------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST(SmokeTrail, DT_SmokeTrail)
	SendPropFloat(SENDINFO(m_SpawnRate), 8, 0, 1, 1024),
	SendPropVector(SENDINFO(m_StartColor), 8, 0, 0, 1),
	SendPropVector(SENDINFO(m_EndColor), 8, 0, 0, 1),
	SendPropFloat(SENDINFO(m_ParticleLifetime), 16, SPROP_ROUNDUP, 0.1, 100),
	SendPropFloat(SENDINFO(m_StopEmitTime), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MinSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MaxSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MinDirectedSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MaxDirectedSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_StartSize), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_EndSize), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_SpawnRadius), -1, SPROP_NOSCALE),
	SendPropBool(SENDINFO(m_bEmit) ),
	SendPropInt(SENDINFO(m_nAttachment), 32 ),	
	SendPropFloat(SENDINFO(m_Opacity), -1, SPROP_NOSCALE),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(env_smoketrail, SmokeTrail);

BEGIN_DATADESC( SmokeTrail )

	DEFINE_FIELD( m_StartColor, FIELD_VECTOR ),
	DEFINE_FIELD( m_EndColor, FIELD_VECTOR ),
	DEFINE_KEYFIELD( m_Opacity, FIELD_FLOAT, "opacity" ),
	DEFINE_KEYFIELD( m_SpawnRate, FIELD_FLOAT, "spawnrate" ),
	DEFINE_KEYFIELD( m_ParticleLifetime, FIELD_FLOAT, "lifetime" ),
	DEFINE_FIELD( m_StopEmitTime, FIELD_TIME ),
	DEFINE_KEYFIELD( m_MinSpeed, FIELD_FLOAT, "minspeed" ),
	DEFINE_KEYFIELD( m_MaxSpeed, FIELD_FLOAT, "maxspeed" ),
	DEFINE_KEYFIELD( m_MinDirectedSpeed, FIELD_FLOAT, "mindirectedspeed" ),
	DEFINE_KEYFIELD( m_MaxDirectedSpeed, FIELD_FLOAT, "maxdirectedspeed" ),
	DEFINE_KEYFIELD( m_StartSize, FIELD_FLOAT, "startsize" ),
	DEFINE_KEYFIELD( m_EndSize, FIELD_FLOAT, "endsize" ),
	DEFINE_KEYFIELD( m_SpawnRadius, FIELD_FLOAT, "spawnradius" ),
	DEFINE_FIELD( m_bEmit, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nAttachment, FIELD_INTEGER ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
SmokeTrail::SmokeTrail()
{
	m_SpawnRate = 10;
	m_StartColor.GetForModify().Init(0.5, 0.5, 0.5);
	m_EndColor.GetForModify().Init(0,0,0);
	m_ParticleLifetime = 5;
	m_StopEmitTime = 0; // Don't stop emitting particles
	m_MinSpeed = 2;
	m_MaxSpeed = 4;
	m_MinDirectedSpeed = m_MaxDirectedSpeed = 0;
	m_StartSize = 35;
	m_EndSize = 55;
	m_SpawnRadius = 2;
	m_bEmit = true;
	m_nAttachment	= 0;
	m_Opacity = 0.5f;
}


//-----------------------------------------------------------------------------
// Parse data from a map file
//-----------------------------------------------------------------------------
bool SmokeTrail::KeyValue( const char *szKeyName, const char *szValue ) 
{
	if ( FStrEq( szKeyName, "startcolor" ) )
	{
		color32 tmp;
		UTIL_StringToColor32( &tmp, szValue );
		m_StartColor.GetForModify().Init( tmp.r / 255.0f, tmp.g / 255.0f, tmp.b / 255.0f );
		return true;
	}

	if ( FStrEq( szKeyName, "endcolor" ) )
	{
		color32 tmp;
		UTIL_StringToColor32( &tmp, szValue );
		m_EndColor.GetForModify().Init( tmp.r / 255.0f, tmp.g / 255.0f, tmp.b / 255.0f );
		return true;
	}

	if ( FStrEq( szKeyName, "emittime" ) )
	{
		m_StopEmitTime = gpGlobals->curtime + atof( szValue );
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}


//-----------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//-----------------------------------------------------------------------------
void SmokeTrail::SetEmit(bool bVal)
{
	m_bEmit = bVal;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : SmokeTrail*
//-----------------------------------------------------------------------------
SmokeTrail* SmokeTrail::CreateSmokeTrail()
{
	CBaseEntity *pEnt = CreateEntityByName(SMOKETRAIL_ENTITYNAME);
	if(pEnt)
	{
		SmokeTrail *pSmoke = dynamic_cast<SmokeTrail*>(pEnt);
		if(pSmoke)
		{
			pSmoke->Activate();
			return pSmoke;
		}
		else
		{
			UTIL_Remove(pEnt);
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Attach the smoke trail to an entity or point 
// Input  : index - entity that has the attachment
//			attachment - point to attach to
//-----------------------------------------------------------------------------
void SmokeTrail::FollowEntity( CBaseEntity *pEntity, const char *pAttachmentName )
{
	// For attachments
	if ( pAttachmentName && pEntity && pEntity->GetBaseAnimating() )
	{
		m_nAttachment = pEntity->GetBaseAnimating()->LookupAttachment( pAttachmentName );
	}
	else
	{
		m_nAttachment = 0;
	}

	BaseClass::FollowEntity( pEntity );
}


//==================================================
// RocketTrail
//==================================================

//Data table
IMPLEMENT_SERVERCLASS_ST(RocketTrail, DT_RocketTrail)
	SendPropFloat(SENDINFO(m_SpawnRate), 8, 0, 1, 1024),
	SendPropVector(SENDINFO(m_StartColor), 8, 0, 0, 1),
	SendPropVector(SENDINFO(m_EndColor), 8, 0, 0, 1),
	SendPropFloat(SENDINFO(m_ParticleLifetime), 16, SPROP_ROUNDUP, 0.1, 100),
	SendPropFloat(SENDINFO(m_StopEmitTime), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MinSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MaxSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_StartSize), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_EndSize), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_SpawnRadius), -1, SPROP_NOSCALE),
	SendPropBool(SENDINFO(m_bEmit)),
	SendPropInt(SENDINFO(m_nAttachment), 32 ),	
	SendPropFloat(SENDINFO(m_Opacity), -1, SPROP_NOSCALE),
	SendPropInt	(SENDINFO(m_bDamaged), 1, SPROP_UNSIGNED),
	SendPropFloat(SENDINFO(m_flFlareScale), -1, SPROP_NOSCALE),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_rockettrail, RocketTrail );

BEGIN_DATADESC( RocketTrail )

	DEFINE_FIELD( m_StartColor, FIELD_VECTOR ),
	DEFINE_FIELD( m_EndColor, FIELD_VECTOR ),
	DEFINE_FIELD( m_Opacity, FIELD_FLOAT ),
	DEFINE_FIELD( m_SpawnRate, FIELD_FLOAT ),
	DEFINE_FIELD( m_ParticleLifetime, FIELD_FLOAT ),
	DEFINE_FIELD( m_StopEmitTime, FIELD_TIME ),
	DEFINE_FIELD( m_MinSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_MaxSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_StartSize, FIELD_FLOAT ),
	DEFINE_FIELD( m_EndSize, FIELD_FLOAT ),
	DEFINE_FIELD( m_SpawnRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_bEmit, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( m_bDamaged, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flFlareScale, FIELD_FLOAT ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
RocketTrail::RocketTrail()
{
	m_SpawnRate = 10;
	m_StartColor.GetForModify().Init(0.5, 0.5, 0.5);
	m_EndColor.GetForModify().Init(0,0,0);
	m_ParticleLifetime = 5;
	m_StopEmitTime = 0; // Don't stop emitting particles
	m_MinSpeed = 2;
	m_MaxSpeed = 4;
	m_StartSize = 35;
	m_EndSize = 55;
	m_SpawnRadius = 2;
	m_bEmit = true;
	m_nAttachment	= 0;
	m_Opacity = 0.5f;
	m_flFlareScale = 1.5;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void RocketTrail::SetEmit(bool bVal)
{
	m_bEmit = bVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : SmokeTrail*
//-----------------------------------------------------------------------------
RocketTrail* RocketTrail::CreateRocketTrail()
{
	CBaseEntity *pEnt = CreateEntityByName( "env_rockettrail" );
	
	if( pEnt != NULL )
	{
		RocketTrail *pTrail = dynamic_cast<RocketTrail*>(pEnt);
		
		if( pTrail != NULL )
		{
			pTrail->Activate();
			return pTrail;
		}
		else
		{
			UTIL_Remove( pEnt );
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Attach the smoke trail to an entity or point 
// Input  : index - entity that has the attachment
//			attachment - point to attach to
//-----------------------------------------------------------------------------
void RocketTrail::FollowEntity( CBaseEntity *pEntity, const char *pAttachmentName )
{
	// For attachments
	if ( pAttachmentName && pEntity && pEntity->GetBaseAnimating() )
	{
		m_nAttachment = pEntity->GetBaseAnimating()->LookupAttachment( pAttachmentName );
	}
	else
	{
		m_nAttachment = 0;
	}

	BaseClass::FollowEntity( pEntity );
}

//==================================================
// SporeTrail
//==================================================

IMPLEMENT_SERVERCLASS_ST( SporeTrail, DT_SporeTrail )
	SendPropFloat	(SENDINFO(m_flSpawnRate), 8, 0, 1, 1024),
	SendPropVector	(SENDINFO(m_vecEndColor), 8, 0, 0, 1),
	SendPropFloat	(SENDINFO(m_flParticleLifetime), 16, SPROP_ROUNDUP, 0.1, 100),
	SendPropFloat	(SENDINFO(m_flStartSize), -1, SPROP_NOSCALE),
	SendPropFloat	(SENDINFO(m_flEndSize), -1, SPROP_NOSCALE),
	SendPropFloat	(SENDINFO(m_flSpawnRadius), -1, SPROP_NOSCALE),
	SendPropBool	(SENDINFO(m_bEmit)),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(env_sporetrail, SporeTrail);

BEGIN_DATADESC( SporeTrail )

	DEFINE_FIELD( m_vecEndColor, FIELD_VECTOR ),
	DEFINE_FIELD( m_flSpawnRate, FIELD_FLOAT ),
	DEFINE_FIELD( m_flParticleLifetime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flStartSize, FIELD_FLOAT ),
	DEFINE_FIELD( m_flEndSize, FIELD_FLOAT ),
	DEFINE_FIELD( m_flSpawnRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_bEmit, FIELD_BOOLEAN ),

END_DATADESC()

SporeTrail::SporeTrail( void )
{
	m_vecEndColor.GetForModify().Init();

	m_flSpawnRate			= 100.0f;
	m_flParticleLifetime	= 1.0f;
	m_flStartSize			= 1.0f;
	m_flEndSize				= 0.0f;
	m_flSpawnRadius			= 16.0f;
	SetRenderColor( 255, 255, 255, 255 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : SporeTrail*
//-----------------------------------------------------------------------------
SporeTrail* SporeTrail::CreateSporeTrail()
{
	CBaseEntity *pEnt = CreateEntityByName( SPORETRAIL_ENTITYNAME );
	
	if(pEnt)
	{
		SporeTrail *pSpore = dynamic_cast<SporeTrail*>(pEnt);
		
		if ( pSpore )
		{
			pSpore->Activate();
			return pSpore;
		}
		else
		{
			UTIL_Remove( pEnt );
		}
	}

	return NULL;
}

//==================================================
// SporeExplosion
//==================================================

IMPLEMENT_SERVERCLASS_ST( SporeExplosion, DT_SporeExplosion )
	SendPropFloat	(SENDINFO(m_flSpawnRate), 8, 0, 1, 1024),
	SendPropFloat	(SENDINFO(m_flParticleLifetime), 16, SPROP_ROUNDUP, 0.1, 100),
	SendPropFloat	(SENDINFO(m_flStartSize), -1, SPROP_NOSCALE),
	SendPropFloat	(SENDINFO(m_flEndSize), -1, SPROP_NOSCALE),
	SendPropFloat	(SENDINFO(m_flSpawnRadius), -1, SPROP_NOSCALE),
	SendPropBool	(SENDINFO(m_bEmit) ),
	SendPropBool	(SENDINFO(m_bDontRemove) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_sporeexplosion, SporeExplosion );

BEGIN_DATADESC( SporeExplosion )

	DEFINE_KEYFIELD( m_flSpawnRate, FIELD_FLOAT, "spawnrate" ),
	DEFINE_FIELD( m_flParticleLifetime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flStartSize, FIELD_FLOAT ),
	DEFINE_FIELD( m_flEndSize, FIELD_FLOAT ),
	DEFINE_FIELD( m_flSpawnRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_bEmit, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "startdisabled" ),
	DEFINE_FIELD( m_bDontRemove, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),

END_DATADESC()

SporeExplosion::SporeExplosion( void )
{
	m_flSpawnRate			= 100.0f;
	m_flParticleLifetime	= 1.0f;
	m_flStartSize			= 1.0f;
	m_flEndSize				= 0.0f;
	m_flSpawnRadius			= 16.0f;
	SetRenderColor( 255, 255, 255, 255 );
	m_bEmit = true;
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void SporeExplosion::Spawn( void )
{
	BaseClass::Spawn();

	m_bEmit = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : SporeExplosion*
//-----------------------------------------------------------------------------
SporeExplosion *SporeExplosion::CreateSporeExplosion()
{
	CBaseEntity *pEnt = CreateEntityByName( SPOREEXPLOSION_ENTITYNAME );
	
	if ( pEnt )
	{
		SporeExplosion *pSpore = dynamic_cast<SporeExplosion*>(pEnt);
		
		if ( pSpore )
		{
			pSpore->Activate();
			return pSpore;
		}
		else
		{
			UTIL_Remove( pEnt );
		}
	}

	return NULL;
}

void SporeExplosion::InputEnable( inputdata_t &inputdata )
{
	m_bDontRemove = true;
	m_bDisabled = false;
	m_bEmit = true;
}

void SporeExplosion::InputDisable( inputdata_t &inputdata )
{
	m_bDontRemove = true;
	m_bDisabled = true;
	m_bEmit = false;
}

BEGIN_DATADESC( CFireTrail )

	DEFINE_FIELD( m_flLifetime, FIELD_FLOAT ),
	DEFINE_FIELD( m_nAttachment, FIELD_INTEGER ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CFireTrail, DT_FireTrail )
	SendPropInt( SENDINFO( m_nAttachment ), 32 ),
	SendPropFloat( SENDINFO( m_flLifetime ), 0, SPROP_NOSCALE ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_fire_trail, CFireTrail );

void CFireTrail::Precache( void )
{
	PrecacheMaterial( "sprites/flamelet1" );
	PrecacheMaterial( "sprites/flamelet2" );
	PrecacheMaterial( "sprites/flamelet3" );
	PrecacheMaterial( "sprites/flamelet4" );
	PrecacheMaterial( "sprites/flamelet5" );
	PrecacheMaterial( "particle/particle_smokegrenade" );
	PrecacheMaterial( "particle/particle_noisesphere" );
}

//-----------------------------------------------------------------------------
// Purpose: Attach the smoke trail to an entity or point 
// Input  : index - entity that has the attachment
//			attachment - point to attach to
//-----------------------------------------------------------------------------
void CFireTrail::FollowEntity( CBaseEntity *pEntity, const char *pAttachmentName )
{
	// For attachments
	if ( pAttachmentName && pEntity && pEntity->GetBaseAnimating() )
	{
		m_nAttachment = pEntity->GetBaseAnimating()->LookupAttachment( pAttachmentName );
	}
	else
	{
		m_nAttachment = 0;
	}

	BaseClass::FollowEntity( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: Create and return a new fire trail entity
//-----------------------------------------------------------------------------
CFireTrail *CFireTrail::CreateFireTrail( void )
{
	CBaseEntity *pEnt = CreateEntityByName( "env_fire_trail" );
	
	if ( pEnt )
	{
		CFireTrail *pTrail = dynamic_cast<CFireTrail*>(pEnt);
		
		if ( pTrail )
		{
			pTrail->Activate();
			return pTrail;
		}
		else
		{
			UTIL_Remove( pEnt );
		}
	}

	return NULL;	
}


//-----------------------------------------------------------------------------
//Data table
//-----------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST(DustTrail, DT_DustTrail)
	SendPropFloat(SENDINFO(m_SpawnRate), 8, 0, 1, 1024),
	SendPropVector(SENDINFO(m_Color), 8, 0, 0, 1),
	SendPropFloat(SENDINFO(m_ParticleLifetime), 16, SPROP_ROUNDUP, 0.1, 100),
	SendPropFloat(SENDINFO(m_StopEmitTime), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MinSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MaxSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MinDirectedSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MaxDirectedSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_StartSize), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_EndSize), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_SpawnRadius), -1, SPROP_NOSCALE),
	SendPropBool(SENDINFO(m_bEmit) ),
	SendPropFloat(SENDINFO(m_Opacity), -1, SPROP_NOSCALE),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_dusttrail, DustTrail);

BEGIN_DATADESC( DustTrail )

	DEFINE_FIELD( m_Color, FIELD_VECTOR ),
	DEFINE_KEYFIELD( m_Opacity, FIELD_FLOAT, "opacity" ),
	DEFINE_KEYFIELD( m_SpawnRate, FIELD_FLOAT, "spawnrate" ),
	DEFINE_KEYFIELD( m_ParticleLifetime, FIELD_FLOAT, "lifetime" ),
	DEFINE_FIELD( m_StopEmitTime, FIELD_TIME ),
	DEFINE_KEYFIELD( m_MinSpeed, FIELD_FLOAT, "minspeed" ),
	DEFINE_KEYFIELD( m_MaxSpeed, FIELD_FLOAT, "maxspeed" ),
	DEFINE_KEYFIELD( m_MinDirectedSpeed, FIELD_FLOAT, "mindirectedspeed" ),
	DEFINE_KEYFIELD( m_MaxDirectedSpeed, FIELD_FLOAT, "maxdirectedspeed" ),
	DEFINE_KEYFIELD( m_StartSize, FIELD_FLOAT, "startsize" ),
	DEFINE_KEYFIELD( m_EndSize, FIELD_FLOAT, "endsize" ),
	DEFINE_KEYFIELD( m_SpawnRadius, FIELD_FLOAT, "spawnradius" ),
	DEFINE_FIELD( m_bEmit, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nAttachment, FIELD_INTEGER ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
DustTrail::DustTrail()
{
	m_SpawnRate = 10;
	m_Color.GetForModify().Init(0.5, 0.5, 0.5);
	m_ParticleLifetime = 5;
	m_StopEmitTime = 0; // Don't stop emitting particles
	m_MinSpeed = 2;
	m_MaxSpeed = 4;
	m_MinDirectedSpeed = m_MaxDirectedSpeed = 0;
	m_StartSize = 35;
	m_EndSize = 55;
	m_SpawnRadius = 2;
	m_bEmit = true;
	m_Opacity = 0.5f;
}


//-----------------------------------------------------------------------------
// Parse data from a map file
//-----------------------------------------------------------------------------
bool DustTrail::KeyValue( const char *szKeyName, const char *szValue ) 
{
	if ( FStrEq( szKeyName, "color" ) )
	{
		color32 tmp;
		UTIL_StringToColor32( &tmp, szValue );
		m_Color.GetForModify().Init( tmp.r / 255.0f, tmp.g / 255.0f, tmp.b / 255.0f );
		return true;
	}

	if ( FStrEq( szKeyName, "emittime" ) )
	{
		m_StopEmitTime = gpGlobals->curtime + atof( szValue );
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}


//-----------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//-----------------------------------------------------------------------------
void DustTrail::SetEmit(bool bVal)
{
	m_bEmit = bVal;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : DustTrail*
//-----------------------------------------------------------------------------
DustTrail* DustTrail::CreateDustTrail()
{
	CBaseEntity *pEnt = CreateEntityByName(DUSTTRAIL_ENTITYNAME);
	if(pEnt)
	{
		DustTrail *pDust = dynamic_cast<DustTrail*>(pEnt);
		if(pDust)
		{
			pDust->Activate();
			return pDust;
		}
		else
		{
			UTIL_Remove(pEnt);
		}
	}

	return NULL;
}
