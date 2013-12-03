//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the server side of a steam jet particle system entity.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "smokestack.h"
#include "particle_light.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//Networking
IMPLEMENT_SERVERCLASS_ST(CSmokeStack, DT_SmokeStack)
	SendPropFloat(SENDINFO(m_SpreadSpeed), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_Speed), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_StartSize), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_EndSize), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_Rate), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_JetLength), 0, SPROP_NOSCALE),
	SendPropInt(SENDINFO(m_bEmit), 1, SPROP_UNSIGNED),
	SendPropFloat(SENDINFO(m_flBaseSpread), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO( m_flRollSpeed ), 0, SPROP_NOSCALE ),

	// Note: the base color is specified in the smokestack entity, but the directional
	// and ambient light must come from env_particlelight entities.
	SendPropVector( SENDINFO_NOCHECK(m_DirLight.m_vPos), 0, SPROP_NOSCALE ),
	SendPropVector( SENDINFO_NOCHECK(m_DirLight.m_vColor), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO_NOCHECK(m_DirLight.m_flIntensity), 0, SPROP_NOSCALE ),

	SendPropVector( SENDINFO_NOCHECK(m_AmbientLight.m_vPos), 0, SPROP_NOSCALE ),
	SendPropVector( SENDINFO_NOCHECK(m_AmbientLight.m_vColor), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO_NOCHECK(m_AmbientLight.m_flIntensity), 0, SPROP_NOSCALE ),

	SendPropVector(SENDINFO(m_vWind), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_flTwist), 0, SPROP_NOSCALE),
	SendPropIntWithMinusOneFlag( SENDINFO(m_iMaterialModel), 16 )

END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_smokestack, CSmokeStack );


//Save/restore

BEGIN_SIMPLE_DATADESC( CSmokeStackLightInfo )
	DEFINE_FIELD( m_vPos,			FIELD_POSITION_VECTOR	),
	DEFINE_FIELD( m_vColor,		FIELD_VECTOR	),
	DEFINE_FIELD( m_flIntensity,	FIELD_FLOAT	),
END_DATADESC()

