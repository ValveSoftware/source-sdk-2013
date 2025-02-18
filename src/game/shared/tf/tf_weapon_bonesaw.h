//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_BONESAW_H
#define TF_WEAPON_BONESAW_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#include "tf_weapon_medigun.h"

#define CTFBonesaw C_TFBonesaw
#endif

enum bonesaw_weapontypes_t
{
	BONESAW_DEFAULT = 0,
	BONESAW_UBER_ONHIT,
	BONESAW_UBER_SAVEDONDEATH,
	BONESAW_RADIUSHEAL,
	BONESAW_TONGS,
};

//=============================================================================
//
// Bonesaw class.
//
class CTFBonesaw : public CTFWeaponBaseMelee
{
public:
	DECLARE_CLASS( CTFBonesaw, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFBonesaw() {}
	virtual void		Activate( void );
	virtual int			GetWeaponID( void ) const { return TF_WEAPON_BONESAW; }

	virtual void		SecondaryAttack();

	virtual bool		DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt );
	int					GetBonesawType( void ) const		{ int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode ); return iMode; };

	virtual void		DoMeleeDamage( CBaseEntity* ent, trace_t& trace ) OVERRIDE;
	
	float				GetProgress( void ) { return 0.f; }
	const char*			GetEffectLabelText( void ) { return "#TF_ORGANS"; }

	float				GetBoneSawSpeedMod( void );

#ifdef GAME_DLL
	virtual void		OnPlayerKill( CTFPlayer *pVictim, const CTakeDamageInfo &info ) OVERRIDE;
#else
	virtual void		OnDataChanged( DataUpdateType_t updateType );
	void				UpdateChargePoseParam( void );
	virtual void		GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM] );
	virtual void		UpdateAttachmentModels( void );
#endif

private:

#ifdef CLIENT_DLL
	int			m_iUberChargePoseParam;
	float		m_flChargeLevel;
#endif

	CTFBonesaw( const CTFBonesaw & ) {}
};

#endif // TF_WEAPON_BONESAW_H
