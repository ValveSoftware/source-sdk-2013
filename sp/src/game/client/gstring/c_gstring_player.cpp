
#include "cbase.h"
#include "c_gstring_player.h"
#include "view_shared.h"
#include "view.h"
#include "flashlighteffect.h"
#include "c_muzzleflash_effect.h"
#include "c_bobmodel.h"
#include "c_firstpersonbody.h"


#define FLASHLIGHT_DISTANCE		1000

//CON_COMMAND( gstring_list_recvproj, "" )
//{
//	for ( C_BaseEntity *pEnt = ClientEntityList().FirstBaseEntity();
//		pEnt;
//		pEnt = ClientEntityList().NextBaseEntity( pEnt ) )
//	{
//		if ( !pEnt->ShouldReceiveProjectedTextures( SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK ) )
//			continue;
//
//		Msg( "%i: %s - %s\n", pEnt->entindex(), pEnt->GetClassName(), pEnt->GetClassname() );
//	}
//}


static ConVar gstring_firstpersonbody_forwardoffset_min( "gstring_firstpersonbody_forwardoffset_min", "13.0" );
static ConVar gstring_firstpersonbody_forwardoffset_max( "gstring_firstpersonbody_forwardoffset_max", "18.0" );


IMPLEMENT_CLIENTCLASS_DT( C_GstringPlayer, DT_CGstringPlayer, CGstringPlayer )

	RecvPropBool( RECVINFO( m_bNightvisionActive ) ),
	RecvPropBool( RECVINFO( m_bHasUseEntity ) ),

END_RECV_TABLE()

C_GstringPlayer::C_GstringPlayer()
	: m_flNightvisionFraction( 0.0f )
	, m_flMuzzleFlashTime( 0.0f )
	, m_pMuzzleFlashEffect( NULL )
	, m_flMuzzleFlashDuration( 0.0f )
	, m_bFlashlightVisible( false )
	, m_pBobViewModel( NULL )
	, m_flBobModelAmount( 0.0f )
	, m_angLastBobAngle( vec3_angle )
	, m_pBodyModel( NULL )
	, m_flMuzzleFlashRoll( 0.0f )
{
	m_bHasUseEntity = false;
}

C_GstringPlayer::~C_GstringPlayer()
{
	delete m_pMuzzleFlashEffect;

	if ( m_pBobViewModel != NULL )
	{
		m_pBobViewModel->Release();
	}

	if ( m_pBodyModel != NULL )
	{
		m_pBodyModel->Release();
	}
}

bool C_GstringPlayer::IsNightvisionActive() const
{
	if ( render && render->GetViewEntity() > gpGlobals->maxClients )
		return false;

	return m_bNightvisionActive;
}

float C_GstringPlayer::GetNightvisionFraction() const
{
	return m_flNightvisionFraction;
}

void C_GstringPlayer::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

void C_GstringPlayer::ClientThink()
{
	BaseClass::ClientThink();

	const float flNightvisionTarget = IsNightvisionActive() ? 1.0f : 0.0f;

	if ( flNightvisionTarget != m_flNightvisionFraction )
	{
		m_flNightvisionFraction = Approach( flNightvisionTarget, m_flNightvisionFraction, gpGlobals->frametime * 5.0f );

		unsigned char r,g,b;
		g_pClientShadowMgr->GetShadowColor( &r, &g, &b );

		Vector v( r / 255.0f, g / 255.0f, b / 255.0f );
		v = Lerp( m_flNightvisionFraction * 0.7f, v, Vector( 1, 1, 1 ) );

		g_pClientShadowMgr->SetShadowColorMaterialsOnly( XYZ( v ) );
	}

	if ( ShouldMuzzleFlash() )
	{
		DisableMuzzleFlash();

		ProcessMuzzleFlashEvent();
	}

	UpdateBodyModel();
}

