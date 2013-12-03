//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "beam_shared.h"
#include "player.h"
#include "gamerules.h"
#include "basecombatweapon.h"
#include "baseviewmodel.h"
#include "vphysics/constraints.h"
#include "physics.h"
#include "in_buttons.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "ndebugoverlay.h"
#include "physics_saverestore.h"
#include "player_pickup.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar phys_gunmass("phys_gunmass", "200");
ConVar phys_gunvel("phys_gunvel", "400");
ConVar phys_gunforce("phys_gunforce", "5e5" );
ConVar phys_guntorque("phys_guntorque", "100" );
ConVar phys_gunglueradius("phys_gunglueradius", "128" );

static int g_physgunBeam;
#define PHYSGUN_BEAM_SPRITE		"sprites/physbeam.vmt"

#define MAX_PELLETS	16

class CWeaponGravityGun;

class CGravityPellet : public CBaseAnimating
{
	DECLARE_CLASS( CGravityPellet, CBaseAnimating );
public:
	DECLARE_DATADESC();

	~CGravityPellet();
	void Precache()
	{
		SetModelName( MAKE_STRING( "models/weapons/glueblob.mdl" ) );
		PrecacheModel( STRING( GetModelName() ) );
		BaseClass::Precache();
	}
	void Spawn()
	{
		Precache();
		SetModel( STRING( GetModelName() ) );
		SetSolid( SOLID_NONE );
		SetMoveType( MOVETYPE_NONE );
		AddEffects( EF_NOSHADOW );
		SetRenderColor( 255, 0, 0 );
		m_isInert = false;
	}

	bool IsInert()
	{
		return m_isInert;
	}
	
	bool MakeConstraint( CBaseEntity *pObject )
	{
		IPhysicsObject *pReference = g_PhysWorldObject;
		if ( GetMoveParent() )
		{
			pReference = GetMoveParent()->VPhysicsGetObject();
		}
		IPhysicsObject *pAttached = pObject->VPhysicsGetObject();
		if ( !pReference || !pAttached )
		{
			return false;
		}

		constraint_fixedparams_t fixed;
		fixed.Defaults();
		fixed.InitWithCurrentObjectState( pReference, pAttached );

		m_pConstraint = physenv->CreateFixedConstraint( pReference, pAttached, NULL, fixed );
		m_pConstraint->SetGameData( (void *)this );

		MakeInert();
		return true;
	}

	void MakeInert()
	{
		SetRenderColor( 64, 64, 128 );
		m_isInert = true;
	}

	void InputOnBreak( inputdata_t &inputdata )
	{
		UTIL_Remove(this);
	}

	IPhysicsConstraint	*m_pConstraint;
	bool				m_isInert;
};

LINK_ENTITY_TO_CLASS(gravity_pellet, CGravityPellet);
PRECACHE_REGISTER(gravity_pellet);

BEGIN_DATADESC( CGravityPellet )

	DEFINE_PHYSPTR( m_pConstraint ),
	DEFINE_FIELD( m_isInert, FIELD_BOOLEAN ),
	// physics system will fire this input if the constraint breaks due to physics
	DEFINE_INPUTFUNC( FIELD_VOID, "ConstraintBroken", InputOnBreak ),

END_DATADESC()


CGravityPellet::~CGravityPellet()
{
	if ( m_pConstraint )
	{
		physenv->DestroyConstraint( m_pConstraint );
	}
}

class CGravControllerPoint : public IMotionEvent
{
	DECLARE_SIMPLE_DATADESC();

public:
	CGravControllerPoint( void );
	~CGravControllerPoint( void );
	void AttachEntity( CBaseEntity *pEntity, IPhysicsObject *pPhys, const Vector &position );
	void DetachEntity( void );
	void SetMaxVelocity( float maxVel )
	{
		m_maxVel = maxVel;
	}
	void SetTargetPosition( const Vector &target )
	{
		m_targetPosition = target;
		if ( m_attachedEntity == NULL )
		{
			m_worldPosition = target;
		}
		m_timeToArrive = gpGlobals->frametime;
	}

	void SetAutoAlign( const Vector &localDir, const Vector &localPos, const Vector &worldAlignDir, const Vector &worldAlignPos )
	{
		m_align = true;
		m_localAlignNormal = -localDir;
		m_localAlignPosition = localPos;
		m_targetAlignNormal = worldAlignDir;
		m_targetAlignPosition = worldAlignPos;
	}

	void ClearAutoAlign()
	{
		m_align = false;
	}

	IMotionEvent::simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );
	Vector			m_localPosition;
	Vector			m_targetPosition;
	Vector			m_worldPosition;
	Vector			m_localAlignNormal;
	Vector			m_localAlignPosition;
	Vector			m_targetAlignNormal;
	Vector			m_targetAlignPosition;
	bool			m_align;
	float			m_saveDamping;
	float			m_maxVel;
	float			m_maxAcceleration;
	Vector			m_maxAngularAcceleration;
	EHANDLE			m_attachedEntity;
	QAngle			m_targetRotation;
	float			m_timeToArrive;

	IPhysicsMotionController *m_controller;
};


BEGIN_SIMPLE_DATADESC( CGravControllerPoint )

	DEFINE_FIELD( m_localPosition,		FIELD_VECTOR ),
	DEFINE_FIELD( m_targetPosition,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_worldPosition,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_localAlignNormal,		FIELD_VECTOR ),
	DEFINE_FIELD( m_localAlignPosition,	FIELD_VECTOR ),
	DEFINE_FIELD( m_targetAlignNormal,	FIELD_VECTOR ),
	DEFINE_FIELD( m_targetAlignPosition,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_align,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_saveDamping,			FIELD_FLOAT ),
	DEFINE_FIELD( m_maxVel,				FIELD_FLOAT ),
	DEFINE_FIELD( m_maxAcceleration,		FIELD_FLOAT ),
	DEFINE_FIELD( m_maxAngularAcceleration,	FIELD_VECTOR ),
	DEFINE_FIELD( m_attachedEntity,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_targetRotation,		FIELD_VECTOR ),
	DEFINE_FIELD( m_timeToArrive,			FIELD_FLOAT ),

	// Physptrs can't be saved in embedded classes... this is to silence classcheck
	// DEFINE_PHYSPTR( m_controller ),

