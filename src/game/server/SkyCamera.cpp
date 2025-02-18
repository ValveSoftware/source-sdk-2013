//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "igamesystem.h"
#include "entitylist.h"
#include "SkyCamera.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// automatically hooks in the system's callbacks
CEntityClassList<CSkyCamera> g_SkyList;
template <> CSkyCamera *CEntityClassList<CSkyCamera>::m_pClassList = NULL;

//-----------------------------------------------------------------------------
// Retrives the current skycamera
//-----------------------------------------------------------------------------
CSkyCamera*	GetCurrentSkyCamera()
{
	return g_SkyList.m_pClassList;
}

CSkyCamera*	GetSkyCameraList()
{
	return g_SkyList.m_pClassList;
}

//=============================================================================

LINK_ENTITY_TO_CLASS( sky_camera, CSkyCamera );

BEGIN_DATADESC( CSkyCamera )

	DEFINE_KEYFIELD( m_skyboxData.scale, FIELD_INTEGER, "scale" ),
	DEFINE_FIELD( m_skyboxData.origin, FIELD_VECTOR ),
	DEFINE_FIELD( m_skyboxData.area, FIELD_INTEGER ),

	// Quiet down classcheck
	// DEFINE_FIELD( m_skyboxData, sky3dparams_t ),

	// This is re-set up in the constructor
	// DEFINE_FIELD( m_pNext, CSkyCamera ),

	// fog data for 3d skybox
	DEFINE_KEYFIELD( m_bUseAngles,						FIELD_BOOLEAN,	"use_angles" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.enable,			FIELD_BOOLEAN, "fogenable" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.blend,			FIELD_BOOLEAN, "fogblend" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.dirPrimary,		FIELD_VECTOR, "fogdir" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.colorPrimary,		FIELD_COLOR32, "fogcolor" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.colorSecondary,	FIELD_COLOR32, "fogcolor2" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.start,			FIELD_FLOAT, "fogstart" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.end,				FIELD_FLOAT, "fogend" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.maxdensity,		FIELD_FLOAT, "fogmaxdensity" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.radial,			FIELD_BOOLEAN, "fogradial" ),

END_DATADESC()


//-----------------------------------------------------------------------------
// List of maps in HL2 that we must apply our skybox fog fixup hack to
//-----------------------------------------------------------------------------
static const char *s_pBogusFogMaps[] =
{
	"d1_canals_01",
	"d1_canals_01a",
	"d1_canals_02",
	"d1_canals_03",
	"d1_canals_09",
	"d1_canals_10",
	"d1_canals_11",
	"d1_canals_12",
	"d1_canals_13",
	"d1_eli_01",
	"d1_trainstation_01",
	"d1_trainstation_03",
	"d1_trainstation_04",
	"d1_trainstation_05",
	"d1_trainstation_06",
	"d3_c17_04",
	"d3_c17_11",
	"d3_c17_12",
	"d3_citadel_01",
	NULL
};

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CSkyCamera::CSkyCamera()
{
	g_SkyList.Insert( this );
	m_skyboxData.fog.maxdensity = 1.0f;
	m_skyboxData.fog.radial = false;
}

CSkyCamera::~CSkyCamera()
{
	g_SkyList.Remove( this );
}

void CSkyCamera::Spawn( void ) 
{ 
	m_skyboxData.origin = GetLocalOrigin();
	m_skyboxData.area = engine->GetArea( m_skyboxData.origin );
	
	Precache();
}


//-----------------------------------------------------------------------------
// Activate!
//-----------------------------------------------------------------------------
void CSkyCamera::Activate( ) 
{
	BaseClass::Activate();

	if ( m_bUseAngles )
	{
		AngleVectors( GetAbsAngles(), &m_skyboxData.fog.dirPrimary.GetForModify() );
		m_skyboxData.fog.dirPrimary.GetForModify() *= -1.0f; 
	}

#ifdef HL2_DLL
	// NOTE! This is a hack. There was a bug in the skybox fog computation
	// on the client DLL that caused it to use the average of the primary and
	// secondary fog color when blending was enabled. The bug is fixed, but to make
	// the maps look the same as before the bug fix without having to download new maps,
	// I have to cheat here and slam the primary and secondary colors to be the average of 
	// the primary and secondary colors.
	if ( m_skyboxData.fog.blend )
	{
		for ( int i = 0; s_pBogusFogMaps[i]; ++i )
		{
			if ( !Q_stricmp( s_pBogusFogMaps[i], STRING(gpGlobals->mapname) ) )
			{
				m_skyboxData.fog.colorPrimary.SetR( ( m_skyboxData.fog.colorPrimary.GetR() + m_skyboxData.fog.colorSecondary.GetR() ) * 0.5f );
				m_skyboxData.fog.colorPrimary.SetG( ( m_skyboxData.fog.colorPrimary.GetG() + m_skyboxData.fog.colorSecondary.GetG() ) * 0.5f );
				m_skyboxData.fog.colorPrimary.SetB( ( m_skyboxData.fog.colorPrimary.GetB() + m_skyboxData.fog.colorSecondary.GetB() ) * 0.5f );
				m_skyboxData.fog.colorPrimary.SetA( ( m_skyboxData.fog.colorPrimary.GetA() + m_skyboxData.fog.colorSecondary.GetA() ) * 0.5f );
				m_skyboxData.fog.colorSecondary = m_skyboxData.fog.colorPrimary;
			}
		}
	}
#endif
}
