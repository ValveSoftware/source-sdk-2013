//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_WEAPON_PASSTIME_GUN_H
#define TF_WEAPON_PASSTIME_GUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"

class CPasstimeBall;

#ifdef CLIENT_DLL
#define CPasstimeBall C_PasstimeBall
#define CPasstimeGun C_PasstimeGun
class C_PasstimeBounceReticle;
#else
#include "passtime_ballcontroller_homing.h"
#endif


//-----------------------------------------------------------------------------
class CPasstimeGun : public CTFWeaponBase, public ITFChargeUpWeapon
{
	DECLARE_CLASS( CPasstimeGun, CTFWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE(); // this has to be here because the client's precache code uses it to get the classname of this entity...

public:
	CPasstimeGun();
	~CPasstimeGun();
	virtual float GetChargeBeginTime() OVERRIDE;
	virtual float GetCurrentCharge() OVERRIDE;
	static bool BValidPassTarget( CTFPlayer *pSource, CTFPlayer *pTarget, HudNotification_t *pReason = 0 );

	struct LaunchParams
	{
		Vector eyePos;
		Vector viewFwd;
		Vector viewRight;
		Vector viewUp;
		Vector traceHullSize;
		float traceHullDistance;
		Vector startPos;
		Vector startDir;
		Vector startVel;
		static LaunchParams Default( CTFPlayer *pPlayer );
	};
	
	static LaunchParams CalcLaunch( CTFPlayer *pPlayer, bool bHoming );

protected:
	virtual int	GetWeaponID() const OVERRIDE { return TF_WEAPON_PASSTIME_GUN; }
	virtual void Spawn() OVERRIDE;
	virtual void Equip( CBaseCombatCharacter *pOwner ) OVERRIDE;
	virtual void Precache() OVERRIDE;
	virtual bool CanHolster() const OVERRIDE;
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo ) OVERRIDE;
	virtual void WeaponReset() OVERRIDE;
	virtual bool CanCharge() OVERRIDE;
	virtual float GetChargeMaxTime() OVERRIDE;
	virtual void UpdateOnRemove() OVERRIDE;
	virtual bool VisibleInWeaponSelection() OVERRIDE;
	virtual acttable_t* ActivityList(int &iActivityCount) OVERRIDE;
	virtual void ItemPostFrame() OVERRIDE;
	virtual void ItemHolsterFrame() OVERRIDE;
	virtual bool Deploy() OVERRIDE;
	virtual bool CanDeploy() OVERRIDE;
	virtual const char *GetWorldModel() const OVERRIDE;
	virtual bool SendWeaponAnim( int actBase ) OVERRIDE;
	virtual Activity GetDrawActivity() OVERRIDE { return ACT_BALL_VM_CATCH; }

	// HasPrimaryAmmo, CanBeSelected, IsEnergyWeapon:
	// these exist to make other code have correct side-effects
	// search for where these are called to see the specifics.
	virtual bool HasPrimaryAmmo() OVERRIDE { return true; }
	virtual bool CanBeSelected() OVERRIDE { return true; }
	virtual bool IsEnergyWeapon() const OVERRIDE { return true; }

#ifdef CLIENT_DLL
	virtual void UpdateAttachmentModels() OVERRIDE;
	virtual void ClientThink() OVERRIDE;
	void UpdateThrowArch();
	void DestroyThrowArch();
	C_PasstimeBounceReticle *m_pBounceReticle;
#endif
	void Throw( CTFPlayer *pOwner );

	enum EThrowState
	{
		THROWSTATE_IDLE,
		THROWSTATE_CHARGING,
		THROWSTATE_CHARGED,
		THROWSTATE_THROWN,
		THROWSTATE_CANCELLED,
		THROWSTATE_DISABLED,
	};

	enum EButtonState
	{
		BUTTONSTATE_UP,					// not pressed
		BUTTONSTATE_PRESSED,			// was just pressed and is down
		BUTTONSTATE_DOWN,				// continues to be down
		BUTTONSTATE_RELEASED,			// was just released and is not down
		BUTTONSTATE_DISABLED,			// ignore input
	};

	struct AttackInputState 
	{
		AttackInputState( int button ) 
			: iButton( button ), eButtonState( BUTTONSTATE_UP )
			, bLatchedUp( false )
		{}
		const int iButton;
		EButtonState eButtonState;
		bool bLatchedUp;

		bool Is( EButtonState state ) const { return eButtonState == state; }
		void Disable() { eButtonState = BUTTONSTATE_DISABLED; }
		void Enable() 
		{ 
			if ( eButtonState == BUTTONSTATE_DISABLED )
				eButtonState = BUTTONSTATE_UP; 
		}
		void Update( int held, int pressed, int released );
		void LatchUp();
		void UnlatchUp();
	};

	int m_iHalloweenAttachmentIndex;
	int m_iAttachmentIndex;
	
	float m_flTargetResetTime;
	float m_flThrowLoopStartTime;
	AttackInputState m_attack, m_attack2;
	CNetworkVar( EThrowState, m_eThrowState );
	CNetworkVar( float, m_fChargeBeginTime );
	CHandle<CBaseCombatWeapon> m_hStoredLastWpn;
#ifdef GAME_DLL
	CPasstimeBallControllerHoming m_ballController;
#endif

};

#endif // TF_WEAPON_PASSTIME_GUN_H  
