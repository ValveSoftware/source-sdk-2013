//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Volumetric dust motes.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "func_dust_shared.h"
#include "te_particlesystem.h"
#include "IEffects.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CFunc_Dust : public CBaseEntity
{
public:
	DECLARE_CLASS( CFunc_Dust, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

					CFunc_Dust();
	virtual 		~CFunc_Dust();


// CBaseEntity overrides.
public:

	virtual void	Spawn();
	virtual void	Activate();
	virtual void	Precache();
	virtual bool	KeyValue( const char *szKeyName, const char *szValue );


// Input handles.
public:
	
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );


// FGD properties.
public:

	CNetworkVar( color32, m_Color );
	CNetworkVar( int, m_SpawnRate );
	
	CNetworkVar( float, m_flSizeMin );
	CNetworkVar( float, m_flSizeMax );

	CNetworkVar( int, m_SpeedMax );

	CNetworkVar( int, m_LifetimeMin );
	CNetworkVar( int, m_LifetimeMax );

	CNetworkVar( int, m_DistMax );

	CNetworkVar( float, m_FallSpeed );

public:

	CNetworkVar( int, m_DustFlags );	// Combination of DUSTFLAGS_

private:	
	int			m_iAlpha;

};


class CFunc_DustMotes : public CFunc_Dust
{
	DECLARE_CLASS( CFunc_DustMotes, CFunc_Dust );
public:
					CFunc_DustMotes();
};


class CFunc_DustCloud : public CFunc_Dust
{
	DECLARE_CLASS( CFunc_DustCloud, CFunc_Dust );
public:
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CFunc_Dust, DT_Func_Dust )
	SendPropInt( SENDINFO(m_Color),	32, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_SpawnRate),	12, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_SpeedMax),	12, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_flSizeMin), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO(m_flSizeMax), 0, SPROP_NOSCALE ),
	SendPropInt( SENDINFO(m_DistMax), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_LifetimeMin), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_LifetimeMax), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_DustFlags), DUST_NUMFLAGS, SPROP_UNSIGNED ),

	SendPropModelIndex( SENDINFO(m_nModelIndex) ),
	SendPropFloat( SENDINFO(m_FallSpeed), 0, SPROP_NOSCALE ),
	SendPropDataTable( SENDINFO_DT( m_Collision ), &REFERENCE_SEND_TABLE(DT_CollisionProperty) ),
END_SEND_TABLE()


