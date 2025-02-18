//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "engine/IEngineSound.h"
#include "particles_simple.h"
#include "particles_localspace.h"
#include "dlight.h"
#include "iefx.h"
#include "clientsideeffects.h"
#include "clienteffectprecachesystem.h"
#include "glow_overlay.h"
#include "effect_dispatch_data.h"
#include "c_te_effect_dispatch.h"
#include "tier0/vprof.h"
#include "tier1/KeyValues.h"
#include "effect_color_tables.h"
#include "iviewrender_beams.h"
#include "view.h"
#include "IEffects.h"
#include "fx.h"
#include "c_te_legacytempents.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//Precahce the effects
#ifndef TF_CLIENT_DLL
CLIENTEFFECT_REGISTER_BEGIN( PrecacheMuzzleFlash )
CLIENTEFFECT_MATERIAL( "effects/muzzleflash1" )
CLIENTEFFECT_MATERIAL( "effects/muzzleflash2" )
CLIENTEFFECT_MATERIAL( "effects/muzzleflash3" )
CLIENTEFFECT_MATERIAL( "effects/muzzleflash4" )
#ifndef CSTRIKE_DLL
CLIENTEFFECT_MATERIAL( "effects/bluemuzzle" )
CLIENTEFFECT_MATERIAL( "effects/gunshipmuzzle" )
CLIENTEFFECT_MATERIAL( "effects/gunshiptracer" )
#ifndef HL2MP
CLIENTEFFECT_MATERIAL( "effects/huntertracer" )
#endif
CLIENTEFFECT_MATERIAL( "sprites/physcannon_bluelight2" )
CLIENTEFFECT_MATERIAL( "effects/combinemuzzle1" )
CLIENTEFFECT_MATERIAL( "effects/combinemuzzle2" )
CLIENTEFFECT_MATERIAL( "effects/combinemuzzle2_nocull" )
#endif
CLIENTEFFECT_REGISTER_END()
#endif

//Whether or not we should emit a dynamic light
ConVar muzzleflash_light( "muzzleflash_light", "1", FCVAR_ARCHIVE );

extern void FX_TracerSound( const Vector &start, const Vector &end, int iTracerType );


//===================================================================
//===================================================================
class CImpactOverlay : public CWarpOverlay
{
public:
	
	virtual bool Update( void )
	{
		m_flLifetime += gpGlobals->frametime;
		
		const float flTotalLifetime = 0.1f;

		if ( m_flLifetime < flTotalLifetime )
		{
			float flColorScale = 1.0f - ( m_flLifetime / flTotalLifetime );

			for( int i=0; i < m_nSprites; i++ )
			{
				m_Sprites[i].m_vColor = m_vBaseColors[i] * flColorScale;
				
				m_Sprites[i].m_flHorzSize += 1.0f * gpGlobals->frametime;
				m_Sprites[i].m_flVertSize += 1.0f * gpGlobals->frametime;
			}
	
			return true;
		}
	
		return false;
	}

public:

	float	m_flLifetime;
	Vector	m_vBaseColors[MAX_SUN_LAYERS];

};

//-----------------------------------------------------------------------------
// Purpose: Play random ricochet sound
// Input  : *pos - 
//-----------------------------------------------------------------------------
void FX_RicochetSound( const Vector& pos )
{
	Vector org = pos;
	CLocalPlayerFilter filter;
 	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "FX_RicochetSound.Ricochet", &org );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : entityIndex - 