void C_GstringPlayer::OverrideView( CViewSetup *pSetup )
{
	Vector velocity;
	EstimateAbsVelocity( velocity );
	float speed = velocity.NormalizeInPlace();

	static float amt = 0.0f;
	static float amtSide = 0.0f;
	float amtGoal = RemapValClamped( speed, 200, 300, 0, 1.5f );

	if ( (GetFlags() & FL_ONGROUND) == 0 ||
		GetMoveType() != MOVETYPE_WALK )
		amtGoal = 0.0f;

	float amtGoalSide = amtGoal;

	float dot_fwd = DotProduct( MainViewForward(), velocity );
	float dot_side = DotProduct( MainViewRight(), velocity );
	amtGoal *= abs( dot_fwd );
	amtGoalSide *= dot_side * 2.0f;

	if ( amt != amtGoal )
		amt = Approach( amtGoal, amt, gpGlobals->frametime * 3.0f );

	if ( amtSide != amtGoalSide )
		amtSide = Approach( amtGoalSide, amtSide, gpGlobals->frametime * 4.0f );

	if ( amt > 0.0f )
	{
		float sine = sin( gpGlobals->curtime * 10.0f ) * amt;
		float sineY = sin( gpGlobals->curtime * 5.0f + M_PI * 0.5f ) * amt;

		pSetup->origin += Vector( 0, 0, 1.0f ) * sine;
		pSetup->angles.x += sine * 1.0f;
		pSetup->angles.y += sineY * 2.0f;
	}

	if ( amtSide != 0.0f )
	{
		pSetup->angles.z += amtSide;
	}

	// shake derived from viewmodel
	C_BaseViewModel *pViewModel = GetViewModel();

	if ( pViewModel != NULL
		&& pViewModel->GetModelPtr() != NULL )
	{
		if ( m_pBobViewModel == NULL )
		{
			const char *pszName = modelinfo->GetModelName( pViewModel->GetModel() );

			if ( pszName && *pszName )
			{
				m_pBobViewModel = new C_BobModel();
			
				m_pBobViewModel->InitializeAsClientEntity( pszName, RENDER_GROUP_OTHER );
			}
		}

		if ( m_pBobViewModel->GetModelIndex() != pViewModel->GetModelIndex() )
		{
			const char *pszName = modelinfo->GetModelName( pViewModel->GetModel() );

			if ( pszName && *pszName )
			{
				m_pBobViewModel->SetModel( pszName );
			}
		}

		if ( m_pBobViewModel->IsDirty() )
		{
			m_pBobViewModel->UpdateDefaultTransforms();
			m_pBobViewModel->SetDirty( false );
		}

		//extern void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );

		if ( !m_pBobViewModel->IsInvalid() )
		{
			m_pBobViewModel->SetSequence( pViewModel->GetSequence() );
			m_pBobViewModel->SetCycle( pViewModel->GetCycle() );

			QAngle ang;
			m_pBobViewModel->GetDeltaTransforms( ang );
			m_angLastBobAngle = ang * 0.15f;
		}
	}

	float flGoalBobAmount = ( m_pBobViewModel
		&& !m_pBobViewModel->IsInvalid()
		&& !m_bHasUseEntity )
		? 1.0f : 0.0f;

	if ( m_flBobModelAmount != flGoalBobAmount )
	{
		m_flBobModelAmount = Approach( flGoalBobAmount, m_flBobModelAmount, gpGlobals->frametime * 5.0f );
	}

	if ( !m_bHasUseEntity )
	{
		pSetup->angles += m_angLastBobAngle * m_flBobModelAmount;
	}
}

void C_GstringPlayer::ProcessMuzzleFlashEvent()
{
	//BaseClass::ProcessMuzzleFlashEvent();

	m_flMuzzleFlashDuration = RandomFloat( 0.025f, 0.045f );
	m_flMuzzleFlashTime = gpGlobals->curtime + m_flMuzzleFlashDuration;
	m_flMuzzleFlashRoll = RandomFloat( 0, 360.0f );
}

