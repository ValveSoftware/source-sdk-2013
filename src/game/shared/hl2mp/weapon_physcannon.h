//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WEAPON_PHYSCANNON_H
#define WEAPON_PHYSCANNON_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#include "vcollide_parse.h"
#include "engine/ivdebugoverlay.h"
#include "iviewrender_beams.h"
#include "beamdraw.h"
#include "c_te_effect_dispatch.h"
#include "model_types.h"
#include "clienteffectprecachesystem.h"
#include "fx_interpvalue.h"
#else
#include "hl2mp_player.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "ai_basenpc.h"
#include "player_pickup.h"
#include "physics_prop_ragdoll.h"
#include "globalstate.h"
#include "props.h"
#include "te_effect_dispatch.h"
#include "util.h"
#endif


#include "hl2mp/weapon_hl2mpbasehlmpcombatweapon.h"
#include "vphysics_interface.h"
#include "vphysics/friction.h"

//-----------------------------------------------------------------------------
// Do we have the super-phys gun?
//-----------------------------------------------------------------------------
bool PlayerHasMegaPhysCannon();

// force the physcannon to drop an object (if carried)
void PhysCannonForceDrop( CBaseCombatWeapon *pActiveWeapon, CBaseEntity *pOnlyIfHoldingThis );
void PhysCannonBeginUpgrade( CBaseAnimating *pAnim );

bool PlayerPickupControllerIsHoldingEntity( CBaseEntity *pPickupController, CBaseEntity *pHeldEntity );
float PlayerPickupGetHeldObjectMass( CBaseEntity *pPickupControllerEntity, IPhysicsObject *pHeldObject );
float PhysCannonGetHeldObjectMass( CBaseCombatWeapon *pActiveWeapon, IPhysicsObject *pHeldObject );

CBaseEntity *PhysCannonGetHeldEntity( CBaseCombatWeapon *pActiveWeapon );

// derive from this so we can add save/load data to it
struct game_shadowcontrol_params_t : public hlshadowcontrol_params_t
{
	DECLARE_SIMPLE_DATADESC();
};


#define PHYSCANNON_BEAM_SPRITE "sprites/orangelight1.vmt"
#define PHYSCANNON_BEAM_SPRITE_NOZ "sprites/orangelight1_noz.vmt"
#define PHYSCANNON_GLOW_SPRITE "sprites/glow04_noz"
#define PHYSCANNON_ENDCAP_SPRITE "sprites/orangeflare1"
#define PHYSCANNON_CENTER_GLOW "sprites/orangecore1"
#define PHYSCANNON_BLAST_SPRITE "sprites/orangecore2"

#ifdef CLIENT_DLL

//----------------------------------------------------------------------------------------------------------------------------------------------------------
//  CPhysCannonEffect class
//----------------------------------------------------------------------------------------------------------------------------------------------------------

class CPhysCannonEffect
{
public:
	CPhysCannonEffect( void ) : m_vecColor( 255, 255, 255 ), m_bVisible( true ), m_nAttachment( -1 ) {};

	void SetAttachment( int attachment ) { m_nAttachment = attachment; }
	int	GetAttachment( void ) const { return m_nAttachment; }

	void SetVisible( bool visible = true ) { m_bVisible = visible; }
	int IsVisible( void ) const { return m_bVisible; }

	void SetColor( const Vector& color ) { m_vecColor = color; }
	const Vector& GetColor( void ) const { return m_vecColor; }

	bool SetMaterial( const char* materialName )
	{
		m_hMaterial.Init( materialName, TEXTURE_GROUP_CLIENT_EFFECTS );
		return ( m_hMaterial != NULL );
	}

	CMaterialReference& GetMaterial( void ) { return m_hMaterial; }

	CInterpolatedValue& GetAlpha( void ) { return m_Alpha; }
	CInterpolatedValue& GetScale( void ) { return m_Scale; }

private:
	CInterpolatedValue	m_Alpha;
	CInterpolatedValue	m_Scale;

	Vector				m_vecColor;
	bool				m_bVisible;
	int					m_nAttachment;
	CMaterialReference	m_hMaterial;
};