BEGIN_DATADESC( CFunc_Dust )

	DEFINE_FIELD( m_DustFlags,FIELD_INTEGER ),

	DEFINE_KEYFIELD( m_Color,		FIELD_COLOR32,	"Color" ),
	DEFINE_KEYFIELD( m_SpawnRate,	FIELD_INTEGER,	"SpawnRate" ),
	DEFINE_KEYFIELD( m_flSizeMin,	FIELD_FLOAT,	"SizeMin" ),
	DEFINE_KEYFIELD( m_flSizeMax,	FIELD_FLOAT,	"SizeMax" ),
	DEFINE_KEYFIELD( m_SpeedMax,		FIELD_INTEGER,	"SpeedMax" ),
	DEFINE_KEYFIELD( m_LifetimeMin,	FIELD_INTEGER,	"LifetimeMin" ),
	DEFINE_KEYFIELD( m_LifetimeMax,	FIELD_INTEGER,	"LifetimeMax" ),
	DEFINE_KEYFIELD( m_DistMax,		FIELD_INTEGER,	"DistMax" ),
	DEFINE_FIELD( m_iAlpha,			FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_FallSpeed,	FIELD_FLOAT,	"FallSpeed" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn",  InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff )


END_DATADESC()

LINK_ENTITY_TO_CLASS( func_dustmotes, CFunc_DustMotes );
LINK_ENTITY_TO_CLASS( func_dustcloud, CFunc_DustCloud );


// ------------------------------------------------------------------------------------- //
// CFunc_DustMotes implementation.
// ------------------------------------------------------------------------------------- //

CFunc_DustMotes::CFunc_DustMotes()
{
	m_DustFlags |= DUSTFLAGS_SCALEMOTES;
}



// ------------------------------------------------------------------------------------- //
// CFunc_Dust implementation.
// ------------------------------------------------------------------------------------- //

CFunc_Dust::CFunc_Dust()
{
	m_DustFlags = DUSTFLAGS_ON;
	m_FallSpeed = 0.0f;
}


CFunc_Dust::~CFunc_Dust()
{
}


void CFunc_Dust::Spawn()
{
	Precache();

	// Bind to our bmodel.
	SetModel( STRING( GetModelName() ) );
	//AddSolidFlags( FSOLID_NOT_SOLID );
	AddSolidFlags( FSOLID_VOLUME_CONTENTS );

	//Since keyvalues can arrive in any order, and UTIL_StringToColor32 stomps alpha,
	//install the alpha value here.
	color32 clr = { m_Color.m_Value.r, m_Color.m_Value.g, m_Color.m_Value.b, (byte)m_iAlpha };
	m_Color.Set( clr );

	BaseClass::Spawn();
}


void CFunc_Dust::Precache()
{
	PrecacheMaterial( "particle/sparkles" );
}

void CFunc_Dust::Activate()
{
	BaseClass::Activate();
}


bool CFunc_Dust::KeyValue( const char *szKeyName, const char *szValue )
{
	if( stricmp( szKeyName, "StartDisabled" ) == 0 )
	{
		if( szValue[0] == '1' )
			m_DustFlags &= ~DUSTFLAGS_ON;
		else
			m_DustFlags |= DUSTFLAGS_ON;
	
		return true;
	}
	else if( stricmp( szKeyName, "Alpha" ) == 0 )
	{
		m_iAlpha = atoi( szValue );
		return true;
	}
	else if( stricmp( szKeyName, "Frozen" ) == 0 )
	{
		if( szValue[0] == '1' )
			m_DustFlags |= DUSTFLAGS_FROZEN;
		else
			m_DustFlags &= ~DUSTFLAGS_FROZEN;
	
		return true;
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}
}


void CFunc_Dust::InputTurnOn( inputdata_t &inputdata )
{
	if( !(m_DustFlags & DUSTFLAGS_ON) )
	{
		m_DustFlags |= DUSTFLAGS_ON;
	}
}


void CFunc_Dust::InputTurnOff( inputdata_t &inputdata )
{
	if( m_DustFlags & DUSTFLAGS_ON )
	{
		m_DustFlags &= ~DUSTFLAGS_ON;
	}
}

//
// Dust
//

class CTEDust : public CTEParticleSystem
{
public:
	DECLARE_CLASS( CTEDust, CTEParticleSystem );
	DECLARE_SERVERCLASS();

					CTEDust( const char *name );
	virtual			~CTEDust( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles ) { };
	
	CNetworkVar( float, m_flSize );
	CNetworkVar( float, m_flSpeed );
	CNetworkVector( m_vecDirection );
};

CTEDust::CTEDust( const char *name ) : BaseClass( name )
{
	m_flSize = 1.0f;
	m_flSpeed = 1.0f;
	m_vecDirection.Init();
}

CTEDust::~CTEDust( void )
{
}

IMPLEMENT_SERVERCLASS_ST( CTEDust, DT_TEDust )
	SendPropFloat( SENDINFO(m_flSize), -1, SPROP_COORD ),
	SendPropFloat( SENDINFO(m_flSpeed), -1, SPROP_COORD ),
	SendPropVector( SENDINFO(m_vecDirection), 4, 0, -1.0f, 1.0f ), // cheap normal
END_SEND_TABLE()

static CTEDust g_TEDust( "Dust" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &pos - 
//			&angles - 
//-----------------------------------------------------------------------------
void TE_Dust( IRecipientFilter& filter, float delay,
	const Vector &pos, const Vector &dir, float size, float speed )
{
	g_TEDust.m_vecOrigin	= pos;
	g_TEDust.m_vecDirection	= dir;
	g_TEDust.m_flSize		= size;
	g_TEDust.m_flSpeed		= speed;

	Assert( dir.Length() < 1.01 );	// make sure it's a normal

	//Send it
	g_TEDust.Create( filter, delay );
}

class CEnvDustPuff : public CPointEntity
{
	DECLARE_CLASS( CEnvDustPuff, CPointEntity );

public:
	
	DECLARE_DATADESC();

protected:

	// Input handlers
	void InputSpawnDust( inputdata_t &inputdata );

	float		m_flScale;
	color32		m_rgbaColor;
};

LINK_ENTITY_TO_CLASS( env_dustpuff, CEnvDustPuff );

BEGIN_DATADESC( CEnvDustPuff )

	DEFINE_KEYFIELD( m_flScale, FIELD_FLOAT, "scale" ),
	DEFINE_KEYFIELD( m_rgbaColor, FIELD_COLOR32, "color" ),

	// Function Pointers
	DEFINE_INPUTFUNC( FIELD_VOID, "SpawnDust", InputSpawnDust ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CEnvDustPuff::InputSpawnDust( inputdata_t &inputdata )
{
	Vector dir;
	AngleVectors( GetAbsAngles(), &dir );

	VectorNormalize( dir );

	g_pEffects->Dust( GetAbsOrigin(), dir, m_flScale, m_flSpeed );
}