void C_GstringPlayer::UpdateFlashlight()
{
	const bool bDoMuzzleflash = m_flMuzzleFlashTime > gpGlobals->curtime || m_flMuzzleFlashDuration > 0.0f;
	const bool bDoFlashlight = !bDoMuzzleflash && IsEffectActive( EF_DIMLIGHT );

	Vector vecForward, vecRight, vecUp, vecPos;
	vecPos = EyePosition();
	EyeVectors( &vecForward, &vecRight, &vecUp );

	if ( m_flMuzzleFlashTime <= gpGlobals->curtime
		&& m_flMuzzleFlashDuration > 0.0f )
	{
		m_flMuzzleFlashDuration = 0.0f;
	}

	m_bFlashlightVisible = bDoFlashlight || bDoMuzzleflash;

	if ( m_bFlashlightVisible )
	{
		ConVarRef scissor( "r_flashlightscissor" );
		scissor.SetValue( "0" );

		C_BaseViewModel *pViewModel = GetViewModel();

		if ( pViewModel != NULL
			&& pViewModel->GetModelPtr() != NULL
			&& pViewModel->GetModelPtr()->GetNumAttachments() >= 1 )
		{
			extern void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );

			matrix3x4_t viewModel;
			pViewModel->GetAttachment( 1, viewModel );

			QAngle ang;
			MatrixAngles( viewModel, ang, vecPos );
			FormatViewModelAttachment( vecPos, false );
			AngleVectors( ang, &vecForward, &vecRight, &vecUp );

			trace_t tr;
			Vector vecIdealPos = vecPos - vecForward * 40.0f;

			UTIL_TraceHull( EyePosition(), vecIdealPos,
				Vector( -6, -6, -6 ), Vector( 6, 6, 6 ), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &tr );

			vecPos = tr.endpos;
		}
	}

	m_vecFlashlightPosition = vecPos;
	m_vecFlashlightForward = vecForward;

#define FLASHLIGHT_FOV_ADJUST 15.0f
#define FLASHLIGHT_FOV_MIN 5.0f

	if ( bDoFlashlight )
	{
		if (!m_pFlashlight)
		{
			// Turned on the headlight; create it.
			m_pFlashlight = new CFlashlightEffect(index);

			if (!m_pFlashlight)
				return;

			m_pFlashlight->TurnOn();
		}

		// Update the light with the new position and direction.
		m_pFlashlight->UpdateLight( vecPos, vecForward, vecRight, vecUp, FLASHLIGHT_DISTANCE );

		m_flFlashlightDot = m_pFlashlight->GetHorizontalFOV() - FLASHLIGHT_FOV_ADJUST;
		m_flFlashlightDot = MAX( m_flFlashlightDot, FLASHLIGHT_FOV_MIN );
		m_flFlashlightDot = cos( DEG2RAD( m_flFlashlightDot ) );
	}
	else if ( m_pFlashlight )
	{
		// Turned off the flashlight; delete it.
		delete m_pFlashlight;
		m_pFlashlight = NULL;
	}

	if ( bDoMuzzleflash )
	{
		if (!m_pMuzzleFlashEffect)
		{
			// Turned on the headlight; create it.
			m_pMuzzleFlashEffect = new C_MuzzleflashEffect();

			if (!m_pMuzzleFlashEffect)
				return;
		}

		float flStrength = ( m_flMuzzleFlashTime - gpGlobals->curtime ) / m_flMuzzleFlashDuration;

		QAngle ang;
		VectorAngles( vecForward, vecUp, ang );
		ang.z = m_flMuzzleFlashRoll;
		AngleVectors( ang, &vecForward, &vecRight, &vecUp );

		// Update the light with the new position and direction.
		m_pMuzzleFlashEffect->UpdateLight( vecPos, vecForward, vecRight, vecUp, flStrength * flStrength );
		
		m_flFlashlightDot = m_pMuzzleFlashEffect->GetHorizontalFOV() - FLASHLIGHT_FOV_ADJUST;
		m_flFlashlightDot = MAX( m_flFlashlightDot, FLASHLIGHT_FOV_MIN );
		m_flFlashlightDot = cos( DEG2RAD( m_flFlashlightDot ) );
	}
	else
	{
		delete m_pMuzzleFlashEffect;
		m_pMuzzleFlashEffect = NULL;
	}
}

bool C_GstringPlayer::IsRenderingFlashlight() const
{
	return m_bFlashlightVisible;
}

void C_GstringPlayer::GetFlashlightPosition( Vector &vecPos ) const
{
	vecPos = m_vecFlashlightPosition;
}

void C_GstringPlayer::GetFlashlightForward( Vector &vecForward ) const
{
	vecForward = m_vecFlashlightForward;
}

float C_GstringPlayer::GetFlashlightDot() const
{
	return m_flFlashlightDot;
}

