//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Stun Stick- beating stick with a zappy end
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "weapon_hl2mpbasebasebludgeon.h"
#include "IEffects.h"
#include "debugoverlay_shared.h"

#ifndef CLIENT_DLL
	#include "npc_metropolice.h"
	#include "te_effect_dispatch.h"
#endif

#ifdef CLIENT_DLL
	
	#include "iviewrender_beams.h"
	#include "beam_shared.h"
	#include "materialsystem/imaterial.h"
	#include "model_types.h"
	#include "c_te_effect_dispatch.h"
	#include "fx_quad.h"
	#include "fx.h"

	extern void DrawHalo( IMaterial* pMaterial, const Vector &source, float scale, float const *color, float flHDRColorScale );
	extern void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar metropolice_move_and_melee;

#define	STUNSTICK_RANGE				75.0f
#define	STUNSTICK_REFIRE			0.8f
#define	STUNSTICK_BEAM_MATERIAL		"sprites/lgtning.vmt"
#define STUNSTICK_GLOW_MATERIAL		"sprites/light_glow02_add"
#define STUNSTICK_GLOW_MATERIAL2	"effects/blueflare1"
#define STUNSTICK_GLOW_MATERIAL_NOZ	"sprites/light_glow02_add_noz"

#ifdef CLIENT_DLL
#define CWeaponStunStick C_WeaponStunStick
#endif

class CWeaponStunStick : public CBaseHL2MPBludgeonWeapon
{
	DECLARE_CLASS( CWeaponStunStick, CBaseHL2MPBludgeonWeapon );
	
public:

	CWeaponStunStick();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

#ifdef CLIENT_DLL
	virtual int				DrawModel( int flags );
	virtual void			ClientThink( void );
	virtual void			OnDataChanged( DataUpdateType_t updateType );
	virtual RenderGroup_t	GetRenderGroup( void );
	virtual void			ViewModelDrawn( C_BaseViewModel *pBaseViewModel );
	
#endif

	virtual void Precache();

	void		Spawn();

	float		GetRange( void )		{ return STUNSTICK_RANGE; }
	float		GetFireRate( void )		{ return STUNSTICK_REFIRE; }


	bool		Deploy( void );
	bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	
	void		Drop( const Vector &vecVelocity );
	void		ImpactEffect( trace_t &traceHit );
	void		SecondaryAttack( void )	{}
	void		SetStunState( bool state );
	bool		GetStunState( void );

#ifndef CLIENT_DLL
	void		Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	int			WeaponMeleeAttack1Condition( float flDot, float flDist );
#endif
	
	float		GetDamageForActivity( Activity hitActivity );

	virtual bool	PlayFleshyHittySoundOnHit() const { return true; }

	CWeaponStunStick( const CWeaponStunStick & );

private:

#ifdef CLIENT_DLL

	#define	NUM_BEAM_ATTACHMENTS	9

	struct stunstickBeamInfo_t
	{
		int IDs[2];		// 0 - top, 1 - bottom
	};

	stunstickBeamInfo_t		m_BeamAttachments[NUM_BEAM_ATTACHMENTS];	// Lookup for arc attachment points on the head of the stick
	int						m_BeamCenterAttachment;						// "Core" of the effect (center of the head)

	void	SetupAttachmentPoints( void );
	void	DrawFirstPersonEffects( void );
	void	DrawThirdPersonEffects( void );
	void	DrawEffects( void );
	bool	InSwing( void );

	bool	m_bSwungLastFrame;

	#define	FADE_DURATION	0.25f

	float	m_flFadeTime;

#endif

	CNetworkVar( bool, m_bActive );
};

//-----------------------------------------------------------------------------
// CWeaponStunStick
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( WeaponStunStick, DT_WeaponStunStick )

BEGIN_NETWORK_TABLE( CWeaponStunStick, DT_WeaponStunStick )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_bActive ) ),
#else
	SendPropInt( SENDINFO( m_bActive ), 1, SPROP_UNSIGNED ),
#endif

END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponStunStick )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_stunstick, CWeaponStunStick );
PRECACHE_WEAPON_REGISTER( weapon_stunstick );


#ifndef CLIENT_DLL

acttable_t	CWeaponStunStick::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_MELEE,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_MELEE,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_MELEE,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_MELEE,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_MELEE,					false },
};

IMPLEMENT_ACTTABLE(CWeaponStunStick);

