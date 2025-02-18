//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TRAINS_H
#define TRAINS_H
#ifdef _WIN32
#pragma once
#endif


#include "entityoutput.h"
#include "pathtrack.h"


// Spawnflags of CPathCorner
#define SF_CORNER_WAITFORTRIG	0x001
#define SF_CORNER_TELEPORT		0x002

// Tracktrain spawn flags
#define SF_TRACKTRAIN_NOPITCH					0x0001
#define SF_TRACKTRAIN_NOCONTROL					0x0002
#define SF_TRACKTRAIN_FORWARDONLY				0x0004
#define SF_TRACKTRAIN_PASSABLE					0x0008
#define SF_TRACKTRAIN_FIXED_ORIENTATION			0x0010
#define SF_TRACKTRAIN_HL1TRAIN					0x0080
#define SF_TRACKTRAIN_USE_MAXSPEED_FOR_PITCH	0x0100
#define SF_TRACKTRAIN_UNBLOCKABLE_BY_PLAYER		0x0200

#define TRAIN_ACTIVE	0x80 
#define TRAIN_NEW		0xc0
#define TRAIN_OFF		0x00
#define TRAIN_NEUTRAL	0x01
#define TRAIN_SLOW		0x02
#define TRAIN_MEDIUM	0x03
#define TRAIN_FAST		0x04 
#define TRAIN_BACK		0x05


enum TrainVelocityType_t
{
        TrainVelocity_Instantaneous = 0,
        TrainVelocity_LinearBlend,
        TrainVelocity_EaseInEaseOut,
};


enum TrainOrientationType_t
{
        TrainOrientation_Fixed = 0,
        TrainOrientation_AtPathTracks,
        TrainOrientation_LinearBlend,
        TrainOrientation_EaseInEaseOut,
};

