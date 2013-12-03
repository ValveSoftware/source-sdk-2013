//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
#else
	#include "hl2mp_player.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponShotgun C_WeaponShotgun
#endif

extern ConVar sk_auto_reload_time;
extern ConVar sk_plr_num_shotgun_pellets;

class CWeaponShotgun : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponShotgun, CBaseHL2MPCombatWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

private:
	CNetworkVar( bool,	m_bNeedPump );		// When emptied completely
	CNetworkVar( bool,	m_bDelayedFire1 );	// Fire primary when finished reloading
	CNetworkVar( bool,	m_bDelayedFire2 );	// Fire secondary when finished reloading
	CNetworkVar( bool,	m_bDelayedReload );	// Reload when finished pump

public:
	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_10DEGREES;
		return cone;
	}

	virtual int				GetMinBurst() { return 1; }
	virtual int				GetMaxBurst() { return 3; }

	bool StartReload( void );
	bool Reload( void );
	void FillClip( void );
	void FinishReload( void );
	void CheckHolsterReload( void );
	void Pump( void );
//	void WeaponIdle( void );
	void ItemHolsterFrame( void );
	void ItemPostFrame( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void DryFire( void );
	virtual float GetFireRate( void ) { return 0.7; };

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponShotgun(void);

private:
	CWeaponShotgun( const CWeaponShotgun & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponShotgun, DT_WeaponShotgun )

BEGIN_NETWORK_TABLE( CWeaponShotgun, DT_WeaponShotgun )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bNeedPump ) ),
	RecvPropBool( RECVINFO( m_bDelayedFire1 ) ),
	RecvPropBool( RECVINFO( m_bDelayedFire2 ) ),
	RecvPropBool( RECVINFO( m_bDelayedReload ) ),
#else
	SendPropBool( SENDINFO( m_bNeedPump ) ),
	SendPropBool( SENDINFO( m_bDelayedFire1 ) ),
	SendPropBool( SENDINFO( m_bDelayedFire2 ) ),
	SendPropBool( SENDINFO( m_bDelayedReload ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponShotgun )
	DEFINE_PRED_FIELD( m_bNeedPump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDelayedFire1, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDelayedFire2, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDelayedReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_shotgun, CWeaponShotgun );
PRECACHE_WEAPON_REGISTER(weapon_shotgun);

#ifndef CLIENT_DLL
acttable_t	CWeaponShotgun::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_SHOTGUN,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_SHOTGUN,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SHOTGUN,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SHOTGUN,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SHOTGUN,					false },
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SHOTGUN,				false },
};

IMPLEMENT_ACTTABLE(CWeaponShotgun);

#endif


