//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef COMBATWEAPON_SHARED_H
#define COMBATWEAPON_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "sharedInterface.h"
#include "vphysics_interface.h"
#include "predictable_entity.h"
#include "soundflags.h"
#include "weapon_parse.h"
#include "baseviewmodel_shared.h"
#include "weapon_proficiency.h"
#include "utlmap.h"

#if defined( CLIENT_DLL )
#define CBaseCombatWeapon C_BaseCombatWeapon
#endif

// Hacky
#if defined ( TF_CLIENT_DLL ) || defined ( TF_DLL )
#include "econ_entity.h"
#endif // TF_CLIENT_DLL || TF_DLL

#if !defined( CLIENT_DLL )
extern void OnBaseCombatWeaponCreated( CBaseCombatWeapon * );
extern void OnBaseCombatWeaponDestroyed( CBaseCombatWeapon * );

void *SendProxy_SendLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );
#endif

class CBasePlayer;
class CBaseCombatCharacter;
class IPhysicsConstraint;
class CUserCmd;

// How many times to display altfire hud hints (per weapon)
#define WEAPON_ALTFIRE_HUD_HINT_COUNT	1
#define WEAPON_RELOAD_HUD_HINT_COUNT	1

//Start with a constraint in place (don't drop to floor)
#define	SF_WEAPON_START_CONSTRAINED	(1<<0)	
#define SF_WEAPON_NO_PLAYER_PICKUP	(1<<1)
#define SF_WEAPON_NO_PHYSCANNON_PUNT (1<<2)

//Percent
#define	CLIP_PERC_THRESHOLD		0.75f	

// Put this in your derived class definition to declare it's activity table
// UNDONE: Cascade these?
#define DECLARE_ACTTABLE()		static acttable_t m_acttable[];\
	virtual acttable_t *ActivityList( int &iActivityCount ) OVERRIDE;

// You also need to include the activity table itself in your class' implementation:
// e.g.
//	acttable_t	CWeaponStunstick::m_acttable[] = 
//	{
//		{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, TRUE },
//	};
//
// The stunstick overrides the ACT_MELEE_ATTACK1 activity, replacing it with ACT_MELEE_ATTACK_SWING.
// This animation is required for this weapon's operation.
//

// Put this after your derived class' definition to implement the accessors for the
// activity table.
// UNDONE: Cascade these?
#define IMPLEMENT_ACTTABLE(className) \
	acttable_t *className::ActivityList( int &iActivityCount ) { iActivityCount = ARRAYSIZE(m_acttable); return m_acttable; }

typedef struct
{
	int			baseAct;
	int			weaponAct;
	bool		required;
} acttable_t;

class CHudTexture;
class Color;

namespace vgui2
{
	typedef unsigned long HFont;
}

// -----------------------------------------
//	Vector cones
// -----------------------------------------
// VECTOR_CONE_PRECALCULATED - this resolves to vec3_origin, but adds some
// context indicating that the person writing the code is not allowing
// FireBullets() to modify the direction of the shot because the shot direction
// being passed into the function has already been modified by another piece of
// code and should be fired as specified. See GetActualShotTrajectory(). 

// NOTE: The way these are calculated is that each component == sin (degrees/2)
#define VECTOR_CONE_PRECALCULATED	vec3_origin
#define VECTOR_CONE_1DEGREES		Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES		Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES		Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES		Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES		Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES		Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES		Vector( 0.06105, 0.06105, 0.06105 )
#define VECTOR_CONE_8DEGREES		Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES		Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES		Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES		Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES		Vector( 0.17365, 0.17365, 0.17365 )

//-----------------------------------------------------------------------------
// Purpose: Base weapon class, shared on client and server
//-----------------------------------------------------------------------------

#if defined USES_ECON_ITEMS
#define BASECOMBATWEAPON_DERIVED_FROM		CEconEntity
#else 
#define BASECOMBATWEAPON_DERIVED_FROM		CBaseAnimating
#endif 

