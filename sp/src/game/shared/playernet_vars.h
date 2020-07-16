//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYERNET_VARS_H
#define PLAYERNET_VARS_H
#ifdef _WIN32
#pragma once
#endif

#include "shared_classnames.h"

#define NUM_AUDIO_LOCAL_SOUNDS	8

// These structs are contained in each player's local data and shared by the client & server

struct fogparams_t
{
	DECLARE_CLASS_NOBASE( fogparams_t );
	DECLARE_EMBEDDED_NETWORKVAR();

#ifndef CLIENT_DLL
	DECLARE_SIMPLE_DATADESC();
#endif

	bool operator !=( const fogparams_t& other ) const;

	CNetworkVector( dirPrimary );
	CNetworkColor32( colorPrimary );
	CNetworkColor32( colorSecondary );
	CNetworkColor32( colorPrimaryLerpTo );
	CNetworkColor32( colorSecondaryLerpTo );
	CNetworkVar( float, start );
	CNetworkVar( float, end );
	CNetworkVar( float, farz );
	CNetworkVar( float, maxdensity );

	CNetworkVar( float, startLerpTo );
	CNetworkVar( float, endLerpTo );
	CNetworkVar( float, lerptime );
	CNetworkVar( float, duration );
	CNetworkVar( bool, enable );
	CNetworkVar( bool, blend );
};

// Crappy. Needs to be here because it wants to use 
#ifdef CLIENT_DLL
#define CFogController C_FogController
#endif

class CFogController;

struct fogplayerparams_t
{
	DECLARE_CLASS_NOBASE( fogplayerparams_t );
	DECLARE_EMBEDDED_NETWORKVAR();

#ifndef CLIENT_DLL
	DECLARE_SIMPLE_DATADESC();
#endif

	CNetworkHandle( CFogController, m_hCtrl );
	float					m_flTransitionTime;

	color32					m_OldColor;
	float					m_flOldStart;
	float					m_flOldEnd;

	color32					m_NewColor;
	float					m_flNewStart;
	float					m_flNewEnd;

	fogplayerparams_t()
	{
		m_hCtrl.Set( NULL );
		m_flTransitionTime = -1.0f;
		m_OldColor.r = m_OldColor.g = m_OldColor.b = m_OldColor.a = 0;
		m_flOldStart = 0.0f;
		m_flOldEnd = 0.0f;
		m_NewColor.r = m_NewColor.g = m_NewColor.b = m_NewColor.a = 0;
		m_flNewStart = 0.0f;
		m_flNewEnd = 0.0f;
	}
};

struct sky3dparams_t
{
	DECLARE_CLASS_NOBASE( sky3dparams_t );
	DECLARE_EMBEDDED_NETWORKVAR();

#ifndef CLIENT_DLL
	DECLARE_SIMPLE_DATADESC();
#endif

	// 3d skybox camera data
	CNetworkVar( int, scale );
	CNetworkVector( origin );
#ifdef MAPBASE
	// Skybox angle support
	CNetworkQAngle( angles );

	// Skybox entity-based option
	CNetworkHandle( CBaseEntity, skycamera );

	// Sky clearcolor
	CNetworkColor32( skycolor );
#endif
	CNetworkVar( int, area );

	// 3d skybox fog data
	CNetworkVarEmbedded( fogparams_t, fog );
};

struct audioparams_t
{
	DECLARE_CLASS_NOBASE( audioparams_t );
	DECLARE_EMBEDDED_NETWORKVAR();

#ifndef CLIENT_DLL
	DECLARE_SIMPLE_DATADESC();
#endif

	CNetworkArray( Vector, localSound, NUM_AUDIO_LOCAL_SOUNDS )
	CNetworkVar( int, soundscapeIndex );	// index of the current soundscape from soundscape.txt
	CNetworkVar( int, localBits );			// if bits 0,1,2,3 are set then position 0,1,2,3 are valid/used
	CNetworkHandle( CBaseEntity, ent );		// the entity setting the soundscape
};

//Tony; new tonemap information.
// In single player the values are coped directly from the single env_tonemap_controller entity.
// This will allow the controller to work as it always did.
// That way nothing in ep2 will break. With these new params, the controller can properly be used in mp.


// Map specific objectives, such as blowing out a wall ( and bringing in more light )
// can still change values on a particular controller as necessary via inputs, but the
// effects will not directly affect any players who are referencing this controller
// unless the option to update on inputs is set. ( otherwise the values are simply cached
// and changes only take effect when the players controller target is changed )

struct tonemap_params_t
{
	DECLARE_CLASS_NOBASE( tonemap_params_t );
	DECLARE_EMBEDDED_NETWORKVAR();

#ifndef CLIENT_DLL
	DECLARE_SIMPLE_DATADESC();
#endif
	tonemap_params_t()
	{
		m_flAutoExposureMin = -1.0f;
		m_flAutoExposureMax = -1.0f;
		m_flTonemapScale = -1.0f;
		m_flBloomScale = -1.0f;
		m_flTonemapRate = -1.0f;
	}
	//Tony; all of these are initialized to -1!
	CNetworkVar( float, m_flTonemapScale );
	CNetworkVar( float, m_flTonemapRate );
	CNetworkVar( float, m_flBloomScale );

	CNetworkVar( float, m_flAutoExposureMin );
	CNetworkVar( float, m_flAutoExposureMax );

// BLEND TODO
//
//	//Tony; Time it takes for a blend to finish, default to 0; this is for the the effect of InputBlendTonemapScale.
//	//When
//	CNetworkVar( float, m_flBlendTime );

	//Tony; these next 4 variables do not have to be networked; but I want to update them on the client whenever m_flBlendTime changes.
	//TODO
};

#endif // PLAYERNET_VARS_H
