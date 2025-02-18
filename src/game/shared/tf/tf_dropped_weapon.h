//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_DROPPED_WEAPON_H
#define TF_DROPPED_WEAPON_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CTFDroppedWeapon C_TFDroppedWeapon
#endif // CLIENT_DLL

#ifdef GAME_DLL
class CTFPlayer;
#endif // GAME_DLL

DECLARE_AUTO_LIST( IDroppedWeaponAutoList );

class CTFDroppedWeapon : public CBaseAnimating, public IDroppedWeaponAutoList
{
public:
	DECLARE_CLASS( CTFDroppedWeapon, CBaseAnimating );
	DECLARE_NETWORKCLASS();

	CTFDroppedWeapon();
	~CTFDroppedWeapon();

	virtual void Spawn() OVERRIDE;

#ifdef CLIENT_DLL
	virtual void OnPreDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual void OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual void ClientThink() OVERRIDE;

	// target id
	virtual bool IsVisibleToTargetID( void ) const OVERRIDE;

	// Draw Attachment models
	virtual bool			OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );

	virtual IMaterial		*GetEconWeaponMaterialOverride( int iTeam ) OVERRIDE;
	virtual void			ModifyEmitSoundParams( EmitSound_t &params ) OVERRIDE;
#endif // CLIENT_DLL

#ifdef GAME_DLL
	static CTFDroppedWeapon *Create( CTFPlayer *pLastOwner, const Vector &vecOrigin, const QAngle &vecAngles, const char *pszModelName, const CEconItemView *pItem );
	void InitDroppedWeapon( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon, bool bSwap, bool bIsSuicide = false );
	void InitPickedUpWeapon( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon );

	void ChargeLevelDegradeThink();
#endif // GAME_DLL

	CEconItemView *GetItem() { return &m_Item; }
	const CEconItemView *GetItem() const { return &m_Item; }

	float GetChargeLevel( void ){ return m_flChargeLevel; }

private:

	CNetworkVarEmbedded( CEconItemView,	m_Item );
	CNetworkVar( float, m_flChargeLevel );

#ifdef GAME_DLL

	CHandle< CTFPlayer > m_hPlayer;

	// preserve weapon ammo in the clip
	void SetItem( const CEconItemView *pItem );

	// preserve ammo count
	int m_nClip;
	int m_nAmmo;
	int m_nDetonated;
	float m_flEnergy;
	float m_flEffectBarRegenTime;
	float m_flNextPrimaryAttack;
	float m_flNextSecondaryAttack;
	bool m_bBroken;
	float m_flMeter;
#endif // GAME_DLL

#ifdef CLIENT_DLL
	void SetupParticleEffect();
	HPARTICLEFFECT		m_effect;

	CHandle< C_BaseAnimating > m_worldmodelStatTrakAddon;

	void UpdateGlowEffect( void );
	void DestroyGlowEffect( void );
	CGlowObject *m_pGlowEffect;
	bool m_bShouldGlowForLocalPlayer;

	CUtlVector<AttachedModelData_t> m_vecAttachedModels;

	float m_flOldChargeLevel;
#endif // CLIENT_DLL
};

#endif // TF_DROPPED_WEAPON_H
