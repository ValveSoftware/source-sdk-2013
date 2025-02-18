//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================//
#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "physpropclientside.h"
#include "tf_shareddefs.h"
#include "engine/ivdebugoverlay.h"

//-----------------------------------------------------------------------------
// Purpose: TF Eject Brass
//-----------------------------------------------------------------------------
void TF_ThrowCigaretteCallback( const CEffectData &data )
{
	C_BaseEntity *pEntity = ClientEntityList().GetEnt( data.entindex() );

	if ( !pEntity )
		return;

	int iTeam = pEntity->GetTeamNumber();

	Vector vForward, vRight, vUp;
	AngleVectors( data.m_vAngles, &vForward, &vRight, &vUp );

	QAngle vecShellAngles;
	VectorAngles( -vUp, vecShellAngles );

	Vector vecVelocity = 180 * vForward +
		random->RandomFloat( -20, 20 ) * vRight +
		random->RandomFloat( 0, 20 ) * vUp;

	vecVelocity.z += 100;

	float flLifeTime = 10.0f;

	const char *pszCigaretteModel = "models/weapons/shells/shell_cigarrette.mdl";	// sic

	model_t *pModel = (model_t *)engine->LoadModel( pszCigaretteModel );
	if ( !pModel )
		return;

	int flags = FTENT_FADEOUT | FTENT_GRAVITY | FTENT_COLLIDEALL | FTENT_ROTATE;

	Assert( pModel );	

	C_LocalTempEntity *pTemp = tempents->SpawnTempModel( pModel, data.m_vOrigin, vecShellAngles, vecVelocity, flLifeTime, FTENT_NEVERDIE );
	if ( pTemp == NULL )
		return;

	pTemp->m_nSkin = ( iTeam == TF_TEAM_RED ) ? 0 : 1;

	pTemp->m_vecTempEntAngVelocity[0] = random->RandomFloat(-512,511);
	pTemp->m_vecTempEntAngVelocity[1] = random->RandomFloat(-255,255);
	pTemp->m_vecTempEntAngVelocity[2] = random->RandomFloat(-255,255);

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

DECLARE_CLIENT_EFFECT( "TF_ThrowCigarette", TF_ThrowCigaretteCallback );

const char *szModelsStr[] = {
	"models/player/gibs/soldiergib007.mdl",
	"models/player/gibs/soldiergib008.mdl"
};

ConVar tf_head_throw_fwd_speed( "tf_head_throw_fwd_speed", "1800", FCVAR_DEVELOPMENTONLY );
ConVar tf_head_throw_side_speed( "tf_head_throw_side_speed", "18", FCVAR_DEVELOPMENTONLY );
ConVar tf_head_throw_up_speed( "tf_head_throw_up_speed", "20", FCVAR_DEVELOPMENTONLY );
ConVar tf_head_throw_vertical_speed( "tf_head_throw_vertical_speed", "800", FCVAR_DEVELOPMENTONLY );
ConVar tf_head_throw_lifetime( "tf_head_throw_lifetime", "20", FCVAR_DEVELOPMENTONLY );
ConVar tf_head_throw_ang_pitch_range( "tf_head_throw_ang_pitch_range", "512", FCVAR_DEVELOPMENTONLY );
ConVar tf_head_throw_ang_yaw_range( "tf_head_throw_ang_yaw_range", "256", FCVAR_DEVELOPMENTONLY );
ConVar tf_head_throw_ang_roll_range( "tf_head_throw_ang_roll_range", "256", FCVAR_DEVELOPMENTONLY );
ConVar tf_head_throw_x_offset( "tf_head_throw_x_offset", "0", FCVAR_DEVELOPMENTONLY );
ConVar tf_head_throw_y_offset( "tf_head_throw_y_offset", "0", FCVAR_DEVELOPMENTONLY );
ConVar tf_head_throw_z_offset( "tf_head_throw_z_offset", "-72", FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TF_ThrowHeadCallback( const CEffectData &data )
{
	C_BaseEntity *pEntity = ClientEntityList().GetEnt( data.entindex() );

	if ( !pEntity )
		return;

	int iTeam = pEntity->GetTeamNumber();

	Vector vForward, vRight, vUp;
	AngleVectors( data.m_vAngles, &vForward, &vRight, &vUp );

	// misyl: nnononononononononononono
	// this model is not with the head at 0 0 0, its at like 0 0 72 cause thats where head is
	// we want it to match anim anyway.
	//QAngle vecShellAngles;
	//VectorAngles( -vUp, vecShellAngles );

	Vector vecVelocity = tf_head_throw_fwd_speed.GetFloat() * vForward +
		random->RandomFloat( -tf_head_throw_side_speed.GetFloat(), tf_head_throw_side_speed.GetFloat() ) * vRight +
		random->RandomFloat( 0, tf_head_throw_up_speed.GetFloat() ) * vUp;

	vecVelocity.z += tf_head_throw_vertical_speed.GetFloat();

//	if ( developer.GetBool() )
//	{
//		debugoverlay->AddBoxOverlay( data.m_vOrigin, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ), vec3_angle, 255, 255, 255, 127, 10.0f );
//	}

	float flLifeTime = tf_head_throw_lifetime.GetFloat();

	for ( int i = 0; i < ARRAYSIZE( szModelsStr ); i++ )
	{
		model_t* pModel = ( model_t* )engine->LoadModel( szModelsStr[ i ] );
		if ( !pModel )
			return;

		Vector vOrigin = data.m_vOrigin +
			Vector( tf_head_throw_x_offset.GetFloat(), tf_head_throw_y_offset.GetFloat(), tf_head_throw_z_offset.GetFloat() );
		int nSkin = ( iTeam == TF_TEAM_RED ) ? 1 : 0;
		C_PhysPropClientside *pProp = tempents->PhysicsProp( pModel, nSkin, vOrigin, vec3_angle, vecVelocity, 0, 0 );
		if ( !pProp )
			continue;

		AngularImpulse vAngVelocity = AngularImpulse(
			random->RandomFloat( -tf_head_throw_ang_pitch_range.GetFloat(), tf_head_throw_ang_pitch_range.GetFloat() ),
			random->RandomFloat( -tf_head_throw_ang_yaw_range.GetFloat(), tf_head_throw_ang_yaw_range.GetFloat() ),
			random->RandomFloat( -tf_head_throw_ang_roll_range.GetFloat(), tf_head_throw_ang_roll_range.GetFloat() ) );
		pProp->ApplyLocalAngularVelocityImpulse( vAngVelocity );

		pProp->StartFadeOut( flLifeTime );
		pProp->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	}
}

DECLARE_CLIENT_EFFECT( "TF_ThrowHead", TF_ThrowHeadCallback );