//----------------------------------------------------------------------------------------------------------------------------------------------------------
//  CPhysCannonEffectBeam class
//----------------------------------------------------------------------------------------------------------------------------------------------------------

class CPhysCannonEffectBeam
{
public:
	CPhysCannonEffectBeam( void ) : m_pBeam( NULL ) {};

	~CPhysCannonEffectBeam( void )
	{
		Release();
	}

	void Release( void )
	{
		if ( m_pBeam != NULL )
		{
			m_pBeam->flags = 0;
			m_pBeam->die = gpGlobals->curtime - 1;

			m_pBeam = NULL;
		}
	}

	void Init( int startAttachment, int endAttachment, CBaseEntity* pEntity, bool firstPerson )
	{
		if ( m_pBeam != NULL )
			return;

		BeamInfo_t beamInfo;

		beamInfo.m_pStartEnt = pEntity;
		beamInfo.m_nStartAttachment = startAttachment;
		beamInfo.m_pEndEnt = pEntity;
		beamInfo.m_nEndAttachment = endAttachment;
		beamInfo.m_nType = TE_BEAMPOINTS;
		beamInfo.m_vecStart = vec3_origin;
		beamInfo.m_vecEnd = vec3_origin;

		beamInfo.m_pszModelName = ( firstPerson ) ? PHYSCANNON_BEAM_SPRITE_NOZ : PHYSCANNON_BEAM_SPRITE;

		beamInfo.m_flHaloScale = 0.0f;
		beamInfo.m_flLife = 0.0f;

		if ( firstPerson )
		{
			beamInfo.m_flWidth = 0.0f;
			beamInfo.m_flEndWidth = 4.0f;
		}
		else
		{
			beamInfo.m_flWidth = 0.5f;
			beamInfo.m_flEndWidth = 2.0f;
		}

		beamInfo.m_flFadeLength = 0.0f;
		beamInfo.m_flAmplitude = 16;
		beamInfo.m_flBrightness = 255.0;
		beamInfo.m_flSpeed = 150.0f;
		beamInfo.m_nStartFrame = 0.0;
		beamInfo.m_flFrameRate = 30.0;
		beamInfo.m_flRed = 255.0;
		beamInfo.m_flGreen = 255.0;
		beamInfo.m_flBlue = 255.0;
		beamInfo.m_nSegments = 8;
		beamInfo.m_bRenderable = true;
		beamInfo.m_nFlags = FBEAM_FOREVER;

		m_pBeam = beams->CreateBeamEntPoint( beamInfo );
	}

	void SetVisible( bool state = true )
	{
		if ( m_pBeam == NULL )
			return;

		m_pBeam->brightness = ( state ) ? 255.0f : 0.0f;
	}

private:
	Beam_t* m_pBeam;
};

#endif

//-----------------------------------------------------------------------------
class CGrabController : public IMotionEvent
{
public:

	CGrabController( void );
	~CGrabController( void );
	void AttachEntity( CBasePlayer *pPlayer, CBaseEntity *pEntity, IPhysicsObject *pPhys, bool bIsMegaPhysCannon, const Vector &vGrabPosition, bool bUseGrabPosition );
	void DetachEntity( bool bClearVelocity );
	void OnRestore();

	bool UpdateObject( CBasePlayer *pPlayer, float flError );

	void SetTargetPosition( const Vector &target, const QAngle &targetOrientation );
	float ComputeError();
	float GetLoadWeight( void ) const { return m_flLoadWeight; }
	void SetAngleAlignment( float alignAngleCosine ) { m_angleAlignment = alignAngleCosine; }
	void SetIgnorePitch( bool bIgnore ) { m_bIgnoreRelativePitch = bIgnore; }
	QAngle TransformAnglesToPlayerSpace( const QAngle &anglesIn, CBasePlayer *pPlayer );
	QAngle TransformAnglesFromPlayerSpace( const QAngle &anglesIn, CBasePlayer *pPlayer );

	CBaseEntity *GetAttached() { return (CBaseEntity *)m_attachedEntity; }

	IMotionEvent::simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );
	float GetSavedMass( IPhysicsObject *pObject );

	QAngle			m_attachedAnglesPlayerSpace;
	Vector			m_attachedPositionObjectSpace;