END_DATADESC()


CGravControllerPoint::CGravControllerPoint( void )
{
	m_attachedEntity = NULL;
}

CGravControllerPoint::~CGravControllerPoint( void )
{
	DetachEntity();
}


void CGravControllerPoint::AttachEntity( CBaseEntity *pEntity, IPhysicsObject *pPhys, const Vector &position )
{
	m_attachedEntity = pEntity;
	pPhys->WorldToLocal( &m_localPosition, position );
	m_worldPosition = position;
	pPhys->GetDamping( NULL, &m_saveDamping );
	float damping = 2;
	pPhys->SetDamping( NULL, &damping );
	m_controller = physenv->CreateMotionController( this );
	m_controller->AttachObject( pPhys, true );
	m_controller->SetPriority( IPhysicsMotionController::HIGH_PRIORITY );
	SetTargetPosition( position );
	m_maxAcceleration = phys_gunforce.GetFloat() * pPhys->GetInvMass();
	m_targetRotation = pEntity->GetAbsAngles();
	float torque = phys_guntorque.GetFloat();
	m_maxAngularAcceleration = torque * pPhys->GetInvInertia();
}

void CGravControllerPoint::DetachEntity( void )
{
	CBaseEntity *pEntity = m_attachedEntity;
	if ( pEntity )
	{
		IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
		if ( pPhys )
		{
			// on the odd chance that it's gone to sleep while under anti-gravity
			pPhys->Wake();
			pPhys->SetDamping( NULL, &m_saveDamping );
		}
	}
	m_attachedEntity = NULL;
	physenv->DestroyMotionController( m_controller );
	m_controller = NULL;

	// UNDONE: Does this help the networking?
	m_targetPosition = vec3_origin;
	m_worldPosition = vec3_origin;
}

void AxisAngleQAngle( const Vector &axis, float angle, QAngle &outAngles )
{
	// map back to HL rotation axes
	outAngles.z = axis.x * angle;
	outAngles.x = axis.y * angle;
	outAngles.y = axis.z * angle;
}

IMotionEvent::simresult_e CGravControllerPoint::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	Vector vel;
	AngularImpulse angVel;

	float fracRemainingSimTime = 1.0;
	if ( m_timeToArrive > 0 )
	{
		fracRemainingSimTime *= deltaTime / m_timeToArrive;
		if ( fracRemainingSimTime > 1 )
		{
			fracRemainingSimTime = 1;
		}
	}
	
	m_timeToArrive -= deltaTime;
	if ( m_timeToArrive < 0 )
	{
		m_timeToArrive = 0;
	}

	float invDeltaTime = (1.0f / deltaTime);
	Vector world;
	pObject->LocalToWorld( &world, m_localPosition );
	m_worldPosition = world;
	pObject->GetVelocity( &vel, &angVel );
	//pObject->GetVelocityAtPoint( world, &vel );
	float damping = 1.0;
	world += vel * deltaTime * damping;
	Vector delta = (m_targetPosition - world) * fracRemainingSimTime * invDeltaTime;
	Vector alignDir;
	linear = vec3_origin;
	angular = vec3_origin;

	if ( m_align )
	{
		QAngle angles;
		Vector origin;
		Vector axis;
		AngularImpulse torque;

		pObject->GetShadowPosition( &origin, &angles );
		// align local normal to target normal
		VMatrix tmp = SetupMatrixOrgAngles( origin, angles );
		Vector worldNormal = tmp.VMul3x3( m_localAlignNormal );
		axis = CrossProduct( worldNormal, m_targetAlignNormal );
		float trig = VectorNormalize(axis);
		float alignRotation = RAD2DEG(asin(trig));
		axis *= alignRotation;
		if ( alignRotation < 10 )
		{
			float dot = DotProduct( worldNormal, m_targetAlignNormal );
			// probably 180 degrees off
			if ( dot < 0 )
			{
				if ( worldNormal.x < 0.5 )
				{
					axis.Init(10,0,0);
				}
				else
				{
					axis.Init(0,0,10);
				}
				alignRotation = 10;
			}
		}
		
		// Solve for the rotation around the target normal (at the local align pos) that will 
		// move the grabbed spot to the destination.
		Vector worldRotCenter = tmp.VMul4x3( m_localAlignPosition );
		Vector rotSrc = world - worldRotCenter;
		Vector rotDest = m_targetPosition - worldRotCenter;

		// Get a basis in the plane perpendicular to m_targetAlignNormal
		Vector srcN = rotSrc;
		VectorNormalize( srcN );
		Vector tangent = CrossProduct( srcN, m_targetAlignNormal );
		float len = VectorNormalize( tangent );

		// needs at least ~5 degrees, or forget rotation (0.08 ~= sin(5))
		if ( len > 0.08 )
		{
			Vector binormal = CrossProduct( m_targetAlignNormal, tangent );

			// Now project the src & dest positions into that plane
			Vector planeSrc( DotProduct( rotSrc, tangent ), DotProduct( rotSrc, binormal ), 0 );
			Vector planeDest( DotProduct( rotDest, tangent ), DotProduct( rotDest, binormal ), 0 );

			float rotRadius = VectorNormalize( planeSrc );
			float destRadius = VectorNormalize( planeDest );
			if ( rotRadius > 0.1 )
			{
				if ( destRadius < rotRadius )
				{
					destRadius = rotRadius;
				}
				//float ratio = rotRadius / destRadius;
				float angleSrc = atan2( planeSrc.y, planeSrc.x );
				float angleDest = atan2( planeDest.y, planeDest.x );
				float angleDiff = angleDest - angleSrc;
				angleDiff = RAD2DEG(angleDiff);
				axis += m_targetAlignNormal * angleDiff;
				//world = m_targetPosition;// + rotDest * (1-ratio);
//				NDebugOverlay::Line( worldRotCenter, worldRotCenter-m_targetAlignNormal*50, 255, 0, 0, false, 0.1 );
//				NDebugOverlay::Line( worldRotCenter, worldRotCenter+tangent*50, 0, 255, 0, false, 0.1 );
//				NDebugOverlay::Line( worldRotCenter, worldRotCenter+binormal*50, 0, 0, 255, false, 0.1 );
			}
		}

		torque = WorldToLocalRotation( tmp, axis, 1 );
		torque *= fracRemainingSimTime * invDeltaTime;
		torque -= angVel * 1.0;	 // damping
		for ( int i = 0; i < 3; i++ )
		{
			if ( torque[i] > 0 )
			{
				if ( torque[i] > m_maxAngularAcceleration[i] )
					torque[i] = m_maxAngularAcceleration[i];
			}
			else
			{
				if ( torque[i] < -m_maxAngularAcceleration[i] )
					torque[i] = -m_maxAngularAcceleration[i];
			}
		}
		torque *= invDeltaTime;
		angular += torque;
		// Calculate an acceleration that pulls the object toward the constraint
		// When you're out of alignment, don't pull very hard
		float factor = fabsf(alignRotation);
		if ( factor < 5 )
		{
			factor = clamp( factor, 0, 5 ) * (1/5);
			alignDir = m_targetAlignPosition - worldRotCenter;
			// Limit movement to the part along m_targetAlignNormal if worldRotCenter is on the backside of 
			// of the target plane (one inch epsilon)!
			float planeForward = DotProduct( alignDir, m_targetAlignNormal );
			if ( planeForward > 1 )
			{
				alignDir = m_targetAlignNormal * planeForward;
			}
			Vector accel = alignDir * invDeltaTime * fracRemainingSimTime * (1-factor) * 0.20 * invDeltaTime;
			float mag = accel.Length();
			if ( mag > m_maxAcceleration )
			{
				accel *= (m_maxAcceleration/mag);
			}
			linear += accel;
		}
		linear -= vel*damping*invDeltaTime;
		// UNDONE: Factor in the change in worldRotCenter due to applied torque!
	}
	else
	{
		// clamp future velocity to max speed
		Vector nextVel = delta + vel;
		float nextSpeed = nextVel.Length();
		if ( nextSpeed > m_maxVel )
		{
			nextVel *= (m_maxVel / nextSpeed);
			delta = nextVel - vel;
		}

		delta *= invDeltaTime;

		float linearAccel = delta.Length();
		if ( linearAccel > m_maxAcceleration )
		{
			delta *= m_maxAcceleration / linearAccel;
		}

		Vector accel;
		AngularImpulse angAccel;
		pObject->CalculateForceOffset( delta, world, &accel, &angAccel );
		
		linear += accel;
		angular += angAccel;
	}
	
	return SIM_GLOBAL_ACCELERATION;
}


