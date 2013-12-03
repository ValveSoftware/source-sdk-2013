//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements visual effects entities: sprites, beams, bubbles, etc.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "beam_shared.h"
#include "ndebugoverlay.h"
#include "filters.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Keeps us from doing strcmps in the tracefilter.
string_t g_iszPhysicsPropClassname;

enum Touch_t
{
	touch_none = 0,
	touch_player_only,
	touch_npc_only,
	touch_player_or_npc,
	touch_player_or_npc_or_physicsprop,
};

class CEnvBeam : public CBeam
{
public:
	DECLARE_CLASS( CEnvBeam, CBeam );

	void	Spawn( void );
	void	Precache( void );
	void	Activate( void );

	void	StrikeThink( void );
	void	UpdateThink( void );
	void	RandomArea( void );
	void	RandomPoint( const Vector &vecSrc );
	void	Zap( const Vector &vecSrc, const Vector &vecDest );

	void	Strike( void );

	bool	PassesTouchFilters(CBaseEntity *pOther);

	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputStrikeOnce( inputdata_t &inputdata );

	void TurnOn( void );
	void TurnOff( void );
	void Toggle( void );
	
	const char *GetDecalName( void ){ return STRING( m_iszDecal );}

	inline bool ServerSide( void )
	{
		if ( m_life == 0 && !HasSpawnFlags(SF_BEAM_RING) )
			return true;

		return false;
	}

	DECLARE_DATADESC();

	void	BeamUpdateVars( void );

	int		m_active;
	int		m_spriteTexture;

	string_t m_iszStartEntity;
	string_t m_iszEndEntity;
	float	m_life;
	float	m_boltWidth;
	float	m_noiseAmplitude;
	int		m_speed;
	float	m_restrike;
	string_t m_iszSpriteName;
	int		m_frameStart;

	float	m_radius;

	Touch_t		m_TouchType;
	string_t	m_iFilterName;
	EHANDLE		m_hFilter;

	string_t		m_iszDecal;

	COutputEvent	m_OnTouchedByEntity;
};

LINK_ENTITY_TO_CLASS( env_beam, CEnvBeam );