//-----------------------------------------------------------------------------
// Collect trace attacks for weapons that fire multiple projectiles per attack that also penetrate
//-----------------------------------------------------------------------------
class CDmgAccumulator
{
public:
	CDmgAccumulator( void );
	~CDmgAccumulator();

#ifdef GAME_DLL
	virtual void Start( void ) { m_bActive = true; }
	virtual void AccumulateMultiDamage( const CTakeDamageInfo &info, CBaseEntity *pEntity );
	virtual void Process( void );

private:
	CTakeDamageInfo					m_updatedInfo;
	CUtlMap< int, CTakeDamageInfo >	m_TargetsDmgInfo;
#endif	// GAME_DLL

private:
	bool							m_bActive;

};

//-----------------------------------------------------------------------------
// Purpose: Client side rep of CBaseTFCombatWeapon 
//-----------------------------------------------------------------------------
// Hacky
class CBaseCombatWeapon : public BASECOMBATWEAPON_DERIVED_FROM
{
public:
	DECLARE_CLASS( CBaseCombatWeapon, BASECOMBATWEAPON_DERIVED_FROM );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
#ifdef GAME_DLL
	DECLARE_ENT_SCRIPTDESC();
#endif

							CBaseCombatWeapon();
	virtual 				~CBaseCombatWeapon();

	virtual bool			IsBaseCombatWeapon( void ) const { return true; }
	virtual CBaseCombatWeapon *MyCombatWeaponPointer( void ) { return this; }

	// A derived weapon class should return true here so that weapon sounds, etc, can
	//  apply the proper filter
	virtual bool			IsPredicted( void ) const { return false; }

	virtual void			Spawn( void );
	virtual void			Precache( void );

	void					MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	// Subtypes are used to manage multiple weapons of the same type on the player.
	virtual int				GetSubType( void ) { return m_iSubType; }
	virtual void			SetSubType( int iType ) { m_iSubType = iType; }

	virtual void			Equip( CBaseCombatCharacter *pOwner );
	virtual void			Drop( const Vector &vecVelocity );

	virtual	int				UpdateClientData( CBasePlayer *pPlayer );

	virtual bool			IsAllowedToSwitch( void );
	virtual bool			CanBeSelected( void );
	virtual bool			VisibleInWeaponSelection( void );
	virtual bool			HasAmmo( void );

	// Weapon Pickup For Player
	virtual void			SetPickupTouch( void );
	virtual void 			DefaultTouch( CBaseEntity *pOther );	// default weapon touch
	virtual void			GiveTo( CBaseEntity *pOther );

	// HUD Hints
	virtual bool			ShouldDisplayAltFireHUDHint();
	virtual void			DisplayAltFireHudHint();	
	virtual void			RescindAltFireHudHint(); ///< undisplay the hud hint and pretend it never showed.

	virtual bool			ShouldDisplayReloadHUDHint();
	virtual void			DisplayReloadHudHint();
	virtual void			RescindReloadHudHint();

	// Weapon client handling
	virtual void			SetViewModelIndex( int index = 0 );
	virtual bool			SendWeaponAnim( int iActivity );
	virtual void			SendViewModelAnim( int nSequence );
	float					GetViewModelSequenceDuration();	// Return how long the current view model sequence is.
	bool					IsViewModelSequenceFinished( void ) const; // Returns if the viewmodel's current animation is finished

	virtual void			SetViewModel();

	virtual bool			HasWeaponIdleTimeElapsed( void );
	virtual void			SetWeaponIdleTime( float time );
	virtual float			GetWeaponIdleTime( void );

	// Weapon selection
	virtual bool			HasAnyAmmo( void );							// Returns true is weapon has ammo
	virtual bool			HasPrimaryAmmo( void );						// Returns true is weapon has ammo
	virtual bool			HasSecondaryAmmo( void );					// Returns true is weapon has ammo
	bool					UsesPrimaryAmmo( void );					// returns true if the weapon actually uses primary ammo
	bool					UsesSecondaryAmmo( void );					// returns true if the weapon actually uses secondary ammo
	void					GiveDefaultAmmo( void );
	
