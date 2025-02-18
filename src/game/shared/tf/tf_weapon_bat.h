//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_BAT_H
#define TF_WEAPON_BAT_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"
#include "tf_weaponbase_grenadeproj.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_shareddefs.h"
#include "tf_viewmodel.h"

#ifdef CLIENT_DLL
#define CTFBat C_TFBat
#define CTFBat_Wood C_TFBat_Wood
#define CTFBat_Fish C_TFBat_Fish
#define CTFStunBall C_TFStunBall
#define CTFBat_Giftwrap C_TFBat_Giftwrap
#define CTFBall_Ornament C_TFBall_Ornament
#endif


//=============================================================================
//
// Bat class.
//
class CTFBat : public CTFWeaponBaseMelee
{
public:
	DECLARE_CLASS( CTFBat, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFBat();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_BAT; }
	virtual bool		BatDeflects() { return false; }
	virtual void		Smack( void );
	virtual void		PlayDeflectionSound( bool bPlayer );

private:

	CTFBat( const CTFBat & ) {}
};


//=============================================================================
//
// CTFBat_Fish class.
//
class CTFBat_Fish : public CTFBat
{
public:
	DECLARE_CLASS( CTFBat_Fish, CTFBat );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_BAT_FISH; }
};

//=============================================================================
//
// CTFBat_Wood class.
//
class CTFBat_Wood : public CTFBat
{
public:
	DECLARE_CLASS( CTFBat_Wood, CTFBat );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFBat_Wood();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_BAT_WOOD; }
	virtual bool		BatDeflects() { return false; }

	virtual void		SecondaryAttack( void );
	void				SecondaryAttackAnim( CTFPlayer *pPlayer );
	virtual bool		SendWeaponAnim( int iActivity );

	virtual bool		CanCreateBall( CTFPlayer* pPlayer );
	virtual void		LaunchBall( void );
	void				LaunchBallThink( void );

	virtual float		InternalGetEffectBarRechargeTime( void ) { return 10.f; }
	virtual int			GetEffectBarAmmo( void ) { return TF_AMMO_GRENADES1; }

#ifdef GAME_DLL
	virtual void		GetBallDynamics( Vector& vecLoc, QAngle& vecAngles, Vector& vecVelocity, AngularImpulse& angImpulse, CTFPlayer* pPlayer );
#endif

#ifdef CLIENT_DLL
	virtual void		SetWeaponVisible( bool visible );

	// Child removal:
	virtual	void		Drop( const Vector &vecVelocity );
	virtual void		WeaponReset( void );
	virtual void		UpdateOnRemove( void );
	virtual void		OnDataChanged( DataUpdateType_t updateType );
	void				AddBallChild( void );
	void				RemoveBallChild( void );
#endif

	virtual void		PickedUpBall( void );

	float				GetProgress( void ) { return GetEffectBarProgress(); }
	const char*			GetEffectLabelText( void ) { return "#TF_BALL"; }

	virtual const char *GetBallViewModelName( void ) const		{ return "models/weapons/v_models/v_baseball.mdl"; }

#ifdef GAME_DLL
	virtual CBaseEntity* CreateBall( void );
#endif

	int					m_iEnemyBallID;

#ifdef CLIENT_DLL
	EHANDLE				m_hStunBallVM; // View model ball.
#endif
};


//=============================================================================
//
// StunBall class.
//
class CTFStunBall : public CTFGrenadePipebombProjectile
{
public:
	DECLARE_CLASS( CTFStunBall, CTFGrenadePipebombProjectile );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	CTFStunBall();

	static CTFStunBall* Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner );
	virtual void		Precache( void );
	virtual void		Spawn( void );
	void				SetInitialSpeed( float flSpd )		{ m_flInitialSpeed = flSpd; }

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_STUNBALL; }
	virtual const char *GetBallModelName( void ) const;
	virtual const char *GetBallViewModelName( void ) const;

	virtual bool		IsAllowedToExplode( void ) OVERRIDE { return false; }
	virtual void		Explode( trace_t *pTrace, int bitsDamageType );
	virtual void		ApplyBallImpactEffectOnVictim( CBaseEntity *pOther );
	virtual void		PipebombTouch( CBaseEntity *pOther );
	virtual void		VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	
	virtual float		GetDamage( void );
	virtual int			GetDamageType( void )				{ return DMG_CLUB; }
	virtual Vector		GetDamageForce( void );

	virtual float		GetShakeAmplitude( void )			{ return 0.0; }
	virtual float		GetShakeRadius( void )				{ return 0.0; }
	void				RemoveBallTrail( void );
	
	virtual bool		IsDestroyable( bool bOrbAttack = false ) OVERRIDE { return ( !bOrbAttack ? false : true ); }
	virtual bool		ShouldBallTouch( CBaseEntity *pOther );

	int					m_iOriginalOwnerID;

private:
	float				m_flInitialSpeed;

	float				m_flBallTrailLife;
	EHANDLE				m_pBallTrail;
#else
	virtual const char *GetTrailParticleName( void );
	virtual void	CreateTrailParticles( void );

#endif

};


//=============================================================================
//
// Winter Holiday 2011 wrapping paper tube and glass ornament ball
//
class CTFBat_Giftwrap : public CTFBat_Wood
{
public:
	DECLARE_CLASS( CTFBat_Giftwrap, CTFBat_Wood );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void		Spawn( void );

	virtual int			GetWeaponID( void ) const				{ return TF_WEAPON_BAT_GIFTWRAP; }

	virtual const char *GetEffectLabelText( void )				{ return "#TF_BALL"; }
	virtual const char *GetBallViewModelName( void ) const		{ return "models/weapons/c_models/c_xms_festive_ornament.mdl"; }

#ifdef GAME_DLL
	virtual CBaseEntity* CreateBall( void );
#endif
};


//=============================================================================
//
// Winter Holiday 2011 wrapping paper tube and glass ornament ball
//
class CTFBall_Ornament : public CTFStunBall
{
public:
	DECLARE_CLASS( CTFBall_Ornament, CTFStunBall );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	static CTFBall_Ornament* Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner );

	virtual void		Precache( void );

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_ORNAMENT_BALL; }
	virtual const char *GetBallModelName( void ) const;
	virtual const char *GetBallViewModelName( void ) const;

	virtual bool		IsAllowedToExplode( void ) OVERRIDE { return true; }
	virtual void		Explode( trace_t *pTrace, int bitsDamageType ) OVERRIDE;

	virtual void		PipebombTouch( CBaseEntity *pOther ) OVERRIDE;
	virtual void		VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ) OVERRIDE;
	void				VPhysicsCollisionThink( void );

	virtual void		ApplyBallImpactEffectOnVictim( CBaseEntity *pOther );

protected:
		Vector		m_vCollisionVelocity;
#endif
};



#endif // TF_WEAPON_BAT_H
