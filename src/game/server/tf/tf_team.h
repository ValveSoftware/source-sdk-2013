//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================
#ifndef TF_TEAM_H
#define TF_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "team.h"
#include "tf_shareddefs.h"

class CBaseObject;

//=============================================================================
// TF Teams.
//
class CTFTeam : public CTeam
{
	DECLARE_CLASS( CTFTeam, CTeam );
	DECLARE_SERVERCLASS();

public:

	CTFTeam();

	// Classes.
//	int				GetNumOfClass( TFClass iClass );

	// CTeam
	virtual void	AddPlayer( CBasePlayer *pPlayer );
	virtual void	RemovePlayer( CBasePlayer *pPlayer );

	// TF Teams.
//	CTFTeam			*GetEnemyTeam();
	void			SetColor( color32 color );
	color32			GetColor( void );

	// Score.
	void			ShowScore( CBasePlayer *pPlayer );

	// Objects.
	void			AddObject( CBaseObject *pObject );
	void			RemoveObject( CBaseObject *pObject );
	bool			IsObjectOnTeam( CBaseObject *pObject ) const;
	int				GetNumObjects( int iObjectType = -1 );
	CBaseObject		*GetObject( int num );

	// Flag Captures
	int				GetFlagCaptures( void ) { return m_nFlagCaptures; }
	int				GetTotalFlagCaptures( void ) const { return m_nTotalFlagCaptures; }
	void			SetFlagCaptures( int nCaptures ) { m_nFlagCaptures = nCaptures; }
	void			IncrementFlagCaptures( void ) { m_nFlagCaptures++; m_nTotalFlagCaptures++; }

	// Roles
	void			SetRole( int iTeamRole ) { m_iRole = iTeamRole; }
	int				GetRole( void ) { return m_iRole; }

	// KOTH Timers
	void			AddKOTHTime( int nTime ) { m_flTotalSecondsKOTHPointOwned += nTime; }
	float			GetKOTHTime() const { return m_flTotalSecondsKOTHPointOwned; }

	// PLR Track
	void			AddPLRTrack( float flPercentTraveled ) { m_flTotalPLRTrackPercentTraveled += flPercentTraveled; }
	float			GetTotalPLRTrackPercentTraveled() const { return m_flTotalPLRTrackPercentTraveled; }

	bool			SetTeamLeader( CBasePlayer *pPlayer );
	CBasePlayer		*GetTeamLeader( void );

private:
	
	color32						m_TeamColor;
	CUtlVector< CHandle<CBaseObject> >	m_aObjects;			// List of team objects.

	CNetworkVar( int, m_nFlagCaptures );
	CNetworkVar( int, m_iRole );
	int m_nTotalFlagCaptures;
	float m_flTotalSecondsKOTHPointOwned;
	float m_flTotalPLRTrackPercentTraveled;

	CNetworkHandle( CBasePlayer, m_hLeader );
};

class CTFTeamManager
{
public:

	CTFTeamManager();

	// Creation/Destruction.
	bool	Init( void );
	void    Shutdown( void );

	bool	IsValidTeam( int iTeam );
	int		GetTeamCount( void );
	CTFTeam *GetTeam( int iTeam );
	CTFTeam *GetSpectatorTeam();

	color32 GetUndefinedTeamColor( void );

	void AddTeamScore( int iTeam, int iScoreToAdd );

	void IncrementFlagCaptures( int iTeam );
	int GetFlagCaptures( int iTeam );

	// Screen prints.
	void PlayerCenterPrint( CBasePlayer *pPlayer, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL );
	void TeamCenterPrint( int iTeam, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL );
	void PlayerTeamCenterPrint( CBasePlayer *pPlayer, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL );

	// Vox

private:

	int		Create( const char *pName, color32 color );

private:

	color32	m_UndefinedTeamColor;
};

extern CTFTeamManager *TFTeamMgr();
extern CTFTeam *GetGlobalTFTeam( int iIndex );

#endif // TF_TEAM_H