	virtual bool			CanHolster( void ) const { return TRUE; };		// returns true if the weapon can be holstered
	virtual bool			DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt );
	virtual bool			CanDeploy( void ) { return true; }			// return true if the weapon's allowed to deploy
	virtual bool			Deploy( void );								// returns true is deploy was successful
	virtual bool			Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual CBaseCombatWeapon *GetLastWeapon( void ) { return this; }
	virtual void			SetWeaponVisible( bool visible );
	virtual bool			IsWeaponVisible( void );
	virtual bool			ReloadOrSwitchWeapons( void );
	virtual void			OnActiveStateChanged( int iOldState ) { return; }
	virtual bool			HolsterOnDetach() { return false; }
	virtual bool			IsHolstered(){ return false; }
	virtual void			Detach() {}

	// Weapon behaviour
	virtual void			ItemPreFrame( void );					// called each frame by the player PreThink
	virtual void			ItemPostFrame( void );					// called each frame by the player PostThink
	virtual void			ItemBusyFrame( void );					// called each frame by the player PostThink, if the player's not ready to attack yet
	virtual void			ItemHolsterFrame( void ) {};			// called each frame by the player PreThink, if the weapon is holstered
	virtual void			WeaponIdle( void );						// called when no buttons pressed
	virtual void			HandleFireOnEmpty();					// Called when they have the attack button down
																	// but they are out of ammo. The default implementation
																	// either reloads, switches weapons, or plays an empty sound.
	virtual bool			CanPerformSecondaryAttack() const;

	virtual bool			ShouldBlockPrimaryFire() { return false; }

#ifdef CLIENT_DLL
	virtual void			CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles ) {}
	virtual int				CalcOverrideModelIndex() OVERRIDE;

	// misyl: If weapon mispred's don't reset all the player's variables.
	virtual bool PredictionErrorShouldResetLatchedForAllPredictables( void ) OVERRIDE;
#endif

	virtual bool			IsWeaponZoomed() { return false; }		// Is this weapon in its 'zoomed in' mode?

	// Reloading
	virtual	void			CheckReload( void );
	virtual void			FinishReload( void );
	virtual void			AbortReload( void );
	virtual bool			Reload( void );
	bool					DefaultReload( int iClipSize1, int iClipSize2, int iActivity );
	bool					ReloadsSingly( void ) const;

	virtual bool			AutoFiresFullClip( void ) const { return false; }
	virtual void			UpdateAutoFire( void );

	// Weapon firing
	virtual void			PrimaryAttack( void );						// do "+ATTACK"
	virtual void			SecondaryAttack( void ) { return; }			// do "+ATTACK2"

	// Firing animations
	virtual Activity		GetPrimaryAttackActivity( void );
	virtual Activity		GetSecondaryAttackActivity( void );
	virtual Activity		GetDrawActivity( void );
	virtual float			GetDefaultAnimSpeed( void ) { return 1.0; }

	// Bullet launch information
	virtual int				GetBulletType( void );
	virtual const Vector&	GetBulletSpread( void );
	virtual Vector			GetBulletSpread( WeaponProficiency_t proficiency )		{ return GetBulletSpread(); }
	virtual float			GetSpreadBias( WeaponProficiency_t proficiency )			{ return 1.0; }
	virtual float			GetFireRate( void );
	virtual int				GetMinBurst() { return 1; }
	virtual int				GetMaxBurst() { return 1; }
	virtual float			GetMinRestTime() { return 0.3; }
	virtual float			GetMaxRestTime() { return 0.6; }
	virtual int				GetRandomBurst() { return random->RandomInt( GetMinBurst(), GetMaxBurst() ); }
	virtual void			WeaponSound( WeaponSound_t sound_type, float soundtime = 0.0f );
	virtual void			StopWeaponSound( WeaponSound_t sound_type );
	virtual const WeaponProficiencyInfo_t *GetProficiencyValues();

	// Autoaim
	virtual float			GetMaxAutoAimDeflection() { return 0.99f; }
	virtual float			WeaponAutoAimScale() { return 1.0f; } // allows a weapon to influence the perceived size of the target's autoaim radius.

	// TF Sprinting functions
	virtual bool			StartSprinting( void ) { return false; };
	virtual bool			StopSprinting( void ) { return false; };

	// TF Injury functions
	virtual float			GetDamage( float flDistance, int iLocation ) { return 0.0; };

	virtual void			SetActivity( Activity act, float duration );
	inline void				SetActivity( Activity eActivity ) { m_Activity = eActivity; }
	inline Activity			GetActivity( void ) const { return m_Activity; }

	virtual void			AddViewKick( void );	// Add in the view kick for the weapon

	virtual char			*GetDeathNoticeName( void );	// Get the string to print death notices with

	CBaseCombatCharacter	*GetOwner() const;
	void					SetOwner( CBaseCombatCharacter *owner );
	virtual void			OnPickedUp( CBaseCombatCharacter *pNewOwner );

	virtual void			AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles ) {};
	virtual float			CalcViewmodelBob( void ) { return 0.0f; };

	// Returns information about the various control panels
	virtual void 			GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
	virtual void			GetControlPanelClassName( int nPanelIndex, const char *&pPanelName );

	virtual bool			ShouldShowControlPanels( void ) { return true; }

	void					Lock( float lockTime, CBaseEntity *pLocker );
	bool					IsLocked( CBaseEntity *pAsker );

	//All weapons can be picked up by NPCs by default
	virtual bool			CanBePickedUpByNPCs( void ) { return true;	}

	virtual int				GetSkinOverride() const { return -1; }

