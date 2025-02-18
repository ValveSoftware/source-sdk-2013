//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Death Calling card version based off the stickybolt code.
//=============================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "fx.h"
#include "decals.h"
#include "iefx.h"
#include "engine/IEngineSound.h"
#include "materialsystem/imaterialvar.h"
#include "IEffects.h"
#include "engine/IEngineTrace.h"
#include "vphysics/constraints.h"
#include "engine/ivmodelinfo.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "engine/ivdebugoverlay.h"
#include "c_te_effect_dispatch.h"
#include "c_tf_player.h"
#include "GameEventListener.h"
#include "tf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IPhysicsSurfaceProps *physprops;
IPhysicsObject *GetWorldPhysObject( void );

extern CBaseEntity *BreakModelCreateSingle( CBaseEntity *pOwner, breakmodel_t *pModel, const Vector &position, 
	const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, int nSkin, const breakablepropparams_t &params );

//-----------------------------------------------------------------------------
// Purpose: Creates a "Calling Card" prop at the victim's location
//-----------------------------------------------------------------------------
void CreateDeathCallingCard(
	const Vector &vecOrigin,
	const QAngle &vAngle,
	const int iVictimIndex,
	const int iShooterIndex,
	const int iCallingCardIndex
) {
	if ( iCallingCardIndex < 1 || iCallingCardIndex >= TF_CALLING_CARD_MODEL_COUNT )
	{
		Warning( "Attempted to Call CreateDeathCallingCard With invalid index %d", iCallingCardIndex );
		return;
	}

	const char* pszModelName = g_pszDeathCallingCardModels[iCallingCardIndex];

	CTFPlayer *pVictim = ToTFPlayer( UTIL_PlayerByIndex( iVictimIndex ) );
	if ( !pVictim )
		return;

	breakablepropparams_t breakParams( vecOrigin, vAngle, vec3_origin, vec3_origin );
	breakParams.impactEnergyScale = 1.0f;

	breakmodel_t breakModel;
	Q_strncpy( breakModel.modelName, pszModelName, sizeof(breakModel.modelName) );

	breakModel.health = 1;
	breakModel.fadeTime = RandomFloat(7,10);
	breakModel.fadeMinDist = 0.0f;
	breakModel.fadeMaxDist = 0.0f;
	breakModel.burstScale = 1.0f;
	breakModel.collisionGroup = COLLISION_GROUP_DEBRIS;
	breakModel.isRagdoll = false;
	breakModel.isMotionDisabled = false;
	breakModel.placementName[0] = 0;
	breakModel.placementIsBone = false;
	breakModel.offset = Vector( 0, 0, 50 );

	CBaseEntity * pBreakModel = BreakModelCreateSingle( 
		pVictim, 
		&breakModel, 
		pVictim->GetAbsOrigin() + Vector( 0, 0, 50 ), 
		QAngle(0, vAngle.y, 0 ),
		vec3_origin, 
		vec3_origin,
		0, 
		breakParams 
	);

	// Scale down the tombstones a bit
	if ( pBreakModel )
    {
        CBaseAnimating *pAnim = dynamic_cast < CBaseAnimating * > ( pBreakModel );
        if ( pAnim )
        {
			pAnim->SetModelScale( 0.9f );
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DeathCallingCard( const CEffectData &data )
{
	CreateDeathCallingCard( 
		data.m_vOrigin, 
		data.m_vAngles, 
		data.m_nAttachmentIndex, // Victim
		data.m_nHitBox,			 // iShooter
		data.m_fFlags			// Calling card Index
	);
}

DECLARE_CLIENT_EFFECT( "TFDeathCallingCard", DeathCallingCard );
