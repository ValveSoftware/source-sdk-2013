//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Auto Repair
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf/halloween/merasmus/merasmus.h"
#include "tf/halloween/merasmus/merasmus_dancer.h"
#include "tf_gamerules.h"
#include "tf_weapon_jar.h"
#include "tf_wheel_of_doom.h"


LINK_ENTITY_TO_CLASS( wheel_of_doom, CWheelOfDoom );

// Data Description
BEGIN_DATADESC( CWheelOfDoom )

	// Keyfields
	DEFINE_KEYFIELD( m_flDuration, FIELD_FLOAT, "effect_duration" ),
	DEFINE_KEYFIELD( m_bHasSpiral, FIELD_BOOLEAN, "has_spiral" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "Spin", Spin ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ClearAllEffects", ClearAllEffects ),

	// Outputs
	DEFINE_OUTPUT( m_EffectApplied, "OnEffectApplied" ),
	DEFINE_OUTPUT( m_EffectExpired, "OnEffectExpired" ),

END_DATADESC()

extern ConVar sv_gravity;

#define WHEEL_SPIN_TIME 5.75f
#define	WHEEL_SPIN_TIME_BIAS 0.3f
#define	WHEEL_FASTEST_SPIN_RATE 0.1f
#define	WHEEL_SLOWEST_SPIN_RATE 0.55f

#define	WHEEL_SPIRAL_GROW_RATE 0.55f
#define	WHEEL_SPIRAL_SHRINK_RATE 0.55f


#define EFFECT_WHAMMY	1<<0
#define EFFECT_DOES_NOT_REAPPLY_ON_SPAWN 1<<1


class CWheelOfDoomSpiral : public CBaseAnimating
{
	DECLARE_CLASS( CWheelOfDoom, CBaseAnimating );
	DECLARE_DATADESC();

public:
	CWheelOfDoomSpiral()
	{
		m_flScale = 0.f;
		SetModelScale( 0.f );
	}

	virtual void Spawn()
	{
		SetThink( NULL );
	}

	virtual void Precache()
	{
		PrecacheModel( "models/props_lakeside_event/wof_plane2.mdl" );
	}

	void GrowAndBecomeVisible()
	{
		RemoveEffects( EF_NODRAW );

		SetThink( &CWheelOfDoomSpiral::GrowThink );
		SetNextThink( gpGlobals->curtime );
	}

	void ShrinkAndHide()
	{
		SetThink( &CWheelOfDoomSpiral::ShrinkThink );
		SetNextThink( gpGlobals->curtime );
	}

private:

	void GrowThink()
	{
		// Grow ourselves over time
		m_flScale += WHEEL_SPIRAL_GROW_RATE * gpGlobals->frametime;

		if( m_flScale >= 1.f )
		{
			m_flScale = 1.f;
			SetThink( NULL );
		}

		SetModelScale( m_flScale );
		
		SetNextThink( gpGlobals->curtime );
	}

	void ShrinkThink()
	{
		// Shrink ourselves over time
		m_flScale -= WHEEL_SPIRAL_SHRINK_RATE * gpGlobals->frametime;

		if( m_flScale <= 0.f )
		{
			m_flScale = 0.f;
			AddEffects( EF_NODRAW );
			SetThink( NULL );
		}

		SetModelScale( m_flScale );
		
		SetNextThink( gpGlobals->curtime );
	}

	float m_flScale;
};

LINK_ENTITY_TO_CLASS( wheel_of_doom_spiral, CWheelOfDoomSpiral );

// Data Description
BEGIN_DATADESC( CWheelOfDoomSpiral )
END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef _WIN32
// Old code, stricter compiler
#pragma warning(disable : 4355) // warning C4355: 'this': used in base member initializer list
#endif
CWheelOfDoom::CWheelOfDoom( void ) :
	m_EffectManager( this ),
	m_pChosenEffect( NULL ),
	m_pSpiral( NULL ),
	m_flFinishBroadcastingEffectTime( 0.f )
{
	AddEffects( EF_NODRAW );

	RegisterEffect( new WOD_UberEffect() );
	RegisterEffect( new WOD_CritsEffect() );
	RegisterEffect( new WOD_SuperSpeedEffect() );
	RegisterEffect( new WOD_SuperJumpEffect() );
	RegisterEffect( new WOD_BigHeadEffect() );
	RegisterEffect( new WOD_SmallHeadEffect() );
	RegisterEffect( new WOD_LowGravityEffect() );
	RegisterEffect( new WOD_Dance(),		EFFECT_DOES_NOT_REAPPLY_ON_SPAWN );
	RegisterEffect( new WOD_Pee(),		EFFECT_WHAMMY | EFFECT_DOES_NOT_REAPPLY_ON_SPAWN );
	RegisterEffect( new WOD_Burn(),		EFFECT_WHAMMY | EFFECT_DOES_NOT_REAPPLY_ON_SPAWN );
	RegisterEffect( new WOD_Ghosts(),	EFFECT_WHAMMY | EFFECT_DOES_NOT_REAPPLY_ON_SPAWN );
}


CWheelOfDoom::~CWheelOfDoom( void )
{
	m_EffectManager.ClearEffects();
	m_vecEffects.PurgeAndDeleteElements();
}


void CWheelOfDoom::RegisterEffect( WOD_BaseEffect* pEffect, int nFlags )
{
	pEffect->SetListFlags( nFlags );
	m_vecEffects.AddToTail( pEffect );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::Precache( void )
{
	PrecacheModel( GetScreenModelName() );
	PrecacheModel( "models/props_lakeside_event/wof_plane2.mdl" );

	PrecacheScriptSound( "Halloween.WheelofFate" );
	PrecacheScriptSound( "Halloween.dance_howl" );
	PrecacheScriptSound( "Halloween.dance_loop" );

	PrecacheScriptSound( "Halloween.HeadlessBossAxeHitWorld" );
	PrecacheScriptSound( "Halloween.LightsOn" );
	PrecacheScriptSound( "Weapon_StickyBombLauncher.BoltForward" );
	PrecacheScriptSound( "TFPlayer.InvulnerableOff" );

	m_EffectManager.Precache();
}

const char* CWheelOfDoom::GetScreenModelName()
{
	return "models/props_lakeside_event/buff_plane.mdl";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::Spawn( void )
{
	Precache();

	SetModel( GetScreenModelName() );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );

	ListenForGameEvent( "player_spawn" );

	SetThink( &CWheelOfDoom::IdleThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( TFGameRules() != NULL )
	{
		TFGameRules()->ClearHalloweenEffectStatus();
	}

	if( m_bHasSpiral )
	{
		m_pSpiral = assert_cast<CWheelOfDoomSpiral*>( CreateEntityByName( "wheel_of_doom_spiral" ) );
		Assert( m_pSpiral );
		m_pSpiral->SetModel( "models/props_lakeside_event/wof_plane2.mdl" );
		m_pSpiral->AddEffects( EF_NODRAW );
		m_pSpiral->SetAbsOrigin( GetAbsOrigin() );
		m_pSpiral->SetAbsAngles( GetAbsAngles() );
		m_pSpiral->SetParent( this );
	}
}

void CWheelOfDoom::FireGameEvent( IGameEvent *gameEvent )
{
	if( FStrEq( gameEvent->GetName(), "player_spawn" ) )
	{
		const int nUserID = gameEvent->GetInt( "userid" );
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByUserId( nUserID ) );
		if( pPlayer )
		{
			m_EffectManager.ApplyAllEffectsToPlayer( pPlayer );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::IdleThink( void )
{
	if( m_EffectManager.UpdateAndClearExpiredEffects() )
	{
		AddEffects( EF_NODRAW );

		m_EffectExpired.FireOutput( this, this );

		// Clear the skin to blank
		SetSkin( 0 );

		// Clear the HUD
		TFGameRules()->ClearHalloweenEffectStatus();

		if( m_bHasSpiral )
		{
			// Make our spirals shrink down and hide themselves
			m_pSpiral->ShrinkAndHide();
			FOR_EACH_VEC( m_vecOtherWODs, i )
			{
				if( m_vecOtherWODs[i]->m_bHasSpiral )
				{
					m_vecOtherWODs[i]->m_pSpiral->ShrinkAndHide();
				}
			}
		}
	}

	//Next update
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CWheelOfDoom::SetSkin( int nSkin )
{
	m_nSkin = nSkin;

	FOR_EACH_VEC( m_vecOtherWODs, i )
	{
		m_vecOtherWODs[i]->m_nSkin = nSkin;
	}
}

void CWheelOfDoom::SetScale( float flScale )
{
	SetModelScale( flScale );

	FOR_EACH_VEC( m_vecOtherWODs, i )
	{
		m_vecOtherWODs[i]->SetModelScale( flScale );
	}
}


void CWheelOfDoom::PlaySound( const char* pszSound )
{
	EmitSound( pszSound );

	FOR_EACH_VEC( m_vecOtherWODs, i )
	{
		m_vecOtherWODs[i]->EmitSound( pszSound );
	}
}


void CWheelOfDoom::SpinThink( void )
{
	if( m_EffectManager.UpdateAndClearExpiredEffects() )
	{
		m_EffectExpired.FireOutput( this, this );

		// Clear the HUD
		TFGameRules()->ClearHalloweenEffectStatus();
	}

	// Are we done spinning?
	if( gpGlobals->curtime > m_flStopSpinTime )
	{
		if( gpGlobals->curtime > m_flStopSpinTime + 1.f )
		{
			//PlaySound( "Halloween.LightsOn" );
			SetScale( 1.f );

			m_EffectApplied.FireOutput( this, this );

			// Apply the effect!
			SetSkin( m_EffectManager.AddEffect( m_pChosenEffect, m_flDuration ) );

			SetThink( &CWheelOfDoom::IdleThink );

			m_flStopSpinTime = 0.f;
			m_flNextTickTime = 0.f;
			m_pChosenEffect = NULL;
		}
	}
	// Is it time for another tick of the wheel?
	else if( gpGlobals->curtime > m_flNextTickTime )
	{
		int nRandSkin = RandomInt( 1, EFFECT_COUNT-1 );
		if( nRandSkin == m_nSkin )
			++nRandSkin;

		// Roll over to 1.  0 is blank so skip that one.
		if( nRandSkin == EFFECT_COUNT )
			nRandSkin = 1;

		SetSkin( nRandSkin );
		SetScale( RemapVal( CalcSpinCompletion(), 0.f, 1.f, 0.3f, 0.9f) );

		m_flNextTickTime = CalcNextTickTime();
	}
	
	//Is it time for Merasmus to announce the spin?
	if (gpGlobals->curtime > m_flNextAnnounceTime && !m_bAnnounced)
	{
		m_bAnnounced = true;
		CMerasmus* pMerasmus = assert_cast< CMerasmus* >( TFGameRules()->GetActiveBoss() );
		if (pMerasmus)
		{
			pMerasmus->PlayHighPrioritySound("Halloween.MerasmusWheelSpin");
		}
		else
		{
			TFGameRules()->BroadcastSound(255,"Halloween.MerasmusWheelSpin");
		}
	}
	
	// Next update
	SetNextThink( gpGlobals->curtime );
}

float CWheelOfDoom::CalcNextTickTime() const
{
	float flProgress = CalcSpinCompletion();
	float flBias = Bias( flProgress, WHEEL_SPIN_TIME_BIAS );
	return gpGlobals->curtime + (( 1 - flBias ) * WHEEL_FASTEST_SPIN_RATE) + (flBias) * WHEEL_SLOWEST_SPIN_RATE;
}


float CWheelOfDoom::CalcSpinCompletion() const
{
	const float& flDuration = WHEEL_SPIN_TIME;
	return ( flDuration - (m_flStopSpinTime - gpGlobals->curtime) ) / flDuration;
}

void CWheelOfDoom::StartSpin( void )
{
	RemoveEffects( EF_NODRAW );
	m_vecOtherWODs.Purge();
	CBaseEntity *pOther = gEntList.FindEntityByClassname( NULL, "wheel_of_doom" );

	// Play the sound of the wheel starting to spin
	if( TFGameRules() )
	{
		TFGameRules()->BroadcastSound( 255, "Halloween.WheelofFate" );
	}

	// Gather all of the other WheelofDoom entities so we can control their screens as we spin
	while ( pOther )
	{
		if( pOther != this )
		{
			m_vecOtherWODs.AddToTail( dynamic_cast<CWheelOfDoom*>( pOther ) );
		}

		// Change the target over
		pOther = gEntList.FindEntityByClassname( pOther, "wheel_of_doom" );
	}

	if( m_bHasSpiral )
	{
		// Make our spirals show up and grow
		m_pSpiral->GrowAndBecomeVisible();
		FOR_EACH_VEC( m_vecOtherWODs, i )
		{
			if( m_vecOtherWODs[i]->m_bHasSpiral )
			{
				m_vecOtherWODs[i]->m_pSpiral->GrowAndBecomeVisible(); 
			}
		}
	}

	// Setup spin logic
	m_flStopSpinTime = gpGlobals->curtime + WHEEL_SPIN_TIME; 
	m_flNextTickTime = CalcNextTickTime();
	m_flNextAnnounceTime = gpGlobals->curtime + 1.6; 
	m_bAnnounced = false;

	m_flFinishBroadcastingEffectTime = m_flStopSpinTime + 10.f;

	SetThink( &CWheelOfDoom::SpinThink );
	SetNextThink( gpGlobals->curtime );
}


void CWheelOfDoom::Spin( inputdata_t& inputdata )
{
	// Remember which effect was chosen so we can apply it once the spinning is done
	m_pChosenEffect = GetRandomEffectWithFlags();
	StartSpin();
}


CWheelOfDoom::WOD_BaseEffect* CWheelOfDoom::GetRandomEffectWithFlags()
{
	int nNumWhammys = 0;
	int nNumGoodEffects = 0;
	CUtlVector<WOD_BaseEffect*> vecMatchingEffects;

	// Collect all of the effects that match our criteria.
	// Buffs in the front of the vector and whammys on the end
	FOR_EACH_VEC( m_vecEffects, i )
	{
		WOD_BaseEffect* pEffect = m_vecEffects[i];

		if( pEffect->GetListFlags() & EFFECT_WHAMMY )
		{
			// Tally up all the whammys
			++nNumWhammys;
			vecMatchingEffects.AddToTail( pEffect );
		}
		else
		{
			++nNumGoodEffects;
			vecMatchingEffects.AddToHead( pEffect );
		}	
	}

	// No matching effects.  Return null
	if( vecMatchingEffects.Count() == 0 )
	{
		return NULL;
	}

	// No Whammies?  Just return a random one
	if( nNumWhammys == 0 )
	{
		return vecMatchingEffects[RandomInt( 0, nNumGoodEffects-1 )];
	}

	// Given n good buffs, give a 1/n+1 chance of hitting a whammy
	int nRand = RandomInt( 0, nNumGoodEffects );

	// Rolled a whammy!
	if( nRand == nNumGoodEffects )
	{
		//Roll again to find out which whammy we get
		nRand = nNumGoodEffects + RandomInt( 0, nNumWhammys-1 );
	}

	return vecMatchingEffects[nRand];
}


void CWheelOfDoom::ClearAllEffects( inputdata_t& inputdata )
{
	m_EffectManager.ClearEffects();
}


bool CWheelOfDoom::IsDoneBoardcastingEffectSound() const
{
	return gpGlobals->curtime > m_flFinishBroadcastingEffectTime;
}


void CWheelOfDoom::DBG_ApplyEffectByName( const char* pszEffectName )
{
	FOR_EACH_VEC( m_vecEffects, i )
	{
		WOD_BaseEffect* pEffect = m_vecEffects[i];
		if( FStrEq( pEffect->GetName(), pszEffectName ) )
		{
			m_EffectManager.AddEffect( pEffect, m_flDuration );
		}
	}
}

CWheelOfDoom::WOD_BaseEffect::WOD_BaseEffect()
{
	m_flExpireTime = 0;
	m_pszEffectAnnouncementSound = NULL;
	m_pszName = NULL;
	m_iListFlags = 0;
}

void CWheelOfDoom::WOD_BaseEffect::InitEffect( float flDefaultDuration )
{
	m_flExpireTime = gpGlobals->curtime + flDefaultDuration;
}

void CWheelOfDoom::WOD_BaseEffect::SetListFlags( int iFlags )
{
	m_iListFlags = iFlags;
}

CWheelOfDoom::EffectManager::~EffectManager()
{
}

int CWheelOfDoom::EffectManager::AddEffect( WOD_BaseEffect* pEffect, float flDefaultDuration )
{
	Assert( pEffect );

	EffectData_t data;
	data.m_pWheel = m_pWheel;
	CollectPlayers( &data.m_vecPlayers );
	pEffect->InitEffect( flDefaultDuration );
	pEffect->ActivateEffect( data );
	
	float flExpireDiff = pEffect->m_flExpireTime - gpGlobals->curtime;
	DevMsg( "[WHEEL OF DOOM]\t Activating: \"%s\" set to expire in %3.2fs\n", pEffect->m_pszName, flExpireDiff );

	if( TFGameRules() )
	{
		CMerasmus* pMerasmus = assert_cast< CMerasmus* >( TFGameRules()->GetActiveBoss() );
		if (pMerasmus)
		{
			pMerasmus->PlayHighPrioritySound(pEffect->m_pszEffectAnnouncementSound);
		}
		else
		{
			TFGameRules()->BroadcastSound(255,pEffect->m_pszEffectAnnouncementSound);
		}
		// Update the HUD
		TFGameRules()->SetHalloweenEffectStatus( int(pEffect->m_nSkin), flExpireDiff );
		SpeakMagicConceptToAllPlayers(pEffect->m_pszEffectAnnouncementSound);
	}

	// Remember this effect
	m_vecActiveEffects.AddToTail( pEffect );

	return pEffect->m_nSkin;
}

void CWheelOfDoom::EffectManager::ApplyAllEffectsToPlayer( CTFPlayer* pPlayer )
{
	EffectData_t data;
	data.m_pWheel = m_pWheel;
	data.m_vecPlayers.AddToTail( pPlayer );

	FOR_EACH_VEC( m_vecActiveEffects, i )
	{
		if( m_vecActiveEffects[i]->GetListFlags() & EFFECT_DOES_NOT_REAPPLY_ON_SPAWN )
			continue;

		m_vecActiveEffects[i]->ActivateEffect( data );
	}
}

void CWheelOfDoom::EffectManager::ClearEffects()
{
	FOR_EACH_VEC( m_vecActiveEffects, i )
	{
		DevMsg( "[WHEEL OF DOOM]\t Deactivating: %s\n", m_vecActiveEffects[i]->m_pszName );

		EffectData_t data;
		data.m_pWheel = m_pWheel;
		CollectPlayers( &data.m_vecPlayers );
		
		m_vecActiveEffects[i]->DeactivateEffect( data );
	}

	m_vecActiveEffects.Purge();
}

bool CWheelOfDoom::EffectManager::UpdateAndClearExpiredEffects()
{
	bool bEffectExpired = false;

	EffectData_t data;
	data.m_pWheel = m_pWheel;
	CollectPlayers( &data.m_vecPlayers );

	FOR_EACH_VEC_BACK( m_vecActiveEffects, i )
	{
		// Check if the effect is expired.  If so, run its DeactivateEffect and remove it
		WOD_BaseEffect* pEffect = m_vecActiveEffects[i];
		if( gpGlobals->curtime > pEffect->m_flExpireTime )
		{
			DevMsg( "[WHEEL OF DOOM]\t Effect expired: %s\n", pEffect->m_pszName );
			bEffectExpired = true;

			pEffect->DeactivateEffect( data );

			m_vecActiveEffects.Remove( i );
		}
		else // If it's not expired, then update
		{
			pEffect->UpdateEffect( data );	
		}
	}

	return bEffectExpired;
}


void CWheelOfDoom::EffectManager::Precache()
{
	FOR_EACH_VEC( m_vecActiveEffects, i )
	{
		PrecacheScriptSound( m_vecActiveEffects[i]->m_pszEffectAnnouncementSound );
	}
}

void CWheelOfDoom::SpeakMagicConceptToAllPlayers( const char* pszEffect )
{
	int iConcept = -1;

	if ( !V_stricmp( pszEffect, "Halloween.MerasmusWheelBigHead") )
	{
		iConcept = MP_CONCEPT_MAGIC_BIGHEAD;
	}
	else if ( !V_stricmp( pszEffect, "Halloween.MerasmusWheelShrunkHead") )
	{
		iConcept = MP_CONCEPT_MAGIC_SMALLHEAD;
	}
	else if ( !V_stricmp( pszEffect, "Halloween.MerasmusWheelGravity") )
	{
		iConcept = MP_CONCEPT_MAGIC_GRAVITY;
	}
	else if ( (!V_stricmp( pszEffect, "Halloween.MerasmusWheelCrits")) || (!V_stricmp( pszEffect, "Halloween.MerasmusWheelUber")) || (!V_stricmp( pszEffect, "Halloween.MerasmusWheelSuperSpeed")))
	{
		iConcept = MP_CONCEPT_MAGIC_GOOD;
	}
	else if ( !V_stricmp( pszEffect, "Halloween.MerasmusWheelDance") )
	{
		iConcept = MP_CONCEPT_MAGIC_DANCE;
	}

	if (iConcept >= 0)
	{
		CUtlVector< CTFPlayer * > playerVector;
		CollectPlayers( &playerVector );
		FOR_EACH_VEC( playerVector, i )
		{
			playerVector[i]->SpeakConceptIfAllowed(iConcept);
		}
	}
}



void CWheelOfDoom::ApplyAttributeToAllPlayers( const char* pszAttribName, float flValue )
{
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector );
	FOR_EACH_VEC( playerVector, i )
	{
		ApplyAttributeToPlayer( playerVector[i], pszAttribName, flValue );
	}
}

void CWheelOfDoom::ApplyAttributeToPlayer( CTFPlayer* pPlayer, const char* pszAttribName, float flValue )
{
	Assert( pPlayer );

	const CEconItemAttributeDefinition *pDef = GetItemSchema()->GetAttributeDefinitionByName( pszAttribName );

	pPlayer->GetAttributeList()->SetRuntimeAttributeValue( pDef, flValue );
	pPlayer->TeamFortress_SetSpeed();
}

void CWheelOfDoom::RemoveAttributeFromAllPlayers( const char* pszAttributeName )
{
	const CEconItemAttributeDefinition *pDef = GetItemSchema()->GetAttributeDefinitionByName( pszAttributeName );

	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector );
	FOR_EACH_VEC( playerVector, i )
	{
		CTFPlayer* pPlayer = playerVector[i];
		pPlayer->GetAttributeList()->RemoveAttribute( pDef );
	}
}

void CWheelOfDoom::RemoveAttributeFromPlayer( CTFPlayer* pPlayer, const char* pszAttribName )
{
	const CEconItemAttributeDefinition *pDef = GetItemSchema()->GetAttributeDefinitionByName( pszAttribName );
	pPlayer->GetAttributeList()->RemoveAttribute( pDef );
}

void CWheelOfDoom::WOD_UberEffect::InitEffect( float flDefaultExpireTime )
{
	m_flExpireTime = gpGlobals->curtime + Min( flDefaultExpireTime, 10.f );
}

void CWheelOfDoom::WOD_UberEffect::ActivateEffect( EffectData_t& data )
{
	float flDuration = m_flExpireTime - gpGlobals->curtime;

	FOR_EACH_VEC( data.m_vecPlayers, i )
	{
		CTFPlayer* pPlayer = data.m_vecPlayers[i];
		pPlayer->m_Shared.AddCond( TF_COND_INVULNERABLE, flDuration );
	}
}


void CWheelOfDoom::WOD_CritsEffect::ActivateEffect( EffectData_t& data )
{
	float flDuration = m_flExpireTime - gpGlobals->curtime;

	FOR_EACH_VEC( data.m_vecPlayers, i )
	{
		CTFPlayer* pPlayer = data.m_vecPlayers[i];
		pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_PUMPKIN, flDuration );
	}
}


void CWheelOfDoom::WOD_SuperSpeedEffect::ActivateEffect( EffectData_t& data )
{
	FOR_EACH_VEC( data.m_vecPlayers, i )
	{
		CTFPlayer* pPlayer = data.m_vecPlayers[i];
		CWheelOfDoom::ApplyAttributeToPlayer( pPlayer, "major move speed bonus", 2.f );
	}
}


void CWheelOfDoom::WOD_SuperSpeedEffect::DeactivateEffect( EffectData_t& data )
{
	FOR_EACH_VEC( data.m_vecPlayers, i )
	{
		CTFPlayer* pPlayer = data.m_vecPlayers[i];
		CWheelOfDoom::RemoveAttributeFromPlayer( pPlayer, "major move speed bonus" );
		// Recalc our max speed
		pPlayer->TeamFortress_SetSpeed();
	}
}


void CWheelOfDoom::WOD_SuperJumpEffect::ActivateEffect( EffectData_t& data )
{
	FOR_EACH_VEC( data.m_vecPlayers, i )
	{
		CTFPlayer* pPlayer = data.m_vecPlayers[i];
		CWheelOfDoom::ApplyAttributeToPlayer( pPlayer, "major increased jump height", 3.f );
		CWheelOfDoom::ApplyAttributeToPlayer( pPlayer, "cancel falling damage", 1.f );
	}
}

void CWheelOfDoom::WOD_SuperJumpEffect::DeactivateEffect( EffectData_t& data )
{
	FOR_EACH_VEC( data.m_vecPlayers, i )
	{
		CTFPlayer* pPlayer = data.m_vecPlayers[i];
		CWheelOfDoom::RemoveAttributeFromPlayer( pPlayer, "major increased jump height" );
		CWheelOfDoom::RemoveAttributeFromPlayer( pPlayer, "cancel falling damage" );
	}
}


void CWheelOfDoom::WOD_BigHeadEffect::ActivateEffect( EffectData_t& data )
{
	FOR_EACH_VEC( data.m_vecPlayers, i )
	{
		CTFPlayer* pPlayer = data.m_vecPlayers[i];
		ApplyAttributeToPlayer( pPlayer, "voice pitch scale", 0.85f );
		ApplyAttributeToPlayer( pPlayer, "head scale", 3.f );
	}
}

void CWheelOfDoom::WOD_BigHeadEffect::DeactivateEffect( EffectData_t& data )
{
	FOR_EACH_VEC( data.m_vecPlayers, i )
	{
		CTFPlayer* pPlayer = data.m_vecPlayers[i];
		RemoveAttributeFromPlayer( pPlayer, "voice pitch scale" );
		RemoveAttributeFromPlayer( pPlayer, "head scale" );
	}
}


void CWheelOfDoom::WOD_SmallHeadEffect::ActivateEffect( EffectData_t& data )
{
	FOR_EACH_VEC( data.m_vecPlayers, i )
	{
		CTFPlayer* pPlayer = data.m_vecPlayers[i];
		ApplyAttributeToPlayer( pPlayer, "voice pitch scale", 1.3f );
		ApplyAttributeToPlayer( pPlayer, "head scale", 0.5f );
	}
}

void CWheelOfDoom::WOD_SmallHeadEffect::DeactivateEffect( EffectData_t& data )
{
	FOR_EACH_VEC( data.m_vecPlayers, i )
	{
		CTFPlayer* pPlayer = data.m_vecPlayers[i];
		RemoveAttributeFromPlayer( pPlayer, "voice pitch scale" );
		RemoveAttributeFromPlayer( pPlayer, "head scale" );
	}
}



void CWheelOfDoom::WOD_LowGravityEffect::ActivateEffect( EffectData_t& /*data*/ )
{
	if ( TFGameRules() )
	{
		TFGameRules()->SetGravityMultiplier( 0.25f );
	}
}

void CWheelOfDoom::WOD_LowGravityEffect::DeactivateEffect( EffectData_t& /*data*/ )
{
	if ( TFGameRules() )
	{
		TFGameRules()->SetGravityMultiplier( 1.0f );
	}
}


void CWheelOfDoom::WOD_Pee::ActivateEffect( EffectData_t& data )
{
	m_vecClouds.Purge();

	// Collect all of the "spawn_cloud" entities
	CBaseEntity *pOther = gEntList.FindEntityByName( NULL, "spawn_cloud" );
	while( pOther )
	{
		m_vecClouds.AddToTail( pOther );	
		pOther = gEntList.FindEntityByName( pOther, "spawn_cloud" );
	}

	m_flNextPeeTime = gpGlobals->curtime + 0.25f;
}

void CWheelOfDoom::WOD_Pee::UpdateEffect( EffectData_t& data )
{
	if( gpGlobals->curtime < m_flNextPeeTime || m_vecClouds.Count() == 0 )
		return;

	m_flNextPeeTime = gpGlobals->curtime + RandomFloat(0.2f, 0.5f);

	// Choose one at random
	int nRandIndex = RandomInt( 0, m_vecClouds.Count() - 1 );
	CBaseEntity* pCloud = m_vecClouds[nRandIndex];
	
	// Get a random point within the brush
	Vector vWorldMins = pCloud->WorldAlignMins();
	Vector vWorldMaxs = pCloud->WorldAlignMaxs();
	Vector vBoxMin = pCloud->GetAbsOrigin() + vWorldMins;
	Vector vBoxMax	= pCloud->GetAbsOrigin() + vWorldMaxs;
	Vector vRandomPos(	RandomFloat( vBoxMin.x, vBoxMax.x ),
						RandomFloat( vBoxMin.y, vBoxMax.y ),
						RandomFloat( vBoxMin.z, vBoxMax.z ) );
	
	// Drop some pee
	CTFProjectile_Jar *pGrenade = static_cast<CTFProjectile_Jar*>( CBaseEntity::CreateNoSpawn( "tf_projectile_jar", vRandomPos, QAngle(0,0,0), NULL ) );
	DispatchSpawn( pGrenade );

	// Random angular impulse
	Vector angImpulse(	RandomFloat( -300.f, 300.f ),
						RandomFloat( -300.f, 300.f ),
						RandomFloat( -300.f, 300.f ) );

	// Add some spin
	IPhysicsObject *pPhysicsObject = pGrenade->VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &vec3_origin, &angImpulse );
	}
}


void CWheelOfDoom::WOD_Burn::InitEffect( float flDefaultDuration )
{
	m_flExpireTime = gpGlobals->curtime + 10.f;
}


void CWheelOfDoom::WOD_Burn::ActivateEffect( EffectData_t& data )
{
	FOR_EACH_VEC( data.m_vecPlayers, i )
	{
		CTFPlayer* pPlayer = data.m_vecPlayers[i];
		pPlayer->m_Shared.SelfBurn( Min( data.m_pWheel->GetDuration(), 10.f ) );
	}
}


void CWheelOfDoom::WOD_Ghosts::ActivateEffect( EffectData_t& data )
{
	if( TFGameRules() )
	{
		TFGameRules()->BeginHaunting( 4, data.m_pWheel->GetDuration() / 2.f, data.m_pWheel->GetDuration() );
	}
}

void CWheelOfDoom::WOD_Ghosts::DeactivateEffect( EffectData_t& data )
{
	FOR_EACH_VEC( data.m_vecPlayers, i )
	{
		CTFPlayer* pPlayer = data.m_vecPlayers[i];
		pPlayer->m_Shared.RemoveCond( TF_COND_BURNING );
	}
}

void CWheelOfDoom::WOD_Dance::InitEffect( float /*flDefaultExpireTime*/ )
{
	// Don't do the same order every time
	m_iCurrentMerasmusCreateInfo = RandomInt( 0, 1 );

	// Ignore the passed in expire time.  We want to have the buff for 5 seconds with 2 taunt cycles
	m_flExpireTime = gpGlobals->curtime + 8.f;
	m_flNextDanceTime = gpGlobals->curtime + 1.5f;

	m_vecDancers.PurgeAndDeleteElements();

	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TEAM_ANY, true );

	// We'll calculate the average position and place merasmus there
	float aMerasmusY[2] = { FLT_MAX, -FLT_MAX };
	float flMerasmusX = 0.0f, flMerasmusZ = 0.0f;
	int nNumPositions = 0;	// Keep a separate count for the average, since there's a case pOther can be NULL below

	FOR_EACH_VEC( playerVector, i )
	{
		CTFPlayer* pPlayer = playerVector[i];

		// Get our params
		const char* pzsTeam = pPlayer->GetTeamNumber() == TF_TEAM_RED ? "red" : "blue";
		int nInfoNumber = GetNumOFTeamDancing( pPlayer->GetTeamNumber() );

		// Cook up the name of the entity we want
		const char* pszTargetName = CFmtStr( "dance_teleport_%s%d", pzsTeam, nInfoNumber );
		// If we got it, then do the magic
		CBaseEntity *pOther = gEntList.FindEntityByName( NULL, pszTargetName );
		if( pOther )
		{
			Dancer_t* dancer = new Dancer_t();

			dancer->m_vecPos	= pOther->GetAbsOrigin();
			dancer->m_vecAngles = pOther->GetAbsAngles();
			dancer->m_hPlayer.Set( pPlayer );

			// Average in the player's position
			aMerasmusY[0] = MIN( aMerasmusY[0], dancer->m_vecPos.y );
			aMerasmusY[1] = MAX( aMerasmusY[1], dancer->m_vecPos.y );
			flMerasmusX += dancer->m_vecPos.x;
			flMerasmusZ = dancer->m_vecPos.z;
			++nNumPositions;

			// Slam them, for the jam
			SlamPosAndAngles( pPlayer, dancer->m_vecPos, dancer->m_vecAngles );

			// Force the halloween taunt.  This will force the player to stand still.
			pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_THRILLER );

			// Remember we're effecting this player
			m_vecDancers.AddToTail( dancer );
		}
	}

	// Average out the Merasmus position
	flMerasmusX /= nNumPositions;

	m_vecMerasmusDancerCreateInfos.AddToTail( MerasmusCreateInfo_t( Vector( flMerasmusX, aMerasmusY[0], flMerasmusZ ), QAngle( 0.0f, 90.0f, 0.0f ) ) );
	m_vecMerasmusDancerCreateInfos.AddToTail( MerasmusCreateInfo_t( Vector( flMerasmusX, aMerasmusY[1], flMerasmusZ ), QAngle( 0.0f,-90.0f, 0.0f ) ) );
}