public:

	// Weapon info accessors for data in the weapon's data file
	const FileWeaponInfo_t	&GetWpnData( void ) const;
	virtual const char		*GetViewModel( int viewmodelindex = 0 ) const;
	virtual const char		*GetWorldModel( void ) const;
	virtual const char		*GetAnimPrefix( void ) const;
	virtual int				GetMaxClip1( void ) const;
	virtual int				GetMaxClip2( void ) const;
	virtual int				GetDefaultClip1( void ) const;
	virtual int				GetDefaultClip2( void ) const;
	virtual int				GetWeight( void ) const;
	virtual bool			AllowsAutoSwitchTo( void ) const;
	virtual bool			AllowsAutoSwitchFrom( void ) const;
	virtual bool			ForceWeaponSwitch( void ) const { return false; }
	virtual int				GetWeaponFlags( void ) const;
	virtual int				GetSlot( void ) const;
	virtual int				GetPosition( void ) const;
	virtual char const		*GetName( void ) const;
	virtual char const		*GetPrintName( void ) const;
	virtual char const		*GetShootSound( int iIndex ) const;
	virtual int				GetRumbleEffect() const;
	virtual bool			UsesClipsForAmmo1( void ) const;
	virtual bool			UsesClipsForAmmo2( void ) const;
	bool					IsMeleeWeapon() const;

	// derive this function if you mod uses encrypted weapon info files
	virtual const unsigned char *GetEncryptionKey( void );

	virtual int				GetPrimaryAmmoType( void )  const { return m_iPrimaryAmmoType; }
	virtual int				GetSecondaryAmmoType( void )  const { return m_iSecondaryAmmoType; }
	virtual int				Clip1() { return m_iClip1; }
	virtual int				Clip2() { return m_iClip2; }

	// Ammo quantity queries for weapons that do not use clips. These are only
	// used to determine how much ammo is in a weapon that does not have an owner.
	// That is, a weapon that's on the ground for the player to get ammo out of.
	int GetPrimaryAmmoCount() { return m_iPrimaryAmmoCount; }
	void SetPrimaryAmmoCount( int count ) { m_iPrimaryAmmoCount = count; }

	int GetSecondaryAmmoCount() { return m_iSecondaryAmmoCount; }
	void SetSecondaryAmmoCount( int count ) { m_iSecondaryAmmoCount = count; }

	virtual CHudTexture const	*GetSpriteActive( void ) const;
	virtual CHudTexture const	*GetSpriteInactive( void ) const;
	virtual CHudTexture const	*GetSpriteAmmo( void ) const;
	virtual CHudTexture const	*GetSpriteAmmo2( void ) const;
	virtual CHudTexture const	*GetSpriteCrosshair( void ) const;
	virtual CHudTexture const	*GetSpriteAutoaim( void ) const;
	virtual CHudTexture const	*GetSpriteZoomedCrosshair( void ) const;
	virtual CHudTexture const	*GetSpriteZoomedAutoaim( void ) const;

	virtual Activity		ActivityOverride( Activity baseAct, bool *pRequired );
	virtual	acttable_t*		ActivityList( int &iActivityCount ) { return NULL; }

	virtual void			Activate( void );

	virtual bool ShouldUseLargeViewModelVROverride() { return false; }
