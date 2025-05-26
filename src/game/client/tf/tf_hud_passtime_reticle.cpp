//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_func_passtime_goal.h"
#include "c_tf_passtime_logic.h"
#include "tf_hud_passtime_reticle.h"
#include "passtime_convars.h"
#include "tf_weapon_passtime_gun.h"
#include "c_tf_player.h"
#include "view.h"
#include "c_tf_playerresource.h"
#include "igameevents.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// The team colors from g_PR are wrong and I couldn't fix that fast enough.
// These colors were sampled from HUD art.
Color GetTeamColor( int iTeam )
{
	switch( iTeam )
	{
	case TF_TEAM_RED: return Color(0xFF, 0x64, 0x64);
	case TF_TEAM_BLUE: return Color(0xA5, 0xDE, 0xFF);
	default: return Color(0xFF, 0xFF, 0xFF);
	};
}
//callback included in crosshair convars
void OnCrosshairSettingsChanged(IConVar* pConVar, const char* pOldValue, float flOldValue);
//reloads for each crosshair type
void ReloadAllPlayerReticles();
void ReloadAllPassReticles();
void ReloadBallReticle();
void ReloadBounceReticle();
void PrecacheAllReticleMaterials();
void PrecacheReticleMaterial( const char *pMaterialName );
static bool g_BounceReticleDirty = false;
//-----------------------------------------------------------------------------
// C_PasstimeReticle
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
C_PasstimeReticle::~C_PasstimeReticle() 
{
	for( int i = 0; i < m_pSprites.Count(); ++i )
	{
		clienteffects->RemoveEffect( m_pSprites[i] );
	}
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::OnClientThink()
{
	if ( !Update() )
	{
		SetAllAlphas( 0 );
	}
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::AddSprite( CFXQuad *pQuad )
{
	Assert( pQuad );
	m_pSprites.AddToTail( pQuad );
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetAllOrigins( const Vector &vec )
{
	for ( int i = 0; i < m_pSprites.Count(); ++i )
	{
		m_pSprites[i]->m_FXData.SetOrigin( vec );
	}
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetAllNormals( const Vector &vec )
{
	for ( int i = 0; i < m_pSprites.Count(); ++i )
	{
		m_pSprites[i]->m_FXData.SetNormal( vec );
	}
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetAllAlphas( byte iA )
{
	auto flA = iA / 255.0f;
	for ( int i = 0; i < m_pSprites.Count(); ++i )
	{
		m_pSprites[i]->m_FXData.SetAlpha( flA, flA );
	}
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetAllScales( float flScale )
{
	for ( int i = 0; i < m_pSprites.Count(); ++i )
	{
		m_pSprites[i]->m_FXData.SetScale( flScale, flScale );
	}
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetOrigin( int i, const Vector &vec )
{
	m_pSprites[i]->m_FXData.SetOrigin( vec );
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetNormal( int i, const Vector &normal )
{
	m_pSprites[i]->m_FXData.SetNormal( normal );
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetAlpha( int i, byte iA )
{
	auto flA = iA / 255.0f;
	m_pSprites[i]->m_FXData.SetAlpha( flA, flA );
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetRgba( int i, byte iR, byte iG, byte iB, byte iA )
{
	m_pSprites[i]->m_FXData.SetColor( iR / 255.0f, iG / 255.0f, iB / 255.0f );
	auto flA = iA / 255.0;
	m_pSprites[i]->m_FXData.SetAlpha( flA, flA );
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::SetScale( int i, float flScale )
{
	m_pSprites[i]->m_FXData.SetScale( flScale, flScale );
}

//-----------------------------------------------------------------------------
void C_PasstimeReticle::ReloadSprites()
{
	DevMsg("ReloadSprites: Clearing %d sprites.\n", m_pSprites.Count());
	for( int i = 0; i < m_pSprites.Count(); ++i )
	{
		if (m_pSprites[i])
		{
			DevMsg("Removing sprite at index %d.\n", i);
			clienteffects->RemoveEffect(m_pSprites[i]);
		}
		else 
		{
			DevMsg("Sprite at index %d is null.\n", i);
		}
	}
	m_pSprites.RemoveAll();
	DevMsg("ReloadSprites: Sprite list cleared. Remaining count: %d.\n", m_pSprites.Count());
}

//-----------------------------------------------------------------------------
// C_PasstimeBallReticle
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
ConVar pf_ball_outline_1_file( "pf_ball_outline_1_file", "p1fix", FCVAR_ARCHIVE, "Sets the first sprite drawn on the ball when not carried by a player.", OnCrosshairSettingsChanged );
ConVar pf_ball_outline_2_file( "pf_ball_outline_2_file", "p2fix", FCVAR_ARCHIVE, "Sets the second sprite drawn on the ball when not carried by a player.", OnCrosshairSettingsChanged );

static const float k_flBallReticleSize = 64;

void C_PasstimeBallReticle::InitializeSprites()
{
	const char* defaultFolder = "reticles/";
	//get material paths from convars and pass to CreateReticleSprite
	//refactored this out of the constructor to allow it to be called from ReloadSprites
	const char* pBallOutlineA = pf_ball_outline_1_file.GetString();
	const char* pBallOutlineB = pf_ball_outline_2_file.GetString();

	if ( !pBallOutlineA || strlen(pBallOutlineA) == 0 )
	{
		pBallOutlineA = "reticles/p1fix"; 
	}
	else if ( strnicmp( pBallOutlineA, defaultFolder, strlen( defaultFolder ) ) != 0 )
	{
        static char fullPath[256];
		snprintf(fullPath, sizeof(fullPath), "%s%s", defaultFolder, pBallOutlineA);
		pBallOutlineA = fullPath;
	}
	if ( !pBallOutlineB || strlen(pBallOutlineB) == 0 )
	{
		pBallOutlineB = "reticles/p2fix"; 
	}
	else if ( strnicmp( pBallOutlineB, defaultFolder, strlen( defaultFolder ) ) != 0 )
	{
		static char fullPath[256];
		snprintf(fullPath, sizeof(fullPath), "%s%s", defaultFolder, pBallOutlineB);
		pBallOutlineB = fullPath;
	}

	AddSprite( CreateReticleSprite( pBallOutlineA, k_flBallReticleSize, 360 ) ); // the O
	AddSprite( CreateReticleSprite( pBallOutlineB, k_flBallReticleSize, 360 ) ); // the ><
}

C_PasstimeBallReticle::C_PasstimeBallReticle()
{
	InitializeSprites();
	PrecacheAllReticleMaterials();
}

//-----------------------------------------------------------------------------
// called from OnCrosshairSettingsChanged when the crosshair convars are changed 
void C_PasstimeBallReticle::ReloadSprites()
{
	C_PasstimeReticle::ReloadSprites();
	InitializeSprites();
}

bool C_PasstimeBallReticle::Update()
{
	if ( !g_pPasstimeLogic || !g_pPasstimeLogic->GetBall() ) 
	{
		return false;
	}

	auto *pBall = g_pPasstimeLogic->GetBall();
	auto *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BaseEntity *pTarget = 0;
	auto bHomingActive = false;
	auto bHaveTarget = g_pPasstimeLogic->GetBallReticleTarget( &pTarget, &bHomingActive );
	if ( !pBall || !pLocalPlayer || !bHaveTarget )
	{
		return false;
	}
	
	auto vecTargetPos = pTarget->WorldSpaceCenter();
	SetAllOrigins( vecTargetPos );
	SetAllNormals( -MainViewForward() );
		
	auto teamColor = GetTeamColor( pTarget->GetTeamNumber() );
	auto iAlpha = ( bHomingActive || pLocalPlayer->m_Shared.IsTargetedForPasstimePass() )
		? (int)( (fmodf( gpGlobals->curtime * 3.0f, 1.0f )) * 255 )
		: 180;

	SetRgba( 0, teamColor.r(), teamColor.g(), teamColor.b(), iAlpha );
	SetRgba( 1, teamColor.r(), teamColor.g(), teamColor.b(), iAlpha );

	auto flDist = (vecTargetPos - MainViewOrigin()).Length();
	auto flScale = RemapValClamped( flDist, 768.0f, 4096.0f, 1.0f, 3.0f );
	flScale *= k_flBallReticleSize;
	if ( bHomingActive || pLocalPlayer->m_Shared.IsTargetedForPasstimePass() )
	{
		flScale *= 2;
	}
	SetScale( 0, flScale );
	SetScale( 1, flScale );

	return true;
}

ConVar pf_goal_outline( "pf_goal_outline", "1", FCVAR_ARCHIVE, "Enables/disables the floating indicator on the enemy goal." );
//-----------------------------------------------------------------------------
// C_PasstimeGoalReticle
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
C_PasstimeGoalReticle::C_PasstimeGoalReticle( C_FuncPasstimeGoal *pGoal )
{
	Assert( pGoal );
	m_hGoal.Set( pGoal );
	AddSprite( CreateReticleSprite( "reticles/p1fix", 256, 50 ) );
	AddSprite( CreateReticleSprite( "reticles/p2fix", 128, 0 ) );
}

//-----------------------------------------------------------------------------
bool C_PasstimeGoalReticle::Update()
{
	if ( !g_pPasstimeLogic || !g_pPasstimeLogic->GetBall() )
	{
		return false;
	}

	// don't show if pf_goal_outline is set to 0
	if ( !pf_goal_outline.GetBool() )
	{
		return false;
	}

	// don't show if ball isn't being carried by local player
	auto *pEnt = g_pPasstimeLogic->GetBall()->GetCarrier();
	if ( !pEnt || (pEnt != C_BasePlayer::GetLocalPlayer()) )
	{
		return false;
	}

	auto *pGoal = m_hGoal.Get();
	if ( !g_pPasstimeLogic || !g_pPasstimeLogic->GetBall() || IsLocalPlayerSpectator() 
		|| !pGoal || pGoal->BGoalTriggerDisabled() || (pGoal->GetTeamNumber() != pEnt->GetTeamNumber()) )
	{
		return false;
	}

	auto teamColor = GetTeamColor( pEnt->GetTeamNumber() );

	auto vec = pGoal->WorldSpaceCenter();
	auto facingFrac = MainViewForward().Dot( (vec - MainViewOrigin()).Normalized() );
	if ( facingFrac < 0.6 )
	{
		return false;
	}
	facingFrac = RemapValClamped( facingFrac, 0.8f, 1.0f, 1.0f, 0.3f );

	// ring
	SetOrigin( 0, vec );
	auto flPulseSpeed = 10.0f;
	auto flPulseFrac = Clamp( FastCos( gpGlobals->curtime * flPulseSpeed ), 0.0f, 1.0f );
	SetRgba( 0, teamColor.r(), teamColor.g(), teamColor.b(), flPulseFrac * (120 * facingFrac) );
	
	// arrow
	float tmp;
	flPulseFrac = 1.0f - modff( gpGlobals->curtime, &tmp );
	SetOrigin( 1, vec + Vector(0, 0, flPulseFrac * 64) );
	SetAllNormals( -MainViewForward() );
	SetRgba( 1, teamColor.r(), teamColor.g(), teamColor.b(), flPulseFrac * (255 * facingFrac) );
	return true;
}

ConVar pf_crosshair_teammate_inner( "pf_crosshair_teammate_inner", "plfix", FCVAR_ARCHIVE, "Material for the inner part of the teammate pass reticle.", OnCrosshairSettingsChanged );
ConVar pf_crosshair_teammate_outer_1( "pf_crosshair_teammate_outer_1", "p1fix", FCVAR_ARCHIVE, "Material for the first outer part of the teammate pass reticle.", OnCrosshairSettingsChanged );
ConVar pf_crosshair_teammate_outer_2( "pf_crosshair_teammate_outer_2", "p2fix", FCVAR_ARCHIVE, "Material for the second outer part of the teammate pass reticle.", OnCrosshairSettingsChanged );
ConVar pf_crosshair_teammate_blink( "pf_crosshair_teammate_blink", "1", FCVAR_ARCHIVE, "Sets whether the teammate pass reticle is solid (0) or glowing (1)." );
ConVar pf_crosshair_teammate_alpha( "pf_crosshair_teammate_alpha", "200", FCVAR_ARCHIVE, "Sets the alpha value of the teammate pass reticle. Only works when pf_crosshair_teammate_blink is set to 0." );

//-----------------------------------------------------------------------------
// C_PasstimePassReticle
//-----------------------------------------------------------------------------

const float kPassReticleScale = 64;
//-----------------------------------------------------------------------------
void C_PasstimePassReticle::InitializeSprites()
{
	const char* defaultFolder = "reticles/";
	// in the base game, there are three sprites for the pass reticle
	// the inner sprite is the soccer ball, and the outer sprites are the O and ><
	const char* innerTeammateSprite = pf_crosshair_teammate_inner.GetString();
	const char* outerTeammateSprite1 = pf_crosshair_teammate_outer_1.GetString();
	const char* outerTeammateSprite2 = pf_crosshair_teammate_outer_2.GetString();

	if ( !innerTeammateSprite || strlen(innerTeammateSprite) == 0 )
	{
		innerTeammateSprite = "reticles/plfix"; 
	}
	else if ( strnicmp( innerTeammateSprite, defaultFolder, strlen( defaultFolder ) ) != 0 )
	{
		static char fullPath[256];
		snprintf(fullPath, sizeof(fullPath), "%s%s", defaultFolder, innerTeammateSprite);
		innerTeammateSprite = fullPath;
	}
	if ( !outerTeammateSprite1 || strlen(outerTeammateSprite1) == 0 )
	{
		outerTeammateSprite1 = "reticles/p1fix"; 
	}
	else if ( strnicmp( outerTeammateSprite1, defaultFolder, strlen( defaultFolder ) ) != 0 )
	{
		static char fullPath[256];
		snprintf(fullPath, sizeof(fullPath), "%s%s", defaultFolder, outerTeammateSprite1);
		outerTeammateSprite1 = fullPath;
	}
	if ( !outerTeammateSprite2 || strlen(outerTeammateSprite2) == 0 )
	{
		outerTeammateSprite2 = "reticles/p2fix"; 
	}
	else if ( strnicmp( outerTeammateSprite2, defaultFolder, strlen( defaultFolder ) ) != 0 )
	{
		static char fullPath[256];
		snprintf(fullPath, sizeof(fullPath), "%s%s", defaultFolder, outerTeammateSprite2);
		outerTeammateSprite2 = fullPath;
	}

	AddSprite( CreateReticleSprite( innerTeammateSprite, kPassReticleScale, 100 ) ); // the *
	AddSprite( CreateReticleSprite( outerTeammateSprite1, kPassReticleScale, 0 ) ); // the O
	AddSprite( CreateReticleSprite( outerTeammateSprite2, kPassReticleScale, 0 ) ); // the ><
}

C_PasstimePassReticle::C_PasstimePassReticle() 
{
	m_flTargetScore = FLT_MAX;
	InitializeSprites();
}

//-----------------------------------------------------------------------------
void C_PasstimePassReticle::ReloadSprites()
{
	C_PasstimeReticle::ReloadSprites();
	InitializeSprites();
}

bool C_PasstimePassReticle::Update()
{
	if ( !g_pPasstimeLogic || !g_pPasstimeLogic->GetBall() || IsLocalPlayerSpectator() )
	{
		return false;
	}

	auto *pBallCarrier = g_pPasstimeLogic->GetBall()->GetCarrier();
	if ( !pBallCarrier )
	{
		return false;
	}

	if ( (pBallCarrier != C_BasePlayer::GetLocalPlayer()) )
	{
		return false;
	}
	
	SetAllNormals( -MainViewForward() );

	// the player's actual pass target always takes precedence, but if it's
	// not set, try to find a candidate and display a hint for that
	auto *pPassTarget = pBallCarrier->m_Shared.GetPasstimePassTarget();
	if ( pPassTarget )
	{
		m_hTarget = pPassTarget;
		auto vecTargetPos = pPassTarget->WorldSpaceCenter();
		SetAllOrigins( vecTargetPos );

		auto teamColor = GetTeamColor( pBallCarrier->GetTeamNumber() );
		auto neutralColor = GetTeamColor( TEAM_UNASSIGNED );

		int iAlpha;
		if (pf_crosshair_teammate_blink.GetBool())
		{
   			iAlpha = (int)((fmodf(gpGlobals->curtime * 3.0f, 1.0f)) * 255);
			SetRgba( 1, neutralColor.r(), neutralColor.g(), neutralColor.b(), 255 );
		}
		else
		{
			iAlpha = pf_crosshair_teammate_alpha.GetInt();
			SetRgba( 1, neutralColor.r(), neutralColor.g(), neutralColor.b(), iAlpha );
		}
		SetRgba( 0, teamColor.r(), teamColor.g(), teamColor.b(), iAlpha );
		SetRgba( 2, teamColor.r(), teamColor.g(), teamColor.b(), iAlpha );

		auto flDist = (vecTargetPos - MainViewOrigin()).Length();
		auto flScale = RemapValClamped( flDist, 768.0f, 8192.0f, 1.0f, 8.0f );
		SetAllScales( flScale * kPassReticleScale * 2 );
	}
	else
	{
		FindPassHintTarget( pBallCarrier );
		if ( !m_hTarget )
		{
			return false;
		}

		auto flPulseSpeed = 20;
		auto flPulseFrac = Clamp( FastCos( gpGlobals->curtime * flPulseSpeed ), 0.3f, 1.0f );


		auto teamColor = GetTeamColor( TEAM_UNASSIGNED );
		auto neutralColor = GetTeamColor( TEAM_UNASSIGNED );
		int iAlpha;
		if (pf_crosshair_teammate_blink.GetBool())
		{
			iAlpha = 200 * flPulseFrac * Clamp( m_flTargetScore + 0.5f, 0.0f, 1.0f );
			SetRgba( 1, neutralColor.r(), neutralColor.g(), neutralColor.b(), 80 );
		}
		else
		{
			iAlpha = pf_crosshair_teammate_alpha.GetInt();
			SetRgba( 1, neutralColor.r(), neutralColor.g(), neutralColor.b(), iAlpha );
		}

		SetRgba( 0, teamColor.r(), teamColor.g(), teamColor.b(), iAlpha );
		SetRgba( 2, teamColor.r(), teamColor.g(), teamColor.b(), iAlpha );
		
		auto vecTargetPos = m_hTarget->WorldSpaceCenter();
		SetAllOrigins( vecTargetPos );
	
		auto flDist = (vecTargetPos - MainViewOrigin()).Length();
		auto flScale = RemapValClamped( flDist, 768.0f, 8192.0f, 1.0f, 8.0f );
		SetAllScales( flScale * kPassReticleScale );
	}

	return true;
}

//-----------------------------------------------------------------------------
extern int HudTransform( const Vector &point, Vector &screen );
void C_PasstimePassReticle::FindPassHintTarget( C_TFPlayer *pLocalPlayer )
{
	m_hTarget = 0;
	m_flTargetScore = -FLT_MAX;

	auto flFovDeg = 70;
	auto flDotFov = cosf( DEG2RAD( flFovDeg / 2.0f ) );
	auto vecViewPos = MainViewOrigin();
	auto vecViewFwd = MainViewForward();

	auto flMaxPassDist = g_pPasstimeLogic->GetMaxPassRange() - 400; // arbitrary, based on TF_MAX_SPEED

	// for each player in front of the local player,
	for( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		auto *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !C_PasstimeGun::BValidPassTarget( pLocalPlayer, pPlayer ) )
		{
			continue;
		}

		auto vecTargetPos = pPlayer->EyePosition();
		auto vecToTarget = vecTargetPos - vecViewPos;
		if ( vecToTarget.NormalizeInPlace() > flMaxPassDist ) 
		{
			// the server may disagree on this, so the client is less permissive
			// when it guesses in order to prevent false positives
			continue; // too far away, probably
		}

		auto fDotTarget = vecToTarget.Dot( vecViewFwd );
		if ( fDotTarget <= flDotFov )
		{
			continue; // not in front
		}

		// determine player's distance from center of screen 
		Vector vecScreenPos;
		auto bBehindViewplane = HudTransform( vecTargetPos, vecScreenPos );
		if ( bBehindViewplane )
		{
			continue; // paranoia
		}
		auto flScore = 0.5f - vecScreenPos.Length2D();
		if ( flScore <= m_flTargetScore )
		{
			continue; // someone else already found that's closer
		}

		// trace to see if they are visible
		// this is the same trace the gun does
		trace_t tr;
		CTraceFilterSimple tracefilter( pLocalPlayer, COLLISION_GROUP_PROJECTILE );
		UTIL_TraceLine( vecViewPos, vecTargetPos, MASK_PLAYERSOLID, &tracefilter, &tr );
		if ( tr.m_pEnt != pPlayer )
		{
			continue; // not visible
		}
	
		m_flTargetScore = flScore;
		m_hTarget = pPlayer;
	}
}

ConVar pf_crosshair_inner_file( "pf_crosshair_inner_file", "plfix", FCVAR_ARCHIVE, "Material for the inner part of the JACK bounce crosshair.", OnCrosshairSettingsChanged );
ConVar pf_crosshair_outer_file( "pf_crosshair_outer_file", "p1fix", FCVAR_ARCHIVE, "Material for the outer part of the JACK bounce crosshair.", OnCrosshairSettingsChanged );
ConVar pf_crosshair_inner_scale( "pf_crosshair_inner_scale", "1", FCVAR_ARCHIVE, "Size of the inner part of the JACK bounce crosshair.", OnCrosshairSettingsChanged );
ConVar pf_crosshair_outer_scale( "pf_crosshair_outer_scale", "1", FCVAR_ARCHIVE, "Size of the outer part of the JACK bounce crosshair.", OnCrosshairSettingsChanged );
ConVar pf_crosshair_outer_spinspeed( "pf_crosshair_outer_spinspeed", "200", FCVAR_ARCHIVE, "Speed of the rotation of the outer part of the JACK bounce crosshair.", OnCrosshairSettingsChanged );

//-----------------------------------------------------------------------------
// C_PasstimeBounceReticle
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_PasstimeBounceReticle::InitializeSprites()
{
	const char* defaultFolder = "reticles/";
    // Get ConVar values
    const char* pInnerBounceSprite = pf_crosshair_inner_file.GetString();
    const char* pOuterBounceSprite = pf_crosshair_outer_file.GetString();
    float pInnerBounceScale = pf_crosshair_inner_scale.GetFloat();
    float pOuterBounceScale = pf_crosshair_outer_scale.GetFloat();
    int pOuterBounceSpinSpeed = pf_crosshair_outer_spinspeed.GetInt();

	//sets the default files for if the user inputs "" 
	if (!pInnerBounceSprite || strlen(pInnerBounceSprite) == 0)
    {
        pInnerBounceSprite = "reticles/plfix"; 
    }
	else if ( strnicmp( pInnerBounceSprite, defaultFolder, strlen( defaultFolder ) ) != 0 )
	{
		static char fullPath[256];
		snprintf(fullPath, sizeof(fullPath), "%s%s", defaultFolder, pInnerBounceSprite);
		pInnerBounceSprite = fullPath;
	}
    if (!pOuterBounceSprite || strlen(pOuterBounceSprite) == 0)
    {
        pOuterBounceSprite = "reticles/p1fix"; 
    }
	else if ( strnicmp( pOuterBounceSprite, defaultFolder, strlen( defaultFolder ) ) != 0 )
	{
		static char fullPath[256];
		snprintf(fullPath, sizeof(fullPath), "%s%s", defaultFolder, pOuterBounceSprite);
		pOuterBounceSprite = fullPath;
	}
	DevMsg("Inner Sprite: %s, Outer Sprite: %s\n", pInnerBounceSprite, pOuterBounceSprite);
    // Add new sprites and scale by factors from convars
    AddSprite(CreateReticleSprite(pInnerBounceSprite, pInnerBounceScale * 160, 0)); // Inner crosshair
    AddSprite(CreateReticleSprite(pOuterBounceSprite, pOuterBounceScale * 80, pOuterBounceSpinSpeed)); // Outer crosshair
}

C_PasstimeBounceReticle::C_PasstimeBounceReticle()
{
	InitializeSprites();
}

ConVar pf_crosshair_inner_color( "pf_crosshair_inner_color", "255 255 0", FCVAR_ARCHIVE, "Sets the color of the inner piece of the JACK bounce crosshair in R G B format.", OnCrosshairSettingsChanged );
ConVar pf_crosshair_outer_color( "pf_crosshair_outer_color", "255 255 255", FCVAR_ARCHIVE, "Sets the color of the outer piece of the JACK bounce crosshair in R G B format.", OnCrosshairSettingsChanged );
ConVar pf_crosshair_inner_a( "pf_crosshair_inner_a", "200", FCVAR_ARCHIVE, "Sets the alpha value of the inner piece of the JACK bounce crosshair." );
ConVar pf_crosshair_outer_a( "pf_crosshair_outer_a", "200", FCVAR_ARCHIVE, "Sets the alpha value of the outer piece of the JACK bounce crosshair." );

ConVar pf_crosshair_inner_teamcolored( "pf_crosshair_inner_teamcolored", "1", FCVAR_ARCHIVE, "If set to 1, the inner piece of the JACK bounce crosshair will be team colored." );
ConVar pf_crosshair_outer_teamcolored( "pf_crosshair_outer_teamcolored", "0", FCVAR_ARCHIVE, "If set to 1, the outer piece of the JACK bounce crosshair will be team colored." );

void C_PasstimeBounceReticle::Show( const Vector &vec, const Vector &normal )
{
	auto *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	auto nTeamNumber = pLocalPlayer->GetTeamNumber();

	SetOrigin( 0, vec );
	SetOrigin( 1, vec );//+ (normal * 16) );
	SetNormal( 0, normal );
	SetNormal( 1, -MainViewForward() );
	int r = 200, g = 200, b = 200;
	if ( g_BounceReticleDirty )
	{
		ReloadSprites();
		g_BounceReticleDirty = false;
	}
	if ( pf_crosshair_inner_teamcolored.GetBool() )
	{
		if ( nTeamNumber )
		{
			Color teamColor = GetTeamColor( nTeamNumber );
			SetRgba( 0, teamColor.r(), teamColor.g(), teamColor.b(), pf_crosshair_inner_a.GetInt() );
		}
	}
	else
	{
		sscanf( pf_crosshair_inner_color.GetString(), "%d %d %d", &r, &g, &b );
		SetRgba( 0, r, g, b, pf_crosshair_inner_a.GetInt() );
	}
	
	if ( pf_crosshair_outer_teamcolored.GetBool() )
	{
		if ( nTeamNumber )
		{
			Color teamColor = GetTeamColor( nTeamNumber );
			SetRgba( 1, teamColor.r(), teamColor.g(), teamColor.b(), pf_crosshair_inner_a.GetInt() );
		}
	}
	else
	{
		sscanf( pf_crosshair_outer_color.GetString(), "%d %d %d", &r, &g, &b );
		SetRgba( 1, r, g, b, pf_crosshair_outer_a.GetInt() );
	}
	
}

void C_PasstimeBounceReticle::ReloadSprites()
{
    //clear existing sprites
    C_PasstimeReticle::ReloadSprites();
	InitializeSprites();
}

void C_PasstimeBounceReticle::Hide()
{
	SetAllAlphas( 0 );
}

bool C_PasstimeBounceReticle::Update()
{
	return !IsLocalPlayerSpectator();
}

ConVar pf_teamicons_alpha( "pf_teamicons_alpha", "100", FCVAR_ARCHIVE, "Sets alpha of the icons drawn on teammates through walls." );
ConVar pf_teamicons_red_file( "pf_teamicons_red_file", "", FCVAR_ARCHIVE, "Sets material file path of the icons drawn on teammates through walls for RED team.", OnCrosshairSettingsChanged );
ConVar pf_teamicons_blu_file( "pf_teamicons_blu_file", "", FCVAR_ARCHIVE, "Sets material file path of the icons drawn on teammates through walls for BLU team.", OnCrosshairSettingsChanged );
ConVar pf_teamicons( "pf_teamicons", "1", FCVAR_ARCHIVE, "Enables/disables the icons drawn on teammates through walls." );
ConVar pf_teamicons_scale( "pf_teamicons_scale", "1", FCVAR_ARCHIVE, "Sets the scale of the icons drawn on teammates through walls." );
//-----------------------------------------------------------------------------
// C_PasstimePlayerReticle
//-----------------------------------------------------------------------------
void C_PasstimePlayerReticle::InitializeSprites()
{
	const char* defaultFolder = "reticles/";
	// Get ConVar values
	const char* teamSpriteRed = pf_teamicons_red_file.GetString();
	const char* teamSpriteBlu = pf_teamicons_blu_file.GetString();
	float teamSpriteScale = pf_teamicons_scale.GetFloat();

	if ( !teamSpriteRed || strlen(teamSpriteRed) == 0 )
	{
		teamSpriteRed = "passtime/hud/passtime_teamicon_red"; 
	}
	else if ( strnicmp( teamSpriteRed, defaultFolder, strlen( defaultFolder ) ) != 0 )
	{
		static char fullPath[256];
		snprintf(fullPath, sizeof(fullPath), "%s%s", defaultFolder, teamSpriteRed);
		teamSpriteRed = fullPath;
	}
	if ( !teamSpriteBlu || strlen(teamSpriteBlu) == 0 )
	{
		teamSpriteBlu = "passtime/hud/passtime_teamicon_blue"; 
	}
	else if ( strnicmp( teamSpriteBlu, defaultFolder, strlen( defaultFolder ) ) != 0 )
	{
		static char fullPath[256];
		snprintf(fullPath, sizeof(fullPath), "%s%s", defaultFolder, teamSpriteBlu);
		teamSpriteBlu = fullPath;
	}

	AddSprite(CreateReticleSprite(teamSpriteRed, teamSpriteScale * 128, 0)); // Red team icon
	AddSprite(CreateReticleSprite(teamSpriteBlu, teamSpriteScale * 128, 0)); // Blue team icon
}

C_PasstimePlayerReticle::C_PasstimePlayerReticle( C_TFPlayer *pPlayer )
{
	m_hPlayer.Set( pPlayer );
	InitializeSprites();
}

//-----------------------------------------------------------------------------
void C_PasstimePlayerReticle::ReloadSprites()
{
	C_PasstimeReticle::ReloadSprites(); // clear existing sprites for this player

	C_TFPlayer *pPlayer = m_hPlayer.Get();
	m_hPlayer.Set( pPlayer );
	InitializeSprites(); // redraw sprites for this player

}

bool C_PasstimePlayerReticle::Update()
{
	if ( !g_pPasstimeLogic ) 
	{
		return false;
	}

	// if pf_teamicons is 0, do not show team icons
	if ( !pf_teamicons.GetBool() )
	{
		return false;
	}

	auto *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	auto *pPlayer = m_hPlayer.Get();
	if ( !pLocalPlayer || pLocalPlayer->IsPlayerDead() 
		|| !pPlayer || pPlayer->IsPlayerDead() )
	{
		return false;
	}

	if ( pPlayer->m_Shared.GetPercentInvisible() > 0 ) 
	{
		// dont' show because player is invisible, friend or foe
		return false;
	}

	auto iFriendsDetail = tf_passtime_player_reticles_friends.GetInt();
	auto iEnemiesDetail = tf_passtime_player_reticles_enemies.GetInt();

	auto bIsDisguisedEnemy = pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) 
		&& ( pPlayer->m_Shared.GetDisguiseTeam() == pLocalPlayer->GetTeamNumber() )
		&& !pPlayer->m_Shared.IsFullyInvisible();

	auto bIsFriend = pLocalPlayer->InSameTeam( pPlayer ) || bIsDisguisedEnemy;

	if ( (bIsFriend && (iFriendsDetail <= 0)) 
		|| (!bIsFriend && (iEnemiesDetail <= 0)) )
	{
		// don't show because disabled
		return false;
	}

	if ( !pLocalPlayer->m_Shared.HasPasstimeBall()
		&& ((bIsFriend && (iFriendsDetail == 1)) 
			|| (!bIsFriend && (iEnemiesDetail == 1))))
	{
		// don't show because not visible unless have ball
		return false;
	}

	if ( pPlayer->IsDormant() )
	{
		// don't show because not getting updated from server for some reason
		// probably not in PVS
		return false;
	}

	auto nTeamNumber = pPlayer->GetTeamNumber();
	if ( !pLocalPlayer->InSameTeam( pPlayer ) && bIsFriend )	// they're not on my team but they're showing as a friend, they must be a spy so use my team color
	{
		nTeamNumber = pLocalPlayer->GetTeamNumber();
	}

	int iShowSprite, iHideSprite;
	if ( nTeamNumber == TF_TEAM_RED )
	{
		iShowSprite = 0;
		iHideSprite = 1;
	}
	else if ( nTeamNumber == TF_TEAM_BLUE )
	{
		iShowSprite = 1;
		iHideSprite = 0;
	}
	else
	{
		return false;
	}

	auto vecTarget = pPlayer->EyePosition();
	int iX, iY;
	auto bOnScreen = GetVectorInHudSpace( vecTarget, iX, iY );
	if ( !bOnScreen )
	{
		return false;
	}

	trace_t	tr;
	CTraceFilterIgnorePlayers tracefilter( pLocalPlayer, COLLISION_GROUP_PROJECTILE );
	UTIL_TraceLine( MainViewOrigin(), vecTarget, MASK_PLAYERSOLID, &tracefilter, &tr );
	if ( tr.fraction == 1 )
	{
		// made it all the way, the guy is visible so hide the icon
		return false;
	}

	auto flDist = (vecTarget - MainViewOrigin()).Length();
	auto flScale = RemapValClamped( flDist, 1024.0f, 4096.0f, 80, 128 );
	flScale *= pf_teamicons_scale.GetFloat(); //multiply by convar scale 
	SetAlpha( iHideSprite, 0 );
	SetAlpha( iShowSprite, pf_teamicons_alpha.GetInt() );
	SetAllScales( flScale );
	SetAllOrigins( vecTarget );
	SetAllNormals( -MainViewForward() );
	return true;
}

//-----------------------------------------------------------------------------
// C_PasstimeAskForBallReticle
//-----------------------------------------------------------------------------
C_PasstimeAskForBallReticle::C_PasstimeAskForBallReticle( C_TFPlayer *pPlayer )
{
	m_hPlayer.Set( pPlayer );
	AddSprite( CreateReticleSprite( "passtime/hud/passtime_pass_to_me_prompt", 128, 0 ) );
	SetRgba( 0, 255, 255, 255, 200 );
}

//-----------------------------------------------------------------------------
bool C_PasstimeAskForBallReticle::Update()
{
	if ( !g_pPasstimeLogic ) 
	{
		return false;
	}

	auto *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	auto *pPlayer = m_hPlayer.Get();

	if ( !pLocalPlayer || !pPlayer )
	{
		return false;
	}

	auto bLocalPlayerObserver = pLocalPlayer->IsObserver();
	if ( !bLocalPlayerObserver && pLocalPlayer->IsPlayerDead() )
	{
		return false;
	}

	if ( (pPlayer->m_Shared.AskForBallTime() < gpGlobals->curtime) || pPlayer->IsPlayerDead() )
	{
		return false;
	}

	if ( !bLocalPlayerObserver && !pLocalPlayer->m_Shared.HasPasstimeBall() && !pPlayer->InSameTeam( pLocalPlayer ) )
	{
		return false;
	}

	auto vecTarget = pPlayer->EyePosition();
	vecTarget.z += 16;
	int iX, iY;
	auto bOnScreen = GetVectorInHudSpace( vecTarget, iX, iY );
	if ( !bOnScreen )
	{
		return false;
	}

	auto flDist = (vecTarget - MainViewOrigin()).Length();
	auto flScale = RemapValClamped( flDist, 1024.0f, 4096.0f, 40, 200 );
	SetAllScales( flScale );
	SetAllOrigins( vecTarget );
	SetAllNormals( -MainViewForward() );
	SetRgba( 0, 255, 255, 255, (((int)(gpGlobals->curtime * 10)) & 1 ? 200 : 0) );
	return true;
}


//-----------------------------------------------------------------------------
// C_PasstimeBallPredictionReticle
//-----------------------------------------------------------------------------
ConVar pf_ball_floor_enabled("pf_ball_floor_enabled", "1", FCVAR_ARCHIVE, "Enables/disables the floor indicator sprite.");

void C_PasstimeBallPredictionReticle::InitializeSprites()
{
    const char* pFloorSprite = "reticles/pf";
	const char* pFloorSpriteOuter = "reticles/b2";

    AddSprite(CreateReticleSprite(pFloorSprite, 128, 200)); // Floor indicator sprite, inner section
	AddSprite(CreateReticleSprite(pFloorSpriteOuter, 128, 0)); // Floor indicator sprite, outer section
}

C_PasstimeBallPredictionReticle::C_PasstimeBallPredictionReticle()
{
    InitializeSprites();
}

bool C_PasstimeBallPredictionReticle::Update()
{
    if (!g_pPasstimeLogic || !g_pPasstimeLogic->GetBall() || !pf_ball_floor_enabled.GetBool())
    {
        return false;
    }

    C_BaseEntity* pBallEntity = g_pPasstimeLogic->GetBall();
    C_PasstimeBall* pBall = dynamic_cast<C_PasstimeBall*>(pBallEntity);
    if (pBall && pBall->GetCarrier())
    {
        // Ball is being carried, don't show the floor indicator
        return false;
    }

    static Vector vBallSpawnPos;
    static bool bSpawnPosSet = false;

	if (pBall && !pBall->GetCarrier() && pBall->IsEffectActive(EF_NODRAW) && bSpawnPosSet) {
		bSpawnPosSet = false;
	}

    if (!bSpawnPosSet && pBall && !pBall->IsEffectActive(EF_NODRAW)) {
        vBallSpawnPos = pBall->WorldSpaceCenter();
        bSpawnPosSet = true;
    }

    if (pBall) {
        Vector velocity = pBall->GetAbsVelocity();
        float speed = velocity.Length();
		Vector ballPos2D = pBall->WorldSpaceCenter();
		Vector spawnPos2D = vBallSpawnPos;
		ballPos2D.z = 0;
		spawnPos2D.z = 0;
		float distFromSpawn = (ballPos2D - spawnPos2D).Length();

        // If the ball is close to spawn and barely moving, suppress indicator
        if (speed < 10.0f && distFromSpawn < 32.0f) {
            return false;
        }
    }

    C_BaseEntity* pTarget = 0;
    bool bHomingActive = false;
    bool bHaveTarget = g_pPasstimeLogic->GetBallReticleTarget(&pTarget, &bHomingActive);
    
    if (!bHaveTarget || !pTarget)
    {
        // We don't have a target, use neutral color
        return false;
    }
    
    // Get team color based on target (same as ball reticle)
    Color teamColor = GetTeamColor(pTarget->GetTeamNumber());

    Vector ballPosition = pBall->WorldSpaceCenter();
    
    // Trace down to find the floor
    trace_t tr;
    UTIL_TraceLine(ballPosition, ballPosition - Vector(0, 0, 5000), MASK_SOLID, pBall, COLLISION_GROUP_NONE, &tr);
    
    if (tr.fraction < 1.0f)
    {
        Vector floorPos = tr.endpos + Vector(0, 0, 1); // Slightly above the floor to prevent z-fighting
        
        SetAllOrigins(floorPos);
        SetAllNormals(tr.plane.normal); // Orient sprite to floor normal
        
        float distFromFloor = (ballPosition - floorPos).Length();
		float alpha = 255.0f; 
		alpha = RemapValClamped(distFromFloor, 16.0f, 1000.0f, 0, 255);
		SetAllAlphas(alpha);
        
        SetRgba(0, teamColor.r(), teamColor.g(), teamColor.b(), alpha);
		SetRgba(1, 255, 255, 255, alpha);

		float scale;
		if (distFromFloor <= 1000.0f) {
			scale = RemapValClamped(distFromFloor, 16.0f, 1000.0f, 1.0f, 0.1f) * 128.0f;
		} else {
			scale = RemapValClamped(distFromFloor, 1000.0f, 8192.0f, 0.1f, 0.05f) * 128.0f;
		}

		SetScale(1, 128.0f);
		SetScale(0, scale);
        
        return true;
    }
    
    return false;
}

void C_PasstimeBallPredictionReticle::ReloadSprites()
{
	C_PasstimeReticle::ReloadSprites();
	InitializeSprites(); 
}
//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CFXQuad *CreateReticleSprite( const char *pModelName, float flScale, float flSpinSpeed )
{
	FXQuadData_t q;
	memset(&q, 0, sizeof(q));
	q.m_Color.Init(1,1,1);
	q.m_flDeltaYaw = flSpinSpeed;
	q.m_flDieTime = FLT_MAX;
	q.m_flEndAlpha = 1;
	q.m_flEndScale = flScale;
	q.m_flLifeTime = 0;
	q.m_flScaleBias = 0;
	q.m_flStartAlpha = 1;
	q.m_flStartScale = flScale;
	q.m_flYaw = 180;
	q.SetMaterial( pModelName );
	q.m_uiFlags = 0;
	q.m_vecNormal.Init(1,0,0);
	q.m_vecOrigin.Init(0,0,0);
	CFXQuad *pQuad = new CFXQuad(q);
	clienteffects->AddEffect( pQuad );
	return pQuad;
}

void OnCrosshairSettingsChanged(IConVar* pConVar, const char* pOldValue, float flOldValue)
{
	const char* pConVarName = pConVar->GetName();

    if (strcmp(pConVarName, "pf_crosshair_inner_file") == 0 || 
        strcmp(pConVarName, "pf_crosshair_outer_file") == 0 ||
        strcmp(pConVarName, "pf_crosshair_inner_scale") == 0 ||
        strcmp(pConVarName, "pf_crosshair_outer_scale") == 0 ||
        strcmp(pConVarName, "pf_crosshair_outer_spinspeed") == 0)
    {
		PrecacheReticleMaterial(((ConVar*)pConVar)->GetString());
		g_BounceReticleDirty = true;
		ReloadBounceReticle();
    }

	else if (strcmp(pConVarName, "pf_teamicons_red_file") == 0 || 
	strcmp(pConVarName, "pf_teamicons_blu_file") == 0)
	{
		ReloadAllPlayerReticles();
	}

	else if (strcmp(pConVarName, "pf_crosshair_teammate_inner") == 0 || 
	strcmp(pConVarName, "pf_crosshair_teammate_outer_1") == 0 ||
	strcmp(pConVarName, "pf_crosshair_teammate_outer_2") == 0)
	{
		ReloadAllPassReticles();
	}

	else if (strcmp(pConVarName, "pf_ball_outline_1_file") == 0 ||
	strcmp(pConVarName, "pf_ball_outline_2_file") == 0)
	{
		ReloadBallReticle();
	}
	else if (strcmp(pConVarName, "pf_crosshair_inner_color") == 0 || 
		strcmp(pConVarName, "pf_crosshair_outer_color") == 0)
	{
		int r = 200, g = 200, b = 200;
		const char *value = ((ConVar*)pConVar)->GetString();
		sscanf( value, "%d %d %d", &r, &g, &b );
		
		// Clamp values
		r = clamp( r, 0, 255 );
		g = clamp( g, 0, 255 );
		b = clamp( b, 0, 255 );
	}

}

void ReloadAllPlayerReticles()
{
	if (!g_pPasstimeLogic)
	{
		return;
	}

    for (int i = 1; i <= MAX_PLAYERS; ++i)
    {
        C_TFPlayer* pPlayer = ToTFPlayer(UTIL_PlayerByIndex(i));
        if (pPlayer && pPlayer->GetPasstimePlayerReticle())
        {
            pPlayer->GetPasstimePlayerReticle()->ReloadSprites();
        }
    }
}

void ReloadAllPassReticles()
{
	if (!g_pPasstimeLogic)
	{
		return;
	}

    if (g_pPasstimeLogic->GetPassReticle())
    {
        g_pPasstimeLogic->GetPassReticle()->ReloadSprites();
    }
}

void ReloadBallReticle()
{
	if (!g_pPasstimeLogic)
	{
		return;
	}

	if (g_pPasstimeLogic->GetBallReticle())
	{
		g_pPasstimeLogic->GetBallReticle()->ReloadSprites();
	}
}

void PrecacheReticleMaterial(const char* pMaterialName)
{
    if (!pMaterialName || strlen(pMaterialName) == 0)
        return;

    static char fullPath[256];
    const char* defaultFolder = "reticles/";

	if (strnicmp(pMaterialName, "passtime/", 9) != 0 && 
        strnicmp(pMaterialName, defaultFolder, strlen(defaultFolder)) != 0)
    {
        snprintf(fullPath, sizeof(fullPath), "%s%s", defaultFolder, pMaterialName);
        pMaterialName = fullPath;
    }

    // Precache the material
    PrecacheMaterial(pMaterialName);
    DevMsg("Precached reticle material: %s\n", pMaterialName);
}

void PrecacheAllReticleMaterials()
{
    PrecacheReticleMaterial("passtime/hud/passtime_ball_reticle_piece_1");
    PrecacheReticleMaterial("passtime/hud/passtime_ball_reticle_piece_2");
    PrecacheReticleMaterial("passtime/hud/passtime_ball_reticle_passlock");
    PrecacheReticleMaterial("passtime/hud/passtime_teamicon_red");
    PrecacheReticleMaterial("passtime/hud/passtime_teamicon_blue");
    PrecacheReticleMaterial(pf_ball_outline_1_file.GetString());
    PrecacheReticleMaterial(pf_ball_outline_2_file.GetString());
    PrecacheReticleMaterial(pf_crosshair_teammate_inner.GetString());
	PrecacheReticleMaterial(pf_crosshair_teammate_outer_1.GetString());
	PrecacheReticleMaterial(pf_crosshair_teammate_outer_2.GetString());
	PrecacheReticleMaterial(pf_crosshair_inner_file.GetString());
	PrecacheReticleMaterial(pf_crosshair_outer_file.GetString());
}

#ifdef CLIENT_DLL
void ReloadBounceReticle()
{
	auto* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pLocalPlayer)
    {
        return;
    }
    C_PasstimeGun* pWeapon = dynamic_cast<C_PasstimeGun*>(pLocalPlayer->GetActiveWeapon());
    if (pWeapon && pWeapon->GetBounceReticle())
    {
        pWeapon->GetBounceReticle()->ReloadSprites();
		g_BounceReticleDirty = false;
    }
	else
	{
		g_BounceReticleDirty = true;
	}
}
#endif
