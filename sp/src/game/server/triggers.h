//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TRIGGERS_H
#define TRIGGERS_H
#ifdef _WIN32
#pragma once
#endif

#ifdef MAPBASE
#include "baseentity.h"
#else
#include "basetoggle.h"
#endif
#include "entityoutput.h"

//
// Spawnflags
//

enum
{
	SF_TRIGGER_ALLOW_CLIENTS				= 0x01,		// Players can fire this trigger
	SF_TRIGGER_ALLOW_NPCS					= 0x02,		// NPCS can fire this trigger
	SF_TRIGGER_ALLOW_PUSHABLES				= 0x04,		// Pushables can fire this trigger
	SF_TRIGGER_ALLOW_PHYSICS				= 0x08,		// Physics objects can fire this trigger
	SF_TRIGGER_ONLY_PLAYER_ALLY_NPCS		= 0x10,		// *if* NPCs can fire this trigger, this flag means only player allies do so
	SF_TRIGGER_ONLY_CLIENTS_IN_VEHICLES		= 0x20,		// *if* Players can fire this trigger, this flag means only players inside vehicles can 
	SF_TRIGGER_ALLOW_ALL					= 0x40,		// Everything can fire this trigger EXCEPT DEBRIS!
	SF_TRIGGER_ONLY_CLIENTS_OUT_OF_VEHICLES	= 0x200,	// *if* Players can fire this trigger, this flag means only players outside vehicles can 
	SF_TRIG_PUSH_ONCE						= 0x80,		// trigger_push removes itself after firing once
	SF_TRIG_PUSH_AFFECT_PLAYER_ON_LADDER	= 0x100,	// if pushed object is player on a ladder, then this disengages them from the ladder (HL2only)
	SF_TRIG_TOUCH_DEBRIS 					= 0x400,	// Will touch physics debris objects
	SF_TRIGGER_ONLY_NPCS_IN_VEHICLES		= 0X800,	// *if* NPCs can fire this trigger, only NPCs in vehicles do so (respects player ally flag too)
	SF_TRIGGER_DISALLOW_BOTS                = 0x1000,   // Bots are not allowed to fire this trigger
#ifdef MAPBASE
	SF_TRIGGER_ALLOW_ITEMS					= 0x2000,	// MOVETYPE_FLYGRAVITY (Weapons, items, flares, etc.) can fire this trigger
#endif
};

// DVS TODO: get rid of CBaseToggle
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef MAPBASE
#define CBaseToggle CBaseEntity
#endif
class CBaseTrigger : public CBaseToggle
{
	DECLARE_CLASS( CBaseTrigger, CBaseToggle );
#ifdef MAPBASE
#undef CBaseToggle
#endif
public:
	CBaseTrigger();
	
	void Activate( void );
	virtual void PostClientActive( void );
	void InitTrigger( void );

	void Enable( void );
	void Disable( void );
	void Spawn( void );
	void UpdateOnRemove( void );
	void TouchTest(  void );

	// Input handlers
	virtual void InputEnable( inputdata_t &inputdata );
	virtual void InputDisable( inputdata_t &inputdata );
	virtual void InputToggle( inputdata_t &inputdata );
	virtual void InputTouchTest ( inputdata_t &inputdata );

	virtual void InputStartTouch( inputdata_t &inputdata );
	virtual void InputEndTouch( inputdata_t &inputdata );

	virtual bool UsesFilter( void ){ return ( m_hFilter.Get() != NULL ); }
	virtual bool PassesTriggerFilters(CBaseEntity *pOther);
	virtual void StartTouch(CBaseEntity *pOther);
	virtual void EndTouch(CBaseEntity *pOther);
	bool IsTouching( CBaseEntity *pOther );
#ifdef MAPBASE_VSCRIPT
	bool ScriptIsTouching( HSCRIPT hOther );
#endif

	CBaseEntity *GetTouchedEntityOfType( const char *sClassName );

	int	 DrawDebugTextOverlays(void);

