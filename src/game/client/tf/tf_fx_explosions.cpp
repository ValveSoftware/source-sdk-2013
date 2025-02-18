//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific explosion effects
//
//=============================================================================//
#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "tf_shareddefs.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_parse.h"
#include "c_basetempentity.h"
#include "tier0/vprof.h"
#include "c_tf_player.h"
#include "econ_item_system.h"
#include "c_tf_fx.h"
#include "networkstringtabledefs.h"

//--------------------------------------------------------------------------------------------------------------
CTFWeaponInfo *GetTFWeaponInfo( int iWeapon )
{
	// Get the weapon information.
	const char *pszWeaponAlias = WeaponIdToAlias( iWeapon );
	if ( !pszWeaponAlias )
	{
		return NULL;
	}

	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pszWeaponAlias );
	if ( hWpnInfo == GetInvalidWeaponInfoHandle() )
	{
		return NULL;
	}

	CTFWeaponInfo *pWeaponInfo = static_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	return pWeaponInfo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFExplosionCallback( const Vector &vecOrigin, const Vector &vecNormal, int iWeaponID, ClientEntityHandle_t hEntity, int nDefID, int nSound, int iCustomParticleIndex )
{
	// Get the weapon information.
	CTFWeaponInfo *pWeaponInfo = NULL;
	switch( iWeaponID )
	{
	case TF_WEAPON_GRENADE_PIPEBOMB:
	case TF_WEAPON_GRENADE_DEMOMAN:
	case TF_WEAPON_PUMPKIN_BOMB:
		pWeaponInfo = GetTFWeaponInfo( TF_WEAPON_PIPEBOMBLAUNCHER );
		break;
	case TF_WEAPON_FLAMETHROWER_ROCKET:
		pWeaponInfo = GetTFWeaponInfo( TF_WEAPON_FLAMETHROWER );
		break;
	default:
		pWeaponInfo = GetTFWeaponInfo( iWeaponID );
		break;
	}

	bool bIsPlayer = false;
	if ( hEntity.Get() )
	{
		C_BaseEntity *pEntity = C_BaseEntity::Instance( hEntity );
		if ( pEntity && pEntity->IsPlayer() )
		{
			bIsPlayer = true;
		}
	}

	// Calculate the angles, given the normal.
	bool bIsWater = ( UTIL_PointContents( vecOrigin ) & CONTENTS_WATER );
	bool bInAir = false;
	QAngle angExplosion( 0.0f, 0.0f, 0.0f );

	// Cannot use zeros here because we are sending the normal at a smaller bit size.
	if ( fabs( vecNormal.x ) < 0.05f && fabs( vecNormal.y ) < 0.05f && fabs( vecNormal.z ) < 0.05f )
	{
		bInAir = true;
		angExplosion.Init();
	}
	else
	{
		VectorAngles( vecNormal, angExplosion );
		bInAir = false;
	}

	// Base explosion effect and sound.
	const char *pszEffect = "ExplosionCore_wall";
	const char *pszSound = "BaseExplosionEffect.Sound";

	// check for a custom particle effect
	if ( iCustomParticleIndex != INVALID_STRING_INDEX )
	{
		pszEffect = GetParticleSystemNameFromIndex( iCustomParticleIndex );
	}

	if ( pWeaponInfo )
	{
		// Explosions.
		if ( iCustomParticleIndex == INVALID_STRING_INDEX )
		{
			if ( bIsWater )
			{
				if ( Q_strlen( pWeaponInfo->m_szExplosionWaterEffect ) > 0 )
				{
					pszEffect = pWeaponInfo->m_szExplosionWaterEffect;
				}
			}
			else
			{
				if ( bIsPlayer || bInAir )
				{
					if ( Q_strlen( pWeaponInfo->m_szExplosionPlayerEffect ) > 0 )
					{
						pszEffect = pWeaponInfo->m_szExplosionPlayerEffect;
					}
				}
				else
				{
					if ( Q_strlen( pWeaponInfo->m_szExplosionEffect ) > 0 )
					{
						pszEffect = pWeaponInfo->m_szExplosionEffect;
					}
				}
			}
		}

		// Sound.
		if ( Q_strlen( pWeaponInfo->m_szExplosionSound ) > 0 )
		{
			// Check for a replacement sound in the itemdef first - defaults to SPECIAL1
			if ( nDefID >= 0 )
			{
				C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( pLocalPlayer )
				{
					CEconItemDefinition *pItemDef = ItemSystem()->GetStaticDataForItemByDefIndex( nDefID );
					if ( pItemDef )
					{
						pszSound = (char *)pItemDef->GetWeaponReplacementSound( pLocalPlayer->GetTeamNumber(), (WeaponSound_t)nSound );
						if ( !pszSound || !pszSound[0] )
						{
							pszSound = pWeaponInfo->m_szExplosionSound;
						}
					}
				}
			}
			else
			{
				pszSound = pWeaponInfo->m_szExplosionSound;
			}
		}
	}
	
	if ( iWeaponID == TF_WEAPON_PUMPKIN_BOMB )
	{
		pszSound = "Halloween.PumpkinExplode";
	}

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, pszSound, &vecOrigin );

	if ( GameRules() )
	{
		pszEffect = GameRules()->TranslateEffectForVisionFilter( "particles", pszEffect );
	}

	DispatchParticleEffect( pszEffect, vecOrigin, angExplosion );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_TETFExplosion : public C_BaseTempEntity
{
public:

	DECLARE_CLASS( C_TETFExplosion, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	C_TETFExplosion( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:

	Vector		m_vecOrigin;
	Vector		m_vecNormal;
	int			m_iWeaponID;
	ClientEntityHandle_t m_hEntity;
	int			m_nDefID;
	int			m_nSound;
	int			m_iCustomParticleIndex;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TETFExplosion::C_TETFExplosion( void )
{
	m_vecOrigin.Init();
	m_vecNormal.Init();
	m_iWeaponID = TF_WEAPON_NONE;
	m_hEntity = INVALID_EHANDLE;
	m_nDefID = -1;
	m_nSound = SPECIAL1;
	m_iCustomParticleIndex = INVALID_STRING_INDEX;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TETFExplosion::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TETFExplosion::PostDataUpdate" );

	TFExplosionCallback( m_vecOrigin, m_vecNormal, m_iWeaponID, m_hEntity, m_nDefID, m_nSound, m_iCustomParticleIndex );
}

static void RecvProxy_ExplosionEntIndex( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int nEntIndex = pData->m_Value.m_Int;
	// The 'new' encoding for INVALID_EHANDLE_INDEX is 2047, but the old encoding
	// was -1. Old demos and replays will use the old encoding so we have to check
	// for it. The field is now unsigned so -1 will not be created in new replays.
	((C_TETFExplosion*)pStruct)->m_hEntity = (nEntIndex == kInvalidEHandleExplosion || nEntIndex == -1) ? INVALID_EHANDLE : ClientEntityList().EntIndexToHandle( nEntIndex );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT( C_TETFExplosion, DT_TETFExplosion, CTETFExplosion )
	RecvPropFloat( RECVINFO( m_vecOrigin[0] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[1] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[2] ) ),
	RecvPropVector( RECVINFO( m_vecNormal ) ),
	RecvPropInt( RECVINFO( m_iWeaponID ) ),
	RecvPropInt( "entindex", 0, SIZEOF_IGNORE, 0, RecvProxy_ExplosionEntIndex ),
	RecvPropInt( RECVINFO( m_nDefID ) ),
	RecvPropInt( RECVINFO( m_nSound ) ),
	RecvPropInt( RECVINFO( m_iCustomParticleIndex ) ),
END_RECV_TABLE()