struct pelletlist_t
{
	DECLARE_SIMPLE_DATADESC();

	Vector						localNormal;	// normal in parent space
	CHandle<CGravityPellet>		pellet;
	EHANDLE						parent;
};

class CWeaponGravityGun : public CBaseCombatWeapon
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CWeaponGravityGun, CBaseCombatWeapon );

	CWeaponGravityGun();
	void Spawn( void );
	void OnRestore( void );
	void Precache( void );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );
	void ItemPostFrame( void );
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo )
	{
		EffectDestroy();
		return BaseClass::Holster();
	}

	bool Reload( void );
	void Equip( CBaseCombatCharacter *pOwner )
	{
		// add constraint ammo
		pOwner->SetAmmoCount( MAX_PELLETS, m_iSecondaryAmmoType );
		BaseClass::Equip( pOwner );
	}
	void Drop(const Vector &vecVelocity)
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		pOwner->SetAmmoCount( 0, m_iSecondaryAmmoType );
		// destroy all constraints
		BaseClass::Drop(vecVelocity);
	}

	bool HasAnyAmmo( void );

	void AttachObject( CBaseEntity *pEdict, const Vector& start, const Vector &end, float distance );
	void DetachObject( void );

	void EffectCreate( void );
	void EffectUpdate( void );
	void EffectDestroy( void );

	void SoundCreate( void );
	void SoundDestroy( void );
	void SoundStop( void );
	void SoundStart( void );
	void SoundUpdate( void );
	void AddPellet( CGravityPellet *pPellet, CBaseEntity *pParent, const Vector &surfaceNormal );
	void DeleteActivePellets();
	void SortPelletsForObject( CBaseEntity *pObject );
	void SetObjectPelletsColor( int r, int g, int b );
	void CreatePelletAttraction( float radius, CBaseEntity *pObject );
	IPhysicsObject *GetPelletPhysObject( int pelletIndex );
	void GetPelletWorldCoords( int pelletIndex, Vector *worldPos, Vector *worldNormal )
	{
		if ( worldPos )
		{
			*worldPos = m_activePellets[pelletIndex].pellet->GetAbsOrigin();
		}
		if ( worldNormal )
		{
			if ( m_activePellets[pelletIndex].parent )
			{
				EntityMatrix tmp;
				tmp.InitFromEntity( m_activePellets[pelletIndex].parent );
				*worldNormal = tmp.LocalToWorldRotation( m_activePellets[pelletIndex].localNormal );
			}
			else
			{
				*worldNormal = m_activePellets[pelletIndex].localNormal;
			}
		}
	}

	int ObjectCaps( void ) 
	{ 
		int caps = BaseClass::ObjectCaps();
		if ( m_active )
		{
			caps |= FCAP_DIRECTIONAL_USE;
		}
		return caps;
	}

	CBaseEntity *GetBeamEntity();

	DECLARE_SERVERCLASS();

private:
	CNetworkVar( int, m_active );
	bool		m_useDown;
	EHANDLE		m_hObject;
	float		m_distance;
	float		m_movementLength;
	float		m_lastYaw;
	int			m_soundState;
	CNetworkVar( int, m_viewModelIndex );
	Vector		m_originalObjectPosition;

	CGravControllerPoint		m_gravCallback;
	pelletlist_t m_activePellets[MAX_PELLETS];
	int			m_pelletCount;
	int			m_objectPelletCount;
	
	int			m_pelletHeld;
	int			m_pelletAttract;
	float		m_glueTime;
	CNetworkVar( bool, m_glueTouching );
};