//			attachmentIndex - 
//			*origin - 
//			*angles - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool FX_GetAttachmentTransform( ClientEntityHandle_t hEntity, int attachmentIndex, Vector *origin, QAngle *angles )
{
	// Validate our input
	if ( ( hEntity == INVALID_EHANDLE ) || ( attachmentIndex < 1 ) )
	{
		if ( origin != NULL )
		{
			*origin = vec3_origin;
		}
		
		if ( angles != NULL )
		{
			*angles = QAngle(0,0,0);
		}

		return false;
	}

	// Get the actual entity
	IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( hEntity );
	if ( pRenderable )
	{
		Vector attachOrigin;
		QAngle attachAngles;

		// Find the attachment's matrix
		pRenderable->GetAttachment( attachmentIndex, attachOrigin, attachAngles );
	
		if ( origin != NULL )
		{
			*origin = attachOrigin;
		}
		
		if ( angles != NULL )
		{
			*angles = attachAngles;
		}
		
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : entityIndex - 
//			attachmentIndex - 
//			&transform - 
//-----------------------------------------------------------------------------
bool FX_GetAttachmentTransform( ClientEntityHandle_t hEntity, int attachmentIndex, matrix3x4_t &transform )
{
	Vector	origin;
	QAngle	angles;

	if ( FX_GetAttachmentTransform( hEntity, attachmentIndex, &origin, &angles ) )
	{
		AngleMatrix( angles, origin, transform );
		return true;
	}

	// Entity doesn't exist
	SetIdentityMatrix( transform );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FX_MuzzleEffect( 
	const Vector &origin, 
	const QAngle &angles, 
	float scale, 
	ClientEntityHandle_t hEntity, 
	unsigned char *pFlashColor,
	bool bOneFrame )
{
	VPROF_BUDGET( "FX_MuzzleEffect", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	
	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "MuzzleFlash" );
	pSimple->SetSortOrigin( origin );
	
	SimpleParticle *pParticle;
	Vector			forward, offset;

	AngleVectors( angles, &forward );
	float flScale = random->RandomFloat( scale-0.25f, scale+0.25f );

	if ( flScale < 0.5f )
	{
		flScale = 0.5f;
	}
	else if ( flScale > 8.0f )
	{
		flScale = 8.0f;
	}

	//
	// Flash
	//

	int i;
	for ( i = 1; i < 9; i++ )
	{
		offset = origin + (forward * (i*2.0f*scale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/muzzleflash%d", random->RandomInt(1,4) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= /*bOneFrame ? 0.0001f : */0.1f;

		pParticle->m_vecVelocity.Init();

		if ( !pFlashColor )
		{
			pParticle->m_uchColor[0]	= 255;
			pParticle->m_uchColor[1]	= 255;
			pParticle->m_uchColor[2]	= 255;
		}
		else
		{
			pParticle->m_uchColor[0]	= pFlashColor[0];
			pParticle->m_uchColor[1]	= pFlashColor[1];
			pParticle->m_uchColor[2]	= pFlashColor[2];
		}

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 128;

		pParticle->m_uchStartSize	= (random->RandomFloat( 6.0f, 9.0f ) * (12-(i))/9) * flScale;
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}

	//
	// Smoke
	//

	/*
	for ( i = 0; i < 4; i++ )
	{
		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "particle/particle_smokegrenade" ), origin );
			
		if ( pParticle == NULL )
			return;

		alpha = random->RandomInt( 32, 84 );
		color = random->RandomInt( 64, 164 );

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= random->RandomFloat( 0.5f, 1.0f );

		pParticle->m_vecVelocity.Random( -0.5f, 0.5f );
		pParticle->m_vecVelocity += forward;
		VectorNormalize( pParticle->m_vecVelocity );

		pParticle->m_vecVelocity	*= random->RandomFloat( 16.0f, 32.0f );
		pParticle->m_vecVelocity[2] += random->RandomFloat( 4.0f, 16.0f );

		pParticle->m_uchColor[0]	= color;
		pParticle->m_uchColor[1]	= color;
		pParticle->m_uchColor[2]	= color;
		pParticle->m_uchStartAlpha	= alpha;
		pParticle->m_uchEndAlpha	= 0;
		pParticle->m_uchStartSize	= random->RandomInt( 4, 8 ) * flScale;
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize*2;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -4.0f, 4.0f );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scale - 
//			attachmentIndex - 
//			bOneFrame - 
//-----------------------------------------------------------------------------
void FX_MuzzleEffectAttached( 
	float scale, 
	ClientEntityHandle_t hEntity, 
	int attachmentIndex, 
	unsigned char *pFlashColor,
	bool bOneFrame )
{
	VPROF_BUDGET( "FX_MuzzleEffect", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	// If the material isn't available, let's not do anything.
	if ( g_Mat_SMG_Muzzleflash[0] == NULL )
	{
		return;
	}
	
	CSmartPtr<CLocalSpaceEmitter> pSimple = CLocalSpaceEmitter::Create( "MuzzleFlash", hEntity, attachmentIndex );
	Assert( pSimple );
	if ( pSimple == NULL )
		return;
	
	// Lock our bounding box
	pSimple->GetBinding().SetBBox( -( Vector( 16, 16, 16 ) * scale ), ( Vector( 16, 16, 16 ) * scale ) );
	
	SimpleParticle *pParticle;
	Vector			forward(1,0,0), offset;

	float flScale = random->RandomFloat( scale-0.25f, scale+0.25f );

	if ( flScale < 0.5f )
	{
		flScale = 0.5f;
	}
	else if ( flScale > 8.0f )
	{
		flScale = 8.0f;
	}

	//
	// Flash
	//

	int i;
	for ( i = 1; i < 9; i++ )
	{
		offset = (forward * (i*2.0f*scale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_SMG_Muzzleflash[random->RandomInt(0,3)], offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= bOneFrame ? 0.0001f : 0.1f;

		pParticle->m_vecVelocity.Init();

		if ( !pFlashColor )
		{
			pParticle->m_uchColor[0]	= 255;
			pParticle->m_uchColor[1]	= 255;
			pParticle->m_uchColor[2]	= 255;
		}
		else
		{
			pParticle->m_uchColor[0]	= pFlashColor[0];
			pParticle->m_uchColor[1]	= pFlashColor[1];
			pParticle->m_uchColor[2]	= pFlashColor[2];
		}

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 128;

		pParticle->m_uchStartSize	= (random->RandomFloat( 6.0f, 9.0f ) * (12-(i))/9) * flScale;
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}


	if ( !ToolsEnabled() )
		return;

	if ( !clienttools->IsInRecordingMode() )
		return;

	C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( hEntity );
	if ( pEnt )
	{
		pEnt->RecordToolMessage();
	}

	// NOTE: Particle system destruction message will be sent by the particle effect itself.
	int nId = pSimple->AllocateToolParticleEffectId();

	KeyValues *msg = new KeyValues( "OldParticleSystem_Create" );
	msg->SetString( "name", "FX_MuzzleEffectAttached" );
	msg->SetInt( "id", nId );
	msg->SetFloat( "time", gpGlobals->curtime );

	KeyValues *pEmitter = msg->FindKey( "DmeSpriteEmitter", true );
	pEmitter->SetInt( "count", 9 );
	pEmitter->SetFloat( "duration", 0 );
	pEmitter->SetString( "material", "effects/muzzleflash2" ); // FIXME - create DmeMultiMaterialSpriteEmitter to support the 4 materials of muzzleflash
	pEmitter->SetInt( "active", true );

	KeyValues *pInitializers = pEmitter->FindKey( "initializers", true );

	KeyValues *pPosition = pInitializers->FindKey( "DmeLinearAttachedPositionInitializer", true );
	pPosition->SetPtr( "entindex", (void*)(intp)pEnt->entindex() );
	pPosition->SetInt( "attachmentIndex", attachmentIndex );
	pPosition->SetFloat( "linearOffsetX", 2.0f * scale );

	// TODO - create a DmeConstantLifetimeInitializer
	KeyValues *pLifetime = pInitializers->FindKey( "DmeRandomLifetimeInitializer", true );
	pLifetime->SetFloat( "minLifetime", bOneFrame ? 1.0f / 24.0f : 0.1f );
	pLifetime->SetFloat( "maxLifetime", bOneFrame ? 1.0f / 24.0f : 0.1f );

	KeyValues *pVelocity = pInitializers->FindKey( "DmeConstantVelocityInitializer", true );
	pVelocity->SetFloat( "velocityX", 0.0f );
	pVelocity->SetFloat( "velocityY", 0.0f );
	pVelocity->SetFloat( "velocityZ", 0.0f );

	KeyValues *pRoll = pInitializers->FindKey( "DmeRandomRollInitializer", true );
	pRoll->SetFloat( "minRoll", 0.0f );
	pRoll->SetFloat( "maxRoll", 360.0f );

	// TODO - create a DmeConstantRollSpeedInitializer
	KeyValues *pRollSpeed = pInitializers->FindKey( "DmeRandomRollSpeedInitializer", true );
	pRollSpeed->SetFloat( "minRollSpeed", 0.0f );
	pRollSpeed->SetFloat( "maxRollSpeed", 0.0f );

	// TODO - create a DmeConstantColorInitializer
	KeyValues *pColor = pInitializers->FindKey( "DmeRandomInterpolatedColorInitializer", true );
	Color color( pFlashColor ? pFlashColor[ 0 ] : 255, pFlashColor ? pFlashColor[ 1 ] : 255, pFlashColor ? pFlashColor[ 2 ] : 255, 255 );
	pColor->SetColor( "color1", color );
	pColor->SetColor( "color2", color );

	// TODO - create a DmeConstantAlphaInitializer
	KeyValues *pAlpha = pInitializers->FindKey( "DmeRandomAlphaInitializer", true );
	pAlpha->SetInt( "minStartAlpha", 255 );
	pAlpha->SetInt( "maxStartAlpha", 255 );
	pAlpha->SetInt( "minEndAlpha", 128 );
	pAlpha->SetInt( "maxEndAlpha", 128 );

	// size = rand(6..9) * indexed(12/9..4/9) * flScale = rand(6..9) * ( 4f + f * i )
	KeyValues *pSize = pInitializers->FindKey( "DmeMuzzleFlashSizeInitializer", true );
	float f = flScale / 9.0f;
	pSize->SetFloat( "indexedBase", 4.0f * f );
	pSize->SetFloat( "indexedDelta", f );
	pSize->SetFloat( "minRandomFactor", 6.0f );
	pSize->SetFloat( "maxRandomFactor", 9.0f );

/*
	KeyValues *pUpdaters = pEmitter->FindKey( "updaters", true );

	pUpdaters->FindKey( "DmePositionVelocityUpdater", true );
	pUpdaters->FindKey( "DmeRollUpdater", true );
	pUpdaters->FindKey( "DmeAlphaLinearUpdater", true );
	pUpdaters->FindKey( "DmeSizeUpdater", true );
*/
	ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
	msg->deleteThis();
}

//-----------------------------------------------------------------------------
// Old-style muzzle flashes
//-----------------------------------------------------------------------------
void MuzzleFlashCallback( const CEffectData &data )
{
	Vector vecOrigin = data.m_vOrigin;
	QAngle vecAngles = data.m_vAngles;
	if ( data.entindex() > 0 )
	{
		IClientRenderable *pRenderable = data.GetRenderable();
		if ( !pRenderable )
			return;

		if ( data.m_nAttachmentIndex )
		{
			//FIXME: We also need to allocate these particles into an attachment space setup
			pRenderable->GetAttachment( data.m_nAttachmentIndex, vecOrigin, vecAngles );
		}
		else
		{
			vecOrigin = pRenderable->GetRenderOrigin();
			vecAngles = pRenderable->GetRenderAngles();
		}
	}

	tempents->MuzzleFlash( vecOrigin, vecAngles, data.m_fFlags & (~MUZZLEFLASH_FIRSTPERSON), data.m_hEntity, (data.m_fFlags & MUZZLEFLASH_FIRSTPERSON) != 0 );	
}

DECLARE_CLIENT_EFFECT( "MuzzleFlash", MuzzleFlashCallback );
 
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&velocity - 
//			scale - 
//			numParticles - 
//			*pColor - 
//			iAlpha - 
//			*pMaterial - 
//			flRoll - 
//			flRollDelta - 
//-----------------------------------------------------------------------------
CSmartPtr<CSimpleEmitter> FX_Smoke( const Vector &origin, const Vector &velocity, float scale, int numParticles, float flDietime, unsigned char *pColor, int iAlpha, const char *pMaterial, float flRoll, float flRollDelta )
{
	VPROF_BUDGET( "FX_Smoke", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "FX_Smoke" );
	pSimple->SetSortOrigin( origin );

	SimpleParticle *pParticle;

	// Smoke
	for ( int i = 0; i < numParticles; i++ )
	{
		PMaterialHandle hMaterial = pSimple->GetPMaterial( pMaterial );
		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial, origin );			
		if ( pParticle == NULL )
			return NULL;

		pParticle->m_flLifetime = 0.0f;
		pParticle->m_flDieTime = flDietime;
		pParticle->m_vecVelocity = velocity;
		for( int j = 0; j < 3; ++j )
		{
			pParticle->m_uchColor[j] = pColor[j];
		}
		pParticle->m_uchStartAlpha	= iAlpha;
		pParticle->m_uchEndAlpha	= 0;
		pParticle->m_uchStartSize	= scale;
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize*2;
		pParticle->m_flRoll	= flRoll;
		pParticle->m_flRollDelta = flRollDelta;
	}

	return pSimple;
}

//-----------------------------------------------------------------------------
// Purpose: Smoke puffs
//-----------------------------------------------------------------------------
void FX_Smoke( const Vector &origin, const QAngle &angles, float scale, int numParticles, unsigned char *pColor, int iAlpha )
{
	VPROF_BUDGET( "FX_Smoke", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector vecVelocity;
	Vector vecForward;

	// Smoke
	for ( int i = 0; i < numParticles; i++ )
	{
		// Velocity
		AngleVectors( angles, &vecForward );
		vecVelocity.Random( -0.5f, 0.5f );
		vecVelocity += vecForward;
		VectorNormalize( vecVelocity );
		vecVelocity	*= random->RandomFloat( 16.0f, 32.0f );
		vecVelocity[2] += random->RandomFloat( 4.0f, 16.0f );

		// Color
		unsigned char particlecolor[3];
		if ( !pColor )
		{
			int color = random->RandomInt( 64, 164 );
			particlecolor[0] = color;
			particlecolor[1] = color;
			particlecolor[2] = color;
		}
		else
		{
			particlecolor[0] = pColor[0];
			particlecolor[1] = pColor[1];
			particlecolor[2] = pColor[2];
		}

		// Alpha
		int alpha = iAlpha;
		if ( alpha == -1 )
		{
			alpha = random->RandomInt( 10, 25 );
		}

		// Scale
		int iSize = random->RandomInt( 4, 8 ) * scale;

		// Roll
		float flRoll = random->RandomInt( 0, 360 );
		float flRollDelta = random->RandomFloat( -4.0f, 4.0f );

		//pParticle->m_uchEndSize		= pParticle->m_uchStartSize*2;

		FX_Smoke( origin, vecVelocity, iSize, 1, random->RandomFloat( 0.5f, 1.0f ), particlecolor, alpha, "particle/particle_smokegrenade", flRoll, flRollDelta );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Smoke Emitter
//			This is an emitter that keeps spitting out particles for its lifetime
//			It won't create particles if it's not in view, so it's best when short lived
//-----------------------------------------------------------------------------
class CSmokeEmitter : public CSimpleEmitter
{
	typedef CSimpleEmitter BaseClass;
public:
	CSmokeEmitter( ClientEntityHandle_t hEntity, int nAttachment, const char *pDebugName ) : CSimpleEmitter( pDebugName ) 
	{
		m_hEntity = hEntity;
		m_nAttachmentIndex = nAttachment;
		m_flDeathTime = 0;
		m_flLastParticleSpawnTime = 0;
	}
	
	// Create
	static CSmokeEmitter *Create( ClientEntityHandle_t hEntity, int nAttachment, const char *pDebugName="smoke" )
	{
		return new CSmokeEmitter( hEntity, nAttachment, pDebugName );
	}

	void SetLifeTime( float flTime )
	{
		m_flDeathTime = gpGlobals->curtime + flTime;
	}

	void SetSpurtAngle( QAngle &vecAngles )
	{
		AngleVectors( vecAngles, &m_vecSpurtForward );
	}

	void SetSpurtColor( const Vector4D &pColor )
	{
		for ( int i = 0; i <= 3; i++ )
		{
			m_SpurtColor[i] = pColor[i];
		}
	}

	void SetSpawnRate( float flRate )
	{
		m_flSpawnRate = flRate;
	}

	void CreateSpurtParticles( void )
	{
		SimpleParticle *pParticle;
		// PMaterialHandle hMaterial = GetPMaterial( "particle/particle_smokegrenade" );

		Vector vecOrigin = m_vSortOrigin;
		IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle(m_hEntity);
		if ( pRenderable && m_nAttachmentIndex )
		{
			QAngle tmp;
			pRenderable->GetAttachment( m_nAttachmentIndex, vecOrigin, tmp );
			SetSortOrigin( vecOrigin );
		}

		// Smoke
		int numParticles = RandomInt( 1,2 );
		for ( int i = 0; i < numParticles; i++ )
		{
			pParticle = (SimpleParticle *) AddParticle( sizeof( SimpleParticle ), g_Mat_DustPuff[0], vecOrigin );			
			if ( pParticle == NULL )
				break;

			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime = RandomFloat( 0.5, 1.0 );

			// Random velocity around the angles forward
			Vector vecVelocity;
			vecVelocity.Random( -0.1f, 0.1f );
			vecVelocity += m_vecSpurtForward;
			VectorNormalize( vecVelocity );
			vecVelocity	*= RandomFloat( 160.0f, 640.0f );
			pParticle->m_vecVelocity = vecVelocity;

			// Randomize the color a little
			int color[3][2];
			for( int j = 0; j < 3; ++j )
			{
				color[j][0] = MAX( 0, m_SpurtColor[j] - 64 );
				color[j][1] = MIN( 255, m_SpurtColor[j] + 64 );
			}
			pParticle->m_uchColor[0] = random->RandomInt( color[0][0], color[0][1] );
			pParticle->m_uchColor[1] = random->RandomInt( color[1][0], color[1][1] );
			pParticle->m_uchColor[2] = random->RandomInt( color[2][0], color[2][1] );

			pParticle->m_uchStartAlpha = m_SpurtColor[3];
			pParticle->m_uchEndAlpha = 0;
			pParticle->m_uchStartSize = RandomInt( 50, 60 );
			pParticle->m_uchEndSize = pParticle->m_uchStartSize*3;
			pParticle->m_flRoll	= RandomFloat( 0, 360 );
			pParticle->m_flRollDelta = RandomFloat( -4.0f, 4.0f );
		}			

		m_flLastParticleSpawnTime = gpGlobals->curtime + m_flSpawnRate;
	}

	virtual void SimulateParticles( CParticleSimulateIterator *pIterator )
	{
		Particle *pParticle = pIterator->GetFirst();
		while ( pParticle )
		{
			// If our lifetime isn't up, create more particles
			if ( m_flDeathTime > gpGlobals->curtime )
			{
				if ( m_flLastParticleSpawnTime <= gpGlobals->curtime )
				{
					CreateSpurtParticles();
				}
			}
			
			pParticle = pIterator->GetNext();
		}

		BaseClass::SimulateParticles( pIterator );
	}


private:
	float		m_flDeathTime;
	float		m_flLastParticleSpawnTime;
	float		m_flSpawnRate;
	Vector		m_vecSpurtForward;
	Vector4D	m_SpurtColor;
	ClientEntityHandle_t m_hEntity;
	int			m_nAttachmentIndex;

	CSmokeEmitter( const CSmokeEmitter & ); // not defined, not accessible
};

//-----------------------------------------------------------------------------
// Purpose: Small hose gas spurt
//-----------------------------------------------------------------------------
void FX_BuildSmoke( Vector &vecOrigin, QAngle &vecAngles, ClientEntityHandle_t hEntity, int nAttachment, float flLifeTime, const Vector4D &pColor )
{
	CSmartPtr<CSmokeEmitter> pSimple = CSmokeEmitter::Create( hEntity, nAttachment, "FX_Smoke" );
	pSimple->SetSortOrigin( vecOrigin );
	pSimple->SetLifeTime( flLifeTime );
	pSimple->SetSpurtAngle( vecAngles );
	pSimple->SetSpurtColor( pColor );
	pSimple->SetSpawnRate( 0.03 );
	pSimple->CreateSpurtParticles();
}

//-----------------------------------------------------------------------------
// Purpose: Green hose gas spurt
//-----------------------------------------------------------------------------
void SmokeCallback( const CEffectData &data )
{
	Vector vecOrigin = data.m_vOrigin;
	QAngle vecAngles = data.m_vAngles;

	Vector4D color( 50,50,50,255 );
	FX_BuildSmoke( vecOrigin, vecAngles, data.m_hEntity, data.m_nAttachmentIndex, 100.0, color ); 
}

DECLARE_CLIENT_EFFECT( "Smoke", SmokeCallback );


//-----------------------------------------------------------------------------
// Purpose: Shockwave for gunship bullet impacts!
// Input  : &pos - 
//
// NOTES:	-Don't draw this effect when the viewer is very far away.
//-----------------------------------------------------------------------------
void FX_GunshipImpact( const Vector &pos, const Vector &normal, float r, float g, float b )
{
	VPROF_BUDGET( "FX_GunshipImpact", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	if ( CImpactOverlay *pOverlay = new CImpactOverlay )
	{
		pOverlay->m_flLifetime	= 0;
		VectorMA( pos, 1.0f, normal, pOverlay->m_vPos ); // Doesn't show up on terrain if you don't do this(sjb)
		pOverlay->m_nSprites	= 1;
		
		pOverlay->m_vBaseColors[0].Init( r, g, b );

		pOverlay->m_Sprites[0].m_flHorzSize = 0.01f;
		pOverlay->m_Sprites[0].m_flVertSize = 0.01f;

		pOverlay->Activate();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : data - 
//-----------------------------------------------------------------------------
void GunshipImpactCallback( const CEffectData & data )
{
	Vector vecPosition;

	vecPosition = data.m_vOrigin;

	FX_GunshipImpact( vecPosition, Vector( 0, 0, 1 ), 100, 0, 200 );
}
DECLARE_CLIENT_EFFECT( "GunshipImpact", GunshipImpactCallback );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CommandPointerCallback( const CEffectData & data )
{
	int size = COLOR_TABLE_SIZE( commandercolors );

	for( int i = 0 ; i < size ; i++ )
	{
		if( commandercolors[ i ].index == data.m_nColor )
		{
			FX_GunshipImpact( data.m_vOrigin, Vector( 0, 0, 1 ), commandercolors[ i ].r, commandercolors[ i ].g, commandercolors[ i ].b );
			return;
		}
	}
}

DECLARE_CLIENT_EFFECT( "CommandPointer", CommandPointerCallback );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FX_GunshipMuzzleEffect( const Vector &origin, const QAngle &angles, float scale, ClientEntityHandle_t hEntity, unsigned char *pFlashColor )
{
	VPROF_BUDGET( "FX_GunshipMuzzleEffect", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "MuzzleFlash" );
	pSimple->SetSortOrigin( origin );
	
	SimpleParticle *pParticle;
	Vector			forward, offset;

	AngleVectors( angles, &forward );

	//
	// Flash
	//
	offset = origin;

	pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "effects/gunshipmuzzle" ), offset );
		
	if ( pParticle == NULL )
		return;

	pParticle->m_flLifetime		= 0.0f;
	pParticle->m_flDieTime		= 0.15f;

	pParticle->m_vecVelocity.Init();

	pParticle->m_uchStartSize	= random->RandomFloat( 40.0, 50.0 );
	pParticle->m_uchEndSize		= pParticle->m_uchStartSize;

	pParticle->m_flRoll			= random->RandomInt( 0, 360 );
	pParticle->m_flRollDelta	= 0.15f;

	pParticle->m_uchColor[0]	= 255;
	pParticle->m_uchColor[1]	= 255;
	pParticle->m_uchColor[2]	= 255;

	pParticle->m_uchStartAlpha	= 255;
	pParticle->m_uchEndAlpha	= 255;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : start - 
//			end - 
//			velocity - 
//			makeWhiz - 
//-----------------------------------------------------------------------------
void FX_GunshipTracer( Vector& start, Vector& end, int velocity, bool makeWhiz )
{
	VPROF_BUDGET( "FX_GunshipTracer", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	vNear, dStart, dEnd, shotDir;
	float	totalDist;

	//Get out shot direction and length
	VectorSubtract( end, start, shotDir );
	totalDist = VectorNormalize( shotDir );

	//Don't make small tracers
	if ( totalDist <= 256 )
		return;

	float length = random->RandomFloat( 128.0f, 256.0f );
	float life = ( totalDist + length ) / velocity;	//NOTENOTE: We want the tail to finish its run as well
	
	//Add it
	FX_AddDiscreetLine( start, shotDir, velocity, length, totalDist, 5.0f, life, "effects/gunshiptracer" );

	if( makeWhiz )
	{
		FX_TracerSound( start, end, TRACER_TYPE_GUNSHIP );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void FX_StriderMuzzleEffect( const Vector &origin, const QAngle &angles, float scale, ClientEntityHandle_t hEntity, unsigned char *pFlashColor )
{
	Vector vecDir;
	AngleVectors( angles, &vecDir );

	float life = 0.3f;
	float speed = 100.0f;

	for( int i = 0 ; i < 5 ; i++ )
	{
		FX_AddDiscreetLine( origin, vecDir, speed, 32, speed * life, 5.0f, life, "effects/bluespark" );
		speed *= 1.5f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : start - 
//			end - 
//			velocity - 
//			makeWhiz - 
//-----------------------------------------------------------------------------
void FX_StriderTracer( Vector& start, Vector& end, int velocity, bool makeWhiz )
{
	VPROF_BUDGET( "FX_StriderTracer", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	vNear, dStart, dEnd, shotDir;
	float	totalDist;

	//Get out shot direction and length
	VectorSubtract( end, start, shotDir );
	totalDist = VectorNormalize( shotDir );

	//Don't make small tracers
	if ( totalDist <= 256 )
		return;

	float length = random->RandomFloat( 64.0f, 128.0f );
	float life = ( totalDist + length ) / velocity;	//NOTENOTE: We want the tail to finish its run as well
	
	//Add it
	FX_AddDiscreetLine( start, shotDir, velocity, length, totalDist, 2.5f, life, "effects/gunshiptracer" );

	if( makeWhiz )
	{
		FX_TracerSound( start, end, TRACER_TYPE_STRIDER );
	}
}

	
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : start - 
//			end - 
//			velocity - 
//			makeWhiz - 
//-----------------------------------------------------------------------------
void FX_HunterTracer( Vector& start, Vector& end, int velocity, bool makeWhiz )
{
	VPROF_BUDGET( "FX_HunterTracer", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	vNear, dStart, dEnd, shotDir;
	float	totalDist;

	// Get out shot direction and length
	VectorSubtract( end, start, shotDir );
	totalDist = VectorNormalize( shotDir );

	// Make short tracers in close quarters
	// float flMinLength = MIN( totalDist, 128.0f );
	// float flMaxLength = MIN( totalDist, 128.0f );

	float length = 128.0f;//random->RandomFloat( flMinLength, flMaxLength );
	float life = ( totalDist + length ) / velocity;	// NOTENOTE: We want the tail to finish its run as well
	
	// Add it
	FX_AddDiscreetLine( start, shotDir, velocity*0.5f, length, totalDist, 2.0f, life, "effects/huntertracer" );

	if( makeWhiz ) 
	{
		FX_TracerSound( start, end, TRACER_TYPE_STRIDER );
	}
}

	
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : start - 
//			end - 
//			velocity - 
//			makeWhiz - 
//-----------------------------------------------------------------------------
void FX_GaussTracer( Vector& start, Vector& end, int velocity, bool makeWhiz )
{
	VPROF_BUDGET( "FX_GaussTracer", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	vNear, dStart, dEnd, shotDir;
	float	totalDist;

	//Get out shot direction and length
	VectorSubtract( end, start, shotDir );
	totalDist = VectorNormalize( shotDir );

	//Don't make small tracers
	if ( totalDist <= 256 )
		return;

	float length = random->RandomFloat( 250.0f, 500.0f );
	float life = ( totalDist + length ) / velocity;	//NOTENOTE: We want the tail to finish its run as well
	
	//Add it
	FX_AddDiscreetLine( start, shotDir, velocity, length, totalDist, random->RandomFloat( 5.0f, 8.0f ), life, "effects/spark" );
}


//-----------------------------------------------------------------------------
// Purpose: Create a tesla effect between two points
//-----------------------------------------------------------------------------
void FX_BuildTesla( 
	C_BaseEntity *pEntity, 
	const Vector &vecOrigin, 
	const Vector &vecEnd,
	const char *pModelName,
	float flBeamWidth,
	const Vector &vColor,
	int nFlags,
	float flTimeVisible )
{
	BeamInfo_t beamInfo;
	beamInfo.m_nType = TE_BEAMTESLA;
	beamInfo.m_vecStart = vecOrigin;
	beamInfo.m_vecEnd = vecEnd;
	beamInfo.m_pszModelName = pModelName;
	beamInfo.m_flHaloScale = 0.0;
	beamInfo.m_flLife = flTimeVisible;
	beamInfo.m_flWidth = flBeamWidth;
	beamInfo.m_flEndWidth = 1;
	beamInfo.m_flFadeLength = 0.3;
	beamInfo.m_flAmplitude = 16;
	beamInfo.m_flBrightness = 200.0;
	beamInfo.m_flSpeed = 0.0;
	beamInfo.m_nStartFrame = 0.0;
	beamInfo.m_flFrameRate = 1.0;
	beamInfo.m_flRed = vColor.x * 255.0;
	beamInfo.m_flGreen = vColor.y * 255.0;
	beamInfo.m_flBlue = vColor.z * 255.0;
	beamInfo.m_nSegments = 20;
	beamInfo.m_bRenderable = true;
	beamInfo.m_nFlags = nFlags;
	
	beams->CreateBeamPoints( beamInfo );
}

void FX_Tesla( const CTeslaInfo &teslaInfo )
{
	C_BaseEntity *pEntity = ClientEntityList().GetEnt( teslaInfo.m_nEntIndex );

	// Send out beams around us
	int iNumBeamsAround = (teslaInfo.m_nBeams * 2) / 3; // (2/3 of the beams are placed around in a circle)
	int iNumRandomBeams = teslaInfo.m_nBeams - iNumBeamsAround;
	int iTotalBeams = iNumBeamsAround + iNumRandomBeams;
	float flYawOffset = RandomFloat(0,360);
	for ( int i = 0; i < iTotalBeams; i++ )
	{
		// Make a couple of tries at it
		int iTries = -1;
		Vector vecForward;
		trace_t tr;
		do
		{
			iTries++;

			// Some beams are deliberatly aimed around the point, the rest are random.
			if ( i < iNumBeamsAround )
			{
				QAngle vecTemp = teslaInfo.m_vAngles;
				vecTemp[YAW] += anglemod( flYawOffset + ((360 / iTotalBeams) * i) );
				AngleVectors( vecTemp, &vecForward );

				// Randomly angle it up or down
				vecForward.z = RandomFloat( -1, 1 );
			}
			else
			{
				vecForward = RandomVector( -1, 1 );
			}
			VectorNormalize( vecForward );

			UTIL_TraceLine( teslaInfo.m_vPos, teslaInfo.m_vPos + (vecForward * teslaInfo.m_flRadius), MASK_SHOT, pEntity, COLLISION_GROUP_NONE, &tr );
		} while ( tr.fraction >= 1.0 && iTries < 3 );

		Vector vecEnd = tr.endpos - (vecForward * 8);

		// Only spark & glow if we hit something
		if ( tr.fraction < 1.0 )
		{
			if ( !EffectOccluded( tr.endpos, 0 ) )
			{
				// Move it towards the camera
				Vector vecFlash = tr.endpos;
				AngleVectors( MainViewAngles(), &vecForward );
				vecFlash -= (vecForward * 8);

				g_pEffects->EnergySplash( vecFlash, -vecForward, false );

				// End glow
				CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "dust" );
				pSimple->SetSortOrigin( vecFlash );
				SimpleParticle *pParticle;
				pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "effects/tesla_glow_noz" ), vecFlash );
				if ( pParticle != NULL )
				{
					pParticle->m_flLifetime = 0.0f;
					pParticle->m_flDieTime	= RandomFloat( 0.5, 1 );
					pParticle->m_vecVelocity = vec3_origin;
					Vector color( 1,1,1 );
					float  colorRamp = RandomFloat( 0.75f, 1.25f );
					pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
					pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
					pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;
					pParticle->m_uchStartSize	= RandomFloat( 6,13 );
					pParticle->m_uchEndSize		= pParticle->m_uchStartSize - 2;
					pParticle->m_uchStartAlpha	= 255;
					pParticle->m_uchEndAlpha	= 10;
					pParticle->m_flRoll			= RandomFloat( 0,360 );
					pParticle->m_flRollDelta	= 0;
				}
			}
		}

		// Build the tesla
		FX_BuildTesla( pEntity, teslaInfo.m_vPos, tr.endpos, teslaInfo.m_pszSpriteName, teslaInfo.m_flBeamWidth, teslaInfo.m_vColor, FBEAM_ONLYNOISEONCE, teslaInfo.m_flTimeVisible );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Tesla effect
//-----------------------------------------------------------------------------
void BuildTeslaCallback( const CEffectData &data )
{
	if ( data.entindex() < 0 )
		return;

	CTeslaInfo teslaInfo;

	teslaInfo.m_vPos = data.m_vOrigin;
	teslaInfo.m_vAngles = data.m_vAngles;
	teslaInfo.m_nEntIndex = data.entindex();
	teslaInfo.m_flBeamWidth = 5;
	teslaInfo.m_vColor.Init( 1, 1, 1 );
	teslaInfo.m_flTimeVisible = 0.3;
	teslaInfo.m_flRadius = 192;
	teslaInfo.m_nBeams = 6;
	teslaInfo.m_pszSpriteName = "sprites/physbeam.vmt";

	FX_Tesla( teslaInfo );
}


//-----------------------------------------------------------------------------
// Purpose: Tesla hitbox
//-----------------------------------------------------------------------------
void FX_BuildTeslaHitbox( 
	C_BaseEntity *pEntity,
	int nStartAttachment,
	int nEndAttachment,
	float flBeamWidth,
	const Vector &vColor,
	float flTimeVisible )
{
	// One along the body
	BeamInfo_t beamInfo;
	beamInfo.m_nType = TE_BEAMTESLA;
	beamInfo.m_vecStart = vec3_origin;
	beamInfo.m_vecEnd = vec3_origin;
	beamInfo.m_pszModelName = "sprites/lgtning.vmt";
	beamInfo.m_flHaloScale = 8.0f;
	beamInfo.m_flLife = 0.01f;
	beamInfo.m_flWidth = random->RandomFloat( 3.0f, 6.0f );
	beamInfo.m_flEndWidth = 0.0f;
	beamInfo.m_flFadeLength = 0.0f;
	beamInfo.m_flAmplitude = random->RandomInt( 16, 32 );
	beamInfo.m_flBrightness = 255.0f;
	beamInfo.m_flSpeed = 32.0;
	beamInfo.m_nStartFrame = 0.0;
	beamInfo.m_flFrameRate = 30.0;
	beamInfo.m_flRed = vColor.x * 255.0;
	beamInfo.m_flGreen = vColor.y * 255.0;
	beamInfo.m_flBlue = vColor.z * 255.0;
	beamInfo.m_nSegments = 32;
	beamInfo.m_bRenderable = true;
	beamInfo.m_pStartEnt = beamInfo.m_pEndEnt = NULL;

	beamInfo.m_nFlags = (FBEAM_USE_HITBOXES);

	beamInfo.m_pStartEnt = pEntity;
	beamInfo.m_nStartAttachment = nStartAttachment;
	beamInfo.m_pEndEnt = pEntity;
	beamInfo.m_nEndAttachment = nEndAttachment;

	beams->CreateBeamEntPoint( beamInfo );

	// One out to the world
	trace_t	tr;
	Vector	randomDir;

	randomDir = RandomVector( -1.0f, 1.0f );
	VectorNormalize( randomDir );

	UTIL_TraceLine( pEntity->WorldSpaceCenter(), pEntity->WorldSpaceCenter() + ( randomDir * 100 ), MASK_SOLID_BRUSHONLY, pEntity, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0f )
	{
		beamInfo.m_nType = TE_BEAMTESLA;
		beamInfo.m_vecStart = vec3_origin;
		beamInfo.m_vecEnd = tr.endpos;
		beamInfo.m_pszModelName = "sprites/lgtning.vmt";
		beamInfo.m_flHaloScale = 8.0f;
		beamInfo.m_flLife = 0.05f;
		beamInfo.m_flWidth = random->RandomFloat( 2.0f, 6.0f );
		beamInfo.m_flEndWidth = 0.0f;
		beamInfo.m_flFadeLength = 0.0f;
		beamInfo.m_flAmplitude = random->RandomInt( 16, 32 );
		beamInfo.m_flBrightness = 255.0f;
		beamInfo.m_flSpeed = 32.0;
		beamInfo.m_nStartFrame = 0.0;
		beamInfo.m_flFrameRate = 30.0;
		beamInfo.m_flRed = vColor.x * 255.0;
		beamInfo.m_flGreen = vColor.y * 255.0;
		beamInfo.m_flBlue = vColor.z * 255.0;
		beamInfo.m_nSegments = 32;
		beamInfo.m_bRenderable = true;
		beamInfo.m_pStartEnt = beamInfo.m_pEndEnt = NULL;

		beamInfo.m_pStartEnt = pEntity;
		beamInfo.m_nStartAttachment = nStartAttachment;

		beams->CreateBeamEntPoint( beamInfo );
	}

	// Create an elight to illuminate the target
	if ( pEntity != NULL )
	{
		dlight_t *el = effects->CL_AllocElight( LIGHT_INDEX_TE_DYNAMIC + pEntity->entindex() );

		// Randomly place it
		el->origin	= pEntity->WorldSpaceCenter() + RandomVector( -32, 32 );

		el->color.r = 235;
		el->color.g = 235;
		el->color.b = 255;
		el->color.exponent = 4;

		el->radius	= random->RandomInt( 32, 128 );
		el->decay	= el->radius / 0.1f;
		el->die		= gpGlobals->curtime + 0.1f;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Tesla effect
//-----------------------------------------------------------------------------
void FX_BuildTeslaHitbox( const CEffectData &data )
{
	Vector vColor( 1, 1, 1 );

	C_BaseEntity *pEntity = ClientEntityList().GetEnt( data.entindex() );
	C_BaseAnimating *pAnimating = pEntity ? pEntity->GetBaseAnimating() : NULL;
	if (!pAnimating)
		return;

	studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( pAnimating->GetModel() );
	if (!pStudioHdr)
		return;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pAnimating->GetHitboxSet() );
	if ( !set )
		return;

	matrix3x4_t	*hitboxbones[MAXSTUDIOBONES];
	if ( !pAnimating->HitboxToWorldTransforms( hitboxbones ) )
		return;

	int nBeamCount = (int)(data.m_flMagnitude + 0.5f);
	for ( int i = 0; i < nBeamCount; ++i )
	{
		int nStartHitBox = random->RandomInt( 1, set->numhitboxes );
		int nEndHitBox = random->RandomInt( 1, set->numhitboxes );
		FX_BuildTeslaHitbox( pEntity, nStartHitBox, nEndHitBox, data.m_flScale, vColor, random->RandomFloat( 0.05f, 0.2f ) );
	}
}

DECLARE_CLIENT_EFFECT( "TeslaHitboxes", FX_BuildTeslaHitbox );


//-----------------------------------------------------------------------------
// Purpose: Tesla effect
//-----------------------------------------------------------------------------
void FX_BuildTeslaZap( const CEffectData &data )
{
	// Build the tesla, only works on entities
	C_BaseEntity *pEntity = data.GetEntity();
	if ( !pEntity )
		return;

	Vector vColor( 1, 1, 1 );

	BeamInfo_t beamInfo;
	beamInfo.m_nType = TE_BEAMTESLA;
	beamInfo.m_pStartEnt = pEntity;
	beamInfo.m_nStartAttachment = data.m_nAttachmentIndex;
	beamInfo.m_pEndEnt = NULL;
	beamInfo.m_vecEnd = data.m_vOrigin;
	beamInfo.m_pszModelName = "sprites/physbeam.vmt";
	beamInfo.m_flHaloScale = 0.0;
	beamInfo.m_flLife = 0.3f;
	beamInfo.m_flWidth = data.m_flScale;
	beamInfo.m_flEndWidth = 1;
	beamInfo.m_flFadeLength = 0.3;
	beamInfo.m_flAmplitude = 16;
	beamInfo.m_flBrightness = 200.0;
	beamInfo.m_flSpeed = 0.0;
	beamInfo.m_nStartFrame = 0.0;
	beamInfo.m_flFrameRate = 1.0;
	beamInfo.m_flRed = vColor.x * 255.0;
	beamInfo.m_flGreen = vColor.y * 255.0;
	beamInfo.m_flBlue = vColor.z * 255.0;
	beamInfo.m_nSegments = 20;
	beamInfo.m_bRenderable = true;
	beamInfo.m_nFlags = 0;
	
	beams->CreateBeamEntPoint( beamInfo );
}

DECLARE_CLIENT_EFFECT( "TeslaZap", FX_BuildTeslaZap );

