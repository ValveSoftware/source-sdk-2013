//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SCRIPT_INTRO_H
#define SCRIPT_INTRO_H
#ifdef _WIN32
#pragma once
#endif


class CPointCamera;


//-----------------------------------------------------------------------------
// Purpose: An entity that's used to control the intro sequence
//-----------------------------------------------------------------------------
class CScriptIntro : public CBaseEntity
{
	DECLARE_CLASS(CScriptIntro, CBaseEntity);
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual void	Spawn( void );
	virtual void	Precache();
	virtual void	Activate( void );
	virtual int 	UpdateTransmitState(void);

	// Inputs
	void	InputSetPlayerViewEntity( inputdata_t &inputdata );
	void	InputSetCameraViewEntity( inputdata_t &inputdata );
	void	InputSetBlendMode( inputdata_t &inputdata );
	void	InputSetNextBlendMode( inputdata_t &inputdata );
	void	InputSetNextFOV( inputdata_t &inputdata );
	void	InputSetFOVBlendTime( inputdata_t &inputdata );
	void	InputSetFOV( inputdata_t &inputdata );
	void	InputSetNextBlendTime( inputdata_t &inputdata );
	void	InputActivate( inputdata_t &inputdata );
	void	InputDeactivate( inputdata_t &inputdata );

	void	InputFadeTo( inputdata_t &inputdata );
	void	InputSetFadeColor( inputdata_t &inputdata );

	bool	GetIncludedPVSOrigin( Vector *pOrigin, CBaseEntity **ppCamera );

private:
	// Think func used to finish the blend off
	void BlendComplete( );

private:
	CNetworkVar( Vector, m_vecPlayerView );
	CNetworkVar( QAngle, m_vecPlayerViewAngles );
	CNetworkVar( Vector, m_vecCameraView );
	CNetworkVar( QAngle, m_vecCameraViewAngles );
	CNetworkVar( int, m_iBlendMode );
	CNetworkVar( int, m_iNextBlendMode );
	CNetworkVar( float, m_flNextBlendTime );
	CNetworkVar( float, m_flBlendStartTime );
	CNetworkVar( int, m_iStartFOV );
	CNetworkVar( bool, m_bActive );

	// Fov & fov blends
	CNetworkVar( int, m_iNextFOV );
	CNetworkVar( float, m_flNextFOVBlendTime );
	CNetworkVar( float, m_flFOVBlendStartTime );
	CNetworkVar( int, m_iFOV );
	CNetworkVar( bool, m_bAlternateFOV );

	// Fades
	CNetworkArray( float, m_flFadeColor, 3 );
	CNetworkVar( float, m_flFadeAlpha);
	CNetworkVar( float, m_flFadeDuration );

	CNetworkVar( EHANDLE, m_hCameraEntity );

	int m_iQueuedBlendMode;
	int m_iQueuedNextBlendMode;
};

extern CHandle<CScriptIntro> g_hIntroScript;

#endif // SCRIPT_INTRO_H