public:
// Server Only Methods
#if !defined( CLIENT_DLL )

	DECLARE_DATADESC();
	virtual void			FallInit( void );						// prepare to fall to the ground
	virtual void			FallThink( void );						// make the weapon fall to the ground after spawning

	// Weapon spawning
	bool					IsConstrained() { return m_pConstraint != NULL; }
	bool					IsInBadPosition ( void );				// Is weapon in bad position to pickup?
	bool					RepositionWeapon ( void );				// Attempts to reposition the weapon in a location where it can be
	virtual void			Materialize( void );					// make a weapon visible and tangible
	void					AttemptToMaterialize( void );			// see if the game rules will let the weapon become visible and tangible
	virtual void			CheckRespawn( void );					// see if this weapon should respawn after being picked up
	CBaseEntity				*Respawn ( void );						// copy a weapon

	static int				GetAvailableWeaponsInBox( CBaseCombatWeapon **pList, int listMax, const Vector &mins, const Vector &maxs );

	// Weapon dropping / destruction
	virtual void			Delete( void );
	void					DestroyItem( void );
	virtual void			Kill( void );

	virtual int				CapabilitiesGet( void ) { return 0; }
	virtual	int				ObjectCaps( void );

	bool					IsRemoveable() { return m_bRemoveable; }
	void					SetRemoveable( bool bRemoveable ) { m_bRemoveable = bRemoveable; }
	
	// Returns bits for	weapon conditions
	virtual bool			WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );	
	virtual	int				WeaponRangeAttack1Condition( float flDot, float flDist );
	virtual	int				WeaponRangeAttack2Condition( float flDot, float flDist );
	virtual	int				WeaponMeleeAttack1Condition( float flDot, float flDist );
	virtual	int				WeaponMeleeAttack2Condition( float flDot, float flDist );

	virtual void			Operator_FrameUpdate( CBaseCombatCharacter  *pOperator );
	virtual void			Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	virtual void			Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary ) { return; }
	// NOTE: This should never be called when a character is operating the weapon.  Animation events should be
	// routed through the character, and then back into CharacterAnimEvent() 
	void					HandleAnimEvent( animevent_t *pEvent );

	virtual int				UpdateTransmitState( void );

	void					InputHideWeapon( inputdata_t &inputdata );
	void					Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual CDmgAccumulator	*GetDmgAccumulator( void ) { return NULL; }

	void					SetSoundsEnabled( bool bSoundsEnabled ) { m_bSoundsEnabled = bSoundsEnabled; }

	void					SetCustomViewModel( const char *pszCustomViewModel );
	void					SetCustomViewModelModelIndex( int nCustomViewModelModelIndex );

// Client only methods
#else

	virtual void			BoneMergeFastCullBloat( Vector &localMins, Vector &localMaxs, const Vector &thisEntityMins, const Vector &thisEntityMaxs  ) const;

	virtual bool			OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options ) 
	{ 
#if defined USES_ECON_ITEMS
		return BaseClass::OnFireEvent( pViewModel, origin, angles, event, options );
#else
		return false; 
#endif
	}

	// Should this object cast shadows?
	virtual ShadowType_t	ShadowCastType();
	virtual void			SetDormant( bool bDormant );
	virtual void			OnDataChanged( DataUpdateType_t updateType );
	virtual void			OnRestore();

	virtual void			RestartParticleEffect( void ) {}

	virtual void			Redraw(void);
	virtual void			ViewModelDrawn( CBaseViewModel *pViewModel );
	// Get the position that bullets are seen coming out. Note: the returned values are different
	// for first person and third person.
	bool					GetShootPosition( Vector &vOrigin, QAngle &vAngles );
	virtual void			DrawCrosshair( void );
	virtual bool			ShouldDrawCrosshair( void ) { return true; }
	
	// Weapon state checking
	virtual bool			IsCarriedByLocalPlayer( void );
	virtual bool			ShouldDrawUsingViewModel( void );
	virtual bool			IsActiveByLocalPlayer( void );

	bool					IsBeingCarried() const;

	// Is the carrier alive?
	bool					IsCarrierAlive() const;

	// Returns the aiment render origin + angles
	virtual int				DrawModel( int flags );
	virtual bool			ShouldDraw( void );
	virtual bool			ShouldDrawPickup( void );
	virtual void			HandleInput( void ) { return; };
	virtual void			OverrideMouseInput( float *x, float *y ) { return; };
	virtual int				KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding ) { return 1; }
	virtual bool			AddLookShift( void ) { return true; };

	virtual void			GetViewmodelBoneControllers(C_BaseViewModel *pViewModel, float controllers[MAXSTUDIOBONECTRLS]) { return; }

	virtual void			NotifyShouldTransmit( ShouldTransmitState_t state );
	WEAPON_FILE_INFO_HANDLE	GetWeaponFileInfoHandle() { return m_hWeaponFileInfo; }

	virtual int				GetWorldModelIndex( void );

	virtual void			GetToolRecordingState( KeyValues *msg );

	virtual void			GetWeaponCrosshairScale( float &flScale ) { flScale = 1.f; }