void C_GstringPlayer::UpdateBodyModel()
{
	if ( m_pBodyModel == NULL )
	{
		m_pBodyModel = new C_FirstpersonBody();
		m_pBodyModel->InitializeAsClientEntity( "models/humans/group02/female_04.mdl", RENDER_GROUP_OPAQUE_ENTITY );
		m_pBodyModel->Spawn();
		m_pBodyModel->AddEffects( EF_NOINTERP );
	}

	QAngle angle = GetRenderAngles();
	angle.x = 0;
	angle.z = 0;

	Vector fwd, right, up;
	AngleVectors( angle, &fwd, &right, &up );

	const float flMovingMinSpeed = 10.0f;

	const float flSpeed = GetAbsVelocity().Length2D();
	const bool bInAir = ( GetFlags() & FL_ONGROUND ) == 0;
	const bool bDuck = m_Local.m_bDucked
		|| m_Local.m_bDucking;
	const bool bMoving = flSpeed > flMovingMinSpeed;

	static float flBackOffset = gstring_firstpersonbody_forwardoffset_min.GetFloat();
	float flBackOffsetDesired = bDuck ?
		gstring_firstpersonbody_forwardoffset_max.GetFloat()
		: gstring_firstpersonbody_forwardoffset_min.GetFloat();

	if ( flBackOffset != flBackOffsetDesired )
	{
		flBackOffset = Approach( flBackOffsetDesired, flBackOffset, gpGlobals->frametime * 25.0f );
	}

	Vector origin = GetRenderOrigin() - fwd * flBackOffset;
	if ( !bDuck
		|| m_Local.m_bInDuckJump && bInAir )
	{
		origin.z += GetViewOffset().z - VEC_VIEW.z;
	}

	m_pBodyModel->m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;
	m_pBodyModel->SetAbsOrigin( origin );
	m_pBodyModel->SetAbsAngles( angle );

	Activity actDesired = ACT_IDLE;
	float flPlaybackrate = 1.0f;

	if ( bInAir )
	{
		actDesired = ACT_JUMP;
	}
	else
	{
		if ( bMoving )
		{
			actDesired = bDuck ? ACT_RUN_CROUCH : ACT_RUN;
		}
		else
		{
			actDesired = bDuck ? ACT_COVER_LOW : ACT_IDLE;
		}
	}

	Vector vecVelocity = GetAbsVelocity();
	vecVelocity.z = 0.0f;
	float flLength = vecVelocity.NormalizeInPlace();

	static bool bWasMoving = false;
	const bool bDoMoveYaw = flLength > flMovingMinSpeed;

	if ( bDoMoveYaw
		&& m_pBodyModel->m_iPoseParam_MoveYaw >= 0 )
	{
		VectorYawRotate( vecVelocity, -angle.y, vecVelocity );

		float flYaw = atan2( vecVelocity.y, vecVelocity.x );
		flYaw = AngleNormalizePositive( flYaw );

		static float flYawLast = 0.0f;

		if ( bWasMoving )
		{
			flYawLast = ApproachAngle( flYaw, flYawLast, gpGlobals->frametime * 10.0f );
		}
		else
		{
			flYawLast = flYaw;
		}

		m_pBodyModel->SetPoseParameter( m_pBodyModel->m_iPoseParam_MoveYaw, RAD2DEG( AngleNormalize( flYawLast ) ) );
	}

	bWasMoving = bDoMoveYaw;

	if ( m_pBodyModel->GetSequenceActivity( m_pBodyModel->GetSequence() )
		!= actDesired )
	{
		m_pBodyModel->SetSequence( m_pBodyModel->SelectWeightedSequence( actDesired ) );
	}

	if ( !bInAir && bMoving )
	{
		float flGroundSpeed = m_pBodyModel->GetSequenceGroundSpeed( m_pBodyModel->GetSequence() );

		if ( flGroundSpeed > 0.0f )
		{
			flPlaybackrate = flSpeed / flGroundSpeed;

			flPlaybackrate = MIN( 3.0f, flPlaybackrate );
		}
	}

	m_pBodyModel->SetPlaybackRate( flPlaybackrate );
	m_pBodyModel->StudioFrameAdvance();

	if ( m_pBodyModel->GetModel() != NULL
		&& m_pBodyModel->GetShadowHandle() == CLIENTSHADOW_INVALID_HANDLE )
	{
		m_pBodyModel->CreateShadow();
	}
}