IMPLEMENT_SERVERCLASS_ST( CWeaponGravityGun, DT_WeaponGravityGun )
	SendPropVector( SENDINFO_NAME(m_gravCallback.m_targetPosition, m_targetPosition), -1, SPROP_COORD ),
	SendPropVector( SENDINFO_NAME(m_gravCallback.m_worldPosition, m_worldPosition), -1, SPROP_COORD ),
	SendPropInt( SENDINFO(m_active), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_glueTouching), 1, SPROP_UNSIGNED ),
	SendPropModelIndex( SENDINFO(m_viewModelIndex) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_physgun, CWeaponGravityGun );
PRECACHE_WEAPON_REGISTER(weapon_physgun);

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_SIMPLE_DATADESC( pelletlist_t )

	DEFINE_FIELD( localNormal,				FIELD_VECTOR ),
	DEFINE_FIELD( pellet,						FIELD_EHANDLE ),
	DEFINE_FIELD( parent,						FIELD_EHANDLE ),

END_DATADESC()

BEGIN_DATADESC( CWeaponGravityGun )

	DEFINE_FIELD( m_active,				FIELD_INTEGER ),
	DEFINE_FIELD( m_useDown,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hObject,				FIELD_EHANDLE ),
	DEFINE_FIELD( m_distance,			FIELD_FLOAT ),
	DEFINE_FIELD( m_movementLength,		FIELD_FLOAT ),
	DEFINE_FIELD( m_lastYaw,				FIELD_FLOAT ),
	DEFINE_FIELD( m_soundState,			FIELD_INTEGER ),
	DEFINE_FIELD( m_viewModelIndex,		FIELD_INTEGER ),
	DEFINE_FIELD( m_originalObjectPosition,	FIELD_POSITION_VECTOR ),
	DEFINE_EMBEDDED( m_gravCallback ),
	// Physptrs can't be saved in embedded classes..
	DEFINE_PHYSPTR( m_gravCallback.m_controller ),
	DEFINE_EMBEDDED_AUTO_ARRAY( m_activePellets ),
	DEFINE_FIELD( m_pelletCount,			FIELD_INTEGER ),
	DEFINE_FIELD( m_objectPelletCount,	FIELD_INTEGER ),
	DEFINE_FIELD( m_pelletHeld,			FIELD_INTEGER ),
	DEFINE_FIELD( m_pelletAttract,		FIELD_INTEGER ),
	DEFINE_FIELD( m_glueTime,			FIELD_TIME ),
	DEFINE_FIELD( m_glueTouching,		FIELD_BOOLEAN ),

END_DATADESC()


enum physgun_soundstate { SS_SCANNING, SS_LOCKEDON };
enum physgun_soundIndex { SI_LOCKEDON = 0, SI_SCANNING = 1, SI_LIGHTOBJECT = 2, SI_HEAVYOBJECT = 3, SI_ON, SI_OFF };


//=========================================================
//=========================================================

CWeaponGravityGun::CWeaponGravityGun()
{
	m_active = false;
	m_bFiresUnderwater = true;
	m_pelletAttract = -1;
	m_pelletHeld = -1;
}

//=========================================================
//=========================================================
void CWeaponGravityGun::Spawn( )
{
	BaseClass::Spawn();
//	SetModel( GetWorldModel() );

	FallInit();
}

void CWeaponGravityGun::OnRestore( void )
{
	BaseClass::OnRestore();

	if ( m_gravCallback.m_controller )
	{
		m_gravCallback.m_controller->SetEventHandler( &m_gravCallback );
	}
}


//=========================================================
//=========================================================
void CWeaponGravityGun::Precache( void )
{
	BaseClass::Precache();

	g_physgunBeam = PrecacheModel(PHYSGUN_BEAM_SPRITE);

	PrecacheScriptSound( "Weapon_Physgun.Scanning" );
	PrecacheScriptSound( "Weapon_Physgun.LockedOn" );
	PrecacheScriptSound( "Weapon_Physgun.Scanning" );
	PrecacheScriptSound( "Weapon_Physgun.LightObject" );
	PrecacheScriptSound( "Weapon_Physgun.HeavyObject" );
}

void CWeaponGravityGun::EffectCreate( void )
{
	EffectUpdate();
	m_active = true;
}


