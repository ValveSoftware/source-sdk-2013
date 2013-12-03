//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A point entity that periodically emits sparks and "bzzt" sounds.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "envspark.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Emits sparks from the given location and plays a random spark sound.
// Input  : pev - 
//			location - 
//-----------------------------------------------------------------------------
void DoSpark( CBaseEntity *ent, const Vector &location, int nMagnitude, int nTrailLength, bool bPlaySound, const Vector &vecDir )
{
	g_pEffects->Sparks( location, nMagnitude, nTrailLength, &vecDir );

	if ( bPlaySound )
	{
		ent->EmitSound( "DoSpark" );
	}
}

const int SF_SPARK_START_ON			= 64;
const int SF_SPARK_GLOW				= 128;
const int SF_SPARK_SILENT			= 256;
const int SF_SPARK_DIRECTIONAL		= 512;

BEGIN_DATADESC( CEnvSpark )

	DEFINE_KEYFIELD( m_flDelay, FIELD_FLOAT, "MaxDelay" ),
	DEFINE_FIELD( m_nGlowSpriteIndex, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_nMagnitude, FIELD_INTEGER, "Magnitude" ),
	DEFINE_KEYFIELD( m_nTrailLength, FIELD_INTEGER, "TrailLength" ),

	// Function Pointers
	DEFINE_FUNCTION( SparkThink ),

	DEFINE_INPUTFUNC( FIELD_VOID, "StartSpark", InputStartSpark ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopSpark", InputStopSpark ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ToggleSpark", InputToggleSpark ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SparkOnce", InputSparkOnce ),

	DEFINE_OUTPUT( m_OnSpark, "OnSpark" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( env_spark, CEnvSpark );


//-----------------------------------------------------------------------------
// Purpose: Constructor! Exciting, isn't it?
//-----------------------------------------------------------------------------
CEnvSpark::CEnvSpark( void )
{
	m_nMagnitude = 1;
	m_nTrailLength = 1;
}


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CEnvSpark::Spawn(void)
{
	SetThink( NULL );
	SetUse( NULL );

	if ( FBitSet(m_spawnflags, SF_SPARK_START_ON ) )
	{
		SetThink( &CEnvSpark::SparkThink );	// start sparking
	}

	SetNextThink( gpGlobals->curtime + 0.1 + random->RandomFloat( 0, 1.5 ) );

	// Negative delays are not allowed
	if ( m_flDelay < 0 )
	{
		m_flDelay = 0;
	}

#ifdef HL1_DLL
	// Don't allow 0 delays in HL1 Port. Enforce a default
	if( m_flDelay == 0 )
	{
		m_flDelay = 1.0f;
	}
#endif//HL1_DLL
	
	Precache( );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvSpark::Precache(void)
{
	m_nGlowSpriteIndex = PrecacheModel( "sprites/glow01.vmt" );

	PrecacheScriptSound( "DoSpark" );
}

extern ConVar phys_pushscale;

//-----------------------------------------------------------------------------
// Purpose: Emits sparks at random intervals.
//-----------------------------------------------------------------------------
void CEnvSpark::SparkThink(void)
{
	SetNextThink( gpGlobals->curtime + 0.1 + random->RandomFloat(0, m_flDelay) );

	Vector vecDir = vec3_origin;
	if ( FBitSet( m_spawnflags, SF_SPARK_DIRECTIONAL ) )
	{
		AngleVectors( GetAbsAngles(), &vecDir );
	}

	DoSpark( this, WorldSpaceCenter(), m_nMagnitude, m_nTrailLength, !( m_spawnflags & SF_SPARK_SILENT ), vecDir );

	m_OnSpark.FireOutput( this, this );

	if (FBitSet(m_spawnflags, SF_SPARK_GLOW))
	{
		CPVSFilter filter( GetAbsOrigin() );
		te->GlowSprite( filter, 0.0, &GetAbsOrigin(), m_nGlowSpriteIndex, 0.2, 1.5, 25 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for starting the sparks.
//-----------------------------------------------------------------------------
void CEnvSpark::InputStartSpark( inputdata_t &inputdata )
{
	StartSpark();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvSpark::StartSpark( void )
{
	SetThink( &CEnvSpark::SparkThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Shoot one spark.
//-----------------------------------------------------------------------------
void CEnvSpark::InputSparkOnce( inputdata_t &inputdata )
{
	SparkThink();
	SetNextThink( TICK_NEVER_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for starting the sparks.
//-----------------------------------------------------------------------------
void CEnvSpark::InputStopSpark( inputdata_t &inputdata )
{
	StopSpark();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvSpark::StopSpark( void )
{
	SetThink( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for toggling the on/off state of the sparks.
//-----------------------------------------------------------------------------
void CEnvSpark::InputToggleSpark( inputdata_t &inputdata )
{
	if ( GetNextThink() == TICK_NEVER_THINK )
	{
		InputStartSpark( inputdata );
	}
	else
	{
		InputStopSpark( inputdata );
	}
}


