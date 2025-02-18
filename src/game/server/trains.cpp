//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Spawn, think, and touch functions for trains, etc.
//
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "trains.h"
#include "ndebugoverlay.h"
#include "entitylist.h"
#include "engine/IEngineSound.h"
#include "soundenvelope.h"
#include "physics_npc_solver.h"
#include "vphysics/friction.h"
#include "hierarchy.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static void PlatSpawnInsideTrigger(edict_t *pevPlatform);

#define SF_PLAT_TOGGLE				0x0001

class CBasePlatTrain : public CBaseToggle
{
	DECLARE_CLASS( CBasePlatTrain, CBaseToggle );

public:
	~CBasePlatTrain();
	bool KeyValue( const char *szKeyName, const char *szValue );
	void Precache( void );

	// This is done to fix spawn flag collisions between this class and a derived class
	virtual bool IsTogglePlat( void ) { return (m_spawnflags & SF_PLAT_TOGGLE) ? true : false; }

	DECLARE_DATADESC();

	void	PlayMovingSound();
	void	StopMovingSound();

	string_t	m_NoiseMoving;	// sound a plat makes while moving
	string_t	m_NoiseArrived;

	CSoundPatch *m_pMovementSound;
#ifdef HL1_DLL
	int			m_MoveSound;
	int			m_StopSound;
#endif

	float	m_volume;			// Sound volume
	float	m_flTWidth;
	float	m_flTLength;
};

BEGIN_DATADESC( CBasePlatTrain )

	DEFINE_KEYFIELD( m_NoiseMoving, FIELD_SOUNDNAME, "noise1" ),
	DEFINE_KEYFIELD( m_NoiseArrived, FIELD_SOUNDNAME, "noise2" ),

#ifdef HL1_DLL
	DEFINE_KEYFIELD( m_MoveSound, FIELD_INTEGER, "movesnd" ),
	DEFINE_KEYFIELD( m_StopSound, FIELD_INTEGER, "stopsnd" ),

#endif 
	DEFINE_SOUNDPATCH( m_pMovementSound ),

	DEFINE_KEYFIELD( m_volume, FIELD_FLOAT, "volume" ),

	DEFINE_FIELD( m_flTWidth, FIELD_FLOAT ),
	DEFINE_FIELD( m_flTLength, FIELD_FLOAT ),
	DEFINE_KEYFIELD( m_flLip, FIELD_FLOAT, "lip" ),
	DEFINE_KEYFIELD( m_flWait, FIELD_FLOAT, "wait" ),
	DEFINE_KEYFIELD( m_flHeight, FIELD_FLOAT, "height" ),

END_DATADESC()


bool CBasePlatTrain::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "rotation"))
	{
		m_vecFinalAngle.x = atof(szValue);
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}


CBasePlatTrain::~CBasePlatTrain()
{
	StopMovingSound();
}

void CBasePlatTrain::PlayMovingSound()
{
	StopMovingSound();
	if(m_NoiseMoving != NULL_STRING )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		CPASAttenuationFilter filter( this );
		m_pMovementSound = controller.SoundCreate( filter, entindex(), CHAN_STATIC, STRING(m_NoiseMoving), ATTN_NORM );
		
		controller.Play( m_pMovementSound, m_volume, PITCH_NORM );
	}
}

void CBasePlatTrain::StopMovingSound()
{
	if ( m_pMovementSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		controller.SoundDestroy( m_pMovementSound );
		m_pMovementSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlatTrain::Precache( void )
{
	//Fill in a default value if necessary
	UTIL_ValidateSoundName( m_NoiseMoving, "Plat.DefaultMoving" );
	UTIL_ValidateSoundName( m_NoiseArrived, "Plat.DefaultArrive" );

#ifdef HL1_DLL
// set the plat's "in-motion" sound
	switch (m_MoveSound)
	{
	default:
	case	0:
		m_NoiseMoving = MAKE_STRING( "Plat.DefaultMoving" );
		break;
	case	1:
		m_NoiseMoving = MAKE_STRING("Plat.BigElev1");
		break;
	case	2:
		m_NoiseMoving = MAKE_STRING("Plat.BigElev2");
		break;
	case	3:
		m_NoiseMoving = MAKE_STRING("Plat.TechElev1");
		break;
	case	4:
		m_NoiseMoving = MAKE_STRING("Plat.TechElev2");
		break;
	case	5:
		m_NoiseMoving = MAKE_STRING("Plat.TechElev3");
		break;
	case	6:
		m_NoiseMoving = MAKE_STRING("Plat.FreightElev1");
		break;
	case	7:
		m_NoiseMoving = MAKE_STRING("Plat.FreightElev2");
		break;
	case	8:
		m_NoiseMoving = MAKE_STRING("Plat.HeavyElev");
		break;
	case	9:
		m_NoiseMoving = MAKE_STRING("Plat.RackElev");
		break;
	case	10:
		m_NoiseMoving = MAKE_STRING("Plat.RailElev");
		break;
	case	11:
		m_NoiseMoving = MAKE_STRING("Plat.SqueakElev");
		break;
	case	12:
		m_NoiseMoving = MAKE_STRING("Plat.OddElev1");
		break;
	case	13:
		m_NoiseMoving = MAKE_STRING("Plat.OddElev2");
		break;
	}

// set the plat's 'reached destination' stop sound
	switch (m_StopSound)
	{
	default:
	case	0:
		m_NoiseArrived = MAKE_STRING( "Plat.DefaultArrive" );
		break;
	case	1:
		m_NoiseArrived = MAKE_STRING("Plat.BigElevStop1");
		break;
	case	2:
		m_NoiseArrived = MAKE_STRING("Plat.BigElevStop2");
		break;
	case	3:
		m_NoiseArrived = MAKE_STRING("Plat.FreightElevStop");
		break;
	case	4:
		m_NoiseArrived = MAKE_STRING("Plat.HeavyElevStop");
		break;
	case	5:
		m_NoiseArrived = MAKE_STRING("Plat.RackStop");
		break;
	case	6:
		m_NoiseArrived = MAKE_STRING("Plat.RailStop");
		break;
	case	7:
		m_NoiseArrived = MAKE_STRING("Plat.SqueakStop");
		break;
	case	8:
		m_NoiseArrived = MAKE_STRING("Plat.QuickStop");
		break;
	}

#endif // HL1_DLL

	//Precache them all
	PrecacheScriptSound( (char *) STRING(m_NoiseMoving) );
	PrecacheScriptSound( (char *) STRING(m_NoiseArrived) );

}


class CFuncPlat : public CBasePlatTrain
{
	DECLARE_CLASS( CFuncPlat, CBasePlatTrain );
public:
	void Spawn( void );
	void Precache( void );
	bool CreateVPhysics();
	void Setup( void );

	virtual void Blocked( CBaseEntity *pOther );
	void PlatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void	CallGoDown( void ) { GoDown(); }
	void	CallHitTop( void  ) { HitTop(); }
	void	CallHitBottom( void ) { HitBottom(); }

	virtual void GoUp( void );
	virtual void GoDown( void );
	virtual void HitTop( void );
	virtual void HitBottom( void );

	void InputToggle(inputdata_t &data);
	void InputGoUp(inputdata_t &data);
	void InputGoDown(inputdata_t &data);

	DECLARE_DATADESC();

private:

	string_t m_sNoise;
};


BEGIN_DATADESC( CFuncPlat )

	DEFINE_FIELD( m_sNoise, FIELD_STRING ),

	// Function Pointers
	DEFINE_FUNCTION( PlatUse ),
	DEFINE_FUNCTION( CallGoDown ),
	DEFINE_FUNCTION( CallHitTop ),
	DEFINE_FUNCTION( CallHitBottom ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GoUp", InputGoUp ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GoDown", InputGoDown ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_plat, CFuncPlat );

//==================================================
// CPlatTrigger 
//==================================================
class CPlatTrigger : public CBaseEntity
{
	DECLARE_CLASS( CPlatTrigger, CBaseEntity );
public:
	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_DONT_SAVE; }
	void SpawnInsideTrigger( CFuncPlat *pPlatform );
	void Touch( CBaseEntity *pOther );
	CFuncPlat *m_pPlatform;
};

void CFuncPlat::Setup( void )
{
	if (m_flTLength == 0)
	{
		m_flTLength = 80;
	}
	
	if (m_flTWidth == 0)
	{
		m_flTWidth = 10;
	}
	
	SetLocalAngles( vec3_angle );
	SetSolid( SOLID_BSP );
	SetMoveType( MOVETYPE_PUSH );

	// Set size and link into world
	SetModel( STRING( GetModelName() ) );

	m_vecPosition1 = GetLocalOrigin();	//Top
	m_vecPosition2 = GetLocalOrigin();	//Bottom

	if ( m_flHeight != 0 )
	{
		m_vecPosition2.z = GetLocalOrigin().z - m_flHeight;
	}
	else
	{
		// NOTE: This works because the angles were set to vec3_angle above
		m_vecPosition2.z = GetLocalOrigin().z - CollisionProp()->OBBSize().z + 8;
	}

	if (m_flSpeed == 0)
	{
		m_flSpeed = 150;
	}

	if ( m_volume == 0.0f )
	{
		m_volume = 0.85f;
	}
}


void CFuncPlat::Precache( )
{
	BaseClass::Precache();
	
	if ( IsTogglePlat() == false )
	{
		// Create the "start moving" trigger
		PlatSpawnInsideTrigger( edict() );
	}
}


void CFuncPlat::Spawn( )
{
	Setup();
	Precache();

	// If this platform is the target of some button, it starts at the TOP position,
	// and is brought down by that button.  Otherwise, it starts at BOTTOM.
	if ( GetEntityName() != NULL_STRING )
	{
		UTIL_SetOrigin( this, m_vecPosition1);
		m_toggle_state = TS_AT_TOP;
		SetUse( &CFuncPlat::PlatUse );
	}
	else
	{
		UTIL_SetOrigin( this, m_vecPosition2);
		m_toggle_state = TS_AT_BOTTOM;
	}
	CreateVPhysics();
}

bool CFuncPlat::CreateVPhysics()
{
	VPhysicsInitShadow( false, false );
	return true;
}


static void PlatSpawnInsideTrigger(edict_t* pevPlatform)
{
	// old code: //GetClassPtr( (CPlatTrigger *)NULL)->SpawnInsideTrigger( GetClassPtr( (CFuncPlat *)pevPlatform ) );
	CPlatTrigger *plattrig = CREATE_UNSAVED_ENTITY( CPlatTrigger, "plat_trigger" );
	plattrig->SpawnInsideTrigger( (CFuncPlat *)GetContainingEntity( pevPlatform ) );
}
		

//
// Create a trigger entity for a platform.
//
void CPlatTrigger::SpawnInsideTrigger( CFuncPlat *pPlatform )
{
	m_pPlatform = pPlatform;
	// Create trigger entity, "point" it at the owning platform, give it a touch method
	SetSolid( SOLID_BSP );
	AddSolidFlags( FSOLID_TRIGGER );
	SetMoveType( MOVETYPE_NONE );
	SetLocalOrigin( pPlatform->GetLocalOrigin() );

	// Establish the trigger field's size
	CCollisionProperty *pCollision = m_pPlatform->CollisionProp();
	Vector vecTMin = pCollision->OBBMins() + Vector ( 25 , 25 , 0 );
	Vector vecTMax = pCollision->OBBMaxs() + Vector ( 25 , 25 , 8 );
	vecTMin.z = vecTMax.z - ( m_pPlatform->m_vecPosition1.z - m_pPlatform->m_vecPosition2.z + 8 );
	if ( pCollision->OBBSize().x <= 50 )
	{
		vecTMin.x = (pCollision->OBBMins().x + pCollision->OBBMaxs().x) / 2;
		vecTMax.x = vecTMin.x + 1;
	}
	if ( pCollision->OBBSize().y <= 50 )
	{
		vecTMin.y = (pCollision->OBBMins().y + pCollision->OBBMaxs().y) / 2;
		vecTMax.y = vecTMin.y + 1;
	}
	UTIL_SetSize ( this, vecTMin, vecTMax );
}


//
// When the platform's trigger field is touched, the platform ???
//
void CPlatTrigger::Touch( CBaseEntity *pOther )
{
	// Ignore touches by non-players
	if ( !pOther->IsPlayer() )
		return;

	// Ignore touches by corpses
	if (!pOther->IsAlive())
		return;
	
	// Make linked platform go up/down.
	if (m_pPlatform->m_toggle_state == TS_AT_BOTTOM)
		m_pPlatform->GoUp();
	else if (m_pPlatform->m_toggle_state == TS_AT_TOP)
		m_pPlatform->SetMoveDoneTime( 1 );// delay going down
}



//-----------------------------------------------------------------------------
// Purpose: Used when a platform is the target of a button.
//			Start bringing platform down.
// Input  : pActivator - 
//			pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CFuncPlat::InputToggle(inputdata_t &data)
{
	if ( IsTogglePlat() )
	{
		if (m_toggle_state == TS_AT_TOP)
			GoDown();
		else if ( m_toggle_state == TS_AT_BOTTOM )
			GoUp();
	}
	else
	{
		SetUse( NULL );

		if (m_toggle_state == TS_AT_TOP)
			GoDown();
	}
}

void CFuncPlat::InputGoUp(inputdata_t &data)
{
	if ( m_toggle_state == TS_AT_BOTTOM )
		GoUp();
}

void CFuncPlat::InputGoDown(inputdata_t &data)
{
	if ( m_toggle_state == TS_AT_TOP )
		GoDown();
}

//-----------------------------------------------------------------------------
// Purpose: Used when a platform is the target of a button.
//			Start bringing platform down.
// Input  : pActivator - 
//			pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CFuncPlat::PlatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( IsTogglePlat() )
	{
		// Top is off, bottom is on
		bool on = (m_toggle_state == TS_AT_BOTTOM) ? true : false;

		if ( !ShouldToggle( useType, on ) )
			return;

		if (m_toggle_state == TS_AT_TOP)
			GoDown();
		else if ( m_toggle_state == TS_AT_BOTTOM )
			GoUp();
	}
	else
	{
		SetUse( NULL );

		if (m_toggle_state == TS_AT_TOP)
			GoDown();
	}
}