	// by default, triggers don't deal with TraceAttack
	void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType) {}

	bool PointIsWithin( const Vector &vecPoint );

#ifdef MAPBASE_VSCRIPT
	bool	ScriptPassesTriggerFilters( HSCRIPT hOther ) { return ToEnt(hOther) ? PassesTriggerFilters( ToEnt(hOther) ) : NULL; }
	HSCRIPT	ScriptGetTouchedEntityOfType( const char *sClassName ) { return ToHScript( GetTouchedEntityOfType(sClassName) ); }

	void	ScriptGetTouchingEntities( HSCRIPT hTable );
#endif

	bool		m_bDisabled;
	string_t	m_iFilterName;
	CHandle<class CBaseFilter>	m_hFilter;

protected:

	// Outputs
	COutputEvent m_OnStartTouch;
	COutputEvent m_OnStartTouchAll;
	COutputEvent m_OnEndTouch;
	COutputEvent m_OnEndTouchAll;
	COutputEvent m_OnTouching;
	COutputEvent m_OnNotTouching;

	// Entities currently being touched by this trigger
	CUtlVector< EHANDLE >	m_hTouchingEntities;

#ifdef MAPBASE
	// We don't descend from CBaseToggle anymore. These have to be defined here now.
	EHANDLE		m_hActivator;
	float		m_flWait;
	string_t	m_sMaster;		// If this button has a master switch, this is the targetname.
								// A master switch must be of the multisource type. If all 
								// of the switches in the multisource have been triggered, then
								// the button will be allowed to operate. Otherwise, it will be
								// deactivated.

	virtual float	GetDelay( void ) { return m_flWait; }
#endif

	DECLARE_DATADESC();
#ifdef MAPBASE_VSCRIPT
	DECLARE_ENT_SCRIPTDESC();
#endif
};

//-----------------------------------------------------------------------------
// Purpose: Variable sized repeatable trigger.  Must be targeted at one or more entities.
//			If "delay" is set, the trigger waits some time after activating before firing.
//			"wait" : Seconds between triggerings. (.2 default/minimum)
//-----------------------------------------------------------------------------
class CTriggerMultiple : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerMultiple, CBaseTrigger );
public:
	void Spawn( void );
	void MultiTouch( CBaseEntity *pOther );
	void MultiWaitOver( void );
	void ActivateMultiTrigger(CBaseEntity *pActivator);

	DECLARE_DATADESC();

	// Outputs
	COutputEvent m_OnTrigger;
};

// Global list of triggers that care about weapon fire
extern CUtlVector< CHandle<CTriggerMultiple> >	g_hWeaponFireTriggers;


//------------------------------------------------------------------------------
// Base VPhysics trigger implementation
// NOTE: This uses vphysics to compute touch events.  It doesn't do a per-frame Touch call, so the 
// Entity I/O is different from a regular trigger
//------------------------------------------------------------------------------
#define SF_VPHYSICS_MOTION_MOVEABLE	0x1000

class CBaseVPhysicsTrigger : public CBaseEntity
{
	DECLARE_CLASS( CBaseVPhysicsTrigger , CBaseEntity );

public:
	DECLARE_DATADESC();

	virtual void Spawn();
	virtual void UpdateOnRemove();
	virtual bool CreateVPhysics();
	virtual void Activate( void );
	virtual bool PassesTriggerFilters(CBaseEntity *pOther);

	// UNDONE: Pass trigger event in or change Start/EndTouch.  Add ITriggerVPhysics perhaps?
	// BUGBUG: If a player touches two of these, his movement will screw up.
	// BUGBUG: If a player uses crouch/uncrouch it will generate touch events and clear the motioncontroller flag
	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );

	void InputToggle( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	

protected:
	bool						m_bDisabled;
	string_t					m_iFilterName;
	CHandle<class CBaseFilter>	m_hFilter;
};

