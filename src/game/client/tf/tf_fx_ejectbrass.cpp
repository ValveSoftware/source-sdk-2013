//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================//
#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "tf_shareddefs.h"
#include "tf_weapon_parse.h"
#include "econ_item_system.h"

#define TE_RIFLE_SHELL 1024
#define TE_PISTOL_SHELL 2048

extern CTFWeaponInfo *GetTFWeaponInfo( int iWeapon );

//-----------------------------------------------------------------------------
// Purpose: TF Eject Brass
//-----------------------------------------------------------------------------
void TF_EjectBrassCallback( const CEffectData &data )
{
	const char *pszBrassModel = NULL;

	// If we got given a definition index, see if it has a brass model override
	if ( data.m_nDamageType )
	{
		CEconItemDefinition *pDef = ItemSystem()->GetStaticDataForItemByDefIndex( data.m_nDamageType );
		if ( pDef )
		{
			pszBrassModel = pDef->GetBrassModelOverride();

			// Allow weapon definitions to disable brass ejection
			if ( pszBrassModel && !pszBrassModel[0] )
				return;
		}
	}

	// Otherwise, use the weapon default
	if ( !pszBrassModel || !pszBrassModel[0] )
	{
		CTFWeaponInfo *pWeaponInfo = GetTFWeaponInfo( data.m_nHitBox );
		if ( pWeaponInfo )
		{
			pszBrassModel = pWeaponInfo->m_szBrassModel;
		}
	}

	if ( !pszBrassModel || !pszBrassModel[0] )
		return;

	Vector vForward, vRight, vUp;
	AngleVectors( data.m_vAngles, &vForward, &vRight, &vUp );

	QAngle vecShellAngles;
	VectorAngles( -vUp, vecShellAngles );
	
	Vector vecVelocity = random->RandomFloat( 130, 180 ) * vForward +
						 random->RandomFloat( -30, 30 ) * vRight +
						 random->RandomFloat( -30, 30 ) * vUp;

	float flLifeTime = 10.0f;

	model_t *pModel = (model_t *)engine->LoadModel( pszBrassModel );
	if ( !pModel )
		return;
	
	int flags = FTENT_FADEOUT | FTENT_GRAVITY | FTENT_COLLIDEALL | FTENT_HITSOUND | FTENT_ROTATE;

	if ( data.m_nHitBox == TF_WEAPON_MINIGUN )
	{
		// More velocity for Jake
		vecVelocity = random->RandomFloat( 130, 250 ) * vForward +
			random->RandomFloat( -100, 100 ) * vRight +
			random->RandomFloat( -30, 80 ) * vUp;
	}

	Assert( pModel );	

	C_LocalTempEntity *pTemp = tempents->SpawnTempModel( pModel, data.m_vOrigin, vecShellAngles, vecVelocity, flLifeTime, FTENT_NEVERDIE );
	if ( pTemp == NULL )
		return;

	pTemp->m_vecTempEntAngVelocity[0] = random->RandomFloat(-512,511);
	pTemp->m_vecTempEntAngVelocity[1] = random->RandomFloat(-255,255);
	pTemp->m_vecTempEntAngVelocity[2] = random->RandomFloat(-255,255);

	pTemp->hitSound = TE_PISTOL_SHELL;

	pTemp->SetGravity( 0.4 );

	pTemp->m_flSpriteScale = 10;

	pTemp->flags = flags;

	// don't collide with owner
	pTemp->clientIndex = data.entindex();
	if ( pTemp->clientIndex < 0 )
	{
		pTemp->clientIndex = 0;
	}

	// ::ShouldCollide decides what this collides with
	pTemp->flags |= FTENT_COLLISIONGROUP;
	pTemp->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
}

DECLARE_CLIENT_EFFECT( "TF_EjectBrass", TF_EjectBrassCallback );