#endif


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponStunStick::CWeaponStunStick( void )
{
	// HACK:  Don't call SetStunState because this tried to Emit a sound before
	//  any players are connected which is a bug
	m_bActive = false;

#ifdef CLIENT_DLL
	m_bSwungLastFrame = false;
	m_flFadeTime = FADE_DURATION;	// Start off past the fade point
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponStunStick::Spawn()
{
	Precache();

	BaseClass::Spawn();
	AddSolidFlags( FSOLID_NOT_STANDABLE );
}

void CWeaponStunStick::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "Weapon_StunStick.Activate" );
	PrecacheScriptSound( "Weapon_StunStick.Deactivate" );

	PrecacheModel( STUNSTICK_BEAM_MATERIAL );
	PrecacheModel( "sprites/light_glow02_add.vmt" );
	PrecacheModel( "effects/blueflare1.vmt" );
	PrecacheModel( "sprites/light_glow02_add_noz.vmt" );
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponStunStick::GetDamageForActivity( Activity hitActivity )
{
	return 40.0f;
}

//-----------------------------------------------------------------------------
// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
//-----------------------------------------------------------------------------
extern ConVar sk_crowbar_lead_time;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponStunStick::ImpactEffect( trace_t &traceHit )
{
	
//#ifndef CLIENT_DLL
	
	CEffectData	data;

	data.m_vNormal = traceHit.plane.normal;
	data.m_vOrigin = traceHit.endpos + ( data.m_vNormal * 4.0f );

	DispatchEffect( "StunstickImpact", data );

//#endif

	//FIXME: need new decals
	UTIL_ImpactTrace( &traceHit, DMG_CLUB );
}

#ifndef CLIENT_DLL


int CWeaponStunStick::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
	CAI_BaseNPC *pNPC	= GetOwner()->MyNPCPointer();
	CBaseEntity *pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	AngularImpulse angVelocity;
	pEnemy->GetVelocity( &vecVelocity, &angVelocity );

	// Project where the enemy will be in a little while, add some randomness so he doesn't always hit
	float dt = sk_crowbar_lead_time.GetFloat();
	dt += random->RandomFloat( -0.3f, 0.2f );
	if ( dt < 0.0f )
		dt = 0.0f;

	Vector vecExtrapolatedPos;
	VectorMA( pEnemy->WorldSpaceCenter(), dt, vecVelocity, vecExtrapolatedPos );

	Vector vecDelta;
	VectorSubtract( vecExtrapolatedPos, pNPC->WorldSpaceCenter(), vecDelta );

	if ( fabs( vecDelta.z ) > 70 )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	Vector vecForward = pNPC->BodyDirection2D( );
	vecDelta.z = 0.0f;
	float flExtrapolatedDot = DotProduct2D( vecDelta.AsVector2D(), vecForward.AsVector2D() );
	if ((flDot < 0.7) && (flExtrapolatedDot < 0.7))
	{
		return COND_NOT_FACING_ATTACK;
	}

	float flExtrapolatedDist = Vector2DNormalize( vecDelta.AsVector2D() );

	if( pEnemy->IsPlayer() )
	{
		//Vector vecDir = pEnemy->GetSmoothedVelocity();
		//float flSpeed = VectorNormalize( vecDir );

		// If player will be in front of me in one-half second, clock his arse.
		Vector vecProjectEnemy = pEnemy->GetAbsOrigin() + (pEnemy->GetAbsVelocity() * 0.35);
		Vector vecProjectMe = GetAbsOrigin();

		if( (vecProjectMe - vecProjectEnemy).Length2D() <= 48.0f )
		{
			return COND_CAN_MELEE_ATTACK1;
		}
	}
/*
	if( metropolice_move_and_melee.GetBool() )
	{
		if( pNPC->IsMoving() )
		{
			flTargetDist *= 1.5f;
		}
	}
*/
	float flTargetDist = 48.0f;
	if ((flDist > flTargetDist) && (flExtrapolatedDist > flTargetDist))
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}


