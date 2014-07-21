//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A base class for model-based doors. The exact movement required to
//			open or close the door is not dictated by this class, only that
//			the door has open, closed, opening, and closing states.
//
//			Doors must satisfy these requirements:
//
//			- Derived classes must support being opened by NPCs.
//			- Never autoclose in the face of a player.
//			- Never close into an NPC.
//
//=============================================================================//

#ifndef BASEPROPDOOR_H
#define BASEPROPDOOR_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"
#include "locksounds.h"
#include "entityoutput.h"

extern ConVar g_debug_doors;

struct opendata_t
{
	Vector vecStandPos;		// Where the NPC should stand.
	Vector vecFaceDir;		// What direction the NPC should face.
	Activity eActivity;		// What activity the NPC should play.
};


abstract_class CBasePropDoor : public CDynamicProp
{
public:

	DECLARE_CLASS( CBasePropDoor, CDynamicProp );
	DECLARE_SERVERCLASS();

	CBasePropDoor( void );

	void Spawn();
	void Precache();
	void Activate();
	int	ObjectCaps();

	void HandleAnimEvent( animevent_t *pEvent );

	// Base class services.
	// Do not make the functions in this block virtual!!
	// {
	inline bool IsDoorOpen();
	inline bool IsDoorAjar();
	inline bool IsDoorOpening();
	inline bool IsDoorClosed();
	inline bool IsDoorClosing();
	inline bool IsDoorLocked();
	inline bool IsDoorBlocked() const;
	inline bool IsNPCOpening(CAI_BaseNPC *pNPC);
	inline bool IsPlayerOpening();
	inline bool IsOpener(CBaseEntity *pEnt);

	bool NPCOpenDoor(CAI_BaseNPC *pNPC);
	bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );
	// }

	// Implement these in your leaf class.
	// {
	virtual bool DoorCanClose( bool bAutoClose ) { return true; }
	virtual bool DoorCanOpen( void ) { return true; }

	virtual void GetNPCOpenData(CAI_BaseNPC *pNPC, opendata_t &opendata) = 0;
	virtual float GetOpenInterval(void) = 0;
	// }

protected:

	enum DoorState_t
	{
		DOOR_STATE_CLOSED = 0,
		DOOR_STATE_OPENING,
		DOOR_STATE_OPEN,
		DOOR_STATE_CLOSING,
		DOOR_STATE_AJAR,
	};

	// dvs: FIXME: make these private
	void DoorClose();

	CBasePropDoor *GetMaster( void ) { return m_hMaster; }
	bool HasSlaves( void ) { return ( m_hDoorList.Count() > 0 ); }

	inline void SetDoorState( DoorState_t eDoorState );

	float m_flAutoReturnDelay;	// How many seconds to wait before automatically closing, -1 never closes automatically.
	CUtlVector< CHandle< CBasePropDoor > >	m_hDoorList;	// List of doors linked to us

	inline CBaseEntity *GetActivator();

private:

	// Implement these in your leaf class.
	// {
	// Called when the door becomes fully open.
	virtual void OnDoorOpened() {}

	// Called when the door becomes fully closed.
	virtual void OnDoorClosed() {}

	// Called to tell the door to start opening.
	virtual void BeginOpening(CBaseEntity *pOpenAwayFrom) = 0;

	// Called to tell the door to start closing.
	virtual void BeginClosing( void ) = 0;

	// Called when blocked to tell the door to stop moving.
	virtual void DoorStop( void ) = 0;

	// Called when blocked to tell the door to continue moving.
	virtual void DoorResume( void ) = 0;
	
	// Called to send the door instantly to its spawn positions.
	virtual void DoorTeleportToSpawnPosition() = 0;
	// }