class CFuncTrackTrain : public CBaseEntity
{
	DECLARE_CLASS( CFuncTrackTrain, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_ENT_SCRIPTDESC();

public:
	CFuncTrackTrain();

	void Spawn( void );
	bool CreateVPhysics( void );
	void Precache( void );
	void UpdateOnRemove();
	void MoveDone();

	virtual int OnTakeDamage( const CTakeDamageInfo &info );

	void Blocked( CBaseEntity *pOther );
	bool KeyValue( const char *szKeyName, const char *szValue );

	virtual int DrawDebugTextOverlays();
	void DrawDebugGeometryOverlays();

	void Next( void );
	void Find( void );
	void NearestPath( void );
	void DeadEnd( void );

	void SetTrack( CPathTrack *track ) { m_ppath = track->Nearest(GetLocalOrigin()); }
	void SetControls( CBaseEntity *pControls );
	bool OnControls( CBaseEntity *pControls );

	void SoundStop( void );
	void SoundUpdate( void );

	void Start( void );
	void Stop( void );

	bool IsDirForward();
	void SetDirForward( bool bForward );
	void SetSpeed( float flSpeed, bool bAccel = false );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void SetSpeedDirAccel( float flNewSpeed );
	
	// Input handlers
	void InputSetSpeed( inputdata_t &inputdata );
	void InputSetSpeedDir( inputdata_t &inputdata );
	void InputSetSpeedReal( inputdata_t &inputdata );
	void InputStop( inputdata_t &inputdata );
	void InputResume( inputdata_t &inputdata );
	void InputReverse( inputdata_t &inputdata );
	void InputStartForward( inputdata_t &inputdata );
	void InputStartBackward( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputSetSpeedDirAccel( inputdata_t &inputdata );
	void InputTeleportToPathTrack( inputdata_t &inputdata );
	void InputSetSpeedForwardModifier( inputdata_t &inputdata );

	static CFuncTrackTrain *Instance( edict_t *pent );

#ifdef TF_DLL
	int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
#endif

	DECLARE_DATADESC();

	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_DIRECTIONAL_USE | FCAP_USE_ONGROUND; }

	virtual void	OnRestore( void );

	float GetMaxSpeed() const { return m_maxSpeed; }
	float GetCurrentSpeed() const { return m_flSpeed; }
	float GetDesiredSpeed() const { return m_flDesiredSpeed;}

	virtual bool IsBaseTrain( void ) const { return true; }
	Vector ScriptGetFuturePosition( float flSeconds, float flMinSpeed );

	void SetSpeedForwardModifier( float flModifier );
	void SetBlockDamage( float flDamage ) { m_flBlockDamage = flDamage; }
	void SetDamageChild( bool bDamageChild ) { m_bDamageChild = bDamageChild; }

private:

	void ArriveAtNode( CPathTrack *pNode );
	void FirePassInputs( CPathTrack *pStart, CPathTrack *pEnd, bool forward );

public:

	// UNDONE: Add accessors?
	CPathTrack	*m_ppath;
	float		m_length;
	
#ifdef HL1_DLL	
	bool		m_bOnTrackChange;		// we don't want to find a new node if we restore while 
										// riding on a func_trackchange
#endif

private:

	TrainVelocityType_t GetTrainVelocityType();
	void UpdateTrainVelocity( CPathTrack *pnext, CPathTrack *pNextNext, const Vector &nextPos, float flInterval );

	TrainOrientationType_t GetTrainOrientationType();
	void UpdateTrainOrientation( CPathTrack *pnext, CPathTrack *pNextNext, const Vector &nextPos, float flInterval );
	void UpdateOrientationAtPathTracks( CPathTrack *pnext, CPathTrack *pNextNext, const Vector &nextPos, float flInterval );
	void UpdateOrientationBlend( TrainOrientationType_t eOrientationType, CPathTrack *pPrev, CPathTrack *pNext, const Vector &nextPos, float flInterval );
	void DoUpdateOrientation( const QAngle &curAngles, const QAngle &angles, float flInterval );

	void TeleportToPathTrack( CPathTrack *pTeleport );


	Vector		m_controlMins;
	Vector		m_controlMaxs;
	Vector		m_lastBlockPos;				// These are used to build a heuristic decision about being temporarily blocked by physics objects
	int			m_lastBlockTick;			// ^^^^^^^
	float		m_flVolume;
	float		m_flBank;
	float		m_oldSpeed;
	float		m_flBlockDamage;			// Damage to inflict when blocked.
	float		m_height;
	float		m_maxSpeed;
	float		m_dir;


	string_t	m_iszSoundMove;				// Looping sound to play while moving. Pitch shifted based on speed.
	string_t	m_iszSoundMovePing;			// Ping sound to play while moving. Interval decreased based on speed.
	string_t	m_iszSoundStart;			// Sound to play when starting to move.
	string_t	m_iszSoundStop;				// Sound to play when stopping.

	float		m_flMoveSoundMinTime;		// The most often to play the move 'ping' sound (used at max speed)
	float		m_flMoveSoundMaxTime;		// The least often to play the move 'ping' sound (used approaching zero speed)
	float		m_flNextMoveSoundTime;

	int			m_nMoveSoundMinPitch;		// The sound pitch to approach as we come to a stop
	int			m_nMoveSoundMaxPitch;		// The sound pitch to approach as we approach our max speed (actually, it's hardcoded to 1000 in/sec)

	TrainOrientationType_t m_eOrientationType;
	TrainVelocityType_t m_eVelocityType;
	bool		m_bSoundPlaying;

	COutputEvent m_OnStart,m_OnNext; 

	bool		m_bManualSpeedChanges;		// set when we want to send entity IO to govern speed and obey our TrainVelocityType_t
	float		m_flDesiredSpeed;			// target speed, when m_bManualSpeedChanges is set
	float		m_flSpeedChangeTime;
	float		m_flAccelSpeed;
	float		m_flDecelSpeed;
	bool		m_bAccelToSpeed;

	float		m_flNextMPSoundTime;
	
	float		m_flSpeedForwardModifier;
	float		m_flUnmodifiedDesiredSpeed;

	bool		m_bDamageChild;
};


#endif // TRAINS_H