void CWeaponGravityGun::EffectUpdate( void )
{
	Vector start, angles, forward, right;
	trace_t tr;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	m_viewModelIndex = pOwner->entindex();
	// Make sure I've got a view model
	CBaseViewModel *vm = pOwner->GetViewModel();
	if ( vm )
	{
		m_viewModelIndex = vm->entindex();
	}

	pOwner->EyeVectors( &forward, &right, NULL );

	start = pOwner->Weapon_ShootPosition();
	Vector end = start + forward * 4096;

	UTIL_TraceLine( start, end, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );
	end = tr.endpos;
	float distance = tr.fraction * 4096;
	if ( tr.fraction != 1 )
	{
		// too close to the player, drop the object
		if ( distance < 36 )
		{
			DetachObject();
			return;
		}
	}

	if ( m_hObject == NULL && tr.DidHitNonWorldEntity() )
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		// inform the object what was hit
		ClearMultiDamage();
		pEntity->DispatchTraceAttack( CTakeDamageInfo( pOwner, pOwner, 0, DMG_PHYSGUN ), forward, &tr );
		ApplyMultiDamage();
		AttachObject( pEntity, start, tr.endpos, distance );
		m_lastYaw = pOwner->EyeAngles().y;
	}

	// Add the incremental player yaw to the target transform
	matrix3x4_t curMatrix, incMatrix, nextMatrix;
	AngleMatrix( m_gravCallback.m_targetRotation, curMatrix );
	AngleMatrix( QAngle(0,pOwner->EyeAngles().y - m_lastYaw,0), incMatrix );
	ConcatTransforms( incMatrix, curMatrix, nextMatrix );
	MatrixAngles( nextMatrix, m_gravCallback.m_targetRotation );
	m_lastYaw = pOwner->EyeAngles().y;

	CBaseEntity *pObject = m_hObject;
	if ( pObject )
	{
		if ( m_useDown )
		{
			if ( pOwner->m_afButtonPressed & IN_USE )
			{
				m_useDown = false;
			}
		}
		else 
		{
			if ( pOwner->m_afButtonPressed & IN_USE )
			{
				m_useDown = true;
			}
		}

		if ( m_useDown )
		{
			pOwner->SetPhysicsFlag( PFLAG_DIROVERRIDE, true );
			if ( pOwner->m_nButtons & IN_FORWARD )
			{
				m_distance = UTIL_Approach( 1024, m_distance, gpGlobals->frametime * 100 );
			}
			if ( pOwner->m_nButtons & IN_BACK )
			{
				m_distance = UTIL_Approach( 40, m_distance, gpGlobals->frametime * 100 );
			}
		}

		if ( pOwner->m_nButtons & IN_WEAPON1 )
		{
			m_distance = UTIL_Approach( 1024, m_distance, m_distance * 0.1 );
		}
		if ( pOwner->m_nButtons & IN_WEAPON2 )
		{
			m_distance = UTIL_Approach( 40, m_distance, m_distance * 0.1 );
		}

		// Send the object a physics damage message (0 damage). Some objects interpret this 
		// as something else being in control of their physics temporarily.
		pObject->TakeDamage( CTakeDamageInfo( this, pOwner, 0, DMG_PHYSGUN ) );

		Vector newPosition = start + forward * m_distance;
		// 24 is a little larger than 16 * sqrt(2) (extent of player bbox)
		// HACKHACK: We do this so we can "ignore" the player and the object we're manipulating
		// If we had a filter for tracelines, we could simply filter both ents and start from "start"
		Vector awayfromPlayer = start + forward * 24;

		UTIL_TraceLine( start, awayfromPlayer, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction == 1 )
		{
			UTIL_TraceLine( awayfromPlayer, newPosition, MASK_SOLID, pObject, COLLISION_GROUP_NONE, &tr );
			Vector dir = tr.endpos - newPosition;
			float distance = VectorNormalize(dir);
			float maxDist = m_gravCallback.m_maxVel * gpGlobals->frametime;
			if ( distance >  maxDist )
			{
				newPosition += dir * maxDist;
		}
		else
		{
			newPosition = tr.endpos;
		}
		}
		else
		{
			newPosition = tr.endpos;
		}

		CreatePelletAttraction( phys_gunglueradius.GetFloat(), pObject );
			
		// If I'm looking more than 20 degrees away from the glue point, then give up
		// This lets the player "gesture" for the glue to let go.
		Vector pelletDir = m_gravCallback.m_worldPosition - start;
		VectorNormalize(pelletDir);
		if ( DotProduct( pelletDir, forward ) < 0.939 )	// 0.939 ~= cos(20deg)
			{
			// lose attach for 2 seconds if you're too far away
			m_glueTime = gpGlobals->curtime + 1;
			}

		if ( m_pelletHeld >= 0 && gpGlobals->curtime > m_glueTime )
		{
			CGravityPellet *pPelletAttract = m_activePellets[m_pelletAttract].pellet;

			g_pEffects->Sparks( pPelletAttract->GetAbsOrigin() );
		}

		m_gravCallback.SetTargetPosition( newPosition );
		Vector dir = (newPosition - pObject->GetLocalOrigin());
		m_movementLength = dir.Length();
	}
	else
	{
		m_gravCallback.SetTargetPosition( end );
	}
	if ( m_pelletHeld >= 0 && gpGlobals->curtime > m_glueTime )
	{
		Vector worldNormal, worldPos;
		GetPelletWorldCoords( m_pelletAttract, &worldPos, &worldNormal );

		m_gravCallback.SetAutoAlign( m_activePellets[m_pelletHeld].localNormal, m_activePellets[m_pelletHeld].pellet->GetLocalOrigin(), worldNormal, worldPos );
	}
	else
	{
		m_gravCallback.ClearAutoAlign();
	}
}

void CWeaponGravityGun::SoundCreate( void )
{
	m_soundState = SS_SCANNING;
	SoundStart();
}


void CWeaponGravityGun::SoundDestroy( void )
{
	SoundStop();
}


void CWeaponGravityGun::SoundStop( void )
{
	switch( m_soundState )
	{
	case SS_SCANNING:
		GetOwner()->StopSound( "Weapon_Physgun.Scanning" );
		break;
	case SS_LOCKEDON:
		GetOwner()->StopSound( "Weapon_Physgun.Scanning" );
		GetOwner()->StopSound( "Weapon_Physgun.LockedOn" );
		GetOwner()->StopSound( "Weapon_Physgun.LightObject" );
		GetOwner()->StopSound( "Weapon_Physgun.HeavyObject" );
		break;
	}
}



//-----------------------------------------------------------------------------
// Purpose: returns the linear fraction of value between low & high (0.0 - 1.0) * scale
//			e.g. UTIL_LineFraction( 1.5, 1, 2, 1 ); will return 0.5 since 1.5 is
//			halfway between 1 and 2
// Input  : value - a value between low & high (clamped)
//			low - the value that maps to zero
//			high - the value that maps to "scale"
//			scale - the output scale
// Output : parametric fraction between low & high
//-----------------------------------------------------------------------------
static float UTIL_LineFraction( float value, float low, float high, float scale )
{
	if ( value < low )
		value = low;
	if ( value > high )
		value = high;

	float delta = high - low;
	if ( delta == 0 )
		return 0;
	
	return scale * (value-low) / delta;
}

void CWeaponGravityGun::SoundStart( void )
{
	CPASAttenuationFilter filter( GetOwner() );
	filter.MakeReliable();

	switch( m_soundState )
	{
	case SS_SCANNING:
		{
			EmitSound( filter, GetOwner()->entindex(), "Weapon_Physgun.Scanning" );
		}
		break;
	case SS_LOCKEDON:
		{
			// BUGBUG - If you start a sound with a pitch of 100, the pitch shift doesn't work!
			
			EmitSound( filter, GetOwner()->entindex(), "Weapon_Physgun.LockedOn" );
			EmitSound( filter, GetOwner()->entindex(), "Weapon_Physgun.Scanning" );
			EmitSound( filter, GetOwner()->entindex(), "Weapon_Physgun.LightObject" );
			EmitSound( filter, GetOwner()->entindex(), "Weapon_Physgun.HeavyObject" );
		}
		break;
	}
													//   volume, att, flags, pitch
}

