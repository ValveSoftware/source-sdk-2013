//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "fx_impact.h"
#include "engine/IEngineSound.h"
#include "c_tf_player.h"
#include "view.h"
#include "tf_gamerules.h"
#include "player_vs_environment/c_tf_tank_boss.h"
#include "decals.h"
#include "clientsideeffects.h"
#include "fx_quad.h"

//-----------------------------------------------------------------------------
// Purpose: Handle weapon impacts
//-----------------------------------------------------------------------------
static int g_MvMRobotImpactCount = 0;
static int g_MvMTankImpactCount = 0;
void ImpactCallback( const CEffectData &data )
{
	trace_t tr;
	Vector vecOrigin, vecStart, vecShotDir;
	int iMaterial, iDamageType, iHitbox;
	short nSurfaceProp;

	C_BaseEntity *pEntity = ParseImpactData( data, &vecOrigin, &vecStart, &vecShotDir, nSurfaceProp, iMaterial, iDamageType, iHitbox );

	if ( !pEntity )
		return;

	bool bImpact = ( data.m_nDamageType != pEntity->GetTeamNumber() || pEntity->GetTeamNumber() < FIRST_GAME_TEAM );

	if ( data.m_nDamageType != pEntity->GetTeamNumber() && pEntity->IsPlayer() )
	{	
		C_TFPlayer *pPlayer = ToTFPlayer( pEntity );

		// Don't impact spies disguised as the same team as this syringe/projectile
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			if ( pPlayer->m_Shared.GetDisguiseTeam() == data.m_nDamageType )
			{
				bImpact = false;
			}
		}
	}

	if ( bImpact )
	{
		// We create lots of impact sounds, especially from Heavy's firing miniguns. To cut down on the number
		// of active sounds, we precull impact sounds that are too far from the current viewpoint. 
		bool bIsMVM = TFGameRules() && TFGameRules()->IsMannVsMachineMode();

		int nApparentTeam = pEntity->GetTeamNumber();

		C_TFPlayer *pPlayer = ToTFPlayer( pEntity );
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			nApparentTeam = pPlayer->m_Shared.GetDisguiseTeam();
		}

		bool bPlaySound = true;
		bool bIsRobotImpact = false;
		
		if ( bIsMVM && pPlayer && nApparentTeam == TF_TEAM_PVE_INVADERS )
		{
			bPlaySound = true;
			bIsRobotImpact = true;
		}
		else
		{
			bPlaySound = (MainViewOrigin() - vecOrigin).LengthSqr() < (1024*1024);
		}

		// If we hit, perform our custom effects and play the sound
		if ( Impact( vecOrigin, vecStart, iMaterial, iDamageType, iHitbox, pEntity, tr ) )
		{
			// Check for custom effects based on the Decal index
			PerformCustomEffects( vecOrigin, tr, vecShotDir, iMaterial, 1 );

			//Play a ricochet sound some of the time
			if ( bPlaySound && random->RandomInt(1,10) <= 3 && (iDamageType == DMG_BULLET) )
			{
				CLocalPlayerFilter filter;
				C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "Bounce.Shrapnel", &vecOrigin );
			}
		}

		if ( bPlaySound )
		{
			// every other one of the mvm impacts are emitted from the world to allow for ~2 impacts playing at a time
			if ( bIsRobotImpact )
			{
				//pEntity->EmitSound( "MVM_Robot.BulletImpact" );
				CLocalPlayerFilter filter;
				if ( g_MvMRobotImpactCount % 4 == 0 )
				{
					if( pPlayer->IsMiniBoss() )
					{
						C_BaseEntity::EmitSound( filter, pEntity->entindex(), "MVM_Giant.BulletImpact", &vecOrigin );
					}
					else
					{
						C_BaseEntity::EmitSound( filter, pEntity->entindex(), "MVM_Robot.BulletImpact", &vecOrigin );
					}
				}
				else
				{
					C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "MVM_Robot.BulletImpact", &vecOrigin );
				}
				g_MvMRobotImpactCount++;
			}
			else if ( bIsMVM && dynamic_cast< C_TFTankBoss* >( pEntity ) )
			{
				//pEntity->EmitSound( "MVM_Robot.BulletImpact" );
				CLocalPlayerFilter filter;
				if ( g_MvMTankImpactCount % 4 == 0 )
				{
					C_BaseEntity::EmitSound( filter, pEntity->entindex(), "MVM_Tank.BulletImpact", &vecOrigin );
				}
				else
				{
					C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "MVM_Tank.BulletImpact", &vecOrigin );
				}
				g_MvMTankImpactCount++;
			}
			else
			{
				PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
			}
		}
	}
}