#if !defined USES_ECON_ITEMS
	// Viewmodel overriding
	virtual bool			ViewModel_IsTransparent( void ) { return IsTransparent(); }
	virtual bool			ViewModel_IsUsingFBTexture( void ) { return UsesPowerOfTwoFrameBufferTexture(); }
	virtual bool			IsOverridingViewmodel( void ) { return false; };
	virtual int				DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags ) { return 0; };
	bool					WantsToOverrideViewmodelAttachments( void ) { return false; }
#endif

#endif // End client-only methods

	virtual bool			CanLower( void ) { return false; }
	virtual bool			Ready( void ) { return false; }
	virtual bool			Lower( void ) { return false; }

	virtual void			HideThink( void );
	virtual bool			CanReload( void );

	virtual float			GetNextSecondaryAttackDelay( void ) { return 0.5f; } // This is for setting the next attack timer from inside SecondaryAttack()

	void SetClip1( int nClip )
	{
		if ( UsesClipsForAmmo1() )
		{
			m_iClip1 = nClip;
		}
		else
		{
			SetPrimaryAmmoCount( m_iClip1 );
			m_iClip1 = WEAPON_NOCLIP;
		}
	}
	void SetClip2( int nClip )
	{
		if ( UsesClipsForAmmo1() )
		{
			m_iClip2 = nClip;
		}
		else
		{
			SetPrimaryAmmoCount( m_iClip2 );
			m_iClip2 = WEAPON_NOCLIP;
		}
	}

private:
	typedef CHandle< CBaseCombatCharacter > CBaseCombatCharacterHandle;
	CNetworkVar( CBaseCombatCharacterHandle, m_hOwner );				// Player carrying this weapon

protected:
#if defined ( TF_CLIENT_DLL ) || defined ( TF_DLL )
	// Regulate crit frequency to reduce client-side seed hacking
	void					AddToCritBucket( float flAmount );
	void					RemoveFromCritBucket( float flAmount ) { m_flCritTokenBucket -= flAmount; }
	bool					IsAllowedToWithdrawFromCritBucket( float flDamage );

	float					m_flCritTokenBucket;
	int						m_nCritChecks;
	int						m_nCritSeedRequests;
#endif // TF

public:

	// Networked fields
	CNetworkVar( int, m_nViewModelIndex );

	// Weapon firing
	CNetworkVar( float, m_flNextPrimaryAttack );						// soonest time ItemPostFrame will call PrimaryAttack
	CNetworkVar( float, m_flNextSecondaryAttack );					// soonest time ItemPostFrame will call SecondaryAttack
	CNetworkVar( float, m_flTimeWeaponIdle );							// soonest time ItemPostFrame will call WeaponIdle
	// Weapon state
	bool					m_bInReload;			// Are we in the middle of a reload;
	bool					m_bFireOnEmpty;			// True when the gun is empty and the player is still holding down the attack key(s)
	bool					m_bFiringWholeClip;		// Are we in the middle of firing the whole clip;
	// Weapon art
	CNetworkVar( int, m_iViewModelIndex );
	CNetworkVar( int, m_iWorldModelIndex );
	// Sounds
	float					m_flNextEmptySoundTime;				// delay on empty sound playing

	Activity				GetIdealActivity( void ) { return m_IdealActivity; }
	int						GetIdealSequence( void ) { return m_nIdealSequence; }

	bool					SetIdealActivity( Activity ideal );
	void					MaintainIdealActivity( void );