void CWeaponGravityGun::SoundUpdate( void )
{
	int newState;
	
	if ( m_hObject )
		newState = SS_LOCKEDON;
	else
		newState = SS_SCANNING;

	if ( newState != m_soundState )
	{
		SoundStop();
		m_soundState = newState;
		SoundStart();
	}

	switch( m_soundState )
	{
	case SS_SCANNING:
		break;
	case SS_LOCKEDON:
		{
			CPASAttenuationFilter filter( GetOwner() );
			filter.MakeReliable();

			float height = m_hObject->GetAbsOrigin().z - m_originalObjectPosition.z;

			// go from pitch 90 to 150 over a height of 500
			int pitch = 90 + (int)UTIL_LineFraction( height, 0, 500, 60 );

			CSoundParameters params;
			if ( GetParametersForSound( "Weapon_Physgun.LockedOn", params, NULL ) )
			{
				EmitSound_t ep( params );
				ep.m_nFlags = SND_CHANGE_VOL | SND_CHANGE_PITCH;
				ep.m_nPitch = pitch;

				EmitSound( filter, GetOwner()->entindex(), ep );
			}

			// attenutate the movement sounds over 200 units of movement
			float distance = UTIL_LineFraction( m_movementLength, 0, 200, 1.0 );

			// blend the "mass" sounds between 50 and 500 kg
			IPhysicsObject *pPhys = m_hObject->VPhysicsGetObject();
			
			float fade = UTIL_LineFraction( pPhys->GetMass(), 50, 500, 1.0 );

			if ( GetParametersForSound( "Weapon_Physgun.LightObject", params, NULL ) )
			{
				EmitSound_t ep( params );
				ep.m_nFlags = SND_CHANGE_VOL;
				ep.m_flVolume = fade * distance;

				EmitSound( filter, GetOwner()->entindex(), ep );
			}

			if ( GetParametersForSound( "Weapon_Physgun.HeavyObject", params, NULL ) )
			{
				EmitSound_t ep( params );
				ep.m_nFlags = SND_CHANGE_VOL;
				ep.m_flVolume = (1.0 - fade) * distance;

				EmitSound( filter, GetOwner()->entindex(), ep );
			}
		}
		break;
	}
}


void CWeaponGravityGun::AddPellet( CGravityPellet *pPellet, CBaseEntity *pAttach, const Vector &surfaceNormal )
{
	Assert(m_pelletCount<MAX_PELLETS);

	m_activePellets[m_pelletCount].localNormal = surfaceNormal;
	if ( pAttach )
	{
		EntityMatrix tmp;
		tmp.InitFromEntity( pAttach );
		m_activePellets[m_pelletCount].localNormal = tmp.WorldToLocalRotation( surfaceNormal );
	}
	m_activePellets[m_pelletCount].pellet = pPellet;
	m_activePellets[m_pelletCount].parent = pAttach;
	m_pelletCount++;
}

void CWeaponGravityGun::SortPelletsForObject( CBaseEntity *pObject )
{
	m_objectPelletCount = 0;
	for ( int i = 0; i < m_pelletCount; i++ )
	{
		// move pellets attached to the active object to the front of the list
		if ( m_activePellets[i].parent == pObject && !m_activePellets[i].pellet->IsInert() )
		{
			if ( i != 0 )
			{
				pelletlist_t tmp = m_activePellets[m_objectPelletCount];
				m_activePellets[m_objectPelletCount] = m_activePellets[i];
				m_activePellets[i] = tmp;
			}
			m_objectPelletCount++;
		}
	}

	SetObjectPelletsColor( 192, 255, 192 );
}

void CWeaponGravityGun::SetObjectPelletsColor( int r, int g, int b )
{
	color32 color;
	color.r = r;
	color.g = g;
	color.b = b;
	color.a = 255;

	for ( int i = 0; i < m_objectPelletCount; i++ )
	{
		CGravityPellet *pPellet = m_activePellets[i].pellet;
		if ( !pPellet || pPellet->IsInert() )
			continue;

		pPellet->m_clrRender = color;
	}
}

CBaseEntity *CWeaponGravityGun::GetBeamEntity()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return NULL;

	// Make sure I've got a view model
	CBaseViewModel *vm = pOwner->GetViewModel();
	if ( vm )
		return vm;

	return pOwner;
}

void CWeaponGravityGun::DeleteActivePellets()
{
	CBaseEntity *pEnt = GetBeamEntity();

	for ( int i = 0; i < m_pelletCount; i++ )
	{
		CGravityPellet *pPellet = m_activePellets[i].pellet;
		if ( !pPellet )
			continue;

		Vector forward;
		AngleVectors( pPellet->GetAbsAngles(), &forward );
		g_pEffects->Dust( pPellet->GetAbsOrigin(), forward, 32, 30 );

		// UNDONE: Probably should just do this client side
		CBeam *pBeam = CBeam::BeamCreate( PHYSGUN_BEAM_SPRITE, 1.5 );
		pBeam->PointEntInit( pPellet->GetAbsOrigin(), pEnt );
		pBeam->SetEndAttachment( 1 );
		pBeam->SetBrightness( 255 );
		pBeam->SetColor( 255, 0, 0 );
		pBeam->RelinkBeam();
		pBeam->LiveForTime( 0.1 );

		UTIL_Remove( pPellet );
	}
	m_pelletCount = 0;
}

