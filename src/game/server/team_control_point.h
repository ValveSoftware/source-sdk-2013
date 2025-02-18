//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TEAM_CONTROL_POINT_H
#define TEAM_CONTROL_POINT_H
#ifdef _WIN32
#pragma once
#endif

#include "basemultiplayerplayer.h"

// Spawnflags
#define SF_CAP_POINT_HIDEFLAG		(1<<0)
#define SF_CAP_POINT_HIDE_MODEL		(1<<1)
#define SF_CAP_POINT_HIDE_SHADOW	(1<<2)
#define SF_CAP_POINT_NO_CAP_SOUNDS	(1<<3)
#define SF_CAP_POINT_BOTS_IGNORE	(1<<4)
//#define SF_CAP_POINT_NO_ANNOUNCER	(1<<4) Unused?

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTeamControlPoint : public CBaseAnimating
{
	DECLARE_CLASS( CTeamControlPoint, CBaseAnimating );
public:
	DECLARE_DATADESC();

	CTeamControlPoint();

	// Derived, game-specific control points must override these functions
public:
	// Used to find game specific entities
	virtual const char *GetControlPointMasterName( void ) { return "team_control_point_master"; }

public:
	virtual void Spawn( void );
	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	virtual void Precache( void );
	virtual int  DrawDebugTextOverlays( void );

	//Inputs
	inline void Enable( inputdata_t &input )	{ SetActive( false ); }
	inline void Disable( inputdata_t &input )	{ SetActive( true ); }
	void		InputReset( inputdata_t &input );
	void		InputSetOwner( inputdata_t &input );
	void		InputShowModel( inputdata_t &input );
	void		InputHideModel( inputdata_t &input );
	void		InputRoundActivate( inputdata_t &inputdata );
	void		InputSetLocked( inputdata_t &inputdata );
	void		InputSetUnlockTime( inputdata_t &inputdata );

	// Owner handling
	void		ForceOwner( int iTeam ); // used when selecting a specific round to play
	void		SetOwner( int iCapTeam, bool bMakeSound = true, int iNumCappers = 0, int *iCappingPlayers = NULL );
	int			GetOwner( void ) const;
	int			GetDefaultOwner( void ) const;
	bool		RandomOwnerOnRestart( void ){ return m_bRandomOwnerOnRestart; }

	void		SetActive( bool active );
	inline bool	IsActive( void ) { return m_bActive; }
	void		AnimThink( void );

	bool		PointIsVisible( void ) { return !( FBitSet( m_spawnflags, SF_CAP_POINT_HIDEFLAG ) ); }

	inline const char *GetName( void ) { return STRING(m_iszPrintName); }
	int			GetCPGroup( void );
	int			GetPointIndex( void ) { return m_iPointIndex; }
	void		SetPointIndex( int index ) { m_iPointIndex = index; }

	int			GetWarnOnCap( void ) { return m_iWarnOnCap; }
	string_t	GetWarnSound( void ) { return m_iszWarnSound; }

	int			GetTeamIcon( int iTeam );

	int			GetCurrentHudIconIndex( void );
	int			GetHudIconIndexForTeam( int iGameTeam );
	int			GetHudOverlayIndexForTeam( int iGameTeam );
	int			GetPreviousPointForTeam( int iGameTeam, int iPrevPoint );

	void		SetCappersRequiredForTeam( int iGameTeam, int iCappers );

	void		CaptureBlocked( CBaseMultiplayerPlayer *pPlayer, CBaseMultiplayerPlayer *pVictim );

	int			PointValue( void );

	bool		HasBeenContested( void ) const;				// return true if this point has ever been contested, false if the enemy has never contested this point yet
	float		LastContestedAt( void );
	void		SetLastContestedAt( float flTime );

	void		UpdateCapPercentage( void );
	float		GetTeamCapPercentage( int iTeam );

	// The specified player took part in capping this point.
	virtual void PlayerCapped( CBaseMultiplayerPlayer *pPlayer );

	// The specified player blocked the enemy team from capping this point.
	virtual void PlayerBlocked( CBaseMultiplayerPlayer *pPlayer );

	void		CaptureEnd( void );
	void		CaptureStart( int iCapTeam, int iNumCappingPlayers, int *pCappingPlayers );
	void		CaptureInterrupted( bool bBlocked );

	virtual void StopLoopingSounds( void );

	bool		IsLocked( void ){ return m_bLocked; }
	bool		ShouldBotsIgnore( void ) { return m_bBotsIgnore; }

	void EXPORT UnlockThink( void );

private:
	void		SendCapString( int iCapTeam, int iNumCappingPlayers, int *pCappingPlayers );
	void		InternalSetOwner( int iCapTeam, bool bMakeSound = true, int iNumCappers = 0, int *iCappingPlayers = NULL );
	void		HandleScoring( int iTeam );
	void		InternalSetLocked( bool bLocked );

	int			m_iTeam;			
	int			m_iDefaultOwner;			// Team that initially owns the cap point
	int			m_iIndex;					// The index of this point in the controlpointArray
	int			m_iWarnOnCap;				// Warn the team that owns the control point when the opposing team starts to capture it.
	string_t	m_iszPrintName;
	string_t	m_iszWarnSound;				// Sound played if the team needs to be warned about this point being captured
	bool		m_bRandomOwnerOnRestart;	// Do we want to randomize the owner after a restart?
	bool		m_bLocked;
	float		m_flUnlockTime;				// Time to unlock

	// We store a copy of this data for each team, +1 for the un-owned state.
	struct perteamdata_t
	{
		perteamdata_t()
		{
			iszCapSound = NULL_STRING;
			iszModel = NULL_STRING;
			iModelBodygroup = -1;
			iIcon = 0;
			iszIcon = NULL_STRING;
			iOverlay = 0;
			iszOverlay = NULL_STRING;
			iPlayersRequired = 0;
			iTimedPoints = 0;
			for ( int i = 0; i < MAX_PREVIOUS_POINTS; i++ )
			{
				iszPreviousPoint[i] = NULL_STRING;
			}
			iTeamPoseParam = 0;
		}

		string_t	iszCapSound;
		string_t	iszModel;
		int			iModelBodygroup;
		int			iTeamPoseParam;
		int			iIcon;
		string_t	iszIcon;
		int			iOverlay;
		string_t	iszOverlay;
		int			iPlayersRequired;
		int			iTimedPoints;
		string_t	iszPreviousPoint[MAX_PREVIOUS_POINTS];
	};
	CUtlVector<perteamdata_t>	m_TeamData;

	COutputEvent	m_OnCapReset;

	COutputEvent	m_OnCapTeam1;
	COutputEvent	m_OnCapTeam2;

	COutputEvent	m_OnOwnerChangedToTeam1;
	COutputEvent	m_OnOwnerChangedToTeam2;

	COutputEvent	m_OnRoundStartOwnedByTeam1;
	COutputEvent	m_OnRoundStartOwnedByTeam2;

	COutputEvent	m_OnUnlocked;

	int			m_bPointVisible;		//should this capture point be visible on the hud?
	int			m_iPointIndex;			//the mapper set index value of this control point

	int			m_iCPGroup;			//the group that this control point belongs to
	bool		m_bActive;			//

	string_t	m_iszName;				//Name used in cap messages

	bool		m_bStartDisabled;

	float		m_flLastContestedAt;

	CSoundPatch *m_pCaptureInProgressSound;
	string_t	m_iszCaptureStartSound;
	string_t	m_iszCaptureEndSound;
	string_t	m_iszCaptureInProgress;
	string_t	m_iszCaptureInterrupted;
	bool		m_bBotsIgnore;
};

#endif // TEAM_CONTROL_POINT_H