#ifdef CLIENT_DLL
	virtual const Vector&	GetViewmodelOffset() { return vec3_origin; }
#endif // CLIENT_DLL

	virtual bool			UsesCenterFireProjectile( void ) const { return false; }

private:
	Activity				m_Activity;
	int						m_nIdealSequence;
	Activity				m_IdealActivity;

	bool					m_bRemoveable;

	int						m_iPrimaryAmmoCount;
	int						m_iSecondaryAmmoCount;

public:

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_nNextThinkTick );

#ifdef CLIENT_DLL
	static void				RecvProxy_WeaponState( const CRecvProxyData *pData, void *pStruct, void *pOut );
#endif
	int						WeaponState() const { return m_iState; }

	// Weapon data
	CNetworkVar( int, m_iState );				// See WEAPON_* definition
	string_t				m_iszName;				// Classname of this weapon.
	CNetworkVar( int, m_iPrimaryAmmoType );		// "primary" ammo index into the ammo info array 
	CNetworkVar( int, m_iSecondaryAmmoType );	// "secondary" ammo index into the ammo info array
	CNetworkVar( int, m_iClip1 );				// number of shots left in the primary weapon clip, -1 it not used
	CNetworkVar( int, m_iClip2 );				// number of shots left in the secondary weapon clip, -1 it not used
	bool					m_bFiresUnderwater;		// true if this weapon can fire underwater
	bool					m_bAltFiresUnderwater;		// true if this weapon can fire underwater
	float					m_fMinRange1;			// What's the closest this weapon can be used?
	float					m_fMinRange2;			// What's the closest this weapon can be used?
	float					m_fMaxRange1;			// What's the furthest this weapon can be used?
	float					m_fMaxRange2;			// What's the furthest this weapon can be used?
	bool					m_bReloadsSingly;		// True if this weapon reloads 1 round at a time
	float					m_fFireDuration;		// The amount of time that the weapon has sustained firing
	int						m_iSubType;

	float					m_flUnlockTime;
	EHANDLE					m_hLocker;				// Who locked this weapon.

	CNetworkVar( bool, m_bFlipViewModel );

	IPhysicsConstraint		*GetConstraint() { return m_pConstraint; }

private:
	WEAPON_FILE_INFO_HANDLE	m_hWeaponFileInfo;
	IPhysicsConstraint		*m_pConstraint;

	int						m_iAltFireHudHintCount;		// How many times has this weapon displayed its alt-fire HUD hint?
	int						m_iReloadHudHintCount;		// How many times has this weapon displayed its reload HUD hint?
	bool					m_bAltFireHudHintDisplayed;	// Have we displayed an alt-fire HUD hint since this weapon was deployed?
	bool					m_bReloadHudHintDisplayed;	// Have we displayed a reload HUD hint since this weapon was deployed?
	float					m_flHudHintPollTime;	// When to poll the weapon again for whether it should display a hud hint.
	float					m_flHudHintMinDisplayTime; // if the hint is squelched before this, reset my counter so we'll display it again.
	
	CNetworkVar( short, m_nCustomViewmodelModelIndex );

	// Server only
#if !defined( CLIENT_DLL )

	bool					m_bSoundsEnabled;

	// Outputs
protected:
	COutputEvent			m_OnPlayerUse;		// Fired when the player uses the weapon.
	COutputEvent			m_OnPlayerPickup;	// Fired when the player picks up the weapon.
	COutputEvent			m_OnNPCPickup;		// Fired when an NPC picks up the weapon.
	COutputEvent			m_OnCacheInteraction;	// For awarding lambda cache achievements in HL2 on 360. See .FGD file for details 

#else // Client .dll only
	bool					m_bJustRestored;

	// Allow weapons resource to access m_hWeaponFileInfo directly
	friend class			WeaponsResource;

protected:	
	int						m_iOldState;

#endif // End Client .dll only
};

#endif // COMBATWEAPON_SHARED_H
