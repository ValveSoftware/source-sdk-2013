//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DOORS_H
#define DOORS_H
#pragma once


#include "locksounds.h"
#include "entityoutput.h"

//Since I'm here, might as well explain how these work.  Base.fgd is the file that connects
//flags to entities.  It is full of lines with this number, a label, and a default value.
//Voila, dynamicly generated checkboxes on the Flags tab of Entity Properties.

// doors
#define SF_DOOR_ROTATE_YAW			0		// yaw by default
#define	SF_DOOR_START_OPEN_OBSOLETE	1
#define SF_DOOR_ROTATE_BACKWARDS	2
#define SF_DOOR_NONSOLID_TO_PLAYER	4
#define SF_DOOR_PASSABLE			8
#define SF_DOOR_ONEWAY				16
#define	SF_DOOR_NO_AUTO_RETURN		32
#define SF_DOOR_ROTATE_ROLL			64
#define SF_DOOR_ROTATE_PITCH		128
#define SF_DOOR_PUSE				256	// door can be opened by player's use button.
#define SF_DOOR_NONPCS				512	// NPC can't open
#define SF_DOOR_PTOUCH				1024 // player touch opens
#define SF_DOOR_LOCKED				2048	// Door is initially locked
#define SF_DOOR_SILENT				4096	// Door plays no audible sound, and does not alert NPCs when opened
#define	SF_DOOR_USE_CLOSES			8192	// Door can be +used to close before its autoreturn delay has expired.
#define SF_DOOR_SILENT_TO_NPCS		16384	// Does not alert NPC's when opened.
#define SF_DOOR_IGNORE_USE			32768	// Completely ignores player +use commands.
#define SF_DOOR_NEW_USE_RULES		65536	// For func_door entities, behave more like prop_door_rotating with respect to +USE (changelist 242482)


enum FuncDoorSpawnPos_t
{
	FUNC_DOOR_SPAWN_CLOSED = 0,
	FUNC_DOOR_SPAWN_OPEN,
};


class CBaseDoor : public CBaseToggle
{
public:
	DECLARE_CLASS( CBaseDoor, CBaseToggle );

	DECLARE_SERVERCLASS();

	void Spawn( void );
	void Precache( void );
	bool CreateVPhysics();
	bool KeyValue( const char *szKeyName, const char *szValue );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual void StartBlocked( CBaseEntity *pOther );
	virtual void Blocked( CBaseEntity *pOther );
	virtual void EndBlocked( void );

	void Activate( void );

	virtual int	ObjectCaps( void ) 
	{
		int flags = BaseClass::ObjectCaps();
		if ( HasSpawnFlags( SF_DOOR_PUSE ) )
			return flags | FCAP_IMPULSE_USE | FCAP_USE_IN_RADIUS;

		return flags;
	};

	DECLARE_DATADESC();

	// This is ONLY used by the node graph to test movement through a door
	void InputSetToggleState( inputdata_t &inputdata );
	virtual void SetToggleState( int state );

	virtual bool IsRotatingDoor() { return false; }
	virtual bool ShouldSavePhysics();
	// used to selectivly override defaults
	void DoorTouch( CBaseEntity *pOther );

	// local functions
	int DoorActivate( );
	void DoorGoUp( void );
	void DoorGoDown( void );
	void DoorHitTop( void );
	void DoorHitBottom( void );
	void UpdateAreaPortals( bool isOpen );
	void Unlock( void );
	void Lock( void );
	int GetDoorMovementGroup( CBaseDoor *pDoorList[], int listMax );

	// Input handlers
	void InputClose( inputdata_t &inputdata );
	void InputLock( inputdata_t &inputdata );
	void InputOpen( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputUnlock( inputdata_t &inputdata );
	void InputSetSpeed( inputdata_t &inputdata );

	Vector m_vecMoveDir;		// The direction of motion for linear moving doors.

	locksound_t m_ls;			// door lock sounds
	
	byte	m_bLockedSentence;	
	byte	m_bUnlockedSentence;

	bool	m_bForceClosed;			// If set, always close, even if we're blocked.
	bool	m_bDoorGroup;
	bool	m_bLocked;				// Whether the door is locked
	bool	m_bIgnoreDebris;
	bool	m_bIgnoreNonPlayerEntsOnBlock;	// Non-player entities should never block.  This variable needs more letters.
	
	FuncDoorSpawnPos_t m_eSpawnPosition;

	float	m_flBlockDamage;		// Damage inflicted when blocked.
	string_t	m_NoiseMoving;		//Start/Looping sound
	string_t	m_NoiseArrived;		//End sound
	string_t	m_NoiseMovingClosed;		//Start/Looping sound
	string_t	m_NoiseArrivedClosed;		//End sound
	string_t	m_ChainTarget;		///< Entity name to pass Touch and Use events to

	CNetworkVar( float, m_flWaveHeight );

	// Outputs
	COutputEvent m_OnBlockedClosing;		// Triggered when the door becomes blocked while closing.
	COutputEvent m_OnBlockedOpening;		// Triggered when the door becomes blocked while opening.
	COutputEvent m_OnUnblockedClosing;		// Triggered when the door becomes unblocked while closing.
	COutputEvent m_OnUnblockedOpening;		// Triggered when the door becomes unblocked while opening.
	COutputEvent m_OnFullyClosed;			// Triggered when the door reaches the fully closed position.
	COutputEvent m_OnFullyOpen;				// Triggered when the door reaches the fully open position.
	COutputEvent m_OnClose;					// Triggered when the door is told to close.
	COutputEvent m_OnOpen;					// Triggered when the door is told to open.
	COutputEvent m_OnLockedUse;				// Triggered when the user tries to open a locked door.

	void			StartMovingSound( void );
	virtual void	StopMovingSound( void );
	void			MovingSoundThink( void );
#ifdef HL1_DLL
	bool		PassesBlockTouchFilter(CBaseEntity *pOther);
	string_t	m_iBlockFilterName;
	EHANDLE		m_hBlockFilter;
#endif
	
	bool		ShouldLoopMoveSound( void ) { return m_bLoopMoveSound; }
	bool		m_bLoopMoveSound;			// Move sound loops until stopped

	virtual bool ShouldBlockNav() const OVERRIDE { return false; }

private:
	void ChainUse( void );	///< Chains +use on through to m_ChainTarget
	void ChainTouch( CBaseEntity *pOther );	///< Chains touch on through to m_ChainTarget
	void SetChaining( bool chaining )	{ m_isChaining = chaining; }	///< Latch to prevent recursion
	bool m_isChaining;

	void CloseAreaPortalsThink( void );	///< Delays turning off area portals when closing doors to prevent visual artifacts
};

#endif // DOORS_H