BEGIN_DATADESC( CSmokeStack )

	//Keyvalue fields
	DEFINE_KEYFIELD( m_StartSize,		FIELD_FLOAT,	"StartSize" ),
	DEFINE_KEYFIELD( m_EndSize,		FIELD_FLOAT,	"EndSize" ),
	DEFINE_KEYFIELD( m_InitialState,	FIELD_BOOLEAN,	"InitialState" ),
	DEFINE_KEYFIELD( m_flBaseSpread,	FIELD_FLOAT,	"BaseSpread" ),
	DEFINE_KEYFIELD( m_flTwist,		FIELD_FLOAT,	"Twist" ),
	DEFINE_KEYFIELD( m_flRollSpeed, FIELD_FLOAT,	"Roll" ),

	DEFINE_FIELD( m_strMaterialModel, FIELD_STRING ),
	DEFINE_FIELD( m_iMaterialModel,FIELD_INTEGER ),

	DEFINE_EMBEDDED( m_AmbientLight ),
	DEFINE_EMBEDDED( m_DirLight ),

	DEFINE_KEYFIELD( m_WindAngle, FIELD_INTEGER,	"WindAngle" ),
	DEFINE_KEYFIELD( m_WindSpeed, FIELD_INTEGER,	"WindSpeed" ),

	//Regular fields
	DEFINE_FIELD( m_vWind,	FIELD_VECTOR ),
	DEFINE_FIELD( m_bEmit,	FIELD_INTEGER ),

	// Inputs
	DEFINE_INPUT( m_JetLength, FIELD_FLOAT, "JetLength" ),
	DEFINE_INPUT( m_SpreadSpeed, FIELD_FLOAT, "SpreadSpeed" ),
	DEFINE_INPUT( m_Speed, FIELD_FLOAT, "Speed" ),
	DEFINE_INPUT( m_Rate, FIELD_FLOAT, "Rate" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Called before spawning, after key values have been set.
//-----------------------------------------------------------------------------
CSmokeStack::CSmokeStack()
{
	memset( &m_AmbientLight, 0, sizeof(m_AmbientLight) ); 
	memset( &m_DirLight, 0, sizeof(m_DirLight) ); 

	IMPLEMENT_NETWORKVAR_CHAIN( &m_AmbientLight );
	IMPLEMENT_NETWORKVAR_CHAIN( &m_DirLight );

	m_flTwist = 0;
	SetRenderColor( 0, 0, 0, 255 );
	m_vWind.GetForModify().Init();
	m_WindAngle = m_WindSpeed = 0;
	m_iMaterialModel = -1;
	m_flRollSpeed = 0.0f;
}


CSmokeStack::~CSmokeStack()
{
}


void CSmokeStack::Spawn( void )
{
	if ( m_InitialState )
	{
		m_bEmit = true;
	}
}


void CSmokeStack::Activate()
{
	DetectInSkybox();

	bool bGotDirLight = false;

	// Find local lights.
	CBaseEntity *pTestEnt = NULL;
	while ( 1 )
	{
		pTestEnt = gEntList.FindEntityByClassname( pTestEnt, PARTICLELIGHT_ENTNAME );
		if ( !pTestEnt )
			break;

		CParticleLight *pLight = (CParticleLight*)pTestEnt;
		if( !FStrEq( STRING(GetEntityName()), STRING(pLight->m_PSName) ) )
			continue;

		CSmokeStackLightInfo *pInfo = &m_AmbientLight;
		if ( pLight->m_bDirectional )
		{
			bGotDirLight = true;
			pInfo = &m_DirLight;
		}

		pInfo->m_flIntensity = pLight->m_flIntensity;
		pInfo->m_vColor = pLight->m_vColor;
		pInfo->m_vPos = pLight->GetAbsOrigin();
	}

	// Put our light colors in 0-1 space.
	m_AmbientLight.m_vColor.GetForModify() /= 255.0f;
	m_DirLight.m_vColor.GetForModify() /= 255.0f;

	BaseClass::Activate();

	// Legacy support..
	if ( m_iMaterialModel == -1 )
		m_iMaterialModel = PrecacheModel( "particle/SmokeStack.vmt" );
}


bool CSmokeStack::KeyValue( const char *szKeyName, const char *szValue )
{
	if( stricmp( szKeyName, "Wind" ) == 0 )
	{
		sscanf( szValue, "%f %f %f", &m_vWind.GetForModify().x, &m_vWind.GetForModify().y, &m_vWind.GetForModify().z );
		return true;
	}
	else if( stricmp( szKeyName, "WindAngle" ) == 0 )
	{
		m_WindAngle = atoi( szValue );
		RecalcWindVector();
		return true;
	}
	else if( stricmp( szKeyName, "WindSpeed" ) == 0 )
	{
		m_WindSpeed = atoi( szValue );
		RecalcWindVector();
		return true;
	}
	else if ( stricmp( szKeyName, "SmokeMaterial" ) == 0 )
	{
		// Make sure we have a vmt extension.
		if ( Q_stristr( szValue, ".vmt" ) )
		{
			m_strMaterialModel = AllocPooledString( szValue );
		}
		else
		{
			char str[512];
			Q_snprintf( str, sizeof( str ), "%s.vmt", szValue );
			m_strMaterialModel = AllocPooledString( str );
		}
		
		const char *pName = STRING( m_strMaterialModel );
		char szStrippedName[512];

		m_iMaterialModel = PrecacheModel( pName );
		Q_StripExtension( pName, szStrippedName, Q_strlen(pName)+1 );

		int iLength = Q_strlen( szStrippedName );
		szStrippedName[iLength-1] = '\0';

		int iCount = 1;
		char str[512];
		Q_snprintf( str, sizeof( str ), "%s%d.vmt", szStrippedName, iCount );
		
		while ( filesystem->FileExists( UTIL_VarArgs( "materials/%s", str ) ) )
		{
			PrecacheModel( str );
			iCount++;
			
			Q_snprintf( str, sizeof( str ), "%s%d.vmt", szStrippedName, iCount );
		}

		return true;
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}
}


void CSmokeStack::Precache()
{
	m_iMaterialModel = PrecacheModel( STRING( m_strMaterialModel ) );
	
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for toggling the steam jet on/off.
//-----------------------------------------------------------------------------
void CSmokeStack::InputToggle( inputdata_t &inputdata )
{
	m_bEmit = !m_bEmit;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning on the steam jet.
//-----------------------------------------------------------------------------
void CSmokeStack::InputTurnOn( inputdata_t &inputdata )
{
	m_bEmit = true;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning off the steam jet.
//-----------------------------------------------------------------------------
void CSmokeStack::InputTurnOff( inputdata_t &inputdata )
{
	m_bEmit = false;
}


void CSmokeStack::RecalcWindVector()
{
	m_vWind = Vector(  
		cos( DEG2RAD( (float)m_WindAngle ) ) * m_WindSpeed, 
		sin( DEG2RAD( (float)m_WindAngle ) ) * m_WindSpeed,
		0 );
	
}