void CWeaponGravityGun::CreatePelletAttraction( float radius, CBaseEntity *pObject )
{
	int nearPellet = -1;
	int objectPellet = -1;
	float best = radius*radius;
	// already have a pellet, check for in range
	if ( m_pelletAttract >= 0 )
	{
		Vector attract, held;
		GetPelletWorldCoords( m_pelletAttract, &attract, NULL );
		GetPelletWorldCoords( m_pelletHeld, &held, NULL );
		float dist = (attract - held).Length();
		if ( dist < radius * 2 )
		{
			nearPellet = m_pelletAttract;
			objectPellet = m_pelletHeld;
			best = dist * dist;
		}
	}

	if ( nearPellet < 0 )
	{

		for ( int i = 0; i < m_objectPelletCount; i++ )
	{
		CGravityPellet *pPellet = m_activePellets[i].pellet;
			if ( !pPellet )
				continue;
			for ( int j = m_objectPelletCount; j < m_pelletCount; j++ )
		{
				CGravityPellet *pTest = m_activePellets[j].pellet;
				if ( !pTest )
				continue;

				if ( pTest->IsInert() )
					continue;
				float distSqr = (pTest->GetAbsOrigin() - pPellet->GetAbsOrigin()).LengthSqr();
			if ( distSqr < best )
			{
					Vector worldPos, worldNormal;
					GetPelletWorldCoords( j, &worldPos, &worldNormal );
					// don't attract backside pellets (unless current pellet - prevent oscillation)
					float dist = DotProduct( worldPos, worldNormal );
					if ( m_pelletAttract == j || DotProduct( pPellet->GetAbsOrigin(), worldNormal ) - dist >= 0 )
					{
				best = distSqr;
						nearPellet = j;
						objectPellet = i;
					}
				}
			}
		}
	}

	m_glueTouching = false;
	if ( nearPellet < 0 || objectPellet < 0 )
	{
		m_pelletAttract = -1;
		m_pelletHeld = -1;
		return;
	}

	if ( nearPellet != m_pelletAttract || objectPellet != m_pelletHeld )
			{
		m_glueTime = gpGlobals->curtime;

		m_pelletAttract = nearPellet;
		m_pelletHeld = objectPellet;
	}

	// check for bonding
	if ( best < 3*3 )
				{
					// This makes the pull towards the pellet stop getting stronger since some part of 
					// the object is touching
		m_glueTouching = true;
		}
	}


IPhysicsObject *CWeaponGravityGun::GetPelletPhysObject( int pelletIndex )
{
	if ( pelletIndex < 0 )
		return NULL;

	CBaseEntity *pEntity = m_activePellets[pelletIndex].parent;
	if ( pEntity )
		return pEntity->VPhysicsGetObject();
	
	return g_PhysWorldObject;
}

void CWeaponGravityGun::EffectDestroy( void )
{
	m_active = false;
	SoundStop();

	DetachObject();
}

void CWeaponGravityGun::DetachObject( void )
{
	m_pelletHeld = -1;
	m_pelletAttract = -1;
	m_glueTouching = false;
	SetObjectPelletsColor( 255, 0, 0 );
	m_objectPelletCount = 0;

	if ( m_hObject )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		Pickup_OnPhysGunDrop( m_hObject, pOwner, DROPPED_BY_CANNON );

		m_gravCallback.DetachEntity();
		m_hObject = NULL;
	}
}

void CWeaponGravityGun::AttachObject( CBaseEntity *pObject, const Vector& start, const Vector &end, float distance )
{
	m_hObject = pObject;
	m_useDown = false;
	IPhysicsObject *pPhysics = pObject ? (pObject->VPhysicsGetObject()) : NULL;
	if ( pPhysics && pObject->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		m_distance = distance;

		m_gravCallback.AttachEntity( pObject, pPhysics, end );
		float mass = pPhysics->GetMass();
		Msg( "Object mass: %.2f lbs (%.2f kg)\n", kg2lbs(mass), mass );
		float vel = phys_gunvel.GetFloat();
		if ( mass > phys_gunmass.GetFloat() )
		{
			vel = (vel*phys_gunmass.GetFloat())/mass;
		}
		m_gravCallback.SetMaxVelocity( vel );
//		Msg( "Object mass: %.2f lbs (%.2f kg) %f %f %f\n", kg2lbs(mass), mass, pObject->GetAbsOrigin().x, pObject->GetAbsOrigin().y, pObject->GetAbsOrigin().z );
//		Msg( "ANG: %f %f %f\n", pObject->GetAbsAngles().x, pObject->GetAbsAngles().y, pObject->GetAbsAngles().z );

		m_originalObjectPosition = pObject->GetAbsOrigin();

		m_pelletAttract = -1;
		m_pelletHeld = -1;

		pPhysics->Wake();
		SortPelletsForObject( pObject );
		
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if( pOwner )
		{
			Pickup_OnPhysGunPickup( pObject, pOwner );
		}
	}
	else
	{
		m_hObject = NULL;
	}
}

//=========================================================
//=========================================================
void CWeaponGravityGun::PrimaryAttack( void )
{
	if ( !m_active )
	{
		SendWeaponAnim( ACT_VM_PRIMARYATTACK );
		EffectCreate();
		SoundCreate();
	}
	else
	{
		EffectUpdate();
		SoundUpdate();
	}
}

void CWeaponGravityGun::SecondaryAttack( void )
{
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.1;
	if ( m_active )
	{
		EffectDestroy();
		SoundDestroy();
		return;
	}

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	Assert( pOwner );

	if ( pOwner->GetAmmoCount(m_iSecondaryAmmoType) <= 0 )
		return;

	m_viewModelIndex = pOwner->entindex();
	// Make sure I've got a view model
	CBaseViewModel *vm = pOwner->GetViewModel();
	if ( vm )
	{
		m_viewModelIndex = vm->entindex();
	}

	Vector forward;
	pOwner->EyeVectors( &forward );

	Vector start = pOwner->Weapon_ShootPosition();
	Vector end = start + forward * 4096;

	trace_t tr;
	UTIL_TraceLine( start, end, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction == 1.0 || (tr.surface.flags & SURF_SKY) )
		return;

	CBaseEntity *pHit = tr.m_pEnt;
	
	if ( pHit->entindex() == 0 )
	{
		pHit = NULL;
	}
	else
	{
		// if the object has no physics object, or isn't a physprop or brush entity, then don't glue
		if ( !pHit->VPhysicsGetObject() || pHit->GetMoveType() != MOVETYPE_VPHYSICS )
			return;
	}

	QAngle angles;
	WeaponSound( SINGLE );
	pOwner->RemoveAmmo( 1, m_iSecondaryAmmoType );

	VectorAngles( tr.plane.normal, angles );
	Vector endPoint = tr.endpos + tr.plane.normal;
	CGravityPellet *pPellet = (CGravityPellet *)CBaseEntity::Create( "gravity_pellet", endPoint, angles, this );
	if ( pHit )
	{
		pPellet->SetParent( pHit );
	}
	AddPellet( pPellet, pHit, tr.plane.normal );

	// UNDONE: Probably should just do this client side
	CBaseEntity *pEnt = GetBeamEntity();
	CBeam *pBeam = CBeam::BeamCreate( PHYSGUN_BEAM_SPRITE, 1.5 );
	pBeam->PointEntInit( endPoint, pEnt );
	pBeam->SetEndAttachment( 1 );
	pBeam->SetBrightness( 255 );
	pBeam->SetColor( 255, 0, 0 );
	pBeam->RelinkBeam();
	pBeam->LiveForTime( 0.1 );

}