void CWheelOfDoom::WOD_Dance::UpdateEffect( EffectData_t& /*data*/ )
{
	bool bShouldSlam = ( m_flNextDanceTime - gpGlobals->curtime ) < 0.2f;

	bool bShouldTaunt = false;
	if( gpGlobals->curtime > m_flNextDanceTime )
	{
		bShouldTaunt = true;
		m_flNextDanceTime = gpGlobals->curtime + 3.5f;
	}

	FOR_EACH_VEC_BACK( m_vecDancers, i )
	{
		Dancer_t* dancer = m_vecDancers[i];
		CTFPlayer* pPlayer = dancer->m_hPlayer.Get();

		// This players is no more (disconnected).  Forget about them.
		if( pPlayer == NULL )
		{
			delete dancer;
			m_vecDancers.Remove(i);
			continue;
		}

		if( bShouldTaunt )
		{
			pPlayer->Taunt();

		}
			
		if( bShouldSlam )
		{
			// Slam them, for the jam
			SlamPosAndAngles( pPlayer, dancer->m_vecPos, dancer->m_vecAngles );
		}
	}

	// Party time.
	if ( bShouldTaunt )
	{
		if ( m_hMerasmusDancer )
		{
			m_hMerasmusDancer->Vanish();	// Poof!
		}

		// Create a new Merasmus
		const MerasmusCreateInfo_t &info = m_vecMerasmusDancerCreateInfos[ m_iCurrentMerasmusCreateInfo % m_vecMerasmusDancerCreateInfos.Count() ];
		m_hMerasmusDancer = (CMerasmusDancer *)CBaseEntity::Create( "merasmus_dancer", info.m_vecPos, info.m_vecAngles );

		if ( m_hMerasmusDancer )
		{
			m_hMerasmusDancer->Dance();
		}

		// Move to the next location
		m_iCurrentMerasmusCreateInfo = ( m_iCurrentMerasmusCreateInfo + 1 ) % m_vecMerasmusDancerCreateInfos.Count();
	}
}

