//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//---------------------------------------------------------
//  Helicopter flags
//---------------------------------------------------------
enum HelicopterFlags_t
{
	BITS_HELICOPTER_GUN_ON			= 0x00000001,	// Gun is on and aiming
	BITS_HELICOPTER_MISSILE_ON		= 0x00000002,	// Missile turrets are on and aiming
};


//---------------------------------------------------------
//---------------------------------------------------------

#define SF_NOWRECKAGE		0x08
#define SF_NOROTORWASH		0x20
#define SF_AWAITINPUT		0x40


//---------------------------------------------------------
//---------------------------------------------------------
#define BASECHOPPER_MAX_SPEED			400.0f
#define BASECHOPPER_MAX_FIRING_SPEED	250.0f
#define BASECHOPPER_MIN_ROCKET_DIST		1000.0f
#define BASECHOPPER_MAX_GUN_DIST		2000.0f


//=========================================================
//=========================================================
class CBaseHelicopter : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CBaseHelicopter, CAI_BaseNPC );

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	void Spawn( void );
	void Precache( void );
	
	void Event_Killed( const CTakeDamageInfo &info );
	void StopLoopingSounds();

	int  BloodColor( void ) { return DONT_BLEED; }
	void GibMonster( void );

	Class_T Classify ( void ) { return CLASS_HUMAN_MILITARY; }

	void CallDyingThink( void ) { DyingThink(); }

	bool HasEnemy( void ) { return GetEnemy() != NULL; }
	void CheckEnemy( CBaseEntity *pEnemy );
	virtual bool ChooseEnemy( void );
	virtual void HelicopterThink( void );
	virtual void HelicopterPostThink( void ) { };
	virtual void FlyTouch( CBaseEntity *pOther );
	virtual void CrashTouch( CBaseEntity *pOther );
	virtual void DyingThink( void );
	virtual void Startup( void );
	virtual void NullThink( void );

	virtual void Flight( void );

	virtual void ShowDamage( void ) {};

	virtual void FlyPathCorners( void );
	void UpdatePlayerDopplerShift( void );

	virtual void Hunt( void );

	virtual bool IsCrashing( void ) { return m_lifeState != LIFE_ALIVE; }
	virtual float GetAcceleration( void ) { return 5; }
	virtual bool HasReachedTarget( void ); 
	virtual void OnReachedTarget( CBaseEntity *pTarget ) {};

	virtual void ApplySidewaysDrag( const Vector &vecRight );
	virtual void ApplyGeneralDrag( void );


	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );

	virtual bool FireGun( void );

	virtual float GetRotorVolume( void ) { return 1.0; }
	virtual void InitializeRotorSound( void );
	virtual void UpdateRotorSoundPitch( int iPitch );

	virtual void AimRocketGun(void) {};
	virtual void FireRocket(  Vector vLaunchPos, Vector vLaunchDir  ) {};

	void	DrawDebugGeometryOverlays(void);

	CSoundPatch *m_pRotorSound;

	float			m_flForce;
	int				m_fHelicopterFlags;

	Vector			m_vecDesiredFaceDir;
	Vector			m_vecDesiredPosition;

	Vector			m_vecGoalOrientation; // orientation of the goal entity.

	float			m_flLastSeen;
	float			m_flPrevSeen;

	int				m_iSoundState;		// don't save this

	Vector			m_vecTarget;
	Vector			m_vecTargetPosition;

	float			m_flMaxSpeed;		// Maximum speed of the helicopter.
	float			m_flMaxSpeedFiring;	// Maximum speed of the helicopter whilst firing guns.

	float			m_flGoalSpeed;		// Goal speed
	float			m_flInitialSpeed;
	float			m_angleVelocity;

	void ChangePathCorner( const char *pszName );

	// Inputs
	void InputChangePathCorner( inputdata_t &inputdata );
	void InputActivate( inputdata_t &inputdata );

	// Outputs
	COutputEvent	m_AtTarget;			// Fired when pathcorner has been reached
	COutputEvent	m_LeaveTarget;		// Fired when pathcorner is left

	float			m_flNextCrashExplosion;
};