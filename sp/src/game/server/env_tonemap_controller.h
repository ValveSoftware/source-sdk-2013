//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Note that this header exists in the Alien Swarm SDK, but not in stock Source SDK 2013.
// Although technically a new Mapbase file, it only serves to move otherwise identical code,
// so most code and repo conventions will pretend it was always there.
//
// --------------------------------------------------------------------
//
// Purpose: 
//
//=============================================================================//

#ifndef ENV_TONEMAP_CONTROLLER_H
#define ENV_TONEMAP_CONTROLLER_H

#include "triggers.h"

// 0 - eyes fully closed / fully black
// 1 - nominal 
// 16 - eyes wide open / fully white

#ifdef MAPBASE // From Alien Swarm SDK
// Spawn Flags
#define SF_TONEMAP_MASTER			0x0001
#endif

//-----------------------------------------------------------------------------
// Purpose: Entity that controls player's tonemap
//-----------------------------------------------------------------------------
class CEnvTonemapController : public CPointEntity
{
	DECLARE_CLASS( CEnvTonemapController, CPointEntity );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	void	Spawn( void );
	int		UpdateTransmitState( void );
	void	UpdateTonemapScaleBlend( void );

#ifdef MAPBASE
	bool	IsMaster( void ) const					{ return HasSpawnFlags( SF_TONEMAP_MASTER ); } // From Alien Swarm SDK
	
	bool	KeyValue( const char *szKeyName, const char *szValue );
#endif

	// Inputs
	void	InputSetTonemapScale( inputdata_t &inputdata );
	void	InputBlendTonemapScale( inputdata_t &inputdata );
	void	InputSetTonemapRate( inputdata_t &inputdata );
	void	InputSetAutoExposureMin( inputdata_t &inputdata );
	void	InputSetAutoExposureMax( inputdata_t &inputdata );
	void	InputUseDefaultAutoExposure( inputdata_t &inputdata );
	void	InputSetBloomScale( inputdata_t &inputdata );
	void	InputUseDefaultBloomScale( inputdata_t &inputdata );
	void	InputSetBloomScaleRange( inputdata_t &inputdata );

private:
	float	m_flBlendTonemapStart;		// HDR Tonemap at the start of the blend
	float	m_flBlendTonemapEnd;		// Target HDR Tonemap at the end of the blend
	float	m_flBlendEndTime;			// Time at which the blend ends
	float	m_flBlendStartTime;			// Time at which the blend started

#ifdef MAPBASE // From Alien Swarm SDK
public:
#endif
	CNetworkVar( bool, m_bUseCustomAutoExposureMin );
	CNetworkVar( bool, m_bUseCustomAutoExposureMax );
	CNetworkVar( bool, m_bUseCustomBloomScale );
	CNetworkVar( float, m_flCustomAutoExposureMin );
	CNetworkVar( float, m_flCustomAutoExposureMax );
	CNetworkVar( float, m_flCustomBloomScale);
	CNetworkVar( float, m_flCustomBloomScaleMinimum);
};

#ifdef MAPBASE // From Alien Swarm SDK
//--------------------------------------------------------------------------------------------------------
class CTonemapTrigger : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTonemapTrigger, CBaseTrigger );
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void StartTouch( CBaseEntity *other );
	virtual void EndTouch( CBaseEntity *other );

	CBaseEntity *GetTonemapController( void ) const;

private:
	string_t m_tonemapControllerName;
	EHANDLE m_hTonemapController;
};


//--------------------------------------------------------------------------------------------------------
inline CBaseEntity *CTonemapTrigger::GetTonemapController( void ) const
{
	return m_hTonemapController.Get();
}


//--------------------------------------------------------------------------------------------------------
// Tonemap Controller System.
class CTonemapSystem : public CAutoGameSystem
{
public:

	// Creation/Init.
	CTonemapSystem( char const *name ) : CAutoGameSystem( name ) 
	{
		m_hMasterController = NULL;
	}

	~CTonemapSystem()
	{
		m_hMasterController = NULL;
	}

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	CBaseEntity *GetMasterTonemapController( void ) const;

private:

	EHANDLE m_hMasterController;
};


//--------------------------------------------------------------------------------------------------------
inline CBaseEntity *CTonemapSystem::GetMasterTonemapController( void ) const
{
	return m_hMasterController.Get();
}

//--------------------------------------------------------------------------------------------------------
CTonemapSystem *TheTonemapSystem( void );
#endif

#endif //ENV_TONEMAP_CONTROLLER_H