void CWheelOfDoom::WOD_Dance::DeactivateEffect( EffectData_t& data )
{
	FOR_EACH_VEC( data.m_vecPlayers, i )
	{
		CTFPlayer* pPlayer = data.m_vecPlayers[i];
		pPlayer->m_Shared.RemoveCond( TF_COND_HALLOWEEN_THRILLER );
	}

	m_vecDancers.Purge();

	if ( m_hMerasmusDancer )
	{
		m_hMerasmusDancer->BlastOff();
		m_vecMerasmusDancerCreateInfos.Purge();
	}
}


int CWheelOfDoom::WOD_Dance::GetNumOFTeamDancing( int nTeam ) const
{
	int nCount = 0;
	FOR_EACH_VEC( m_vecDancers, i )
	{
		if( m_vecDancers[i]->m_hPlayer.Get()->GetTeamNumber() == nTeam )
			nCount++;
	}

	return nCount;
}

void CWheelOfDoom::WOD_Dance::SlamPosAndAngles( CTFPlayer* pPlayer, const Vector& vPos, const QAngle& vAng )
{
	// This calls: SetAbsOrigin(), SetAbsVelocity( vec3_origin ), SetLocalAngles(), SnapEyeAngles()
	pPlayer->Teleport( &vPos, &vAng, &vec3_origin );
	pPlayer->pl.v_angle = vAng;
}
