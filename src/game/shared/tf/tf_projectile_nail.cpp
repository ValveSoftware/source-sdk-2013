//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Nail
//
//=============================================================================
#include "cbase.h"
#include "tf_projectile_nail.h"
#include "tf_gamerules.h"

#ifdef CLIENT_DLL
#include "c_basetempentity.h"
#include "c_te_legacytempents.h"
#include "c_te_effect_dispatch.h"
#include "input.h"
#include "c_tf_player.h"
#include "cliententitylist.h"
#endif

#ifdef GAME_DLL
#include "tf_player.h"
#endif


//=============================================================================
//
// TF Syringe Projectile functions (Server specific).
//
#define SYRINGE_MODEL				"models/weapons/w_models/w_syringe_proj.mdl"
#define SYRINGE_LEECH_MODEL			"models/weapons/c_models/c_leechgun/c_leech_proj.mdl"
#define SYRINGE_DISPATCH_EFFECT		"ClientProjectile_Syringe"

LINK_ENTITY_TO_CLASS( tf_projectile_syringe, CTFProjectile_Syringe );
PRECACHE_REGISTER( tf_projectile_syringe );


short g_sModelIndexSyringe;
void PrecacheSyringe(void *pUser)
{
	g_sModelIndexSyringe = modelinfo->GetModelIndex( SYRINGE_MODEL );
}

PRECACHE_REGISTER_FN(PrecacheSyringe);

//-----------------------------------------------------------------------------
// CTFProjectile_Syringe
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#define SYRINGE_GRAVITY		0.3f
#define SYRINGE_VELOCITY	1000.0f

//-----------------------------------------------------------------------------
// Purpose: Model path comes from the launcher's "custom projectile model"
// attribute; falls back to the stock syringe.
//-----------------------------------------------------------------------------
static const char *GetSyringeModelPath( CBaseEntity *pLauncher, CAttribute_String &attrModel )
{
	static CSchemaAttributeDefHandle pAttrDef_CustomModel( "custom projectile model" );
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( pLauncher );
	if ( pWeapon && pAttrDef_CustomModel && pWeapon->GetAttributeContainer() )
	{
		CEconItemView *pItem = pWeapon->GetAttributeContainer()->GetItem();
		if ( pItem && pItem->FindAttribute( pAttrDef_CustomModel, &attrModel ) && !attrModel.value().empty() )
			return attrModel.value().c_str();
	}
	return SYRINGE_MODEL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFBaseProjectile *CTFProjectile_Syringe::Create( 
	const Vector &vecOrigin, 
	const QAngle &vecAngles, 
	CTFWeaponBaseGun *pLauncher /*= NULL*/,
	CBaseEntity *pOwner /*= NULL*/, 
	CBaseEntity *pScorer /*= NULL*/, 
	bool bCritical /*= false */
) {
	// The client branch of the base Create uses this index for the shooter's own predicted syringe.
	CAttribute_String attrModel;
	int nModelIndex = modelinfo->GetModelIndex( GetSyringeModelPath( pLauncher, attrModel ) );

	return CTFBaseProjectile::Create( "tf_projectile_syringe", vecOrigin, vecAngles, pOwner, SYRINGE_VELOCITY, nModelIndex, SYRINGE_DISPATCH_EFFECT, pScorer, bCritical, vec3_origin, vec3_origin, pLauncher );
}

//-----------------------------------------------------------------------------
// Purpose: Model for the server entity. The launcher is set before Spawn().
//-----------------------------------------------------------------------------
const char *CTFProjectile_Syringe::GetProjectileModelName( void )
{
#ifdef GAME_DLL
	CAttribute_String attrModel;
	return STRING( AllocPooledString( GetSyringeModelPath( GetLauncher(), attrModel ) ) );
#else
	return SYRINGE_MODEL;
#endif
}

#ifdef GAME_DLL
void CTFProjectile_Syringe::Precache( void )
{
	PrecacheModel( SYRINGE_MODEL );
	PrecacheModel( SYRINGE_LEECH_MODEL ); // keep in sync with the schema attribute value
	BaseClass::Precache();
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFProjectile_Syringe::PhysicsSolidMaskForEntity( void ) const
{ 
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_REDTEAM | CONTENTS_BLUETEAM;
}

//-----------------------------------------------------------------------------
float CTFProjectile_Syringe::GetGravity( void )
{
	return SYRINGE_GRAVITY;
}


#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
void GetSyringeTrailParticleName( CTFPlayer *pPlayer, CAttribute_String *attrParticleName, bool bCritical )
{
	int iTeamNumber = TF_TEAM_RED;	
	if ( pPlayer )
	{
		iTeamNumber = pPlayer->GetTeamNumber();
		CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
		if ( pWeapon )
		{
			static CSchemaAttributeDefHandle pAttrDef_ParticleName( "projectile particle name" );
			CEconItemView *pItem = pWeapon->GetAttributeContainer()->GetItem();
			if ( pAttrDef_ParticleName && pItem )
			{
				if ( pItem->FindAttribute( pAttrDef_ParticleName, attrParticleName ) )
				{
					const char * pParticleName = attrParticleName->value().c_str();
					if ( iTeamNumber == TF_TEAM_BLUE && V_stristr( pParticleName, "_teamcolor_red" ))
					{
						static char pBlue[256];
						V_StrSubst( attrParticleName->value().c_str(), "_teamcolor_red", "_teamcolor_blue", pBlue, 256 );
						attrParticleName->set_value( pBlue );
					}
					return;
				}
			}
		}
	}
	
	if ( iTeamNumber == TF_TEAM_BLUE )
	{
		attrParticleName->set_value( bCritical ? "nailtrails_medic_blue_crit" : "nailtrails_medic_blue" );
	}
	else
	{
		attrParticleName->set_value( bCritical ? "nailtrails_medic_red_crit" : "nailtrails_medic_red" );
	}
	return;
}

//-----------------------------------------------------------------------------
// Purpose: For Syringe Projectiles, Add effects
//-----------------------------------------------------------------------------
void ClientsideProjectileSyringeCallback( const CEffectData &data )
{
	// Get the syringe and add it to the client entity list, so we can attach a particle system to it.
	C_TFPlayer *pPlayer = dynamic_cast<C_TFPlayer*>( ClientEntityList().GetBaseEntityFromHandle( data.m_hEntity ) );
	if ( pPlayer )
	{
		C_LocalTempEntity *pSyringe = ClientsideProjectileCallback( data, SYRINGE_GRAVITY );
		if ( pSyringe )
		{
			CAttribute_String attrParticleName;
			
			pSyringe->m_nSkin = ( pPlayer->GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;
			bool bCritical = ( ( data.m_nDamageType & DMG_CRITICAL ) != 0 );
			GetSyringeTrailParticleName( pPlayer, &attrParticleName, bCritical );

			pSyringe->AddParticleEffect( attrParticleName.value().c_str() );
			pSyringe->AddEffects( EF_NOSHADOW );
			pSyringe->flags |= FTENT_USEFASTCOLLISIONS;
		}
	}
}

DECLARE_CLIENT_EFFECT( SYRINGE_DISPATCH_EFFECT, ClientsideProjectileSyringeCallback );

#endif