void CWeaponStunStick::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_MELEE_HIT:
		{
			// Trace up or down based on where the enemy is...
			// But only if we're basically facing that direction
			Vector vecDirection;
			AngleVectors( GetAbsAngles(), &vecDirection );

			CBaseEntity *pEnemy = pOperator->MyNPCPointer() ? pOperator->MyNPCPointer()->GetEnemy() : NULL;
			if ( pEnemy )
			{
				Vector vecDelta;
				VectorSubtract( pEnemy->WorldSpaceCenter(), pOperator->Weapon_ShootPosition(), vecDelta );
				VectorNormalize( vecDelta );
				
				Vector2D vecDelta2D = vecDelta.AsVector2D();
				Vector2DNormalize( vecDelta2D );
				if ( DotProduct2D( vecDelta2D, vecDirection.AsVector2D() ) > 0.8f )
				{
					vecDirection = vecDelta;
				}
			}

			Vector vecEnd;
			VectorMA( pOperator->Weapon_ShootPosition(), 32, vecDirection, vecEnd );
			// Stretch the swing box down to catch low level physics objects
			CBaseEntity *pHurt = pOperator->CheckTraceHullAttack( pOperator->Weapon_ShootPosition(), vecEnd, 
				Vector(-16,-16,-40), Vector(16,16,16), GetDamageForActivity( GetActivity() ), DMG_CLUB, 0.5f, false );
			
			// did I hit someone?
			if ( pHurt )
			{
				// play sound
				WeaponSound( MELEE_HIT );

				CBasePlayer *pPlayer = ToBasePlayer( pHurt );

				bool bFlashed = false;
				
				// Punch angles
				if ( pPlayer != NULL && !(pPlayer->GetFlags() & FL_GODMODE) )
				{
					float yawKick = random->RandomFloat( -48, -24 );

					//Kick the player angles
					pPlayer->ViewPunch( QAngle( -16, yawKick, 2 ) );

					Vector	dir = pHurt->GetAbsOrigin() - GetAbsOrigin();

					// If the player's on my head, don't knock him up
					if ( pPlayer->GetGroundEntity() == pOperator )
					{
						dir = vecDirection;
						dir.z = 0;
					}

					VectorNormalize(dir);

					dir *= 500.0f;

					//If not on ground, then don't make them fly!
					if ( !(pPlayer->GetFlags() & FL_ONGROUND ) )
						 dir.z = 0.0f;

					//Push the target back
					pHurt->ApplyAbsVelocityImpulse( dir );

					if ( !bFlashed )
					{
						color32 red = {128,0,0,128};
						UTIL_ScreenFade( pPlayer, red, 0.5f, 0.1f, FFADE_IN );
					}
					
					// Force the player to drop anyting they were holding
					pPlayer->ForceDropOfCarriedPhysObjects();
				}
				
				// do effect?
			}
			else
			{
				WeaponSound( MELEE_MISS );
			}
		}
		break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Sets the state of the stun stick