//
// Platform is at top, now starts moving down.
//
void CFuncPlat::GoDown( void )
{
	PlayMovingSound();

	ASSERT(m_toggle_state == TS_AT_TOP || m_toggle_state == TS_GOING_UP);
	m_toggle_state = TS_GOING_DOWN;
	SetMoveDone(&CFuncPlat::CallHitBottom);
	LinearMove(m_vecPosition2, m_flSpeed);
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncPlat::HitBottom( void )
{
	StopMovingSound();

	if ( m_NoiseArrived != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_WEAPON;
		ep.m_pSoundName =  STRING(m_NoiseArrived);
		ep.m_flVolume = m_volume;
		ep.m_SoundLevel = SNDLVL_NORM;

		EmitSound( filter, entindex(), ep );
	}

	ASSERT(m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_AT_BOTTOM;
}


//
// Platform is at bottom, now starts moving up
//
void CFuncPlat::GoUp( void )
{
	PlayMovingSound();
	
	ASSERT(m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_GOING_UP;
	SetMoveDone(&CFuncPlat::CallHitTop);
	LinearMove(m_vecPosition1, m_flSpeed);
}


//
// Platform has hit top.  Pauses, then starts back down again.
//
void CFuncPlat::HitTop( void )
{
	StopMovingSound();

	if ( m_NoiseArrived != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_WEAPON;
		ep.m_pSoundName =  STRING(m_NoiseArrived);
		ep.m_flVolume = m_volume;
		ep.m_SoundLevel = SNDLVL_NORM;

		EmitSound( filter, entindex(), ep );
	}
	
	ASSERT(m_toggle_state == TS_GOING_UP);
	m_toggle_state = TS_AT_TOP;

	if ( !IsTogglePlat() )
	{
		// After a delay, the platform will automatically start going down again.
		SetMoveDone( &CFuncPlat::CallGoDown );
		SetMoveDoneTime( 3 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when we are blocked.
//-----------------------------------------------------------------------------
void CFuncPlat::Blocked( CBaseEntity *pOther )
{
	DevMsg( 2, "%s Blocked by %s\n", GetClassname(), pOther->GetClassname() );

	// Hurt the blocker a little
	pOther->TakeDamage( CTakeDamageInfo( this, this, 1, DMG_CRUSH ) );

	if (m_sNoise != NULL_STRING)
	{
		StopSound(entindex(), CHAN_STATIC, (char*)STRING(m_sNoise));
	}
	
	// Send the platform back where it came from
	ASSERT(m_toggle_state == TS_GOING_UP || m_toggle_state == TS_GOING_DOWN);
	if (m_toggle_state == TS_GOING_UP)
	{
		GoDown();
	}
	else if (m_toggle_state == TS_GOING_DOWN)
	{
		GoUp ();
	}
}


class CFuncPlatRot : public CFuncPlat
{
	DECLARE_CLASS( CFuncPlatRot, CFuncPlat );
public:
	void Spawn( void );
	void SetupRotation( void );

	virtual void	GoUp( void );
	virtual void	GoDown( void );
	virtual void	HitTop( void );
	virtual void	HitBottom( void );
	
	void			RotMove( QAngle &destAngle, float time );
	DECLARE_DATADESC();

	QAngle	m_end, m_start;
};

LINK_ENTITY_TO_CLASS( func_platrot, CFuncPlatRot );

BEGIN_DATADESC( CFuncPlatRot )

	DEFINE_FIELD( m_end, FIELD_VECTOR ),
	DEFINE_FIELD( m_start, FIELD_VECTOR ),

END_DATADESC()


void CFuncPlatRot::SetupRotation( void )
{
	if ( m_vecFinalAngle.x != 0 )		// This plat rotates too!
	{
		CBaseToggle::AxisDir();
		m_start	= GetLocalAngles();
		m_end = GetLocalAngles() + m_vecMoveAng * m_vecFinalAngle.x;
	}
	else
	{
		m_start = vec3_angle;
		m_end = vec3_angle;
	}
	if ( GetEntityName() != NULL_STRING )	// Start at top
	{
		SetLocalAngles( m_end );
	}
}


void CFuncPlatRot::Spawn( void )
{
	BaseClass::Spawn();
	SetupRotation();
}

void CFuncPlatRot::GoDown( void )
{
	BaseClass::GoDown();
	RotMove( m_start, GetMoveDoneTime() );
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncPlatRot::HitBottom( void )
{
	BaseClass::HitBottom();
	SetLocalAngularVelocity( vec3_angle );
	SetLocalAngles( m_start );
}


//
// Platform is at bottom, now starts moving up
//
void CFuncPlatRot::GoUp( void )
{
	BaseClass::GoUp();
	RotMove( m_end, GetMoveDoneTime() );
}


//
// Platform has hit top.  Pauses, then starts back down again.
//
void CFuncPlatRot::HitTop( void )
{
	BaseClass::HitTop();
	SetLocalAngularVelocity( vec3_angle );
	SetLocalAngles( m_end );
}


void CFuncPlatRot::RotMove( QAngle &destAngle, float time )
{
	// set destdelta to the vector needed to move
	QAngle vecDestDelta = destAngle - GetLocalAngles();

	// Travel time is so short, we're practically there already;  so make it so.
	if ( time >= 0.1)
		SetLocalAngularVelocity( vecDestDelta * (1.0 / time) );
	else
	{
		SetLocalAngularVelocity( vecDestDelta );
		SetMoveDoneTime( 1 );
	}
}


class CFuncTrain : public CBasePlatTrain
{
	DECLARE_CLASS( CFuncTrain, CBasePlatTrain );
public:
	void Spawn( void );
	void Precache( void );
	void Activate( void );
	void OnRestore( void );

	void SetupTarget( void );
	void Blocked( CBaseEntity *pOther );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void Wait( void );
	void Next( void );

	//Inputs
	void InputToggle(inputdata_t &data);
	void InputStart(inputdata_t &data);
	void InputStop(inputdata_t &data);

	void Start( void );
	void Stop( void );

	DECLARE_DATADESC();

public:
	EHANDLE		m_hCurrentTarget;
	
	bool		m_activated;
	EHANDLE		m_hEnemy;
	float		m_flBlockDamage;		// Damage to inflict when blocked.
	float		m_flNextBlockTime;
	string_t m_iszLastTarget;

};

LINK_ENTITY_TO_CLASS( func_train, CFuncTrain );


BEGIN_DATADESC( CFuncTrain )

	DEFINE_FIELD( m_hCurrentTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_activated, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hEnemy, FIELD_EHANDLE ),
	DEFINE_FIELD( m_iszLastTarget, FIELD_STRING ),
	DEFINE_FIELD( m_flNextBlockTime, FIELD_TIME ),

	DEFINE_KEYFIELD( m_flBlockDamage, FIELD_FLOAT, "dmg" ),

	// Function Pointers
	DEFINE_FUNCTION( Wait ),
	DEFINE_FUNCTION( Next ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Start",	InputStart ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Stop",	InputStop ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Handles a train being blocked by an entity.
// Input  : pOther - What was hit.
//-----------------------------------------------------------------------------
void CFuncTrain::Blocked( CBaseEntity *pOther )
{
	if ( gpGlobals->curtime < m_flNextBlockTime )
		return;

	m_flNextBlockTime = gpGlobals->curtime + 0.5;
	
	//Inflict damage
	pOther->TakeDamage( CTakeDamageInfo( this, this, m_flBlockDamage, DMG_CRUSH ) );
}


void CFuncTrain::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	//If we've been waiting to be retriggered, move to the next destination
	if ( m_spawnflags & SF_TRAIN_WAIT_RETRIGGER )
	{
		// Move toward my target
		m_spawnflags &= ~SF_TRAIN_WAIT_RETRIGGER;
		Next();
	}
	else
	{
		m_spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		
		// Pop back to last target if it's available
		if ( m_hEnemy )
		{
			m_target = m_hEnemy->GetEntityName();
		}

		SetNextThink( TICK_NEVER_THINK );
		SetLocalVelocity( vec3_origin );
		
		if ( m_NoiseArrived != NULL_STRING )
		{
			CPASAttenuationFilter filter( this );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_VOICE;
			ep.m_pSoundName =  STRING(m_NoiseArrived);
			ep.m_flVolume = m_volume;
			ep.m_SoundLevel = SNDLVL_NORM;

			EmitSound( filter, entindex(), ep );
		}
	}
}

void CFuncTrain::Wait( void )
{
	//If we're moving passed a path track, then trip its output
	variant_t emptyVariant;
	m_hCurrentTarget->AcceptInput( "InPass", this, this, emptyVariant, 0 );

	// need pointer to LAST target.
	if ( m_hCurrentTarget->HasSpawnFlags( SF_TRAIN_WAIT_RETRIGGER ) || HasSpawnFlags( SF_TRAIN_WAIT_RETRIGGER ) )
    {
		AddSpawnFlags( SF_TRAIN_WAIT_RETRIGGER );
        
		// Clear the sound channel.
		StopMovingSound();
		
		if ( m_NoiseArrived != NULL_STRING )
		{
			CPASAttenuationFilter filter( this );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_VOICE;
			ep.m_pSoundName =  STRING(m_NoiseArrived);
			ep.m_flVolume = m_volume;
			ep.m_SoundLevel = SNDLVL_NORM;

			EmitSound( filter, entindex(), ep );
		}

		SetMoveDoneTime( -1 );
        
		return;
    }
    
	//NOTENOTE: -1 wait will wait forever
	if ( m_flWait != 0 )
	{
		SetMoveDoneTime( m_flWait );

		StopMovingSound();
		
		if ( m_NoiseArrived != NULL_STRING )
		{
			CPASAttenuationFilter filter( this );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_VOICE;
			ep.m_pSoundName =  STRING(m_NoiseArrived);
			ep.m_flVolume = m_volume;
			ep.m_SoundLevel = SNDLVL_NORM;

			EmitSound( filter, entindex(), ep );
		}

		SetMoveDone( &CFuncTrain::Next );
	}
	else
	{
		// Do it right now
		Next();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Advances the train to the next path corner on the path.
//-----------------------------------------------------------------------------
void CFuncTrain::Next( void )
{
	//Find our next target
	CBaseEntity	*pTarg = GetNextTarget();

    //If none, we're done
	if ( pTarg == NULL )
	{
		//Stop the moving sound
		StopMovingSound();

		// Play stop sound
		if ( m_NoiseArrived != NULL_STRING )
		{
			CPASAttenuationFilter filter( this );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_VOICE;
			ep.m_pSoundName =  STRING(m_NoiseArrived);
			ep.m_flVolume = m_volume;
			ep.m_SoundLevel = SNDLVL_NORM;

			EmitSound( filter, entindex(), ep );
		}

		return;
	}

	// Save last target in case we need to find it again
	m_iszLastTarget = m_target;

	m_target = pTarg->m_target;
	m_flWait = pTarg->GetDelay();

	// If our target has a speed, take it
	if ( m_hCurrentTarget && m_hCurrentTarget->m_flSpeed != 0 )
	{
        m_flSpeed = m_hCurrentTarget->m_flSpeed;
		DevMsg( 2, "Train %s speed to %4.2f\n", GetDebugName(), m_flSpeed );
	}

	// Keep track of this since path corners change our target for us
	m_hCurrentTarget = pTarg;
    m_hEnemy = pTarg;

	//Check for teleport
	if ( m_hCurrentTarget->HasSpawnFlags( SF_CORNER_TELEPORT ) )
	{
		IncrementInterpolationFrame();

		// This is supposed to place the center of the func_train at the target's origin.
		// FIXME: This is totally busted! It's using the wrong space for the computation...
		UTIL_SetOrigin( this, pTarg->GetLocalOrigin() - CollisionProp()->OBBCenter() );
		
		// Get on with doing the next path corner.
		Wait(); 
	}
	else
	{
		// Normal linear move
		PlayMovingSound();

		SetMoveDone( &CFuncTrain::Wait );

		// This is supposed to place the center of the func_train at the target's origin.
		// FIXME: This is totally busted! It's using the wrong space for the computation...
		LinearMove ( pTarg->GetLocalOrigin() - CollisionProp()->OBBCenter(), m_flSpeed );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called after all the entities spawn.
//-----------------------------------------------------------------------------
void CFuncTrain::Activate( void )
{
	BaseClass::Activate();

	// Not yet active, so teleport to first target
	if ( m_activated == false )
	{
		SetupTarget();

		m_activated = true;

		if ( m_hCurrentTarget.Get() == NULL )
			return;

		// This is supposed to place the center of the func_train at the target's origin.
		// FIXME: This is totally busted! It's using the wrong space for the computation...
		UTIL_SetOrigin( this, m_hCurrentTarget->GetLocalOrigin() - CollisionProp()->OBBCenter() );
		if ( GetSolid() == SOLID_BSP )
		{
			VPhysicsInitShadow( false, false );
		}

		// Start immediately if not triggered
		if ( !GetEntityName() )
		{	
			SetMoveDoneTime( 0.1 );
			SetMoveDone( &CFuncTrain::Next );
		}
		else
		{
			m_spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTrain::SetupTarget( void )
{
	// Find our target whenever we don't have one (level transition)
	if ( !m_hCurrentTarget )
	{
		CBaseEntity	*pTarg = gEntList.FindEntityByName( NULL, m_target );

		if ( pTarg == NULL )
		{
			Msg( "Can't find target of train %s\n", STRING(m_target) );
			return;
		}
		
		// Keep track of this since path corners change our target for us
		m_target = pTarg->m_target;
		m_hCurrentTarget = pTarg;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTrain::Spawn( void )
{
	Precache();
	
	if ( m_flSpeed == 0 )
	{
		m_flSpeed = 100;
	}
	
	if ( !m_target )
	{
		Warning("FuncTrain '%s' has no target.\n", GetDebugName());
	}
	
	if ( m_flBlockDamage == 0 )
	{
		m_flBlockDamage = 2;
	}
	
	SetMoveType( MOVETYPE_PUSH );
	SetSolid( SOLID_BSP );
	SetModel( STRING( GetModelName() ) );
	if ( m_spawnflags & SF_TRACKTRAIN_PASSABLE )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
	}

	m_activated = false;

	if ( m_volume == 0.0f )
	{
		m_volume = 0.85f;
	}
}


void CFuncTrain::Precache( void )
{
	BaseClass::Precache();
}


void CFuncTrain::OnRestore( void )
{
	BaseClass::OnRestore();

	// Are we moving?
	if ( IsMoving() )
	{
		// Continue moving to the same target
		m_target = m_iszLastTarget;
	}

	SetupTarget();
}


void CFuncTrain::InputToggle( inputdata_t &data )
{
	//If we've been waiting to be retriggered, move to the next destination
	if( HasSpawnFlags( SF_TRAIN_WAIT_RETRIGGER ) )
	{
		Start();
	}
	else
	{
		Stop();
	}
}


void CFuncTrain::InputStart( inputdata_t &data )
{
	Start();
}


void CFuncTrain::InputStop( inputdata_t &data )
{
	Stop();
}


void CFuncTrain::Start( void )
{
	//start moving
	if( HasSpawnFlags( SF_TRAIN_WAIT_RETRIGGER ) )
	{
		// Move toward my target
		RemoveSpawnFlags( SF_TRAIN_WAIT_RETRIGGER );
		Next();
	}
}


void CFuncTrain::Stop( void )
{
	//stop moving
	if( !HasSpawnFlags( SF_TRAIN_WAIT_RETRIGGER ) )
	{
		AddSpawnFlags( SF_TRAIN_WAIT_RETRIGGER );
		
		// Pop back to last target if it's available
		if ( m_hEnemy )
		{
			m_target = m_hEnemy->GetEntityName();
		}

		SetNextThink( TICK_NEVER_THINK );
		SetAbsVelocity( vec3_origin );
		
		if ( m_NoiseArrived != NULL_STRING )
		{
			CPASAttenuationFilter filter( this );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_VOICE;
			ep.m_pSoundName =  STRING(m_NoiseArrived);
			ep.m_flVolume = m_volume;
			ep.m_SoundLevel = SNDLVL_NORM;

			EmitSound( filter, entindex(), ep );
		}

		//Do not teleport to our final move destination
		SetMoveDone( NULL );
		SetMoveDoneTime( -1 );
	}
}

BEGIN_DATADESC( CFuncTrackTrain )

	DEFINE_KEYFIELD( m_length, FIELD_FLOAT, "wheels" ),
	DEFINE_KEYFIELD( m_height, FIELD_FLOAT, "height" ),
	DEFINE_KEYFIELD( m_maxSpeed, FIELD_FLOAT, "startspeed" ),
	DEFINE_KEYFIELD( m_flBank, FIELD_FLOAT, "bank" ),
	DEFINE_KEYFIELD( m_flBlockDamage, FIELD_FLOAT, "dmg" ),
	DEFINE_KEYFIELD( m_iszSoundMove, FIELD_SOUNDNAME, "MoveSound" ),
	DEFINE_KEYFIELD( m_iszSoundMovePing, FIELD_SOUNDNAME, "MovePingSound" ),
	DEFINE_KEYFIELD( m_iszSoundStart, FIELD_SOUNDNAME, "StartSound" ),
	DEFINE_KEYFIELD( m_iszSoundStop, FIELD_SOUNDNAME, "StopSound" ),
	DEFINE_KEYFIELD( m_nMoveSoundMinPitch, FIELD_INTEGER, "MoveSoundMinPitch" ),
	DEFINE_KEYFIELD( m_nMoveSoundMaxPitch, FIELD_INTEGER, "MoveSoundMaxPitch" ),
	DEFINE_KEYFIELD( m_flMoveSoundMinTime, FIELD_FLOAT, "MoveSoundMinTime" ),
	DEFINE_KEYFIELD( m_flMoveSoundMaxTime, FIELD_FLOAT, "MoveSoundMaxTime" ),
	DEFINE_FIELD( m_flNextMoveSoundTime, FIELD_TIME ),
	DEFINE_KEYFIELD( m_eVelocityType, FIELD_INTEGER, "velocitytype" ),
	DEFINE_KEYFIELD( m_eOrientationType, FIELD_INTEGER, "orientationtype" ),

	DEFINE_FIELD( m_ppath, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_dir, FIELD_FLOAT ),
	DEFINE_FIELD( m_controlMins, FIELD_VECTOR ),
	DEFINE_FIELD( m_controlMaxs, FIELD_VECTOR ),
	DEFINE_FIELD( m_flVolume, FIELD_FLOAT ),
	DEFINE_FIELD( m_oldSpeed, FIELD_FLOAT ),
	//DEFINE_FIELD( m_lastBlockPos, FIELD_POSITION_VECTOR ), // temp values for blocking, don't save
	//DEFINE_FIELD( m_lastBlockTick, FIELD_INTEGER ),

	DEFINE_FIELD( m_bSoundPlaying, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_bManualSpeedChanges, FIELD_BOOLEAN, "ManualSpeedChanges" ),
	DEFINE_KEYFIELD( m_flAccelSpeed, FIELD_FLOAT, "ManualAccelSpeed" ),
	DEFINE_KEYFIELD( m_flDecelSpeed, FIELD_FLOAT, "ManualDecelSpeed" ),

#ifdef HL1_DLL
	DEFINE_FIELD( m_bOnTrackChange, FIELD_BOOLEAN ),
#endif

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Stop", InputStop ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartForward", InputStartForward ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartBackward", InputStartBackward ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Resume", InputResume ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Reverse", InputReverse ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeed", InputSetSpeed ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeedDir", InputSetSpeedDir ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeedReal", InputSetSpeedReal ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeedDirAccel", InputSetSpeedDirAccel ),
	DEFINE_INPUTFUNC( FIELD_STRING, "TeleportToPathTrack", InputTeleportToPathTrack ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeedForwardModifier", InputSetSpeedForwardModifier ),

	// Outputs
	DEFINE_OUTPUT( m_OnStart, "OnStart" ),
	DEFINE_OUTPUT( m_OnNext, "OnNextPoint" ),

	// Function Pointers
	DEFINE_FUNCTION( Next ),
	DEFINE_FUNCTION( Find ),
	DEFINE_FUNCTION( NearestPath ),
	DEFINE_FUNCTION( DeadEnd ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_tracktrain, CFuncTrackTrain );


//-----------------------------------------------------------------------------
// Datatable
//-----------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST( CFuncTrackTrain, DT_FuncTrackTrain )
END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CFuncTrackTrain::CFuncTrackTrain()
{
#ifdef _DEBUG
	m_controlMins.Init();
	m_controlMaxs.Init();
#endif

	// These defaults match old func_tracktrains. Changing these defaults would
	// require a vmf_tweak of older content to keep it from breaking.
	m_eOrientationType = TrainOrientation_AtPathTracks;
	m_eVelocityType = TrainVelocity_Instantaneous;
	m_lastBlockPos.Init();
	m_lastBlockTick = gpGlobals->tickcount;

	m_flSpeedForwardModifier = 1.0f;
	m_flUnmodifiedDesiredSpeed = 0.0f;

	m_bDamageChild = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CFuncTrackTrain::DrawDebugTextOverlays( void )
{
	int nOffset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf( tempstr,sizeof(tempstr), "angles: %g %g %g", (double)GetLocalAngles()[PITCH], (double)GetLocalAngles()[YAW], (double)GetLocalAngles()[ROLL] );
		EntityText( nOffset, tempstr, 0 );
		nOffset++;

		float flCurSpeed = GetLocalVelocity().Length();
		Q_snprintf( tempstr,sizeof(tempstr), "current speed (goal): %g (%g)", (double)flCurSpeed, (double)m_flSpeed );
		EntityText( nOffset, tempstr, 0 );
		nOffset++;

		Q_snprintf( tempstr,sizeof(tempstr), "max speed: %g", (double)m_maxSpeed );
		EntityText( nOffset, tempstr, 0 );
		nOffset++;
	}

	return nOffset;
}


void CFuncTrackTrain::DrawDebugGeometryOverlays()
{
	BaseClass::DrawDebugGeometryOverlays();
	if (m_debugOverlays & OVERLAY_BBOX_BIT) 
	{
		NDebugOverlay::Box( GetAbsOrigin(), -Vector(4,4,4),Vector(4,4,4), 255, 0, 255, 0, 0);
		Vector out;
		VectorTransform( Vector(m_length,0,0), EntityToWorldTransform(), out );
		NDebugOverlay::Box( out, -Vector(4,4,4),Vector(4,4,4), 255, 0, 255, 0, 0);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncTrackTrain::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "volume"))
	{
		m_flVolume = (float) (atoi(szValue));
		m_flVolume *= 0.1f;
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that stops the train.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::InputStop( inputdata_t &inputdata )
{
	Stop();
}


//------------------------------------------------------------------------------
// Purpose: Input handler that starts the train moving.
//------------------------------------------------------------------------------
void CFuncTrackTrain::InputResume( inputdata_t &inputdata )
{
	m_flSpeed = m_oldSpeed;
	Start();
}


//------------------------------------------------------------------------------
// Purpose: Input handler that reverses the trains current direction of motion.
//------------------------------------------------------------------------------
void CFuncTrackTrain::InputReverse( inputdata_t &inputdata )
{
	SetDirForward( !IsDirForward() );
	SetSpeed( m_flSpeed );
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether we are travelling forward along our path.
//-----------------------------------------------------------------------------
bool CFuncTrackTrain::IsDirForward()
{
	return ( m_dir == 1 );
}


//-----------------------------------------------------------------------------
// Purpose: Sets whether we go forward or backward along our path.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::SetDirForward( bool bForward )
{
	if ( bForward && ( m_dir != 1 ) )
	{
		// Reverse direction.
		if ( m_ppath && m_ppath->GetPrevious() )
		{
			m_ppath = m_ppath->GetPrevious();
		}

		m_dir = 1;
	}
	else if ( !bForward && ( m_dir != -1 ) )
	{
		// Reverse direction.
		if ( m_ppath && m_ppath->GetNext() )
		{
			m_ppath = m_ppath->GetNext();
		}

		m_dir = -1;
	}
}


//------------------------------------------------------------------------------
// Purpose: Input handler that starts the train moving.
//------------------------------------------------------------------------------
void CFuncTrackTrain::InputStartForward( inputdata_t &inputdata )
{
	SetDirForward( true );
	SetSpeed( m_maxSpeed );
}


//------------------------------------------------------------------------------
// Purpose: Input handler that starts the train moving.
//------------------------------------------------------------------------------
void CFuncTrackTrain::InputStartBackward( inputdata_t &inputdata )
{
	SetDirForward( false );
	SetSpeed( m_maxSpeed );
}


//------------------------------------------------------------------------------
// Purpose: Starts the train moving.
//------------------------------------------------------------------------------
void CFuncTrackTrain::Start( void )
{
	m_OnStart.FireOutput(this,this);
	Next();
}


//-----------------------------------------------------------------------------
// Purpose: Toggles the train between moving and not moving.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::InputToggle( inputdata_t &inputdata )
{
	if ( m_flSpeed == 0 )
	{
		SetSpeed( m_maxSpeed );
	}
	else
	{
		SetSpeed( 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles player use so players can control the speed of the train.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// player +USE
	if ( useType == USE_SET )
	{
		float delta = value;

		delta = ((int)(m_flSpeed * 4) / (int)m_maxSpeed)*0.25 + 0.25 * delta;
		if ( delta > 1 )
			delta = 1;
		else if ( delta < -0.25 )
			delta = -0.25;
		if ( m_spawnflags & SF_TRACKTRAIN_FORWARDONLY )
		{
			if ( delta < 0 )
				delta = 0;
		}
		SetDirForward( delta >= 0 );
		delta = fabs(delta);
		SetSpeed( m_maxSpeed * delta );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that sets the speed of the train.
// Input  : Float speed from 0 to max speed, in units per second.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::InputSetSpeedReal( inputdata_t &inputdata )
{
	SetSpeed( clamp( inputdata.value.Float(), 0.f, m_maxSpeed ) );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that sets the speed of the train.
// Input  : Float speed scale from 0 to 1.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::InputSetSpeed( inputdata_t &inputdata )
{
	float flScale = clamp( inputdata.value.Float(), 0.f, 1.f );
	SetSpeed( m_maxSpeed * flScale );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that sets the speed of the train and the direction
//			based on the sign of the speed.
// Input  : Float speed scale from -1 to 1. Negatives values indicate a reversed
//			direction.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::InputSetSpeedDir( inputdata_t &inputdata )
{
	float newSpeed = inputdata.value.Float();
	SetDirForward( newSpeed >= 0 );
	newSpeed = fabs(newSpeed);
	float flScale = clamp( newSpeed, 0.f, 1.f );
	SetSpeed( m_maxSpeed * flScale );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that sets the speed of the train and the direction
//			based on the sign of the speed, and accels/decels to that speed
// Input  : Float speed scale from -1 to 1. Negatives values indicate a reversed
//			direction.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::InputSetSpeedDirAccel( inputdata_t &inputdata )
{
	SetSpeedDirAccel( inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTrackTrain::SetSpeedDirAccel( float flNewSpeed )
{
	float newSpeed = flNewSpeed;
	SetDirForward( newSpeed >= 0 );
	newSpeed = fabs( newSpeed );
	float flScale = clamp( newSpeed, 0.f, 1.f );
	SetSpeed( m_maxSpeed * flScale, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTrackTrain::InputSetSpeedForwardModifier( inputdata_t &inputdata )
{
	SetSpeedForwardModifier( inputdata.value.Float() ) ;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTrackTrain::SetSpeedForwardModifier( float flModifier )
{
	float flSpeedForwardModifier = flModifier;
	flSpeedForwardModifier = fabs( flSpeedForwardModifier );

	m_flSpeedForwardModifier = clamp( flSpeedForwardModifier, 0.f, 1.f );
	SetSpeed( m_flUnmodifiedDesiredSpeed, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTrackTrain::InputTeleportToPathTrack( inputdata_t &inputdata )
{
	const char *pszName = inputdata.value.String();
	CPathTrack *pTrack = dynamic_cast<CPathTrack*>( gEntList.FindEntityByName( NULL, pszName ) );

	if ( pTrack )
	{
		TeleportToPathTrack( pTrack );
		m_ppath = pTrack;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the speed of the train to the given value in units per second.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::SetSpeed( float flSpeed, bool bAccel /*= false */  )
{
	m_bAccelToSpeed = bAccel;

 	m_flUnmodifiedDesiredSpeed = flSpeed;
	float flOldSpeed = m_flSpeed;

	// are we using a speed forward modifier?
	if ( m_flSpeedForwardModifier < 1.0 && m_dir > 0 )
	{
		flSpeed = flSpeed * m_flSpeedForwardModifier;
	}

	if ( m_bAccelToSpeed )
	{
		m_flDesiredSpeed = fabs( flSpeed ) * m_dir;
		m_flSpeedChangeTime = gpGlobals->curtime;

		if ( m_flSpeed == 0 && abs(m_flDesiredSpeed) > 0 )
		{
			m_flSpeed = 0.1;	// little push to get us going
		}

		Start();

		return;		
	}

	m_flSpeed = fabs( flSpeed ) * m_dir;

	if ( m_flSpeed != flOldSpeed)
	{
		// Changing speed.
		if ( m_flSpeed != 0 )
		{
			if ( flOldSpeed == 0 )
			{
				// Starting to move.
				Start();
			}
			else
			{
				// Continuing to move.
				Next();
			}
		}
		else
		{
			// Stopping.
			Stop();
		}
	}

	DevMsg( 2, "TRAIN(%s), speed to %.2f\n", GetDebugName(), m_flSpeed );
}


//-----------------------------------------------------------------------------
// Purpose: Stops the train.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::Stop( void )
{
	SetLocalVelocity( vec3_origin );
	SetLocalAngularVelocity( vec3_angle );
	m_oldSpeed = m_flSpeed;
	m_flSpeed = 0;
	SoundStop();
	SetThink(NULL);
}

static CBaseEntity *FindPhysicsBlockerForHierarchy( CBaseEntity *pParentEntity )
{
	CUtlVector<CBaseEntity *> list;
	GetAllInHierarchy( pParentEntity, list );
	CBaseEntity *pPhysicsBlocker = NULL;
	float maxForce = 0;
	for ( int i = 0; i < list.Count(); i++ )
	{
		IPhysicsObject *pPhysics = list[i]->VPhysicsGetObject();
		if ( pPhysics )
		{
			IPhysicsFrictionSnapshot *pSnapshot = pPhysics->CreateFrictionSnapshot();
			while ( pSnapshot->IsValid() )
			{
				IPhysicsObject *pOther = pSnapshot->GetObject(1);
				CBaseEntity *pOtherEntity = static_cast<CBaseEntity *>(pOther->GetGameData());
				if ( pOtherEntity && pOtherEntity->GetMoveType() == MOVETYPE_VPHYSICS )
				{
					Vector normal;
					pSnapshot->GetSurfaceNormal(normal);
					float dot = DotProduct( pParentEntity->GetAbsVelocity(), pSnapshot->GetNormalForce() * normal );
					if ( !pPhysicsBlocker || dot > maxForce )
					{
						pPhysicsBlocker = pOtherEntity;
						maxForce = dot;
					}
				}
				pSnapshot->NextFrictionData();
			}
			pPhysics->DestroyFrictionSnapshot( pSnapshot );
		}
	}
	return pPhysicsBlocker;
}

//-----------------------------------------------------------------------------
// Purpose: Called when we are blocked by another entity.
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CFuncTrackTrain::Blocked( CBaseEntity *pOther )
{
	// Blocker is on-ground on the train
	if ( ( pOther->GetFlags() & FL_ONGROUND ) && pOther->GetGroundEntity() == this )
	{
		DevMsg( 1, "TRAIN(%s): Blocked by %s\n", GetDebugName(), pOther->GetClassname() );
		float deltaSpeed = fabs(m_flSpeed);
		if ( deltaSpeed > 50 )
			deltaSpeed = 50;

		Vector vecNewVelocity;
		pOther->GetVelocity( &vecNewVelocity );
		if ( !vecNewVelocity.z )
		{
			pOther->ApplyAbsVelocityImpulse( Vector(0,0,deltaSpeed) );
		}
		return;
	}
	else
	{
		Vector vecNewVelocity;
		vecNewVelocity = pOther->GetAbsOrigin() - GetAbsOrigin();
		VectorNormalize(vecNewVelocity);
		vecNewVelocity *= m_flBlockDamage;
		pOther->SetAbsVelocity( vecNewVelocity );
	}
	if ( HasSpawnFlags(SF_TRACKTRAIN_UNBLOCKABLE_BY_PLAYER) )
	{
		CBaseEntity *pPhysicsBlocker = FindPhysicsBlockerForHierarchy(this);
		if ( pPhysicsBlocker )
		{
			// This code keeps track of how long this train has been blocked
			// The heuristic here is to keep instantaneous blocks from invoking the somewhat
			// heavy-handed solver (which will disable collisions until we're clear) in cases
			// where physics can solve it easily enough.
			int ticksBlocked = gpGlobals->tickcount - m_lastBlockTick;
			float dist = 0.0f;
			// wait at least 10 ticks and make sure the train isn't actually moving before really blocking
			const int MIN_BLOCKED_TICKS = 10;
			if ( ticksBlocked > MIN_BLOCKED_TICKS )
			{
				dist = (GetAbsOrigin() - m_lastBlockPos).Length();
				// must have moved at least 10% of normal velocity over the blocking interval, or we're being blocked
				float minLength = GetAbsVelocity().Length() * TICK_INTERVAL * MIN_BLOCKED_TICKS * 0.10f;
				if ( dist < minLength )
				{
					// been stuck for more than one tick without moving much?
					// yes, disable collisions with the physics object most likely to be blocking us
					EntityPhysics_CreateSolver( this, pPhysicsBlocker, true, 4.0f );
				}
			}
			// first time blocking or moved too far since last block, reset
			if ( dist > 1.0f || m_lastBlockTick < 0  )
			{
				m_lastBlockPos = GetAbsOrigin();
				m_lastBlockTick = gpGlobals->tickcount;
			}
		}
		// unblockable shouldn't damage the player in this case
		if ( pOther->IsPlayer() )
			return;
	}

	DevWarning( 2, "TRAIN(%s): Blocked by %s (dmg:%.2f)\n", GetDebugName(), pOther->GetClassname(), m_flBlockDamage );
	if ( m_flBlockDamage <= 0 )
		return;

	// we can't hurt this thing, so we're not concerned with it
	pOther->TakeDamage( CTakeDamageInfo( this, this, m_flBlockDamage, DMG_CRUSH ) );
}


extern void FixupAngles( QAngle &v );

#define TRAIN_MAXSPEED		1000	// approx max speed for sound pitch calculation


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTrackTrain::SoundStop( void )
{
	// if sound playing, stop it
	if ( m_bSoundPlaying )
	{
		if ( m_iszSoundMove != NULL_STRING )
		{
			StopSound( entindex(), CHAN_STATIC, STRING( m_iszSoundMove ) );
		}

		if ( m_iszSoundStop != NULL_STRING )
		{
			CPASAttenuationFilter filter( this );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_ITEM;
			ep.m_pSoundName =  STRING(m_iszSoundStop);
			ep.m_flVolume = m_flVolume;
			ep.m_SoundLevel = SNDLVL_NORM;

			EmitSound( filter, entindex(), ep );
		}
	}

	m_bSoundPlaying = false;
}


//-----------------------------------------------------------------------------
// Purpose: Update pitch based on speed, start sound if not playing.
//			NOTE: when train goes through transition, m_bSoundPlaying should become
//			false, which will cause the looped sound to restart.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::SoundUpdate( void )
{
	if ( ( !m_iszSoundMove ) && ( !m_iszSoundStart ) && ( !m_iszSoundMovePing ))
	{
		return;
	}

	// In multiplayer, only update the sound once a second
	if ( g_pGameRules->IsMultiplayer() && m_bSoundPlaying )
	{
		if ( m_flNextMPSoundTime > gpGlobals->curtime )
			return;

		m_flNextMPSoundTime = gpGlobals->curtime + 1.0;
	}

	float flSpeedRatio = 0;
	if ( HasSpawnFlags( SF_TRACKTRAIN_USE_MAXSPEED_FOR_PITCH ) )
	{
		flSpeedRatio = clamp( fabs( m_flSpeed ) / m_maxSpeed, 0.f, 1.f );
	}
	else
	{
		flSpeedRatio = clamp( fabs( m_flSpeed ) / TRAIN_MAXSPEED, 0.f, 1.f );
	}

	float flpitch = RemapVal( flSpeedRatio, 0, 1, m_nMoveSoundMinPitch, m_nMoveSoundMaxPitch );

	CPASAttenuationFilter filter( this );
	CPASAttenuationFilter filterReliable( this );
	filterReliable.MakeReliable();

	Vector vecWorldSpaceCenter = WorldSpaceCenter();

	if (!m_bSoundPlaying)
	{
		if ( m_iszSoundStart != NULL_STRING )
		{
			EmitSound_t ep;
			ep.m_nChannel = CHAN_ITEM;
			ep.m_pSoundName =  STRING(m_iszSoundStart);
			ep.m_flVolume = m_flVolume;
			ep.m_SoundLevel = SNDLVL_NORM;
			ep.m_pOrigin = &vecWorldSpaceCenter;

			EmitSound( filter, entindex(), ep );
		}

		if ( m_iszSoundMove != NULL_STRING )
		{
			EmitSound_t ep;
			ep.m_nChannel = CHAN_STATIC;
			ep.m_pSoundName =  STRING(m_iszSoundMove);
			ep.m_flVolume = m_flVolume;
			ep.m_SoundLevel = SNDLVL_NORM;
			ep.m_nPitch = (int)flpitch;
			ep.m_pOrigin = &vecWorldSpaceCenter;

			EmitSound( filterReliable, entindex(), ep );
		}

		// We've just started moving. Delay the next move ping sound.
		m_flNextMoveSoundTime = gpGlobals->curtime + RemapVal( flSpeedRatio, 0, 1, m_flMoveSoundMaxTime, m_flMoveSoundMinTime );

		m_bSoundPlaying = true;
	} 
	else
	{
		if ( m_iszSoundMove != NULL_STRING )
		{
			// update pitch
			EmitSound_t ep;
			ep.m_nChannel = CHAN_STATIC;
			ep.m_pSoundName =  STRING(m_iszSoundMove);
			ep.m_flVolume = m_flVolume;
			ep.m_SoundLevel = SNDLVL_NORM;
			ep.m_nPitch = (int)flpitch;
			ep.m_nFlags = SND_CHANGE_PITCH;
			ep.m_pOrigin = &vecWorldSpaceCenter;

			// In multiplayer, don't make this reliable
			if ( g_pGameRules->IsMultiplayer() )
			{
				EmitSound( filter, entindex(), ep );
			}
			else
			{
				EmitSound( filterReliable, entindex(), ep );
			}
		}

		if ( ( m_iszSoundMovePing != NULL_STRING ) && ( gpGlobals->curtime > m_flNextMoveSoundTime ) )
		{
			EmitSound(STRING(m_iszSoundMovePing));
			m_flNextMoveSoundTime = gpGlobals->curtime + RemapVal( flSpeedRatio, 0, 1, m_flMoveSoundMaxTime, m_flMoveSoundMinTime );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pNode - 
//-----------------------------------------------------------------------------
void CFuncTrackTrain::ArriveAtNode( CPathTrack *pNode )
{
	// BUGBUG: This is wrong.  We need to fire all targets between the one we've passed and the one
	// we've switched to.
	FirePassInputs( pNode, pNode->GetNext(), true );

	//
	// Disable train controls if this path track says to do so.
	//
	if ( pNode->HasSpawnFlags( SF_PATH_DISABLE_TRAIN ) )
	{
		m_spawnflags |= SF_TRACKTRAIN_NOCONTROL;
	}
	
	//
	// Don't override the train speed if it's under user control.
	//
	if ( m_spawnflags & SF_TRACKTRAIN_NOCONTROL )
	{
		//
		// Don't copy speed from path track if it is 0 (uninitialized).
		//
		if ( pNode->m_flSpeed != 0 )
		{
			SetSpeed( pNode->m_flSpeed );
			DevMsg( 2, "TrackTrain %s arrived at %s, speed to %4.2f\n", GetDebugName(), pNode->GetDebugName(), pNode->m_flSpeed );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Controls how the train accelerates as it moves along the path.
//-----------------------------------------------------------------------------
TrainVelocityType_t CFuncTrackTrain::GetTrainVelocityType()
{
	return m_eVelocityType;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pnext - 
//-----------------------------------------------------------------------------
void CFuncTrackTrain::UpdateTrainVelocity( CPathTrack *pPrev, CPathTrack *pNext, const Vector &nextPos, float flInterval )
{
	switch ( GetTrainVelocityType() )
	{
		case TrainVelocity_Instantaneous:
		{
			Vector velDesired = nextPos - GetLocalOrigin();
			VectorNormalize( velDesired );
			velDesired *= fabs( m_flSpeed );
			SetLocalVelocity( velDesired );
			break;
		}

		case TrainVelocity_LinearBlend:
		case TrainVelocity_EaseInEaseOut:
		{
			if ( m_bAccelToSpeed )
			{
				float flPrevSpeed = m_flSpeed;
				float flNextSpeed = m_flDesiredSpeed;

				if ( flPrevSpeed != flNextSpeed )
				{
					float flSpeedChangeTime = ( abs(flNextSpeed) > abs(flPrevSpeed) ) ? m_flAccelSpeed : m_flDecelSpeed;
					m_flSpeed = UTIL_Approach( m_flDesiredSpeed, m_flSpeed, flSpeedChangeTime * gpGlobals->frametime );
				}
			}
			else if ( pPrev && pNext )
			{
				// Get the speed to blend from.
				float flPrevSpeed = m_flSpeed;
				if ( pPrev->m_flSpeed != 0 )
				{
					flPrevSpeed = pPrev->m_flSpeed;
				}

				// Get the speed to blend to.
				float flNextSpeed = flPrevSpeed;
				if ( pNext->m_flSpeed != 0 )
				{
					flNextSpeed = pNext->m_flSpeed;
				}

				// If they're different, do the blend.
				if ( flPrevSpeed != flNextSpeed )
				{
					Vector vecSegment = pNext->GetLocalOrigin() - pPrev->GetLocalOrigin();
					float flSegmentLen = vecSegment.Length();
					if ( flSegmentLen )
					{
						Vector vecCurOffset = GetLocalOrigin() - pPrev->GetLocalOrigin();
						float p = vecCurOffset.Length() / flSegmentLen;
						if ( GetTrainVelocityType() == TrainVelocity_EaseInEaseOut )
						{
							p = SimpleSplineRemapVal( p, 0.0f, 1.0f, 0.0f, 1.0f );
						}

						m_flSpeed = m_dir * ( flPrevSpeed * ( 1 - p ) + flNextSpeed * p );
					}
				}
				else
				{
					m_flSpeed = m_dir * flPrevSpeed;
				}
			}

			Vector velDesired = nextPos - GetLocalOrigin();
			VectorNormalize( velDesired );
			velDesired *= fabs( m_flSpeed );
			SetLocalVelocity( velDesired );
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Controls how the train blends angles as it moves along the path.
//-----------------------------------------------------------------------------
TrainOrientationType_t CFuncTrackTrain::GetTrainOrientationType()
{
#ifdef HL1_DLL
	return TrainOrientation_AtPathTracks;
#else
	return m_eOrientationType;
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pnext - 
//-----------------------------------------------------------------------------
void CFuncTrackTrain::UpdateTrainOrientation( CPathTrack *pPrev, CPathTrack *pNext, const Vector &nextPos, float flInterval )
{
	// FIXME: old way of doing fixed orienation trains, remove!
	if ( HasSpawnFlags( SF_TRACKTRAIN_FIXED_ORIENTATION ) )
		return;

	// Trains *can* work in local space, but only if all elements of the track share
	// the same move parent as the train.
	Assert( !pPrev || (pPrev->GetMoveParent() == GetMoveParent()) );

	switch ( GetTrainOrientationType() )
	{
		case TrainOrientation_Fixed:
		{
			// Fixed orientation. Do nothing.
			break;
		}

		case TrainOrientation_AtPathTracks:
		{
			UpdateOrientationAtPathTracks( pPrev, pNext, nextPos, flInterval );
			break;
		}

		case TrainOrientation_EaseInEaseOut:
		case TrainOrientation_LinearBlend:
		{
			UpdateOrientationBlend( GetTrainOrientationType(), pPrev, pNext, nextPos, flInterval );
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Adjusts our angles as we hit each path track. This is for support of
//			trains with wheels that round corners a la HL1 trains.
// FIXME: move into path_track, have the angles come back from LookAhead
//-----------------------------------------------------------------------------
void CFuncTrackTrain::UpdateOrientationAtPathTracks( CPathTrack *pPrev, CPathTrack *pNext, const Vector &nextPos, float flInterval )
{
	if ( !m_ppath )
		return;

	Vector nextFront = GetLocalOrigin();

	CPathTrack *pNextNode = NULL;

	nextFront.z -= m_height;
	if ( m_length > 0 )
	{
		m_ppath->LookAhead( nextFront, IsDirForward() ? m_length : -m_length, 0, &pNextNode );
	}
	else
	{
		m_ppath->LookAhead( nextFront, IsDirForward() ? 100 : -100, 0, &pNextNode );
	}
	nextFront.z += m_height;

	Vector vecFaceDir = nextFront - GetLocalOrigin();
	if ( !IsDirForward() )
	{
		vecFaceDir *= -1;
	}
	QAngle angles;
	VectorAngles( vecFaceDir, angles );
	// !!!  All of this crap has to be done to make the angles not wrap around, revisit this.
	FixupAngles( angles );

	// Wrapped with this bool so we don't affect old trains
	if ( m_bManualSpeedChanges )
	{
		if ( pNextNode && pNextNode->GetOrientationType() == TrackOrientation_FacePathAngles )
		{
			angles = pNextNode->GetOrientation( IsDirForward() );
		}
	}

	QAngle curAngles = GetLocalAngles();
	FixupAngles( curAngles );

	if ( !pPrev || (vecFaceDir.x == 0 && vecFaceDir.y == 0) )
		angles = curAngles;

	DoUpdateOrientation( curAngles, angles, flInterval );
}


//-----------------------------------------------------------------------------
// Purpose: Blends our angles using one of two orientation blending types.
//			ASSUMES that eOrientationType is either LinearBlend or EaseInEaseOut.
//			FIXME: move into path_track, have the angles come back from LookAhead
//-----------------------------------------------------------------------------
void CFuncTrackTrain::UpdateOrientationBlend( TrainOrientationType_t eOrientationType, CPathTrack *pPrev, CPathTrack *pNext, const Vector &nextPos, float flInterval )
{
	// Get the angles to blend from.
	QAngle angPrev = pPrev->GetOrientation( IsDirForward() );
	FixupAngles( angPrev );

	// Get the angles to blend to. 
	QAngle angNext;
	if ( pNext )
	{
		angNext = pNext->GetOrientation( IsDirForward() );
		FixupAngles( angNext );
	}
	else
	{
		// At a dead end, just use the last path track's angles.
		angNext = angPrev;
	}

	if ( m_spawnflags & SF_TRACKTRAIN_NOPITCH )
	{
		angNext[PITCH] = angPrev[PITCH];
	}

	// Calculate our parametric distance along the path segment from 0 to 1.
	float p = 0;
	if ( pPrev && ( angPrev != angNext ) )
	{
		Vector vecSegment = pNext->GetLocalOrigin() - pPrev->GetLocalOrigin();
		float flSegmentLen = vecSegment.Length();
		if ( flSegmentLen )
		{
			Vector vecCurOffset = GetLocalOrigin() - pPrev->GetLocalOrigin();
			p = vecCurOffset.Length() / flSegmentLen;
		}
	}

	if ( eOrientationType == TrainOrientation_EaseInEaseOut )
	{
		p = SimpleSplineRemapVal( p, 0.0f, 1.0f, 0.0f, 1.0f );
	}

	//Msg( "UpdateOrientationFacePathAngles: %s->%s, p=%f, ", pPrev->GetDebugName(), pNext->GetDebugName(), p );

	Quaternion qtPrev;
	Quaternion qtNext;
	
	AngleQuaternion( angPrev, qtPrev );
	AngleQuaternion( angNext, qtNext );

	QAngle angNew = angNext;
	float flAngleDiff = QuaternionAngleDiff( qtPrev, qtNext );
	if ( flAngleDiff )
	{
		Quaternion qtNew;
		QuaternionSlerp( qtPrev, qtNext, p, qtNew );
		QuaternionAngles( qtNew, angNew );
	}

	if ( m_spawnflags & SF_TRACKTRAIN_NOPITCH )
	{
		angNew[PITCH] = angPrev[PITCH];
	}

	DoUpdateOrientation( GetLocalAngles(), angNew, flInterval );
}


//-----------------------------------------------------------------------------
// Purpose: Sets our angular velocity to approach the target angles over the given interval.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::DoUpdateOrientation( const QAngle &curAngles, const QAngle &angles, float flInterval )
{
	float vy, vx;
	if ( !(m_spawnflags & SF_TRACKTRAIN_NOPITCH) )
	{
		vx = UTIL_AngleDistance( angles.x, curAngles.x );
	}
	else
	{
		vx = 0;
	}

	vy = UTIL_AngleDistance( angles.y, curAngles.y );
	
	// HACKHACK: Clamp really small angular deltas to avoid rotating movement on things
	// that are close enough
	if ( fabs(vx) < 0.1 )
	{
		vx = 0;
	}
	if ( fabs(vy) < 0.1 )
	{
		vy = 0;
	}

	if ( flInterval == 0 )
	{
		// Avoid dividing by zero
		flInterval = 0.1;
	}

	QAngle vecAngVel( vx / flInterval, vy / flInterval, GetLocalAngularVelocity().z );

	if ( m_flBank != 0 )
	{
		if ( vecAngVel.y < -5 )
		{
			vecAngVel.z = UTIL_AngleDistance( UTIL_ApproachAngle( -m_flBank, curAngles.z, m_flBank*2 ), curAngles.z);
		}
		else if ( vecAngVel.y > 5 )
		{
			vecAngVel.z = UTIL_AngleDistance( UTIL_ApproachAngle( m_flBank, curAngles.z, m_flBank*2 ), curAngles.z);
		}
		else
		{
			vecAngVel.z = UTIL_AngleDistance( UTIL_ApproachAngle( 0, curAngles.z, m_flBank*4 ), curAngles.z) * 4;
		}
	}
	
	SetLocalAngularVelocity( vecAngVel );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTeleport - 
//-----------------------------------------------------------------------------
void CFuncTrackTrain::TeleportToPathTrack( CPathTrack *pTeleport )
{
	QAngle angCur = GetLocalAngles();

	Vector nextPos = pTeleport->GetLocalOrigin();
	Vector look = nextPos;
	pTeleport->LookAhead( look, m_length, 0 );

	QAngle nextAngles;
	if ( HasSpawnFlags( SF_TRACKTRAIN_FIXED_ORIENTATION ) || ( look == nextPos ) )
	{
		nextAngles = GetLocalAngles();
	}
	else
	{
		nextAngles = pTeleport->GetOrientation( IsDirForward() );
		if ( HasSpawnFlags( SF_TRACKTRAIN_NOPITCH ) )
		{
			nextAngles[PITCH] = angCur[PITCH];
		}
	}

	Teleport( &pTeleport->GetLocalOrigin(), &nextAngles, NULL );
	SetLocalAngularVelocity( vec3_angle );

	variant_t emptyVariant;
	pTeleport->AcceptInput( "InTeleport", this, this, emptyVariant, 0 );
}


//-----------------------------------------------------------------------------
// Purpose: Advances the train to the next path corner on the path.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::Next( void )
{
	if ( !m_flSpeed )
	{
		DevMsg( 2, "TRAIN(%s): Speed is 0\n", GetDebugName() );
		SoundStop();
		return;
	}

	if ( !m_ppath )
	{	
		DevMsg( 2, "TRAIN(%s): Lost path\n", GetDebugName() );
		SoundStop();
		m_flSpeed = 0;
		return;
	}

	SoundUpdate();

	//
	// Based on our current position and speed, look ahead along our path and see
	// where we should be in 0.1 seconds.
	//
	Vector nextPos = GetLocalOrigin();
	float flSpeed = m_flSpeed;

	nextPos.z -= m_height;
	CPathTrack *pNextNext = NULL;
	CPathTrack *pNext = m_ppath->LookAhead( nextPos, flSpeed * 0.1, 1, &pNextNext );
	//Assert( pNext != NULL );

	// If we're moving towards a dead end, but our desired speed goes in the opposite direction
	// this fixes us from stalling
	if ( m_bManualSpeedChanges && ( ( flSpeed < 0 ) != ( m_flDesiredSpeed < 0 ) ) )
	{
		if ( !pNext )
			pNext = m_ppath;
	}

	if (m_debugOverlays & OVERLAY_BBOX_BIT) 
	{
		if ( pNext != NULL )
		{
			NDebugOverlay::Line( GetAbsOrigin(), pNext->GetAbsOrigin(), 255, 0, 0, true, 0.1 );
			NDebugOverlay::Line( pNext->GetAbsOrigin(), pNext->GetAbsOrigin() + Vector( 0,0,32), 255, 0, 0, true, 0.1 );
			NDebugOverlay::Box( pNext->GetAbsOrigin(), Vector( -8, -8, -8 ), Vector( 8, 8, 8 ), 255, 0, 0, 0, 0.1 );
		}

		if ( pNextNext != NULL )
		{
			NDebugOverlay::Line( GetAbsOrigin(), pNextNext->GetAbsOrigin(), 0, 255, 0, true, 0.1 );
			NDebugOverlay::Line( pNextNext->GetAbsOrigin(), pNextNext->GetAbsOrigin() + Vector( 0,0,32), 0, 255, 0, true, 0.1 );
			NDebugOverlay::Box( pNextNext->GetAbsOrigin(), Vector( -8, -8, -8 ), Vector( 8, 8, 8 ), 0, 255, 0, 0, 0.1 );
		}
	}

	nextPos.z += m_height;

	// Trains *can* work in local space, but only if all elements of the track share
	// the same move parent as the train.
	Assert( !pNext || (pNext->GetMoveParent() == GetMoveParent()) );

	if ( pNext )
	{
		UpdateTrainVelocity( pNext, pNextNext, nextPos, gpGlobals->frametime );
		UpdateTrainOrientation( pNext, pNextNext, nextPos, gpGlobals->frametime );

		if ( pNext != m_ppath )
		{
			//
			// We have reached a new path track. Fire its OnPass output.
			//
			m_ppath = pNext;
			ArriveAtNode( pNext );
#ifdef HL1_DLL
			m_bOnTrackChange = false;
#endif

			//
			// See if we should teleport to the next path track.
			//
			CPathTrack *pTeleport = pNext->GetNext();
			if ( ( pTeleport != NULL ) && pTeleport->HasSpawnFlags( SF_PATH_TELEPORT ) )
			{
				TeleportToPathTrack( pTeleport );
			}
		}

		m_OnNext.FireOutput( pNext, this );

		SetThink( &CFuncTrackTrain::Next );
		SetMoveDoneTime( 0.5 );
		SetNextThink( gpGlobals->curtime );
		SetMoveDone( NULL );
	}
	else
	{
		//
		// We've reached the end of the path, stop.
		//
		SoundStop();
		SetLocalVelocity(nextPos - GetLocalOrigin());
		SetLocalAngularVelocity( vec3_angle );
		float distance = GetLocalVelocity().Length();
		m_oldSpeed = m_flSpeed;

		m_flSpeed = 0;
		
		// Move to the dead end
		
		// Are we there yet?
		if ( distance > 0 )
		{
			// no, how long to get there?
			float flTime = distance / fabs( m_oldSpeed );
			SetLocalVelocity( GetLocalVelocity() * (m_oldSpeed / distance) );
			SetMoveDone( &CFuncTrackTrain::DeadEnd );
			SetNextThink( TICK_NEVER_THINK );
			SetMoveDoneTime( flTime );
		}
		else
		{
			DeadEnd();
		}
	}
}


void CFuncTrackTrain::FirePassInputs( CPathTrack *pStart, CPathTrack *pEnd, bool forward )
{
	CPathTrack *pCurrent = pStart;

	// swap if going backward
	if ( !forward )
	{
		pCurrent = pEnd;
		pEnd = pStart;
	}
	variant_t emptyVariant;

	while ( pCurrent && pCurrent != pEnd )
	{
		//Msg("Fired pass on %s\n", STRING(pCurrent->GetEntityName()) );
		pCurrent->AcceptInput( "InPass", this, this, emptyVariant, 0 );
		pCurrent = forward ? pCurrent->GetNext() : pCurrent->GetPrevious();
	}
}


void CFuncTrackTrain::DeadEnd( void )
{
	// Fire the dead-end target if there is one
	CPathTrack *pTrack, *pNext;

	pTrack = m_ppath;

	DevMsg( 2, "TRAIN(%s): Dead end ", GetDebugName() );
	// Find the dead end path node
	// HACKHACK -- This is bugly, but the train can actually stop moving at a different node depending on it's speed
	// so we have to traverse the list to it's end.
	if ( pTrack )
	{
		if ( m_oldSpeed < 0 )
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetPrevious(), true );
				if ( pNext )
					pTrack = pNext;
			} while ( pNext );
		}
		else
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetNext(), true );
				if ( pNext )
					pTrack = pNext;
			} while ( pNext );
		}
	}

	SetLocalVelocity( vec3_origin );
	SetLocalAngularVelocity( vec3_angle );
	if ( pTrack )
	{
		DevMsg( 2, "at %s\n", pTrack->GetDebugName() );
		variant_t emptyVariant;
		pTrack->AcceptInput( "InPass", this, this, emptyVariant, 0 );
	}
	else
	{
		DevMsg( 2, "\n" );
	}
}


void CFuncTrackTrain::SetControls( CBaseEntity *pControls )
{
	Vector offset = pControls->GetLocalOrigin();

	m_controlMins = pControls->WorldAlignMins() + offset;
	m_controlMaxs = pControls->WorldAlignMaxs() + offset;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the entity's origin is within the controls region.
//-----------------------------------------------------------------------------
bool CFuncTrackTrain::OnControls( CBaseEntity *pTest )
{
	Vector offset = pTest->GetLocalOrigin() - GetLocalOrigin();

	if ( m_spawnflags & SF_TRACKTRAIN_NOCONTROL )
		return false;

	// Transform offset into local coordinates
	VMatrix tmp = SetupMatrixAngles( GetLocalAngles() );
	// rotate into local space
	Vector local = tmp.VMul3x3Transpose( offset );

	/*
	NDebugOverlay::Box( GetLocalOrigin(), m_controlMins, m_controlMaxs,
				255, 0, 0, 100, 5.0 );

	NDebugOverlay::Box( GetLocalOrigin() + local, Vector(-5,-5,-5), Vector(5,5,5),
				0, 0, 255, 100, 5.0 );
	*/

	if ( local.x >= m_controlMins.x && local.y >= m_controlMins.y && local.z >= m_controlMins.z &&
		 local.x <= m_controlMaxs.x && local.y <= m_controlMaxs.y && local.z <= m_controlMaxs.z )
		 return true;

	return false;
}


void CFuncTrackTrain::Find( void )
{
	m_ppath = (CPathTrack *)gEntList.FindEntityByName( NULL, m_target );
	if ( !m_ppath )
		return;

	if ( !FClassnameIs( m_ppath, "path_track" ) 
#ifndef PORTAL	//env_portal_path_track is a child of path_track and would like to get found
		 && !FClassnameIs( m_ppath, "env_portal_path_track" )
#endif //#ifndef PORTAL
		)
	{
		Warning( "func_track_train must be on a path of path_track\n" );
		Assert(0);
		m_ppath = NULL;
		return;
	}



	Vector nextPos = m_ppath->GetLocalOrigin();
	Vector look = nextPos;
	m_ppath->LookAhead( look, m_length, 0 );
	nextPos.z += m_height;
	look.z += m_height;

	QAngle nextAngles;
	if ( HasSpawnFlags( SF_TRACKTRAIN_FIXED_ORIENTATION ) )
	{
		nextAngles = GetLocalAngles();
	}
	else
	{
		VectorAngles( look - nextPos, nextAngles );
		if ( HasSpawnFlags( SF_TRACKTRAIN_NOPITCH ) )
		{
			nextAngles.x = 0;
		}
	}

	Teleport( &nextPos, &nextAngles, NULL );

	ArriveAtNode( m_ppath );

	if ( m_flSpeed != 0 )
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
		SetThink( &CFuncTrackTrain::Next );
		SoundUpdate();
	}
}


void CFuncTrackTrain::NearestPath( void )
{
	CBaseEntity *pTrack = NULL;
	CBaseEntity *pNearest = NULL;
	float dist, closest;

	closest = 1024;

	for ( CEntitySphereQuery sphere( GetAbsOrigin(), 1024 ); ( pTrack = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		// filter out non-tracks
		if ( !(pTrack->GetFlags() & (FL_CLIENT|FL_NPC)) && FClassnameIs( pTrack, "path_track" ) )
		{
			dist = (GetAbsOrigin() - pTrack->GetAbsOrigin()).Length();
			if ( dist < closest )
			{
				closest = dist;
				pNearest = pTrack;
			}
		}
	}

	if ( !pNearest )
	{
		Msg( "Can't find a nearby track !!!\n" );
		SetThink(NULL);
		return;
	}

	DevMsg( 2, "TRAIN: %s, Nearest track is %s\n", GetDebugName(), pNearest->GetDebugName() );
	// If I'm closer to the next path_track on this path, then it's my real path
	pTrack = ((CPathTrack *)pNearest)->GetNext();
	if ( pTrack )
	{
		if ( (GetLocalOrigin() - pTrack->GetLocalOrigin()).Length() < (GetLocalOrigin() - pNearest->GetLocalOrigin()).Length() )
			pNearest = pTrack;
	}

	m_ppath = (CPathTrack *)pNearest;

	if ( m_flSpeed != 0 )
	{
		SetMoveDoneTime( 0.1 );
		SetMoveDone( &CFuncTrackTrain::Next );
	}
}

void CFuncTrackTrain::OnRestore( void )
{
	BaseClass::OnRestore();
	if ( !m_ppath 
#ifdef HL1_DLL
		&& !m_bOnTrackChange
#endif	
		)
	{
		NearestPath();
		SetThink( NULL );
	}
}


CFuncTrackTrain *CFuncTrackTrain::Instance( edict_t *pent )
{ 
	CBaseEntity *pEntity = CBaseEntity::Instance( pent );
	if ( FClassnameIs( pEntity, "func_tracktrain" ) )
		return (CFuncTrackTrain *)pEntity;
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTrackTrain::Spawn( void )
{
	if ( m_maxSpeed == 0 )
	{
		if ( m_flSpeed == 0 )
		{
			m_maxSpeed = 100;
		}
		else
		{
			m_maxSpeed = m_flSpeed;
		}
	}

	if ( m_nMoveSoundMinPitch == 0 )
	{
		m_nMoveSoundMinPitch = 60;
	}

	if ( m_nMoveSoundMaxPitch == 0 )
	{
		m_nMoveSoundMaxPitch = 200;
	}

	SetLocalVelocity(vec3_origin);
	SetLocalAngularVelocity( vec3_angle );

	m_dir = 1;

	if ( !m_target )
	{
		Msg("FuncTrackTrain '%s' has no target.\n", GetDebugName());
	}

	SetModel( STRING( GetModelName() ) );
	SetMoveType( MOVETYPE_PUSH );

#ifdef HL1_DLL
	// BUGBUG: For now, just force this for testing.  Remove if we want to tag all of the trains in the levels
	SetSolid( SOLID_BSP );
#else
	SetSolid( HasSpawnFlags( SF_TRACKTRAIN_HL1TRAIN ) ? SOLID_BSP : SOLID_VPHYSICS );
	//SetSolid( SOLID_VPHYSICS );
#endif

	if ( HasSpawnFlags( SF_TRACKTRAIN_UNBLOCKABLE_BY_PLAYER ) )
	{
		AddFlag( FL_UNBLOCKABLE_BY_PLAYER );
	}
	if ( m_spawnflags & SF_TRACKTRAIN_PASSABLE )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
	}

	m_controlMins = CollisionProp()->OBBMins();
	m_controlMaxs = CollisionProp()->OBBMaxs();
	m_controlMaxs.z += 72;
// start trains on the next frame, to make sure their targets have had
// a chance to spawn/activate
	SetThink( &CFuncTrackTrain::Find );
	SetNextThink( gpGlobals->curtime );
	Precache();

	CreateVPhysics();
}


bool CFuncTrackTrain::CreateVPhysics( void )
{
	VPhysicsInitShadow( false, false );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Precaches the train sounds.
//-----------------------------------------------------------------------------
void CFuncTrackTrain::Precache( void )
{
	if (m_flVolume == 0.0)
	{
		m_flVolume = 1.0;
	}

	if ( m_iszSoundMove != NULL_STRING )
	{
		PrecacheScriptSound( STRING( m_iszSoundMove ) );
	}

	if ( m_iszSoundMovePing != NULL_STRING )
	{
		PrecacheScriptSound( STRING( m_iszSoundMovePing ) );
	}

	if ( m_iszSoundStart != NULL_STRING )
	{
		PrecacheScriptSound( STRING( m_iszSoundStart ) );
	}

	if ( m_iszSoundStop != NULL_STRING )
	{
		PrecacheScriptSound( STRING( m_iszSoundStop ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTrackTrain::UpdateOnRemove()
{
	SoundStop();
	BaseClass::UpdateOnRemove();
}

void CFuncTrackTrain::MoveDone()
{
	m_lastBlockPos.Init();
	m_lastBlockTick = -1;
	BaseClass::MoveDone();
}

int CFuncTrackTrain::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( m_bDamageChild )
	{
		if ( FirstMoveChild() )
		{
			FirstMoveChild()->TakeDamage( info );
		}

		return 0;
	}
	else
	{
		return BaseClass::OnTakeDamage( info );
	}
}



//-----------------------------------------------------------------------------
// Purpose: Defines the volume of space that the player must stand in to
//			control the train
//-----------------------------------------------------------------------------
class CFuncTrainControls : public CBaseEntity
{
	DECLARE_CLASS( CFuncTrainControls, CBaseEntity );
public:
	void Spawn( void );
	void Find( void );

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CFuncTrainControls )

	// Function Pointers
	DEFINE_FUNCTION( Find ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_traincontrols, CFuncTrainControls );


void CFuncTrainControls::Find( void )
{
	CBaseEntity *pTarget = NULL;

	do 
	{
		pTarget = gEntList.FindEntityByName( pTarget, m_target );
	} while ( pTarget && !FClassnameIs(pTarget, "func_tracktrain") );

	if ( !pTarget )
	{
		Msg( "No train %s\n", STRING(m_target) );
		return;
	}

	CFuncTrackTrain *ptrain = (CFuncTrackTrain*) pTarget;
	ptrain->SetControls( this );
	
	SetThink( NULL );
}


void CFuncTrainControls::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );
	AddEffects( EF_NODRAW );

	Assert( GetParent() && "func_traincontrols needs parent to properly align to train" );
	
	SetThink( &CFuncTrainControls::Find );
	SetNextThink( gpGlobals->curtime );
}


#define SF_TRACK_ACTIVATETRAIN		0x00000001
#define SF_TRACK_RELINK				0x00000002
#define SF_TRACK_ROTMOVE			0x00000004
#define SF_TRACK_STARTBOTTOM		0x00000008
#define SF_TRACK_DONT_MOVE			0x00000010


typedef enum { TRAIN_SAFE, TRAIN_BLOCKING, TRAIN_FOLLOWING } TRAIN_CODE;


//-----------------------------------------------------------------------------
// This entity is a rotating/moving platform that will carry a train to a new track.
// It must be larger in X-Y planar area than the train, since it must contain the
// train within these dimensions in order to operate when the train is near it.
//-----------------------------------------------------------------------------
class CFuncTrackChange : public CFuncPlatRot
{
	DECLARE_CLASS( CFuncTrackChange, CFuncPlatRot );
public:
	void Spawn( void );
	void Precache( void );

//	virtual void	Blocked( void );
	virtual void	GoUp( void );
	virtual void	GoDown( void );

	void			Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void			Find( void );
	TRAIN_CODE		EvaluateTrain( CPathTrack *pcurrent );
	void			UpdateTrain( QAngle &dest );
	virtual void	HitBottom( void );
	virtual void	HitTop( void );
	void			Touch( CBaseEntity *pOther );
	virtual void	UpdateAutoTargets( int toggleState );
	virtual	bool	IsTogglePlat( void ) { return true; }

	void			DisableUse( void ) { m_use = 0; }
	void			EnableUse( void ) { m_use = 1; }
	int				UseEnabled( void ) { return m_use; }

	DECLARE_DATADESC();

	CPathTrack		*m_trackTop;
	CPathTrack		*m_trackBottom;

	CFuncTrackTrain	*m_train;

	string_t		m_trackTopName;
	string_t		m_trackBottomName;
	string_t		m_trainName;
	TRAIN_CODE		m_code;
	int				m_targetState;
	int				m_use;
};

LINK_ENTITY_TO_CLASS( func_trackchange, CFuncTrackChange );

BEGIN_DATADESC( CFuncTrackChange )

	DEFINE_GLOBAL_FIELD( m_trackTop, FIELD_CLASSPTR ),
	DEFINE_GLOBAL_FIELD( m_trackBottom, FIELD_CLASSPTR ),
	DEFINE_GLOBAL_FIELD( m_train, FIELD_CLASSPTR ),
	DEFINE_GLOBAL_KEYFIELD( m_trackTopName, FIELD_STRING, "toptrack" ),
	DEFINE_GLOBAL_KEYFIELD( m_trackBottomName, FIELD_STRING, "bottomtrack" ),
	DEFINE_GLOBAL_KEYFIELD( m_trainName, FIELD_STRING, "train" ),
	DEFINE_FIELD( m_code, FIELD_INTEGER ),
	DEFINE_FIELD( m_targetState, FIELD_INTEGER ),
	DEFINE_FIELD( m_use, FIELD_INTEGER ),

	// Function Pointers
	DEFINE_FUNCTION( Find ),

END_DATADESC()


void CFuncTrackChange::Spawn( void )
{
	Setup();
	if ( FBitSet( m_spawnflags, SF_TRACK_DONT_MOVE ) )
		m_vecPosition2.z = GetLocalOrigin().z;

	SetupRotation();

	if ( FBitSet( m_spawnflags, SF_TRACK_STARTBOTTOM ) )
	{
		UTIL_SetOrigin( this, m_vecPosition2);
		m_toggle_state = TS_AT_BOTTOM;
		SetLocalAngles( m_start );
		m_targetState = TS_AT_TOP;
	}
	else
	{
		UTIL_SetOrigin( this, m_vecPosition1);
		m_toggle_state = TS_AT_TOP;
		SetLocalAngles( m_end );
		m_targetState = TS_AT_BOTTOM;
	}

	EnableUse();
	SetThink( &CFuncTrackChange::Find );
	SetNextThink( gpGlobals->curtime + 2 );
	Precache();
}


void CFuncTrackChange::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "FuncTrackChange.Blocking" );
}


// UNDONE: Filter touches before re-evaluating the train.
void CFuncTrackChange::Touch( CBaseEntity *pOther )
{
}


void CFuncTrackChange::Find( void )
{
	// Find track entities
	CBaseEntity *target;

	target = gEntList.FindEntityByName( NULL, m_trackTopName );
	if ( target )
	{
		m_trackTop = (CPathTrack*) target;
		target = gEntList.FindEntityByName( NULL, m_trackBottomName );
		if ( target )
		{
			m_trackBottom = (CPathTrack*) target;
			target = gEntList.FindEntityByName( NULL, m_trainName );
			if ( target )
			{
				m_train = (CFuncTrackTrain *)gEntList.FindEntityByName( NULL, m_trainName );
				if ( !m_train )
				{
					Warning( "Can't find train for track change! %s\n", STRING(m_trainName) );
					Assert(0);
					return;
				}
				Vector center = WorldSpaceCenter();
				m_trackBottom = m_trackBottom->Nearest( center );
				m_trackTop = m_trackTop->Nearest( center );
				UpdateAutoTargets( m_toggle_state );
				SetThink( NULL );
				return;
			}
			else
			{
				Warning( "Can't find train for track change! %s\n", STRING(m_trainName) );
				Assert(0);
				target = gEntList.FindEntityByName( NULL, m_trainName );
			}
		}
		else
		{
			Warning( "Can't find bottom track for track change! %s\n", STRING(m_trackBottomName) );
			Assert(0);
		}
	}
	else
	{
		Warning( "Can't find top track for track change! %s\n", STRING(m_trackTopName) );
		Assert(0);
	}
}


TRAIN_CODE CFuncTrackChange::EvaluateTrain( CPathTrack *pcurrent )
{
	// Go ahead and work, we don't have anything to switch, so just be an elevator
	if ( !pcurrent || !m_train )
		return TRAIN_SAFE;

	if ( m_train->m_ppath == pcurrent || (pcurrent->m_pprevious && m_train->m_ppath == pcurrent->m_pprevious) ||
		 (pcurrent->m_pnext && m_train->m_ppath == pcurrent->m_pnext) )
	{
		if ( m_train->m_flSpeed != 0 )
			return TRAIN_BLOCKING;

		Vector dist = GetLocalOrigin() - m_train->GetLocalOrigin();
		float length = dist.Length2D();
		if ( length < m_train->m_length )		// Empirically determined close distance
			return TRAIN_FOLLOWING;
		else if ( length > (150 + m_train->m_length) )
			return TRAIN_SAFE;

		return TRAIN_BLOCKING;
	}
	
	return TRAIN_SAFE;
}


void CFuncTrackChange::UpdateTrain( QAngle &dest )
{
	float time = GetMoveDoneTime();

	m_train->SetAbsVelocity( GetAbsVelocity() );
	m_train->SetLocalAngularVelocity( GetLocalAngularVelocity() );
	m_train->SetMoveDoneTime( time );

	// Attempt at getting the train to rotate properly around the origin of the trackchange
	if ( time <= 0 )
		return;

	Vector offset = m_train->GetLocalOrigin() - GetLocalOrigin();
	QAngle delta = dest - GetLocalAngles();
	// Transform offset into local coordinates
	Vector forward, right, up;
	AngleVectorsTranspose( delta, &forward, &right, &up );
	Vector local;
	local.x = DotProduct( offset, forward );
	local.y = DotProduct( offset, right );
	local.z = DotProduct( offset, up );

	local = local - offset;
	m_train->SetAbsVelocity( GetAbsVelocity() + (local * (1.0/time)) );
}


void CFuncTrackChange::GoDown( void )
{
	if ( m_code == TRAIN_BLOCKING )
		return;

	// HitBottom may get called during CFuncPlat::GoDown(), so set up for that
	// before you call GoDown()

	UpdateAutoTargets( TS_GOING_DOWN );
	// If ROTMOVE, move & rotate
	if ( FBitSet( m_spawnflags, SF_TRACK_DONT_MOVE ) )
	{
		SetMoveDone( &CFuncTrackChange::CallHitBottom );
		m_toggle_state = TS_GOING_DOWN;
		AngularMove( m_start, m_flSpeed );
	}
	else
	{
		BaseClass::GoDown();
		SetMoveDone( &CFuncTrackChange::CallHitBottom );
		RotMove( m_start, GetMoveDoneTime() );
	}
	// Otherwise, rotate first, move second

	// If the train is moving with the platform, update it
	if ( m_code == TRAIN_FOLLOWING )
	{
		UpdateTrain( m_start );
		m_train->m_ppath = NULL;
#ifdef HL1_DLL
		m_train->m_bOnTrackChange = true;
#endif
	}
}


//
// Platform is at bottom, now starts moving up
//
void CFuncTrackChange::GoUp( void )
{
	if ( m_code == TRAIN_BLOCKING )
		return;

	// HitTop may get called during CFuncPlat::GoUp(), so set up for that
	// before you call GoUp();

	UpdateAutoTargets( TS_GOING_UP );
	if ( FBitSet( m_spawnflags, SF_TRACK_DONT_MOVE ) )
	{
		m_toggle_state = TS_GOING_UP;
		SetMoveDone( &CFuncTrackChange::CallHitTop );
		AngularMove( m_end, m_flSpeed );
	}
	else
	{
		// If ROTMOVE, move & rotate
		BaseClass::GoUp();
		SetMoveDone( &CFuncTrackChange::CallHitTop );
		RotMove( m_end, GetMoveDoneTime() );
	}
	
	// Otherwise, move first, rotate second

	// If the train is moving with the platform, update it
	if ( m_code == TRAIN_FOLLOWING )
	{
		UpdateTrain( m_end );
		m_train->m_ppath = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Normal track change
// Input  : toggleState - 
//-----------------------------------------------------------------------------
void CFuncTrackChange::UpdateAutoTargets( int toggleState )
{
	if ( !m_trackTop || !m_trackBottom )
		return;

	if ( toggleState == TS_AT_TOP )
	{
		m_trackTop->RemoveSpawnFlags( SF_PATH_DISABLED );
	}
	else
	{
		m_trackTop->AddSpawnFlags( SF_PATH_DISABLED );
	}

	if ( toggleState == TS_AT_BOTTOM )
	{
		m_trackBottom->RemoveSpawnFlags( SF_PATH_DISABLED );
	}
	else
	{
		m_trackBottom->AddSpawnFlags( SF_PATH_DISABLED );
	}
}


void CFuncTrackChange::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_toggle_state != TS_AT_TOP && m_toggle_state != TS_AT_BOTTOM )
		return;

	// If train is in "safe" area, but not on the elevator, play alarm sound
	if ( m_toggle_state == TS_AT_TOP )
		m_code = EvaluateTrain( m_trackTop );
	else if ( m_toggle_state == TS_AT_BOTTOM )
		m_code = EvaluateTrain( m_trackBottom );
	else
		m_code = TRAIN_BLOCKING;
	if ( m_code == TRAIN_BLOCKING )
	{
		// Play alarm and return
		EmitSound( "FuncTrackChange.Blocking" );
		return;
	}

	// Otherwise, it's safe to move
	// If at top, go down
	// at bottom, go up

	DisableUse();
	if (m_toggle_state == TS_AT_TOP)
		GoDown();
	else
		GoUp();
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncTrackChange::HitBottom( void )
{
	BaseClass::HitBottom();
	if ( m_code == TRAIN_FOLLOWING )
	{
//		UpdateTrain();
		m_train->SetTrack( m_trackBottom );
	}
	SetMoveDone( NULL );
	SetMoveDoneTime( -1 );

	UpdateAutoTargets( m_toggle_state );

	EnableUse();
}


//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncTrackChange::HitTop( void )
{
	BaseClass::HitTop();
	if ( m_code == TRAIN_FOLLOWING )
	{
//		UpdateTrain();
		m_train->SetTrack( m_trackTop );
	}
	
	// Don't let the plat go back down
	SetMoveDone( NULL );
	SetMoveDoneTime( -1 );
	UpdateAutoTargets( m_toggle_state );
	EnableUse();
}


class CFuncTrackAuto : public CFuncTrackChange
{
	DECLARE_CLASS( CFuncTrackAuto, CFuncTrackChange );
public:
	void			Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void	UpdateAutoTargets( int toggleState );
	void			TriggerTrackChange( inputdata_t &inputdata );

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CFuncTrackAuto )
	DEFINE_INPUTFUNC( FIELD_VOID, "Trigger", TriggerTrackChange ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_trackautochange, CFuncTrackAuto );


// Auto track change
void CFuncTrackAuto::UpdateAutoTargets( int toggleState )
{
	CPathTrack *pTarget, *pNextTarget;

	if ( !m_trackTop || !m_trackBottom )
		return;

	if ( m_targetState == TS_AT_TOP )
	{
		pTarget = m_trackTop->GetNext();
		pNextTarget = m_trackBottom->GetNext();
	}
	else
	{
		pTarget = m_trackBottom->GetNext();
		pNextTarget = m_trackTop->GetNext();
	}
	if ( pTarget )
	{
		pTarget->RemoveSpawnFlags( SF_PATH_DISABLED );
		if ( m_code == TRAIN_FOLLOWING && m_train && m_train->m_flSpeed == 0 )
		{
			m_train->SetSpeed( pTarget->m_flSpeed );
			m_train->Use( this, this, USE_SET, 0 );
		}
	}

	if ( pNextTarget )
	{
		pNextTarget->AddSpawnFlags( SF_PATH_DISABLED );
	}
}


void CFuncTrackAuto::TriggerTrackChange ( inputdata_t &inputdata )
{
	CPathTrack *pTarget;

	if ( !UseEnabled() )
		return;

	if ( m_toggle_state == TS_AT_TOP )
		pTarget = m_trackTop;
	else if ( m_toggle_state == TS_AT_BOTTOM )
		pTarget = m_trackBottom;
	else
		pTarget = NULL;

	if ( inputdata.pActivator && FClassnameIs( inputdata.pActivator, "func_tracktrain" ) )
	{
		m_code = EvaluateTrain( pTarget );
		// Safe to fire?
		if ( m_code == TRAIN_FOLLOWING && m_toggle_state != m_targetState )
		{
			DisableUse();
			if (m_toggle_state == TS_AT_TOP)
				GoDown();
			else
				GoUp();
		}
	}
	else
	{
		if ( pTarget )
			pTarget = pTarget->GetNext();
		if ( pTarget && m_train->m_ppath != pTarget && ShouldToggle( USE_TOGGLE, m_targetState ) )
		{
			if ( m_targetState == TS_AT_TOP )
				m_targetState = TS_AT_BOTTOM;
			else
				m_targetState = TS_AT_TOP;
		}

		UpdateAutoTargets( m_targetState );
	}
}


void CFuncTrackAuto::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CPathTrack *pTarget;

	if ( !UseEnabled() )
		return;

	if ( m_toggle_state == TS_AT_TOP )
		pTarget = m_trackTop;
	else if ( m_toggle_state == TS_AT_BOTTOM )
		pTarget = m_trackBottom;
	else
		pTarget = NULL;

	if ( FClassnameIs( pActivator, "func_tracktrain" ) )
	{
		m_code = EvaluateTrain( pTarget );
		// Safe to fire?
		if ( m_code == TRAIN_FOLLOWING && m_toggle_state != m_targetState )
		{
			DisableUse();
			if (m_toggle_state == TS_AT_TOP)
				GoDown();
			else
				GoUp();
		}
	}
	else
	{
		if ( pTarget )
			pTarget = pTarget->GetNext();
		if ( pTarget && m_train->m_ppath != pTarget && ShouldToggle( useType, m_targetState ) )
		{
			if ( m_targetState == TS_AT_TOP )
				m_targetState = TS_AT_BOTTOM;
			else
				m_targetState = TS_AT_TOP;
		}

		UpdateAutoTargets( m_targetState );
	}
}