private:
	// Compute the max speed for an attached object
	void ComputeMaxSpeed( CBaseEntity *pEntity, IPhysicsObject *pPhysics );

	game_shadowcontrol_params_t	m_shadow;
	float			m_timeToArrive;
	float			m_errorTime;
	float			m_error;
	float			m_contactAmount;
	float			m_angleAlignment;
	bool			m_bCarriedEntityBlocksLOS;
	bool			m_bIgnoreRelativePitch;

	float			m_flLoadWeight;
	float			m_savedRotDamping[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	float			m_savedMass[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	EHANDLE			m_attachedEntity;
	QAngle			m_vecPreferredCarryAngles;
	bool			m_bHasPreferredCarryAngles;


	IPhysicsMotionController *m_controller;
	int				m_frameCount;
	friend class CWeaponPhysCannon;
};


//----------------------------------------------------------------------------------------------------------------------------------------------------------
//  CWeaponPhysCannon class
//----------------------------------------------------------------------------------------------------------------------------------------------------------

class CGrabController;

#ifdef CLIENT_DLL
#define CWeaponPhysCannon C_WeaponPhysCannon
#endif

class CWeaponPhysCannon : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponPhysCannon, CBaseHL2MPCombatWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponPhysCannon( void );
	~CWeaponPhysCannon( void );

	void	Drop( const Vector &vecVelocity );
	void	Precache();
	virtual void	OnRestore();
	virtual void	StopLoopingSounds();
	virtual void	UpdateOnRemove(void);
	void	PrimaryAttack();
	void	SecondaryAttack();
	void	WeaponIdle();
	void	ItemPreFrame();
	void	ItemPostFrame();

	void	ForceDrop( void );
	bool	DropIfEntityHeld( CBaseEntity *pTarget );	// Drops its held entity if it matches the entity passed in
	CGrabController &GetGrabController() { return m_grabController; }

	bool	CanHolster( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	bool	Deploy( void );

	bool	HasAnyAmmo( void ) { return true; }

	virtual void SetViewModel( void );
	virtual const char *GetShootSound( int iIndex ) const;
	
#ifndef CLIENT_DLL
	CNetworkQAngle	( m_attachedAnglesPlayerSpace );
#else
	QAngle m_attachedAnglesPlayerSpace;
#endif

	CNetworkVector	( m_attachedPositionObjectSpace );

	CNetworkHandle( CBaseEntity, m_hAttachedObject );

	EHANDLE m_hOldAttachedObject;

	bool	CanPickupObject( CBaseEntity *pTarget );
protected:
	enum FindObjectResult_t
	{
		OBJECT_FOUND = 0,
		OBJECT_NOT_FOUND,
		OBJECT_BEING_DETACHED,
	};

	void	DoEffect( int effectType, Vector *pos = NULL );

	void	OpenElements( void );
	void	CloseElements( void );

	// Pickup and throw objects.
	void	CheckForTarget( void );
	
#ifndef CLIENT_DLL
	bool	AttachObject( CBaseEntity *pObject, const Vector &vPosition );
	FindObjectResult_t		FindObject( void );
	CBaseEntity *FindObjectInCone( const Vector &vecOrigin, const Vector &vecDir, float flCone );
#endif	// !CLIENT_DLL

	void	UpdateObject( void );
	void	DetachObject( bool playSound = true, bool wasLaunched = false );
	void	LaunchObject( const Vector &vecDir, float flForce );
	void	StartEffects( void );	// Initialize all sprites and beams
	void	StopEffects( bool stopSound = true );	// Hide all effects temporarily
	void	DestroyEffects( void );	// Destroy all sprites and beams

	// Punt objects - this is pointing at an object in the world and applying a force to it.
	void	PuntNonVPhysics( CBaseEntity *pEntity, const Vector &forward, trace_t &tr );
	void	PuntVPhysics( CBaseEntity *pEntity, const Vector &forward, trace_t &tr );

	// Velocity-based throw common to punt and launch code.
	void	ApplyVelocityBasedForce( CBaseEntity *pEntity, const Vector &forward );

	// Physgun effects
	void	DoEffectClosed( void );
	void	DoEffectReady( void );
	void	DoEffectHolding( void );
	void	DoEffectLaunch( Vector *pos );
	void	DoEffectNone( void );
	void	DoEffectIdle( void );

	// Trace length
	float	TraceLength();

	// Sprite scale factor 
	float	SpriteScaleFactor();

	float			GetLoadPercentage();
	CSoundPatch		*GetMotorSound( void );

	void	DryFire( void );
	void	PrimaryFireEffect( void );

#ifndef CLIENT_DLL
	// What happens when the physgun picks up something 
	void	Physgun_OnPhysGunPickup( CBaseEntity *pEntity, CBasePlayer *pOwner, PhysGunPickup_t reason );
#endif	// !CLIENT_DLL

#ifdef CLIENT_DLL

	enum EffectType_t
	{
		PHYSCANNON_CORE = 0,
		
		PHYSCANNON_BLAST,

		PHYSCANNON_GLOW1,	// Must be in order!
		PHYSCANNON_GLOW2,
		PHYSCANNON_GLOW3,
		PHYSCANNON_GLOW4,
		PHYSCANNON_GLOW5,
		PHYSCANNON_GLOW6,

		PHYSCANNON_ENDCAP1,	// Must be in order!
		PHYSCANNON_ENDCAP2,
		PHYSCANNON_ENDCAP3,	// Only used in third-person!

		NUM_PHYSCANNON_PARAMETERS	// Must be last!
	};

#define	NUM_GLOW_SPRITES ((CWeaponPhysCannon::PHYSCANNON_GLOW6-CWeaponPhysCannon::PHYSCANNON_GLOW1)+1)
#define NUM_ENDCAP_SPRITES ((CWeaponPhysCannon::PHYSCANNON_ENDCAP3-CWeaponPhysCannon::PHYSCANNON_ENDCAP1)+1)

#define	NUM_PHYSCANNON_BEAMS	3

	virtual int		DrawModel( int flags );
	virtual void	ViewModelDrawn( C_BaseViewModel *pBaseViewModel );
	virtual bool	IsTransparent( void );
	virtual void	OnDataChanged( DataUpdateType_t type );
	virtual void	ClientThink( void );
	
	void			ManagePredictedObject( void );
	void			DrawEffects( void );
	void			GetEffectParameters( EffectType_t effectID, color32 &color, float &scale, IMaterial **pMaterial, Vector &vecAttachment );
	void			DrawEffectSprite( EffectType_t effectID );
	inline bool		IsEffectVisible( EffectType_t effectID );
	void			UpdateElementPosition( void );

	// We need to render opaque and translucent pieces
	RenderGroup_t	GetRenderGroup( void ) {	return RENDER_GROUP_TWOPASS;	}

	CInterpolatedValue		m_ElementParameter;							// Used to interpolate the position of the articulated elements
	CPhysCannonEffect		m_Parameters[NUM_PHYSCANNON_PARAMETERS];	// Interpolated parameters for the effects
	CPhysCannonEffectBeam	m_Beams[NUM_PHYSCANNON_BEAMS];				// Beams

	int				m_nOldEffectState;	// Used for parity checks
	bool			m_bOldOpen;			// Used for parity checks

	void			NotifyShouldTransmit( ShouldTransmitState_t state );

#endif	// CLIENT_DLL

	int		m_nChangeState;				// For delayed state change of elements
	float	m_flCheckSuppressTime;		// Amount of time to suppress the checking for targets
	bool	m_flLastDenySoundPlayed;	// Debounce for deny sound
	int		m_nAttack2Debounce;

	CNetworkVar( bool,	m_bActive );
	CNetworkVar( int,	m_EffectState );		// Current state of the effects on the gun
	CNetworkVar( bool,	m_bOpen );

	bool	m_bResetOwnerEntity;
	
	float	m_flElementDebounce;

	CSoundPatch			*m_sndMotor;		// Whirring sound for the gun
	
	CGrabController		m_grabController;

	float	m_flRepuntObjectTime;
	EHANDLE m_hLastPuntedObject;

private:
	CWeaponPhysCannon( const CWeaponPhysCannon & );

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif
};

#endif // WEAPON_PHYSCANNON_H