BEGIN_DATADESC( CEnvBeam )

	DEFINE_FIELD( m_active, FIELD_INTEGER ),
	DEFINE_FIELD( m_spriteTexture, FIELD_INTEGER ),

	DEFINE_KEYFIELD( m_iszStartEntity, FIELD_STRING, "LightningStart" ),
	DEFINE_KEYFIELD( m_iszEndEntity, FIELD_STRING, "LightningEnd" ),
	DEFINE_KEYFIELD( m_life, FIELD_FLOAT, "life" ),
	DEFINE_KEYFIELD( m_boltWidth, FIELD_FLOAT, "BoltWidth" ),
	DEFINE_KEYFIELD( m_noiseAmplitude, FIELD_FLOAT, "NoiseAmplitude" ),
	DEFINE_KEYFIELD( m_speed, FIELD_INTEGER, "TextureScroll" ),
	DEFINE_KEYFIELD( m_restrike, FIELD_FLOAT, "StrikeTime" ),
	DEFINE_KEYFIELD( m_iszSpriteName, FIELD_STRING, "texture" ),
	DEFINE_KEYFIELD( m_frameStart, FIELD_INTEGER, "framestart" ),
	DEFINE_KEYFIELD( m_radius, FIELD_FLOAT, "Radius" ),
	DEFINE_KEYFIELD( m_TouchType, FIELD_INTEGER, "TouchType" ),
	DEFINE_KEYFIELD( m_iFilterName,	FIELD_STRING,	"filtername" ),
	DEFINE_KEYFIELD( m_iszDecal, FIELD_STRING, "decalname" ),

	DEFINE_FIELD( m_hFilter,	FIELD_EHANDLE ),

	// Function Pointers
	DEFINE_FUNCTION( StrikeThink ),
	DEFINE_FUNCTION( UpdateThink ),

	// Input functions
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StrikeOnce", InputStrikeOnce ),

	DEFINE_OUTPUT( m_OnTouchedByEntity, "OnTouchedByEntity" ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvBeam::Spawn( void )
{
	if ( !m_iszSpriteName )
	{
		SetThink( &CEnvBeam::SUB_Remove );
		return;
	}

	BaseClass::Spawn();

	m_noiseAmplitude = MIN(MAX_BEAM_NOISEAMPLITUDE, m_noiseAmplitude);

	// Check for tapering
	if ( HasSpawnFlags( SF_BEAM_TAPEROUT ) )
	{
		SetWidth( m_boltWidth );
		SetEndWidth( 0 );
	}
	else
	{
		SetWidth( m_boltWidth );
		SetEndWidth( GetWidth() );	// Note: EndWidth is not scaled
	}

	if ( ServerSide() )
	{
		SetThink( &CEnvBeam::UpdateThink );
		SetNextThink( gpGlobals->curtime );
		SetFireTime( gpGlobals->curtime );

		if ( GetEntityName() != NULL_STRING )
		{
			if ( !(m_spawnflags & SF_BEAM_STARTON) )
			{
				AddEffects( EF_NODRAW );
				m_active = 0;
				SetNextThink( TICK_NEVER_THINK );
			}
			else
			{
				m_active = 1;
			}
		}
	}
	else
	{
		m_active = 0;
		if ( !GetEntityName() || FBitSet(m_spawnflags, SF_BEAM_STARTON) )
		{
			SetThink( &CEnvBeam::StrikeThink );
			SetNextThink( gpGlobals->curtime + 1.0f );
		}
	}

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvBeam::Precache( void )
{
	if ( !Q_stristr( STRING(m_iszSpriteName), ".vmt" ) )
	{
		// HACK/YWB:  This was almost always the laserbeam.spr, so alloc'ing the name a second time with the proper extension isn't going to
		//  kill us on memrory.
		//Warning( "Level Design Error:  %s (%i:%s) Sprite name (%s) missing .vmt extension!\n",
		//	STRING( m_iClassname ), entindex(), GetEntityName(), STRING(m_iszSpriteName) );

		char fixedname[ 512 ];
		Q_strncpy( fixedname, STRING( m_iszSpriteName ), sizeof( fixedname ) );

		Q_SetExtension( fixedname, ".vmt", sizeof( fixedname ) );
		
		m_iszSpriteName = AllocPooledString( fixedname );
	}

	g_iszPhysicsPropClassname = AllocPooledString( "prop_physics" );

	m_spriteTexture = PrecacheModel( STRING(m_iszSpriteName) );
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvBeam::Activate( void )
{
	// Get a handle to my filter entity if there is one
	if (m_iFilterName != NULL_STRING)
	{
		m_hFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iFilterName ));
	}

	BaseClass::Activate();

	if ( ServerSide() )
		BeamUpdateVars();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to turn the lightning on either continually or for
//			interval refiring.
//-----------------------------------------------------------------------------
void CEnvBeam::InputTurnOn( inputdata_t &inputdata )
{
	if ( !m_active )
	{
		TurnOn();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to turn the lightning off.
//-----------------------------------------------------------------------------
void CEnvBeam::InputTurnOff( inputdata_t &inputdata )
{
	if ( m_active )
	{
		TurnOff();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to toggle the lightning on/off.
//-----------------------------------------------------------------------------
void CEnvBeam::InputToggle( inputdata_t &inputdata )
{
	if ( m_active )
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for making the beam strike once. This will not affect
//			any interval refiring that might be going on. If the lifetime is set
//			to zero (infinite) it will turn on and stay on.
//-----------------------------------------------------------------------------
void CEnvBeam::InputStrikeOnce( inputdata_t &inputdata )
{
	Strike();
}


//-----------------------------------------------------------------------------
// Purpose: Turns the lightning on. If it is set for interval refiring, it will
//			begin doing so. If it is set to be continually on, it will do so.
//-----------------------------------------------------------------------------
void CEnvBeam::TurnOn( void )
{
	m_active = 1;

	if ( ServerSide() )
	{
		RemoveEffects( EF_NODRAW );
		DoSparks( GetAbsStartPos(), GetAbsEndPos() );

		SetThink( &CEnvBeam::UpdateThink );
		SetNextThink( gpGlobals->curtime );
		SetFireTime( gpGlobals->curtime );
	}
	else
	{
		SetThink( &CEnvBeam::StrikeThink );
		SetNextThink( gpGlobals->curtime );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvBeam::TurnOff( void )
{
	m_active = 0;

	if ( ServerSide() )
	{
		AddEffects( EF_NODRAW );
	}

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}


//-----------------------------------------------------------------------------
// Purpose: Think function for striking at intervals.
//-----------------------------------------------------------------------------
void CEnvBeam::StrikeThink( void )
{
	if ( m_life != 0 )
	{
		if ( m_spawnflags & SF_BEAM_RANDOM )
			SetNextThink( gpGlobals->curtime + m_life + random->RandomFloat( 0, m_restrike ) );
		else
			SetNextThink( gpGlobals->curtime + m_life + m_restrike );
	}
	m_active = 1;

	if (!m_iszEndEntity)
	{
		if (!m_iszStartEntity)
		{
			RandomArea( );
		}
		else
		{
			CBaseEntity *pStart = RandomTargetname( STRING(m_iszStartEntity) );
			if (pStart != NULL)
			{
				RandomPoint( pStart->GetAbsOrigin() );
			}
			else
			{
				Msg( "env_beam: unknown entity \"%s\"\n", STRING(m_iszStartEntity) );
			}
		}
		return;
	}

	Strike();
}


//-----------------------------------------------------------------------------
// Purpose: Strikes once for its configured lifetime.
//-----------------------------------------------------------------------------
void CEnvBeam::Strike( void )
{
	CBroadcastRecipientFilter filter;

	CBaseEntity *pStart = RandomTargetname( STRING(m_iszStartEntity) );
	CBaseEntity *pEnd = RandomTargetname( STRING(m_iszEndEntity) );

	if ( pStart == NULL || pEnd == NULL )
		return;

	m_speed = clamp( (int) m_speed, 0, (int) MAX_BEAM_SCROLLSPEED );
	
	int pointStart = IsStaticPointEntity( pStart );
	int pointEnd = IsStaticPointEntity( pEnd );

	if ( pointStart || pointEnd )
	{
		if ( m_spawnflags & SF_BEAM_RING )
		{
			// don't work
			return;
		}

		te->BeamEntPoint( filter, 0.0,
			pointStart ? 0 : pStart->entindex(),
			pointStart ? &pStart->GetAbsOrigin() : NULL,
			pointEnd ? 0 : pEnd->entindex(),
			pointEnd ? &pEnd->GetAbsOrigin() : NULL,
			m_spriteTexture,
			0,	// No halo
			m_frameStart,
			(int)m_flFrameRate,
			m_life,
			m_boltWidth,
			m_boltWidth,	// End width
			0,				// No fade
			m_noiseAmplitude,
			m_clrRender->r,	m_clrRender->g,	m_clrRender->b,	m_clrRender->a,
			m_speed );
	}
	else
	{
		if ( m_spawnflags & SF_BEAM_RING)
		{
			te->BeamRing( filter, 0.0,
				pStart->entindex(), 
				pEnd->entindex(), 
				m_spriteTexture, 
				0,	// No halo
				m_frameStart,
				(int)m_flFrameRate,
				m_life,
				m_boltWidth,
				0,	// No spread
				m_noiseAmplitude,
				m_clrRender->r,
				m_clrRender->g,
				m_clrRender->b,
				m_clrRender->a,
				m_speed );
		}
		else
		{
			te->BeamEnts( filter, 0.0,
				pStart->entindex(), 
				pEnd->entindex(), 
				m_spriteTexture,
				0,	// No halo
				m_frameStart,
				(int)m_flFrameRate,
				m_life,
				m_boltWidth,
				m_boltWidth,	// End width
				0,				// No fade
				m_noiseAmplitude,
				m_clrRender->r,
				m_clrRender->g,
				m_clrRender->b,
				m_clrRender->a,
				m_speed );

		}
	}

	DoSparks( pStart->GetAbsOrigin(), pEnd->GetAbsOrigin() );
	if ( m_flDamage > 0 )
	{
		trace_t tr;
		UTIL_TraceLine( pStart->GetAbsOrigin(), pEnd->GetAbsOrigin(), MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );
		BeamDamageInstant( &tr, m_flDamage );
	}
	
}


class CTraceFilterPlayersNPCs : public ITraceFilter
{
public:
	bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		if ( pEntity )
		{
			if ( pEntity->IsPlayer() || pEntity->MyNPCPointer() )
				return true;
		}

		return false;
	}
	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_ENTITIES_ONLY;
	}
};

class CTraceFilterPlayersNPCsPhysicsProps : public ITraceFilter
{
public:
	bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		if ( pEntity )
		{
			if ( pEntity->IsPlayer() || pEntity->MyNPCPointer() || pEntity->m_iClassname == g_iszPhysicsPropClassname )
				return true;
		}

		return false;
	}
	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_ENTITIES_ONLY;
	}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CEnvBeam::PassesTouchFilters(CBaseEntity *pOther)
{
	bool fPassedSoFar = false;

	// Touched some player or NPC!
	if( m_TouchType != touch_npc_only )
	{
		if( pOther->IsPlayer() )
		{
			fPassedSoFar = true;
		}
	}

	if( m_TouchType != touch_player_only )
	{
		if( pOther->IsNPC() )
		{
			fPassedSoFar = true;
		}
	}

	if( m_TouchType == touch_player_or_npc_or_physicsprop )
	{
		if( pOther->m_iClassname == g_iszPhysicsPropClassname )
		{
			fPassedSoFar = true;
		}
	}

	if( fPassedSoFar )
	{
		CBaseFilter* pFilter = (CBaseFilter*)(m_hFilter.Get());
		return (!pFilter) ? true : pFilter->PassesFilter( this, pOther );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvBeam::UpdateThink( void )
{
	// Apply damage every 1/10th of a second.
	if ( ( m_flDamage > 0 ) && ( gpGlobals->curtime >= m_flFireTime + 0.1 ) )
	{
		trace_t tr;
		UTIL_TraceLine( GetAbsStartPos(), GetAbsEndPos(), MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );
		BeamDamage( &tr );
		// BeamDamage calls RelinkBeam, so no need to call it again.
	}
	else
	{
		RelinkBeam();
	}

	if( m_TouchType != touch_none )
	{
		trace_t tr;
		Ray_t ray;
		ray.Init( GetAbsStartPos(), GetAbsEndPos() );

		if( m_TouchType == touch_player_or_npc_or_physicsprop )
		{
			CTraceFilterPlayersNPCsPhysicsProps traceFilter;
			enginetrace->TraceRay( ray, MASK_SHOT, &traceFilter, &tr );
		}
		else
		{
			CTraceFilterPlayersNPCs traceFilter;
			enginetrace->TraceRay( ray, MASK_SHOT, &traceFilter, &tr );
		}

		if( tr.fraction != 1.0 && PassesTouchFilters( tr.m_pEnt ) )
		{
			m_OnTouchedByEntity.FireOutput( tr.m_pEnt, this, 0 );
			return;
		}
	}

	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecSrc - 
//			&vecDest - 
//-----------------------------------------------------------------------------
void CEnvBeam::Zap( const Vector &vecSrc, const Vector &vecDest )
{
	CBroadcastRecipientFilter filter;

	te->BeamPoints( filter, 0.0,
		&vecSrc, 
		&vecDest, 
		m_spriteTexture, 
		0,	// No halo
		m_frameStart,
		(int)m_flFrameRate,
		m_life,
		m_boltWidth,
		m_boltWidth,	// End width
		0,				// No fade
		m_noiseAmplitude,
		m_clrRender->r,
		m_clrRender->g,
		m_clrRender->b,
		m_clrRender->a,
		m_speed );

	DoSparks( vecSrc, vecDest );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvBeam::RandomArea( void )
{
	int iLoops = 0;

	for (iLoops = 0; iLoops < 10; iLoops++)
	{
		Vector vecSrc = GetAbsOrigin();

		Vector vecDir1 = Vector( random->RandomFloat( -1.0, 1.0 ), random->RandomFloat( -1.0, 1.0 ),random->RandomFloat( -1.0, 1.0 ) );
		VectorNormalize( vecDir1 );
		trace_t	tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir1 * m_radius, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr1 );

		if (tr1.fraction == 1.0)
			continue;

		Vector vecDir2;
		do {
			vecDir2 = Vector( random->RandomFloat( -1.0, 1.0 ), random->RandomFloat( -1.0, 1.0 ),random->RandomFloat( -1.0, 1.0 ) );
		} while (DotProduct(vecDir1, vecDir2 ) > 0);
		VectorNormalize( vecDir2 );
		trace_t	tr2;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir2 * m_radius, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr2 );

		if (tr2.fraction == 1.0)
			continue;

		if ((tr1.endpos - tr2.endpos).Length() < m_radius * 0.1)
			continue;

		UTIL_TraceLine( tr1.endpos, tr2.endpos, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr2 );

		if (tr2.fraction != 1.0)
			continue;

 		Zap( tr1.endpos, tr2.endpos );

		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : vecSrc - 
//-----------------------------------------------------------------------------
void CEnvBeam::RandomPoint( const Vector &vecSrc )
{
	int iLoops = 0;

	for (iLoops = 0; iLoops < 10; iLoops++)
	{
		Vector vecDir1 = Vector( random->RandomFloat( -1.0, 1.0 ), random->RandomFloat( -1.0, 1.0 ),random->RandomFloat( -1.0, 1.0 ) );
		VectorNormalize( vecDir1 );
		trace_t	tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir1 * m_radius, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr1 );

		if ((tr1.endpos - vecSrc).Length() < m_radius * 0.1)
			continue;

		if (tr1.fraction == 1.0)
			continue;

		Zap( vecSrc, tr1.endpos );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvBeam::BeamUpdateVars( void )
{
	CBaseEntity *pStart = gEntList.FindEntityByName( NULL, m_iszStartEntity );
	CBaseEntity *pEnd = gEntList.FindEntityByName( NULL, m_iszEndEntity );

	if (( pStart == NULL ) || ( pEnd == NULL ))
	{
		return;
	}

	m_nNumBeamEnts = 2;

	m_speed = clamp( (int) m_speed, 0, (int) MAX_BEAM_SCROLLSPEED );

	// NOTE: If the end entity is the beam itself (and the start entity
	// isn't *also* the beam itself, we've got problems. This is a problem
	// because SetAbsStartPos actually sets the entity's origin.
	if ( ( pEnd == this ) && ( pStart != this ) )
	{
		DevMsg("env_beams cannot have the end entity be the beam itself\n"
			"unless the start entity is also the beam itself!\n" );
		Assert(0);
	}

	SetModelName( m_iszSpriteName );
	SetTexture( m_spriteTexture );

	SetType( BEAM_ENTPOINT );

	if ( IsStaticPointEntity( pStart ) )
	{
		SetAbsStartPos( pStart->GetAbsOrigin() );
	}
	else
	{
		SetStartEntity( pStart );
	}

	if ( IsStaticPointEntity( pEnd ) )
	{
		SetAbsEndPos( pEnd->GetAbsOrigin() );
	}
	else
	{
		SetEndEntity( pEnd );
	}

	RelinkBeam();

	SetWidth( MIN(MAX_BEAM_WIDTH, m_boltWidth) );
	SetNoise( MIN(MAX_BEAM_NOISEAMPLITUDE, m_noiseAmplitude) );
	SetFrame( m_frameStart );
	SetScrollRate( m_speed );
	if ( m_spawnflags & SF_BEAM_SHADEIN )
	{
		SetBeamFlags( FBEAM_SHADEIN );
	}
	else if ( m_spawnflags & SF_BEAM_SHADEOUT )
	{
		SetBeamFlags( FBEAM_SHADEOUT );
	}
}