DECLARE_CLIENT_EFFECT( "Impact", ImpactCallback );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFSplashCallbackHelper( const CEffectData &data, const char *pszEffectName )
{
	Vector	normal;

	AngleVectors( data.m_vAngles, &normal );

	CSmartPtr<CNewParticleEffect> pEffect = CNewParticleEffect::Create( NULL, pszEffectName );
	if ( pEffect->IsValid() )
	{
		pEffect->SetSortOrigin( data.m_vOrigin );
		pEffect->SetControlPoint( 0, data.m_vOrigin );
		pEffect->SetControlPointOrientation( 0, Vector(1,0,0), Vector(0,1,0), Vector(0,0,1) );
	}
}

void TFSplashCallback( const CEffectData &data )
{
	TFSplashCallbackHelper( data, "water_bulletsplash01" );
}
DECLARE_CLIENT_EFFECT( "tf_gunshotsplash", TFSplashCallback );

void TFSplashCallbackMinigun( const CEffectData &data )
{
	TFSplashCallbackHelper( data, "water_bulletsplash01_minigun" );
}
DECLARE_CLIENT_EFFECT( "tf_gunshotsplash_minigun", TFSplashCallbackMinigun );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFPaintBombSplashCallback( const CEffectData &data )
{
	C_BaseEntity *pEntity = data.GetEntity();
	if ( !pEntity )
		return;

	trace_t tr;

	// Get a color if it exists
	Color cColor;
	if ( data.m_bCustomColors )
	{
		cColor.SetColor( data.m_CustomColors.m_vecColor1.x * 255, data.m_CustomColors.m_vecColor1.y * 255, data.m_CustomColors.m_vecColor1.z * 255, 255 );
	}
	else
	{
		cColor.SetColor( 255, 255, 255, 255 );
	}

	CTFPlayer *pPlayer = ToTFPlayer( pEntity );

	// Build out the decal name from nMaterial
	int nPaintIndex = data.m_nMaterial;

	// Decals
	// Special case for world entity with hitbox:
	if ( (pEntity->index == 0) && (data.m_nHitBox != 0) )
	{
		int nMaterial = decalsystem->GetDecalIndexForName( VarArgs( "%s%d%s", "TF_Paint_", nPaintIndex, "_Prop" ) );
		staticpropmgr->AddColorDecalToStaticProp( data.m_vStart, data.m_vOrigin, data.m_nHitBox - 1, nMaterial, false, tr, true, cColor );
	}
	else
	{
		int nMaterial = 0;
		if ( pEntity->index == 0 )
		{
			nMaterial = decalsystem->GetDecalIndexForName( VarArgs( "%s%d%s", "TF_Paint_", nPaintIndex, "_World" ) );
		}
		else if ( pPlayer )
		{
			nMaterial = decalsystem->GetDecalIndexForName( VarArgs( "%s%d%s", "TF_Paint_", nPaintIndex, "_Player" ) );
		}
		else
		{
			nMaterial = decalsystem->GetDecalIndexForName( VarArgs( "%s%d%s", "TF_Paint_", nPaintIndex, "_Prop" ) );
		}
		pEntity->AddColoredDecal( data.m_vStart, data.m_vOrigin, data.m_vOrigin, data.m_nHitBox, nMaterial, false, tr, cColor );
	}
}
DECLARE_CLIENT_EFFECT( "tf_paint_bomb", TFPaintBombSplashCallback );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFEnergyShieldImpactCallback( const CEffectData &data )
{
	// Customized EnergySplash

	C_BaseEntity *pEntity = data.GetEntity();
	if ( !pEntity )
		return;

	Vector vecPos = data.m_vOrigin;
	Vector vecNormal = data.m_vNormal;
	Vector vecOffset = vecPos + ( vecNormal * 2.f );
	
	float flMagnitude = data.m_flMagnitude;
	float flFlashAlpha = 0.25f * flMagnitude;
	float flLingerAlpha = 0.2f * flMagnitude;
	float flFlashLife = 0.1f * flMagnitude;
	float flLingerLife = 0.2f * flMagnitude;

	float flRed = random->RandomFloat( 0.7f, 1.f );
	float flGreen = random->RandomFloat( 0.25f, 0.45f );
	float flBlue = random->RandomFloat( 0.25f, 0.45f );

	// Impact
	FX_AddQuad( vecPos,								// Origin
				vecNormal,							// Normal
				16.f * flMagnitude,					// Start size
				0.f,								// End size
				0.5f, 								// Size bias
				flFlashAlpha,						// Start alpha
				0.f,								// End alpha
				0.1f,								// Alpha bias
				random->RandomInt( 0, 360 ), 		// Yaw
				0.f,								// Delta Yaw
				Vector( flRed, flGreen, flBlue ), 	// Color
				flFlashLife, 						// Lifetime
				"effects/circle_nocull",
				(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );

	// Burn
	FX_AddQuad( vecPos,								// Origin
				vecNormal,							// Normal
				6.f * flMagnitude,					// Start size
				18.f * flMagnitude,					// End size
				0.75f,								// Size bias
				flLingerAlpha,						// Start alpha
				0.f,								// End alpha
				0.4f,								// Alpha bias
				random->RandomInt( 0, 360 ),		// Yaw
				0.f,								// Delta Yaw
				Vector( flRed, flGreen, flBlue ),	// Color
				flLingerLife,						// Lifetime
				"effects/circle_nocull",
				(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );

	CSmartPtr< CSimpleEmitter > pEmitter;
	pEmitter = CSimpleEmitter::Create( "C_EntityDissolve" );
	pEmitter->SetSortOrigin( vecPos );

	PMaterialHandle pMaterialSpark = pEmitter->GetPMaterial( "effects/spark" );
	if ( !pMaterialSpark )
		return;

	SimpleParticle *sParticle;
	for ( int j = 0; j < 6; j++ )
	{
		vecOffset.x = random->RandomFloat( -8.f, 8.f );
		vecOffset.y = random->RandomFloat( -8.f, 8.f );
		vecOffset.z = random->RandomFloat( 0.f, 4.f );

		vecOffset += vecPos;

		sParticle = (SimpleParticle *) pEmitter->AddParticle( sizeof(SimpleParticle), pMaterialSpark, vecOffset );
		if ( !sParticle )
			return;

		sParticle->m_vecVelocity = Vector( random->RandomFloat( -8.f, 8.f ), random->RandomFloat( -8.f, 8.f ), random->RandomFloat( 16.f, 64.f ) );
		sParticle->m_uchStartSize = random->RandomFloat( 1.5f, 3.f );
		sParticle->m_flDieTime	= random->RandomFloat( 0.2f, 0.4f );
		sParticle->m_flLifetime	= 0.f;
		sParticle->m_flRoll	= random->RandomInt( 0, 360 );
		
		float flAlpha = 255.f;
		sParticle->m_flRollDelta = random->RandomFloat( -16.f, 16.f );
		sParticle->m_uchColor[0] = random->RandomInt( 120, 150 );
		sParticle->m_uchColor[1] = random->RandomInt( 40, 70 );
		sParticle->m_uchColor[2] = random->RandomInt( 40, 70 );
		sParticle->m_uchStartAlpha = flAlpha;
		sParticle->m_uchEndAlpha = 0;
		sParticle->m_uchEndSize	= 0;
	}
}
DECLARE_CLIENT_EFFECT( "EnergyShieldImpact", TFEnergyShieldImpactCallback );