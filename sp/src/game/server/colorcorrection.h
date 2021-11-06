//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Note that this header exists in the Alien Swarm SDK, but not in stock Source SDK 2013.
// Although technically a new Mapbase file, it only serves to move otherwise identical code,
// so most code and repo conventions will pretend it was always there.
//
// --------------------------------------------------------------------
//
// Purpose: Color correction entity.
//
//=============================================================================//

#ifndef COLOR_CORRECTION_H
#define COLOR_CORRECTION_H
#ifdef _WIN32
#pragma once
#endif

#include <string.h>
#include "cbase.h"
#ifdef MAPBASE // From Alien Swarm SDK
#include "GameEventListener.h"

// Spawn Flags
#define SF_COLORCORRECTION_MASTER		0x0001
#define SF_COLORCORRECTION_CLIENTSIDE	0x0002
#endif
 
//------------------------------------------------------------------------------
// FIXME: This really should inherit from something	more lightweight
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Purpose : Shadow control entity
//------------------------------------------------------------------------------
class CColorCorrection : public CBaseEntity
{
	DECLARE_CLASS( CColorCorrection, CBaseEntity );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CColorCorrection();

	void Spawn( void );
	int  UpdateTransmitState();
	void Activate( void );

	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

#ifdef MAPBASE // From Alien Swarm SDK
	bool IsMaster( void ) const { return HasSpawnFlags( SF_COLORCORRECTION_MASTER ); }

	bool IsClientSide( void ) const { return HasSpawnFlags( SF_COLORCORRECTION_CLIENTSIDE ); }

	bool IsExclusive( void ) const { return m_bExclusive; }
#endif

	// Inputs
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	void	InputSetFadeInDuration ( inputdata_t &inputdata );
	void	InputSetFadeOutDuration ( inputdata_t &inputdata );
#ifdef MAPBASE
	void	InputSetMinFalloff( inputdata_t &inputdata );
	void	InputSetMaxFalloff( inputdata_t &inputdata );
#endif

private:
	void	FadeIn ( void );
	void	FadeOut ( void );

	void FadeInThink( void );	// Fades lookup weight from Cur->MaxWeight 
	void FadeOutThink( void );	// Fades lookup weight from CurWeight->0.0

	
	
#ifdef MAPBASE // From Alien Swarm SDK
	CNetworkVar( float, m_flFadeInDuration );	// Duration for a full 0->MaxWeight transition
	CNetworkVar( float, m_flFadeOutDuration );	// Duration for a full Max->0 transition
#else
	float	m_flFadeInDuration;		// Duration for a full 0->MaxWeight transition
	float	m_flFadeOutDuration;	// Duration for a full Max->0 transition
#endif
	float	m_flStartFadeInWeight;
	float	m_flStartFadeOutWeight;
	float	m_flTimeStartFadeIn;
	float	m_flTimeStartFadeOut;
	
#ifdef MAPBASE // From Alien Swarm SDK
	CNetworkVar( float, m_flMaxWeight );
#else
	float	m_flMaxWeight;
#endif

	bool	m_bStartDisabled;
	CNetworkVar( bool, m_bEnabled );
#ifdef MAPBASE // From Alien Swarm SDK
	CNetworkVar( bool, m_bMaster );
	CNetworkVar( bool, m_bClientSide );
	CNetworkVar( bool, m_bExclusive );
#endif

	CNetworkVar( float, m_MinFalloff );
	CNetworkVar( float, m_MaxFalloff );
	CNetworkVar( float, m_flCurWeight );
	CNetworkString( m_netlookupFilename, MAX_PATH );

	string_t	m_lookupFilename;
};

#ifdef MAPBASE // From Alien Swarm SDK
//=============================================================================
//
// ColorCorrection Controller System. Just a place to store a master controller
//
class CColorCorrectionSystem : public CAutoGameSystem, public CGameEventListener
{
public:

	// Creation/Init.
	CColorCorrectionSystem( char const *name ) : CAutoGameSystem( name ) 
	{
		m_hMasterController = NULL;
	}

	~CColorCorrectionSystem()
	{
		m_hMasterController = NULL;
	}

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void FireGameEvent( IGameEvent *pEvent );
	CColorCorrection *GetMasterColorCorrection( void )			{ return m_hMasterController; }

private:

	void InitMasterController( void );
	CHandle< CColorCorrection > m_hMasterController;
};

CColorCorrectionSystem *ColorCorrectionSystem( void );
#endif

#endif // COLOR_CORRECTION_H