//-----------------------------------------------------------------------------
// Purpose: Hurts anything that touches it. If the trigger has a targetname,
//			firing it will toggle state.
//-----------------------------------------------------------------------------
class CTriggerHurt : public CBaseTrigger
{
public:
	CTriggerHurt()
	{
		// This field came along after levels were built so the field defaults to 20 here in the constructor.
		m_flDamageCap = 20.0f;
#ifdef MAPBASE
		// Uh, same here.
		m_flHurtRate = 0.5f;
#endif
	}

	DECLARE_CLASS( CTriggerHurt, CBaseTrigger );

	void Spawn( void );
	void RadiationThink( void );
	void HurtThink( void );
	void Touch( CBaseEntity *pOther );
	void EndTouch( CBaseEntity *pOther );
	bool HurtEntity( CBaseEntity *pOther, float damage );
	int HurtAllTouchers( float dt );

#ifdef MAPBASE
	bool KeyValue( const char *szKeyName, const char *szValue );
#endif

	DECLARE_DATADESC();

	float	m_flOriginalDamage;	// Damage as specified by the level designer.
	float	m_flDamage;			// Damage per second.
	float	m_flDamageCap;		// Maximum damage per second.
	float	m_flLastDmgTime;	// Time that we last applied damage.
	float	m_flDmgResetTime;	// For forgiveness, the time to reset the counter that accumulates damage.
	int		m_bitsDamageInflict;	// DMG_ damage type that the door or tigger does
	int		m_damageModel;
	bool	m_bNoDmgForce;		// Should damage from this trigger impart force on what it's hurting
#ifdef MAPBASE
	float	m_flHurtRate;
#endif

	enum
	{
		DAMAGEMODEL_NORMAL = 0,
		DAMAGEMODEL_DOUBLE_FORGIVENESS,
	};

	// Outputs
	COutputEvent m_OnHurt;
	COutputEvent m_OnHurtPlayer;

	CUtlVector<EHANDLE>	m_hurtEntities;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTriggerCamera : public CBaseEntity
{
public:
	DECLARE_CLASS( CTriggerCamera, CBaseEntity );
	// script description
	DECLARE_ENT_SCRIPTDESC();

#ifdef MAPBASE
	CTriggerCamera();

	void UpdateOnRemove();
#endif

	void Spawn( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	void Enable( void );
	void Disable( void );

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void FollowTarget( void );
	void StartCameraShot( const char *pszShotType, CBaseEntity *pSceneEntity, CBaseEntity *pActor1, CBaseEntity *pActor2, float duration );
	int ScriptGetFov(void);
	void ScriptSetFov(int iFOV, float rate);
#ifdef MAPBASE
	void MoveThink( void );
#endif
	void Move(void);

	// Always transmit to clients so they know where to move the view to
	virtual int UpdateTransmitState();
	
	DECLARE_DATADESC();

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

#ifdef MAPBASE
	void InputSetFOV( inputdata_t &inputdata );
	void InputSetFOVRate( inputdata_t &inputdata );
#endif

private:
	EHANDLE m_hPlayer;
	EHANDLE m_hTarget;

	// used for moving the camera along a path (rail rides)
	CBaseEntity *m_pPath;
	string_t m_sPath;
	float m_flWait;
	float m_flReturnTime;
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	int	  m_state;
	Vector m_vecMoveDir;

#ifdef MAPBASE
	float m_fov;
	float m_fovSpeed;

	bool m_bDontSetPlayerView;
#endif

	string_t m_iszTargetAttachment;
	int	  m_iAttachmentIndex;
	bool  m_bSnapToGoal;

#if HL2_EPISODIC
	bool  m_bInterpolatePosition;

	// these are interpolation vars used for interpolating the camera over time
	Vector m_vStartPos, m_vEndPos;
	float m_flInterpStartTime;

	const static float kflPosInterpTime; // seconds
#endif

	int   m_nPlayerButtons;
	int m_nOldTakeDamage;

private:
	COutputEvent m_OnEndFollow;
#ifdef MAPBASE
	COutputEvent m_OnStartFollow;
#endif
};

#endif // TRIGGERS_H
