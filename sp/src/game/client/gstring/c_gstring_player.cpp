
#include "cbase.h"
#include "c_gstring_player.h"
#include "view_shared.h"
#include "view.h"
#include "flashlighteffect.h"
#include "c_muzzleflash_effect.h"

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


IMPLEMENT_CLIENTCLASS_DT( C_GstringPlayer, DT_CGstringPlayer, CGstringPlayer )

	RecvPropBool( RECVINFO( m_bNightvisionActive ) ),

END_RECV_TABLE()

C_GstringPlayer::C_GstringPlayer()
	: m_flNightvisionFraction( 0.0f )
	, m_flMuzzleFlashTime( 0.0f )
	, m_pMuzzleFlashEffect( NULL )
	, m_flMuzzleFlashDuration( 0.0f )
{
}

C_GstringPlayer::~C_GstringPlayer()
{
	delete m_pMuzzleFlashEffect;
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
}

void C_GstringPlayer::ProcessMuzzleFlashEvent()
{
	//BaseClass::ProcessMuzzleFlashEvent();

	m_flMuzzleFlashDuration = RandomFloat( 0.025f, 0.045f );
	m_flMuzzleFlashTime = gpGlobals->curtime + m_flMuzzleFlashDuration;
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

	if ( bDoFlashlight || bDoMuzzleflash )
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
		}
	}

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
	}
	else if (m_pFlashlight)
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

		// Update the light with the new position and direction.
		m_pMuzzleFlashEffect->UpdateLight( vecPos, vecForward, vecRight, vecUp, flStrength * flStrength );
	}
	else
	{
		delete m_pMuzzleFlashEffect;
		m_pMuzzleFlashEffect = NULL;
	}
}