//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponShotgun::StartReload( void )
{
	if ( m_bNeedPump )
		return false;

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;


	int j = MIN(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	SendWeaponAnim( ACT_SHOTGUN_RELOAD_START );

	// Make shotgun shell visible
	SetBodygroup(1,0);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();

	m_bInReload = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponShotgun::Reload( void )
{
	// Check that StartReload was called first
	if (!m_bInReload)
	{
		Warning("ERROR: Shotgun Reload called incorrectly!\n");
	}

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	int j = MIN(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	FillClip();
	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(RELOAD);
	SendWeaponAnim( ACT_VM_RELOAD );

	pOwner->m_flNextAttack = gpGlobals->curtime;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponShotgun::FinishReload( void )
{
	// Make shotgun shell invisible
	SetBodygroup(1,1);

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return;

	m_bInReload = false;

	// Finish reload animation
	SendWeaponAnim( ACT_SHOTGUN_RELOAD_FINISH );

	pOwner->m_flNextAttack = gpGlobals->curtime;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponShotgun::FillClip( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return;

	// Add them to the clip
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		if ( Clip1() < GetMaxClip1() )
		{
			m_iClip1++;
			pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play weapon pump anim
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponShotgun::Pump( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();

	if ( pOwner == NULL )
		return;
	
	m_bNeedPump = false;

	if ( m_bDelayedReload )
	{
		m_bDelayedReload = false;
		StartReload();
	}
	
	WeaponSound( SPECIAL1 );

	// Finish reload animation
	SendWeaponAnim( ACT_SHOTGUN_PUMP );

	pOwner->m_flNextAttack	= gpGlobals->curtime + SequenceDuration();
	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponShotgun::DryFire( void )
{
	WeaponSound(EMPTY);
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponShotgun::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		return;
	}

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_iClip1 -= 1;

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector	vecSrc		= pPlayer->Weapon_ShootPosition( );
	Vector	vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );	

	FireBulletsInfo_t info( 7, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType );
	info.m_pAttacker = pPlayer;

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets( info );
	
	QAngle punch;
	punch.Init( SharedRandomFloat( "shotgunpax", -2, -1 ), SharedRandomFloat( "shotgunpay", -2, 2 ), 0 );
	pPlayer->ViewPunch( punch );

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	m_bNeedPump = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponShotgun::SecondaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		return;
	}

	pPlayer->m_nButtons &= ~IN_ATTACK2;
	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(WPN_DOUBLE);

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_iClip1 -= 2;	// Shotgun uses same clip for primary and secondary attacks

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );	

	FireBulletsInfo_t info( 12, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType );
	info.m_pAttacker = pPlayer;

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets( info );
	pPlayer->ViewPunch( QAngle(SharedRandomFloat( "shotgunsax", -5, 5 ),0,0) );

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	m_bNeedPump = true;
}

//-----------------------------------------------------------------------------
// Purpose: Override so shotgun can do mulitple reloads in a row
//-----------------------------------------------------------------------------
void CWeaponShotgun::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
	{
		return;
	}

	if ( m_bNeedPump && ( pOwner->m_nButtons & IN_RELOAD ) )
	{
		m_bDelayedReload = true;
	}

	if (m_bInReload)
	{
		// If I'm primary firing and have one round stop reloading and fire
		if ((pOwner->m_nButtons & IN_ATTACK ) && (m_iClip1 >=1) && !m_bNeedPump )
		{
			m_bInReload		= false;
			m_bNeedPump		= false;
			m_bDelayedFire1 = true;
		}
		// If I'm secondary firing and have two rounds stop reloading and fire
		else if ((pOwner->m_nButtons & IN_ATTACK2 ) && (m_iClip1 >=2) && !m_bNeedPump )
		{
			m_bInReload		= false;
			m_bNeedPump		= false;
			m_bDelayedFire2 = true;
		}
		else if (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			// If out of ammo end reload
			if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <=0)
			{
				FinishReload();
				return;
			}
			// If clip not full reload again
			if (m_iClip1 < GetMaxClip1())
			{
				Reload();
				return;
			}
			// Clip full, stop reloading
			else
			{
				FinishReload();
				return;
			}
		}
	}
	else
	{			
		// Make shotgun shell invisible
		SetBodygroup(1,1);
	}

	if ((m_bNeedPump) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		Pump();
		return;
	}
	
	// Shotgun uses same timing and ammo for secondary attack
	if ((m_bDelayedFire2 || pOwner->m_nButtons & IN_ATTACK2)&&(m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		m_bDelayedFire2 = false;
		
		if ( (m_iClip1 <= 1 && UsesClipsForAmmo1()))
		{
			// If only one shell is left, do a single shot instead	
			if ( m_iClip1 == 1 )
			{
				PrimaryAttack();
			}
			else if (!pOwner->GetAmmoCount(m_iPrimaryAmmoType))
			{
				DryFire();
			}
			else
			{
				StartReload();
			}
		}

		// Fire underwater?
		else if (GetOwner()->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			if ( pOwner->m_afButtonPressed & IN_ATTACK )
			{
				 m_flNextPrimaryAttack = gpGlobals->curtime;
			}
			SecondaryAttack();
		}
	}
	else if ( (m_bDelayedFire1 || pOwner->m_nButtons & IN_ATTACK) && m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		m_bDelayedFire1 = false;
		if ( (m_iClip1 <= 0 && UsesClipsForAmmo1()) || ( !UsesClipsForAmmo1() && !pOwner->GetAmmoCount(m_iPrimaryAmmoType) ) )
		{
			if (!pOwner->GetAmmoCount(m_iPrimaryAmmoType))
			{
				DryFire();
			}
			else
			{
				StartReload();
			}
		}
		// Fire underwater?
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
			if ( pPlayer && pPlayer->m_afButtonPressed & IN_ATTACK )
			{
				 m_flNextPrimaryAttack = gpGlobals->curtime;
			}
			PrimaryAttack();
		}
	}

	if ( pOwner->m_nButtons & IN_RELOAD && UsesClipsForAmmo1() && !m_bInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		StartReload();
	}
	else 
	{
		// no fire buttons down
		m_bFireOnEmpty = false;

		if ( !HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime ) 
		{
			// weapon isn't useable, switch.
			if ( !(GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && pOwner->SwitchToNextBestWeapon( this ) )
			{
				m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
				return;
			}
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if ( m_iClip1 <= 0 && !(GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < gpGlobals->curtime )
			{
				if (StartReload())
				{
					// if we've successfully started to reload, we're done
					return;
				}
			}
		}

		WeaponIdle( );
		return;
	}

}



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponShotgun::CWeaponShotgun( void )
{
	m_bReloadsSingly = true;

	m_bNeedPump		= false;
	m_bDelayedFire1 = false;
	m_bDelayedFire2 = false;

	m_fMinRange1		= 0.0;
	m_fMaxRange1		= 500;
	m_fMinRange2		= 0.0;
	m_fMaxRange2		= 200;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponShotgun::ItemHolsterFrame( void )
{
	// Must be player held
	if ( GetOwner() && GetOwner()->IsPlayer() == false )
		return;

	// We can't be active
	if ( GetOwner()->GetActiveWeapon() == this )
		return;

	// If it's been longer than three seconds, reload
	if ( ( gpGlobals->curtime - m_flHolsterTime ) > sk_auto_reload_time.GetFloat() )
	{
		// Reset the timer
		m_flHolsterTime = gpGlobals->curtime;
	
		if ( GetOwner() == NULL )
			return;

		if ( m_iClip1 == GetMaxClip1() )
			return;

		// Just load the clip with no animations
		int ammoFill = MIN( (GetMaxClip1() - m_iClip1), GetOwner()->GetAmmoCount( GetPrimaryAmmoType() ) );
		
		GetOwner()->RemoveAmmo( ammoFill, GetPrimaryAmmoType() );
		m_iClip1 += ammoFill;
	}
}

//==================================================
// Purpose: 
//==================================================
/*
void CWeaponShotgun::WeaponIdle( void )
{
	//Only the player fires this way so we can cast
	CBasePlayer *pPlayer = GetOwner()

	if ( pPlayer == NULL )
		return;

	//If we're on a target, play the new anim
	if ( pPlayer->IsOnTarget() )
	{
		SendWeaponAnim( ACT_VM_IDLE_ACTIVE );
	}
}
*/