private:

	// Main entry points for the door base behaviors.
	// Do not make the functions in this block virtual!!
	// {
	bool DoorActivate();
	void DoorOpen( CBaseEntity *pOpenAwayFrom );
	void OpenIfUnlocked(CBaseEntity *pActivator, CBaseEntity *pOpenAwayFrom);

	void DoorOpenMoveDone();
	void DoorCloseMoveDone();
	void DoorAutoCloseThink();

	void Lock();
	void Unlock();

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void OnUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	inline bool WillAutoReturn() { return m_flAutoReturnDelay != -1; }

	void StartBlocked(CBaseEntity *pOther);
	void OnStartBlocked( CBaseEntity *pOther );
	void MasterStartBlocked( CBaseEntity *pOther );

	void Blocked(CBaseEntity *pOther);
	void EndBlocked(void);
	void OnEndBlocked( void );

	void UpdateAreaPortals(bool bOpen);

	// Input handlers
	void InputClose(inputdata_t &inputdata);
	void InputLock(inputdata_t &inputdata);
	void InputOpen(inputdata_t &inputdata);
	void InputOpenAwayFrom(inputdata_t &inputdata);
	void InputToggle(inputdata_t &inputdata);
	void InputUnlock(inputdata_t &inputdata);

	void SetDoorBlocker( CBaseEntity *pBlocker );

	void SetMaster( CBasePropDoor *pMaster ) { m_hMaster = pMaster; }

	void CalcDoorSounds();
	// }

	int		m_nHardwareType;
	
	DoorState_t m_eDoorState;	// Holds whether the door is open, closed, opening, or closing.

	locksound_t m_ls;			// The sounds the door plays when being locked, unlocked, etc.
	EHANDLE		m_hActivator;		
	
	bool	m_bLocked;				// True if the door is locked.
	EHANDLE	m_hBlocker;				// Entity blocking the door currently
	bool	m_bFirstBlocked;		// Marker for being the first door (in a group) to be blocked (needed for motion control)

	bool m_bForceClosed;			// True if this door must close no matter what.

	string_t m_SoundMoving;
	string_t m_SoundOpen;
	string_t m_SoundClose;

	// dvs: FIXME: can we remove m_flSpeed from CBaseEntity?
	//float m_flSpeed;			// Rotation speed when opening or closing in degrees per second.

	DECLARE_DATADESC();

	string_t m_SlaveName;

	CHandle< CBasePropDoor > m_hMaster;

	static void RegisterPrivateActivities();

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
};


void CBasePropDoor::SetDoorState( DoorState_t eDoorState )
{
	m_eDoorState = eDoorState;
}

bool CBasePropDoor::IsDoorOpen()
{
	return m_eDoorState == DOOR_STATE_OPEN;
}

bool CBasePropDoor::IsDoorAjar()
{
	return ( m_eDoorState == DOOR_STATE_AJAR );
}

bool CBasePropDoor::IsDoorOpening()
{
	return m_eDoorState == DOOR_STATE_OPENING;
}

bool CBasePropDoor::IsDoorClosed()
{
	return m_eDoorState == DOOR_STATE_CLOSED;
}

bool CBasePropDoor::IsDoorClosing()
{
	return m_eDoorState == DOOR_STATE_CLOSING;
}

bool CBasePropDoor::IsDoorLocked()
{
	return m_bLocked;
}

CBaseEntity *CBasePropDoor::GetActivator()
{
	return m_hActivator;
}

bool CBasePropDoor::IsDoorBlocked() const
{
	return ( m_hBlocker != NULL );
}

bool CBasePropDoor::IsNPCOpening( CAI_BaseNPC *pNPC )
{
	return ( pNPC == ( CAI_BaseNPC * )GetActivator() );
}

inline bool CBasePropDoor::IsPlayerOpening()
{
	return ( GetActivator() && GetActivator()->IsPlayer() );
}

inline bool CBasePropDoor::IsOpener(CBaseEntity *pEnt)
{
	return ( GetActivator() == pEnt );
}

#endif // BASEPROPDOOR_H
