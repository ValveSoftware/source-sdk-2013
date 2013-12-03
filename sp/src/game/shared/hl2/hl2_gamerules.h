//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Game rules for Half-Life 2.
//
//=============================================================================//

#ifndef HL2_GAMERULES_H
#define HL2_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif

#include "gamerules.h"
#include "singleplay_gamerules.h"
#include "hl2_shareddefs.h"

#ifdef CLIENT_DLL
	#define CHalfLife2 C_HalfLife2
	#define CHalfLife2Proxy C_HalfLife2Proxy
#endif


class CHalfLife2Proxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CHalfLife2Proxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};


class CHalfLife2 : public CSingleplayRules
{
public:
	DECLARE_CLASS( CHalfLife2, CSingleplayRules );

	// Damage Query Overrides.
	virtual bool			Damage_IsTimeBased( int iDmgType );
	// TEMP:
	virtual int				Damage_GetTimeBased( void );
	
	virtual bool			ShouldCollide( int collisionGroup0, int collisionGroup1 );
	virtual bool			ShouldUseRobustRadiusDamage(CBaseEntity *pEntity);
#ifndef CLIENT_DLL
	virtual bool			ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target );
	virtual float			GetAutoAimScale( CBasePlayer *pPlayer );
	virtual float			GetAmmoQuantityScale( int iAmmoIndex );
	virtual void			LevelInitPreEntity();
#endif

private:
	// Rules change for the mega physgun
	CNetworkVar( bool, m_bMegaPhysgun );

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.

	CHalfLife2();
	virtual ~CHalfLife2() {}

	virtual void			Think( void );

	virtual bool			ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	virtual void			PlayerSpawn( CBasePlayer *pPlayer );

	virtual void			InitDefaultAIRelationships( void );
	virtual const char*		AIClassText(int classType);
	virtual const char *GetGameDescription( void ) { return "Half-Life 2"; }

	// Ammo
	virtual void			PlayerThink( CBasePlayer *pPlayer );
	virtual float			GetAmmoDamage( CBaseEntity *pAttacker, CBaseEntity *pVictim, int nAmmoType );

	virtual bool			ShouldBurningPropsEmitLight();
public:

	bool AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );

	bool	NPC_ShouldDropGrenade( CBasePlayer *pRecipient );
	bool	NPC_ShouldDropHealth( CBasePlayer *pRecipient );
	void	NPC_DroppedHealth( void );
	void	NPC_DroppedGrenade( void );
	bool	MegaPhyscannonActive( void ) { return m_bMegaPhysgun;	}
	
	virtual bool IsAlyxInDarknessMode();

private:

	float	m_flLastHealthDropTime;
	float	m_flLastGrenadeDropTime;

	void AdjustPlayerDamageTaken( CTakeDamageInfo *pInfo );
	float AdjustPlayerDamageInflicted( float damage );

	int						DefaultFOV( void ) { return 75; }
#endif
};


//-----------------------------------------------------------------------------
// Gets us at the Half-Life 2 game rules
//-----------------------------------------------------------------------------
inline CHalfLife2* HL2GameRules()
{
#if ( !defined( HL2_DLL ) && !defined( HL2_CLIENT_DLL ) ) || defined( HL2MP )
	Assert( 0 );	// g_pGameRules is NOT an instance of CHalfLife2 and bad things happen
#endif

	return static_cast<CHalfLife2*>(g_pGameRules);
}



#endif // HL2_GAMERULES_H