//-----------------------------------------------------------------------------
void CWeaponStunStick::SetStunState( bool state )
{
	m_bActive = state;

	if ( m_bActive )
	{
		//FIXME: START - Move to client-side

		Vector vecAttachment;
		QAngle vecAttachmentAngles;

		GetAttachment( 1, vecAttachment, vecAttachmentAngles );
		g_pEffects->Sparks( vecAttachment );

		//FIXME: END - Move to client-side

		EmitSound( "Weapon_StunStick.Activate" );
	}
	else
	{
		EmitSound( "Weapon_StunStick.Deactivate" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponStunStick::Deploy( void )
{
	SetStunState( true );

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponStunStick::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( BaseClass::Holster( pSwitchingTo ) == false )
		return false;

	SetStunState( false );
	SetWeaponVisible( false );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecVelocity - 
//-----------------------------------------------------------------------------
void CWeaponStunStick::Drop( const Vector &vecVelocity )
{
	SetStunState( false );

#ifndef CLIENT_DLL
	UTIL_Remove( this );
#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponStunStick::GetStunState( void )
{
	return m_bActive;
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Get the attachment point on a viewmodel that a base weapon is using
//-----------------------------------------------------------------------------
bool UTIL_GetWeaponAttachment( C_BaseCombatWeapon *pWeapon, int attachmentID, Vector &absOrigin, QAngle &absAngles )
{
	// This is already correct in third-person
	if ( pWeapon && pWeapon->ShouldDrawUsingViewModel() == false )
	{
		return pWeapon->GetAttachment( attachmentID, absOrigin, absAngles );
	}

	// Otherwise we need to translate the attachment to the viewmodel's version and reformat it
	CBasePlayer *pOwner = ToBasePlayer( pWeapon->GetOwner() );
	
	if ( pOwner != NULL )
	{
		int ret = pOwner->GetViewModel()->GetAttachment( attachmentID, absOrigin, absAngles );
		FormatViewModelAttachment( absOrigin, true );

		return ret;
	}

	// Wasn't found
	return false;
}

#define	BEAM_ATTACH_CORE_NAME	"sparkrear"

//-----------------------------------------------------------------------------
// Purpose: Sets up the attachment point lookup for the model
//-----------------------------------------------------------------------------
void C_WeaponStunStick::SetupAttachmentPoints( void )
{
	// Setup points for both types of views
	if ( ShouldDrawUsingViewModel() )
	{
		const char *szBeamAttachNamesTop[NUM_BEAM_ATTACHMENTS] =
		{
			"spark1a","spark2a","spark3a","spark4a",
			"spark5a","spark6a","spark7a","spark8a",
			"spark9a",
		};

		const char *szBeamAttachNamesBottom[NUM_BEAM_ATTACHMENTS] =
		{
			"spark1b","spark2b","spark3b","spark4b",
			"spark5b","spark6b","spark7b","spark8b",
			"spark9b",
		};
		
		// Lookup and store all connections
		for ( int i = 0; i < NUM_BEAM_ATTACHMENTS; i++ )
		{
			m_BeamAttachments[i].IDs[0] = LookupAttachment( szBeamAttachNamesTop[i] );
			m_BeamAttachments[i].IDs[1] = LookupAttachment( szBeamAttachNamesBottom[i] );
		}

		// Setup the center beam point
		m_BeamCenterAttachment = LookupAttachment( BEAM_ATTACH_CORE_NAME );
	}
	else
	{
		// Setup the center beam point
		m_BeamCenterAttachment = 1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws the stunstick model (with extra effects)
//-----------------------------------------------------------------------------
int C_WeaponStunStick::DrawModel( int flags )
{
	if ( ShouldDraw() == false )
		return 0;

	// Only render these on the transparent pass
	if ( flags & STUDIO_TRANSPARENCY )
	{
		DrawEffects();
		return 1;
	}

	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: Randomly adds extra effects
//-----------------------------------------------------------------------------
void C_WeaponStunStick::ClientThink( void )
{
	if ( InSwing() == false )
	{
		if ( m_bSwungLastFrame )
		{
			// Start fading
			m_flFadeTime = gpGlobals->curtime;
			m_bSwungLastFrame = false;
		}

		return;
	}

	// Remember if we were swinging last frame
	m_bSwungLastFrame = InSwing();

	if ( IsEffectActive( EF_NODRAW ) )
		return;

	if ( ShouldDrawUsingViewModel() )
	{
		// Update our effects
		if ( gpGlobals->frametime != 0.0f && ( random->RandomInt( 0, 3 ) == 0 ) )
		{		
			Vector	vecOrigin;
			QAngle	vecAngles;

			// Inner beams
			BeamInfo_t beamInfo;

			int attachment = random->RandomInt( 0, 15 );

			UTIL_GetWeaponAttachment( this, attachment, vecOrigin, vecAngles );
			::FormatViewModelAttachment( vecOrigin, false );

			CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
			CBaseEntity *pBeamEnt = pOwner->GetViewModel();

			beamInfo.m_vecStart = vec3_origin;
			beamInfo.m_pStartEnt= pBeamEnt;
			beamInfo.m_nStartAttachment = attachment;

			beamInfo.m_pEndEnt	= NULL;
			beamInfo.m_nEndAttachment = -1;
			beamInfo.m_vecEnd = vecOrigin + RandomVector( -8, 8 );

			beamInfo.m_pszModelName = STUNSTICK_BEAM_MATERIAL;
			beamInfo.m_flHaloScale = 0.0f;
			beamInfo.m_flLife = 0.05f;
			beamInfo.m_flWidth = random->RandomFloat( 1.0f, 2.0f );
			beamInfo.m_flEndWidth = 0;
			beamInfo.m_flFadeLength = 0.0f;
			beamInfo.m_flAmplitude = random->RandomFloat( 16, 32 );
			beamInfo.m_flBrightness = 255.0;
			beamInfo.m_flSpeed = 0.0;
			beamInfo.m_nStartFrame = 0.0;
			beamInfo.m_flFrameRate = 1.0f;
			beamInfo.m_flRed = 255.0f;;
			beamInfo.m_flGreen = 255.0f;
			beamInfo.m_flBlue = 255.0f;
			beamInfo.m_nSegments = 16;
			beamInfo.m_bRenderable = true;
			beamInfo.m_nFlags = 0;
			
			beams->CreateBeamEntPoint( beamInfo );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Starts the client-side version thinking
//-----------------------------------------------------------------------------
void C_WeaponStunStick::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		SetupAttachmentPoints();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tells us we're always a translucent entity
//-----------------------------------------------------------------------------
RenderGroup_t C_WeaponStunStick::GetRenderGroup( void )
{
	return RENDER_GROUP_TWOPASS;
}

//-----------------------------------------------------------------------------
// Purpose: Tells us we're always a translucent entity
//-----------------------------------------------------------------------------
bool C_WeaponStunStick::InSwing( void )
{
	int activity = GetActivity();

	// FIXME: This is needed until the actual animation works
	if ( ShouldDrawUsingViewModel() == false )
		return true;

	// These are the swing activities this weapon can play
	if ( activity == GetPrimaryAttackActivity() || 
		 activity == GetSecondaryAttackActivity() ||
		 activity == ACT_VM_MISSCENTER ||
		 activity == ACT_VM_MISSCENTER2 )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Draw our special effects
//-----------------------------------------------------------------------------
void C_WeaponStunStick::DrawThirdPersonEffects( void )
{
	Vector	vecOrigin;
	QAngle	vecAngles;
	float	color[3];
	float	scale;

	CMatRenderContextPtr pRenderContext( materials );
	IMaterial *pMaterial = materials->FindMaterial( STUNSTICK_GLOW_MATERIAL, NULL, false );
	pRenderContext->Bind( pMaterial );

	// Get bright when swung
	if ( InSwing() )
	{
		color[0] = color[1] = color[2] = 0.4f;
		scale = 22.0f;
	}
	else
	{
		color[0] = color[1] = color[2] = 0.1f;
		scale = 20.0f;
	}
	
	// Draw an all encompassing glow around the entire head
	UTIL_GetWeaponAttachment( this, m_BeamCenterAttachment, vecOrigin, vecAngles );
	DrawHalo( pMaterial, vecOrigin, scale, color );

	if ( InSwing() )
	{
		pMaterial = materials->FindMaterial( STUNSTICK_GLOW_MATERIAL2, NULL, false );
		pRenderContext->Bind( pMaterial );

		color[0] = color[1] = color[2] = random->RandomFloat( 0.6f, 0.8f );
		scale = random->RandomFloat( 4.0f, 6.0f );

		// Draw an all encompassing glow around the entire head
		UTIL_GetWeaponAttachment( this, m_BeamCenterAttachment, vecOrigin, vecAngles );
		DrawHalo( pMaterial, vecOrigin, scale, color );

		// Update our effects
		if ( gpGlobals->frametime != 0.0f && ( random->RandomInt( 0, 5 ) == 0 ) )
		{
			Vector	vecOrigin;
			QAngle	vecAngles;

			GetAttachment( 1, vecOrigin, vecAngles );

			Vector	vForward;
			AngleVectors( vecAngles, &vForward );

			Vector vEnd = vecOrigin - vForward * 1.0f;

			// Inner beams
			BeamInfo_t beamInfo;

			beamInfo.m_vecStart = vEnd;
			Vector	offset = RandomVector( -12, 8 );

			offset += Vector(4,4,4);
			beamInfo.m_vecEnd = vecOrigin + offset;

			beamInfo.m_pStartEnt= cl_entitylist->GetEnt( BEAMENT_ENTITY( entindex() ) );
			beamInfo.m_pEndEnt	= cl_entitylist->GetEnt( BEAMENT_ENTITY( entindex() ) );
			beamInfo.m_nStartAttachment = 1;
			beamInfo.m_nEndAttachment = -1;
			
			beamInfo.m_nType = TE_BEAMTESLA;
			beamInfo.m_pszModelName = STUNSTICK_BEAM_MATERIAL;
			beamInfo.m_flHaloScale = 0.0f;
			beamInfo.m_flLife = 0.01f;
			beamInfo.m_flWidth = random->RandomFloat( 1.0f, 3.0f );
			beamInfo.m_flEndWidth = 0;
			beamInfo.m_flFadeLength = 0.0f;
			beamInfo.m_flAmplitude = random->RandomFloat( 1, 2 );
			beamInfo.m_flBrightness = 255.0;
			beamInfo.m_flSpeed = 0.0;
			beamInfo.m_nStartFrame = 0.0;
			beamInfo.m_flFrameRate = 1.0f;
			beamInfo.m_flRed = 255.0f;;
			beamInfo.m_flGreen = 255.0f;
			beamInfo.m_flBlue = 255.0f;
			beamInfo.m_nSegments = 16;
			beamInfo.m_bRenderable = true;
			beamInfo.m_nFlags = FBEAM_SHADEOUT;
			
			beams->CreateBeamPoints( beamInfo );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw our special effects
//-----------------------------------------------------------------------------
void C_WeaponStunStick::DrawFirstPersonEffects( void )
{
	Vector	vecOrigin;
	QAngle	vecAngles;
	float	color[3];
	float	scale;

	CMatRenderContextPtr pRenderContext( materials );
	IMaterial *pMaterial = materials->FindMaterial( STUNSTICK_GLOW_MATERIAL_NOZ, NULL, false );
	// FIXME: Needs to work with new IMaterial system!
	pRenderContext->Bind( pMaterial );

	// Find where we are in the fade
	float fadeAmount = RemapValClamped( gpGlobals->curtime, m_flFadeTime, m_flFadeTime + FADE_DURATION, 1.0f, 0.1f );

	// Get bright when swung
	if ( InSwing() )
	{
		color[0] = color[1] = color[2] = 0.4f;
		scale = 22.0f;
	}
	else
	{
		color[0] = color[1] = color[2] = 0.4f * fadeAmount;
		scale = 20.0f;
	}
	
	if ( color[0] > 0.0f )
	{
		// Draw an all encompassing glow around the entire head
		UTIL_GetWeaponAttachment( this, m_BeamCenterAttachment, vecOrigin, vecAngles );
		DrawHalo( pMaterial, vecOrigin, scale, color );
	}

	// Draw bright points at each attachment location
	for ( int i = 0; i < (NUM_BEAM_ATTACHMENTS*2)+1; i++ )
	{
		if ( InSwing() )
		{
			color[0] = color[1] = color[2] = random->RandomFloat( 0.05f, 0.5f );
			scale = random->RandomFloat( 4.0f, 5.0f );
		}
		else
		{
			color[0] = color[1] = color[2] = random->RandomFloat( 0.05f, 0.5f ) * fadeAmount;
			scale = random->RandomFloat( 4.0f, 5.0f ) * fadeAmount;
		}

		if ( color[0] > 0.0f )
		{
			UTIL_GetWeaponAttachment( this, i, vecOrigin, vecAngles );
			DrawHalo( pMaterial, vecOrigin, scale, color );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw our special effects
//-----------------------------------------------------------------------------
void C_WeaponStunStick::DrawEffects( void )
{
	if ( ShouldDrawUsingViewModel() )
	{
		DrawFirstPersonEffects();
	}
	else
	{
		DrawThirdPersonEffects();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Viewmodel was drawn
//-----------------------------------------------------------------------------
void C_WeaponStunStick::ViewModelDrawn( C_BaseViewModel *pBaseViewModel )
{
	// Don't bother when we're not deployed
	if ( IsWeaponVisible() )
	{
		// Do all our special effects
		DrawEffects();
	}

	BaseClass::ViewModelDrawn( pBaseViewModel );
}

//-----------------------------------------------------------------------------
// Purpose: Draw a cheap glow quad at our impact point (with sparks)
//-----------------------------------------------------------------------------
void StunstickImpactCallback( const CEffectData &data )
{
	float scale = random->RandomFloat( 16, 32 );

	FX_AddQuad( data.m_vOrigin, 
				data.m_vNormal, 
				scale,
				scale*2.0f,
				1.0f, 
				1.0f,
				0.0f,
				0.0f,
				random->RandomInt( 0, 360 ), 
				0,
				Vector( 1.0f, 1.0f, 1.0f ), 
				0.1f, 
				"sprites/light_glow02_add",
				0 );

	FX_Sparks( data.m_vOrigin, 1, 2, data.m_vNormal, 6, 64, 256 );
}

DECLARE_CLIENT_EFFECT( "StunstickImpact", StunstickImpactCallback );

#endif