void CWeaponGravityGun::WeaponIdle( void )
{
	if ( HasWeaponIdleTimeElapsed() )
	{
		SendWeaponAnim( ACT_VM_IDLE );
		if ( m_active )
		{
			CBaseEntity *pObject = m_hObject;
			// pellet is touching object, so glue it
			if ( pObject && m_glueTouching )
			{
				CGravityPellet *pPellet = m_activePellets[m_pelletAttract].pellet;
				if ( pPellet->MakeConstraint( pObject ) )
				{
					WeaponSound( SPECIAL1 );
					m_flNextPrimaryAttack = gpGlobals->curtime + 0.75;
					m_activePellets[m_pelletHeld].pellet->MakeInert();
				}
			}

			EffectDestroy();
			SoundDestroy();
		}
	}
}

void CWeaponGravityGun::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
		return;

	if ( pOwner->m_afButtonPressed & IN_ATTACK2 )
	{
		SecondaryAttack();
	}
	else if ( pOwner->m_nButtons & IN_ATTACK )
	{
		PrimaryAttack();
	}
	else if ( pOwner->m_afButtonPressed & IN_RELOAD )
	{
		Reload();
	}
	// -----------------------
	//  No buttons down
	// -----------------------
	else 
	{
		WeaponIdle( );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGravityGun::HasAnyAmmo( void )
{
	//Always report that we have ammo
	return true;
}

//=========================================================
//=========================================================
bool CWeaponGravityGun::Reload( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner->GetAmmoCount(m_iSecondaryAmmoType) != MAX_PELLETS )
	{
		pOwner->SetAmmoCount( MAX_PELLETS, m_iSecondaryAmmoType );
		DeleteActivePellets();
		WeaponSound( RELOAD );
		return true;
	}

	return false;
}

#define NUM_COLLISION_TESTS 2500
void CC_CollisionTest( const CCommand &args )
{
	if ( !physenv )
		return;

	Msg( "Testing collision system\n" );
	int i;
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_start");
	Vector start = pSpot->GetAbsOrigin();
	static Vector *targets = NULL;
	static bool first = true;
	static float test[2] = {1,1};
	if ( first )
	{
		targets = new Vector[NUM_COLLISION_TESTS];
		float radius = 0;
		float theta = 0;
		float phi = 0;
		for ( i = 0; i < NUM_COLLISION_TESTS; i++ )
		{
			radius += NUM_COLLISION_TESTS * 123.123;
			radius = fabs(fmod(radius, 128));
			theta += NUM_COLLISION_TESTS * 76.76;
			theta = fabs(fmod(theta, DEG2RAD(360)));
			phi += NUM_COLLISION_TESTS * 1997.99;
			phi = fabs(fmod(phi, DEG2RAD(180)));
			
			float st, ct, sp, cp;
			SinCos( theta, &st, &ct );
			SinCos( phi, &sp, &cp );

			targets[i].x = radius * ct * sp;
			targets[i].y = radius * st * sp;
			targets[i].z = radius * cp;
			
			// make the trace 1024 units long
			Vector dir = targets[i] - start;
			VectorNormalize(dir);
			targets[i] = start + dir * 1024;
		}
		first = false;
	}

	//Vector results[NUM_COLLISION_TESTS];

	int testType = 0;
	if ( args.ArgC() >= 2 )
	{
		testType = atoi( args[1] );
	}
	float duration = 0;
	Vector size[2];
	size[0].Init(0,0,0);
	size[1].Init(16,16,16);
	unsigned int dots = 0;

	for ( int j = 0; j < 2; j++ )
	{
		float startTime = engine->Time();
		if ( testType == 1 )
		{
			const CPhysCollide *pCollide = g_PhysWorldObject->GetCollide();
			trace_t tr;

			for ( i = 0; i < NUM_COLLISION_TESTS; i++ )
			{
				physcollision->TraceBox( start, targets[i], -size[j], size[j], pCollide, vec3_origin, vec3_angle, &tr );
				dots += physcollision->ReadStat(0);
				//results[i] = tr.endpos;
			}
		}
		else
		{
			testType = 0;
			CBaseEntity *pWorld = GetContainingEntity( INDEXENT(0) );
			trace_t tr;

			for ( i = 0; i < NUM_COLLISION_TESTS; i++ )
			{
				UTIL_TraceModel( start, targets[i], -size[j], size[j], pWorld, COLLISION_GROUP_NONE, &tr );
				//results[i] = tr.endpos;
			}
		}

		duration += engine->Time() - startTime;
	}
	test[testType] = duration;
	Msg("%d collisions in %.2f ms (%u dots)\n", NUM_COLLISION_TESTS, duration*1000, dots );
	Msg("Current speed ratio: %.2fX BSP:JGJK\n", test[1] / test[0] );
#if 0
	int red = 255, green = 0, blue = 0;
	for ( i = 0; i < NUM_COLLISION_TESTS; i++ )
	{
		NDebugOverlay::Line( start, results[i], red, green, blue, false, 2 );
	}
#endif
}
static ConCommand collision_test("collision_test", CC_CollisionTest, "Tests collision system", FCVAR_CHEAT );
