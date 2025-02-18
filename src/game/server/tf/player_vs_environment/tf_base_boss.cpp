//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#include "tf_gamerules.h"
#include "tf_base_boss.h"
#include "entity_currencypack.h"
#include "tf_gamestats.h"
#include "tf_player.h"

LINK_ENTITY_TO_CLASS( base_boss, CTFBaseBoss );

PRECACHE_REGISTER( base_boss );

IMPLEMENT_SERVERCLASS_ST( CTFBaseBoss, DT_TFBaseBoss)
	SendPropFloat( SENDINFO(m_lastHealthPercentage), 11, SPROP_NOSCALE, 0.0, 1.0 ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFBaseBoss )

	DEFINE_KEYFIELD( m_initialHealth, FIELD_INTEGER, "health" ),
	DEFINE_KEYFIELD( m_modelString, FIELD_STRING, "model" ),
	DEFINE_KEYFIELD( m_speed, FIELD_FLOAT, "speed" ),
	DEFINE_KEYFIELD( m_startDisabled, FIELD_INTEGER, "start_disabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeed", InputSetSpeed ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetStepHeight", InputSetStepHeight ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMaxJumpHeight", InputSetMaxJumpHeight ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetHealth", InputSetHealth ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxHealth", InputSetMaxHealth ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddHealth", InputAddHealth ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "RemoveHealth", InputRemoveHealth ),

	DEFINE_OUTPUT( m_outputOnHealthBelow90Percent,	"OnHealthBelow90Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow80Percent,	"OnHealthBelow80Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow70Percent,	"OnHealthBelow70Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow60Percent,	"OnHealthBelow60Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow50Percent,	"OnHealthBelow50Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow40Percent,	"OnHealthBelow40Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow30Percent,	"OnHealthBelow30Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow20Percent,	"OnHealthBelow20Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow10Percent,	"OnHealthBelow10Percent" ),

	DEFINE_OUTPUT( m_outputOnKilled, "OnKilled" ),

	DEFINE_THINKFUNC( BossThink ),

END_DATADESC()

BEGIN_ENT_SCRIPTDESC( CTFBaseBoss, NextBotCombatCharacter, "Team Fortress 2 Base Boss" )
	DEFINE_SCRIPTFUNC( SetResolvePlayerCollisions, "" )
END_SCRIPTDESC();

ConVar tf_base_boss_speed( "tf_base_boss_speed", "75", FCVAR_CHEAT );
ConVar tf_base_boss_max_turn_rate( "tf_base_boss_max_turn_rate", "25", FCVAR_CHEAT );

extern void HandleRageGain( CTFPlayer *pPlayer, unsigned int iRequiredBuffFlags, float flDamage, float fInverseRageGainScale );

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
float CTFBaseBossLocomotion::GetRunSpeed( void ) const
{
	CTFBaseBoss *boss = (CTFBaseBoss *)GetBot()->GetEntity();
	return boss->GetMaxSpeed();
}


//--------------------------------------------------------------------------------------
void CTFBaseBossLocomotion::FaceTowards( const Vector &target )
{
	CTFBaseBoss *pTank = static_cast< CTFBaseBoss* >( GetBot()->GetEntity() );

	const float deltaT = GetUpdateInterval();

	QAngle angles = pTank->GetLocalAngles();

	float desiredYaw = UTIL_VecToYaw( target - GetFeet() );

	float angleDiff = UTIL_AngleDiff( desiredYaw, angles.y );

	float deltaYaw = tf_base_boss_max_turn_rate.GetFloat() * deltaT;

	if ( angleDiff < -deltaYaw )
	{
		angles.y -= deltaYaw;
	}
	else if ( angleDiff > deltaYaw )
	{
		angles.y += deltaYaw;
	}
	else
	{
		angles.y += angleDiff;
	}

	Vector forward, right;
	pTank->GetVectors( NULL, &right, NULL );

	forward = CrossProduct( GetGroundNormal(), right );

	float desiredPitch = UTIL_VecToPitch( forward );

	angleDiff = UTIL_AngleDiff( desiredPitch, angles.x );

	float deltaPitch = tf_base_boss_max_turn_rate.GetFloat() * deltaT;

	if ( angleDiff < -deltaPitch )
	{
		angles.x -= deltaPitch;
	}
	else if ( angleDiff > deltaPitch )
	{
		angles.x += deltaPitch;
	}
	else
	{
		angles.x += angleDiff;
	}

	pTank->SetLocalAngles( angles );
	pTank->UpdateCollisionBounds();
}


//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
CTFBaseBoss::CTFBaseBoss()
{
	m_modelString = NULL_STRING;
	m_lastHealthPercentage = 1.0f;
	m_speed = tf_base_boss_speed.GetFloat();
	m_locomotor = new CTFBaseBossLocomotion( this );
	m_currencyValue = TF_BASE_BOSS_CURRENCY;
	m_initialHealth = 0;

	m_bResolvePlayerCollisions = true;
}


//--------------------------------------------------------------------------------------
CTFBaseBoss::~CTFBaseBoss()
{
	delete m_locomotor;
}


//--------------------------------------------------------------------------------------
void CTFBaseBoss::Precache( void )
{
	if ( m_modelString != NULL_STRING )
	{
		PrecacheModel( STRING( m_modelString ) );
	}

	BaseClass::Precache();
}


//--------------------------------------------------------------------------------------
void CTFBaseBoss::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	if ( m_modelString != NULL_STRING )
	{
		SetModel( STRING( m_modelString ) );
	}

	m_isEnabled = m_startDisabled ? false : true;

	SetHealth( m_initialHealth );
	SetMaxHealth( m_initialHealth );

	if ( TFGameRules() )
	{
		TFGameRules()->AddActiveBoss( this );
	}

	m_lastHealthPercentage = 1.0f;
	m_damagePoseParameter = -1;

	SetThink( &CTFBaseBoss::BossThink );
	SetNextThink( gpGlobals->curtime );
}

int CTFBaseBoss::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}


//--------------------------------------------------------------------------------------
void CTFBaseBoss::ResolvePlayerCollision( CTFPlayer *player )
{
	Vector bossGlobalMins = WorldAlignMins() + GetAbsOrigin();
	Vector bossGlobalMaxs = WorldAlignMaxs() + GetAbsOrigin();

	Vector playerGlobalMins = player->WorldAlignMins() + player->GetAbsOrigin();
	Vector playerGlobalMaxs = player->WorldAlignMaxs() + player->GetAbsOrigin();

	Vector newPlayerPos = player->GetAbsOrigin();


	if ( playerGlobalMins.x > bossGlobalMaxs.x ||
		 playerGlobalMaxs.x < bossGlobalMins.x ||
		 playerGlobalMins.y > bossGlobalMaxs.y ||
		 playerGlobalMaxs.y < bossGlobalMins.y ||
		 playerGlobalMins.z > bossGlobalMaxs.z ||
		 playerGlobalMaxs.z < bossGlobalMins.z )
	{
		// no overlap
		return;
	}

	Vector toPlayer = player->WorldSpaceCenter() - WorldSpaceCenter();

	Vector overlap;
	float signX, signY, signZ;

	if ( toPlayer.x >= 0 )
	{
		overlap.x = bossGlobalMaxs.x - playerGlobalMins.x;
		signX = 1.0f;
	}
	else
	{
		overlap.x = playerGlobalMaxs.x - bossGlobalMins.x;
		signX = -1.0f;
	}

	if ( toPlayer.y >= 0 )
	{
		overlap.y = bossGlobalMaxs.y - playerGlobalMins.y;
		signY = 1.0f;
	}
	else
	{
		overlap.y = playerGlobalMaxs.y - bossGlobalMins.y;
		signY = -1.0f;
	}

	if ( toPlayer.z >= 0 )
	{
		overlap.z = bossGlobalMaxs.z - playerGlobalMins.z;
		signZ = 1.0f;
	}
	else
	{
		// don't push player underground
 		overlap.z = 99999.9f; // playerGlobalMaxs.z - bossGlobalMins.z;
 		signZ = -1.0f;
	}

	float bloat = 5.0f;

	if ( overlap.x < overlap.y )
	{
		if ( overlap.x < overlap.z )
		{
			// X is least overlap
			newPlayerPos.x += signX * ( overlap.x + bloat );
		}
		else
		{
			// Z is least overlap
			newPlayerPos.z += signZ * ( overlap.z + bloat );
		}
	}
	else if ( overlap.z < overlap.y )
	{
		// Z is least overlap
		newPlayerPos.z += signZ * ( overlap.z + bloat );
	}
	else
	{
		// Y is least overlap
		newPlayerPos.y += signY * ( overlap.y + bloat );
	}

	// check if new location is valid
	trace_t result;
	Ray_t ray;
	ray.Init( newPlayerPos, newPlayerPos, player->WorldAlignMins(), player->WorldAlignMaxs() );
	UTIL_TraceRay( ray, MASK_PLAYERSOLID, player, COLLISION_GROUP_PLAYER_MOVEMENT, &result );

	if ( result.DidHit() )
	{
		// Trace down from above to find safe ground
		ray.Init( newPlayerPos + Vector( 0.0f, 0.0f, 32.0f ), newPlayerPos, player->WorldAlignMins(), player->WorldAlignMaxs() );
		UTIL_TraceRay( ray, MASK_PLAYERSOLID, player, COLLISION_GROUP_PLAYER_MOVEMENT, &result );

		if ( result.startsolid )
		{
			// player was crushed against something
			player->TakeDamage( CTakeDamageInfo( this, this, 99999.9f, DMG_CRUSH ) );
			return;
		}
		else
		{
			// Use the trace end position
			newPlayerPos = result.endpos;
		}
	}

	player->SetAbsOrigin( newPlayerPos );
}


//--------------------------------------------------------------------------------------
void CTFBaseBoss::Touch( CBaseEntity *pOther )
{
	BaseClass::Touch( pOther );

	if ( pOther && pOther->IsBaseObject() )
	{
		// ran over an engineer building - destroy it
		pOther->TakeDamage( CTakeDamageInfo( this, this, 99999.9f, DMG_CRUSH ) );
	}
}


//--------------------------------------------------------------------------------------
void CTFBaseBoss::BossThink( void )
{
	SetNextThink( gpGlobals->curtime );

	if ( m_damagePoseParameter < 0 )
	{
		m_damagePoseParameter = LookupPoseParameter( "damage" );
	}

	if ( m_damagePoseParameter >= 0 )
	{
		// Avoid dividing by zero
		if ( GetMaxHealth() )
		{
			SetPoseParameter( m_damagePoseParameter, 1.0f - ( (float)GetHealth() / (float)GetMaxHealth() ) );
		}
		else
		{
			SetPoseParameter( m_damagePoseParameter, 1.0f );
		}
	}

	if ( !m_isEnabled )
	{
		return;
	}

	Update();

	if ( m_bResolvePlayerCollisions )
	{
		CUtlVector< CTFPlayer * > playerVector;
		CollectPlayers( &playerVector, TEAM_ANY, COLLECT_ONLY_LIVING_PLAYERS );
		for( int i=0; i<playerVector.Count(); ++i )
		{
			ResolvePlayerCollision( playerVector[i] );
		}
	}
}

//--------------------------------------------------------------------------------------
void CTFBaseBoss::Event_Killed( const CTakeDamageInfo &info )
{
	m_outputOnKilled.FireOutput( this, this );

	// drop some loot!
	m_currencyValue = GetCurrencyValue();

	int nRemainingMoney = m_currencyValue;

	QAngle angRand = vec3_angle;

	while( nRemainingMoney > 0 )
	{
		int nAmount = 0;

		if ( nRemainingMoney >= 100 )
		{
			nAmount = 25;
		}
		else if ( nRemainingMoney >= 40 )
		{
			nAmount = 10;
		}
		else if ( nRemainingMoney >= 5 )
		{
			nAmount = 5;
		}
		else
		{
			nAmount = nRemainingMoney;
		}

		nRemainingMoney -= nAmount;

		angRand.y = RandomFloat( -180.0f, 180.0f );

		CCurrencyPackCustom *pCurrencyPack = assert_cast< CCurrencyPackCustom* >( CBaseEntity::CreateNoSpawn( "item_currencypack_custom", WorldSpaceCenter(), angRand, this ) );
		
		if ( pCurrencyPack )
		{
			pCurrencyPack->SetAmount( nAmount );

			Vector vecImpulse = RandomVector( -1,1 );
			vecImpulse.z = RandomFloat( 5.0f, 20.0f );
			VectorNormalize( vecImpulse );
			Vector vecVelocity = vecImpulse * 250.0 * RandomFloat( 1.0f, 4.0f );

			DispatchSpawn( pCurrencyPack );
			pCurrencyPack->DropSingleInstance( vecVelocity, this, 0, 0 );
		}
	}

	BaseClass::Event_Killed( info );

	UTIL_Remove( this );
}

void CTFBaseBoss::UpdateOnRemove()
{
	if ( TFGameRules() )
	{
		TFGameRules()->RemoveActiveBoss( this );
	}

	BaseClass::UpdateOnRemove();
}

int CTFBaseBoss::OnTakeDamage( const CTakeDamageInfo &rawInfo )
{
	CTakeDamageInfo info = rawInfo;

	if ( TFGameRules() )
	{
		TFGameRules()->ApplyOnDamageModifyRules( info, this, true );
	}

	// On damage Rage
	// Give the soldier/pyro some rage points for dealing/taking damage.
	if ( info.GetDamage() && info.GetAttacker() != this )
	{
		CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );

		// Buff flag 1: we get rage when we deal damage. Here, that means the soldier that attacked
		// gets rage when we take damage.
		HandleRageGain( pAttacker, kRageBuffFlag_OnDamageDealt, info.GetDamage(), 6.0f );

		// Buff 5: our pyro attacker get rage when we're damaged by fire
		if ( ( info.GetDamageType() & DMG_BURN ) != 0 || ( info.GetDamageType() & DMG_PLASMA ) != 0 )
		{
			HandleRageGain( pAttacker, kRageBuffFlag_OnBurnDamageDealt, info.GetDamage(), 30.f );
		}

		if ( pAttacker && info.GetWeapon() )
		{
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
			if ( pWeapon )
			{
				pWeapon->ApplyOnHitAttributes( this, pAttacker, info );
			}
		}
	}

	return BaseClass::OnTakeDamage( info );
}

//--------------------------------------------------------------------------------------
int CTFBaseBoss::OnTakeDamage_Alive( const CTakeDamageInfo &rawInfo )
{
	if ( !rawInfo.GetAttacker() || rawInfo.GetAttacker()->GetTeamNumber() == GetTeamNumber() )
	{
		// no friendly fire damage
		return 0;
	}

	CTakeDamageInfo info = rawInfo;

	// weapon-specific damage modification
	ModifyDamage( &info );

	if ( TFGameRules() )
	{
		CTFGameRules::DamageModifyExtras_t outParams;
		info.SetDamage( TFGameRules()->ApplyOnDamageAliveModifyRules( info, this, outParams ) );
	}

	// fire event for client combat text, beep, etc.
	IGameEvent *event = gameeventmanager->CreateEvent( "npc_hurt" );
	if ( event )
	{

		event->SetInt( "entindex", entindex() );
		event->SetInt( "health", MAX( 0, GetHealth() ) );
		event->SetInt( "damageamount", info.GetDamage() );
		event->SetBool( "crit", ( info.GetDamageType() & DMG_CRITICAL ) ? true : false );

		CTFPlayer *attackerPlayer = ToTFPlayer( info.GetAttacker() );
		if ( attackerPlayer )
		{
			event->SetInt( "attacker_player", attackerPlayer->GetUserID() );

			if ( attackerPlayer->GetActiveTFWeapon() )
			{
				event->SetInt( "weaponid", attackerPlayer->GetActiveTFWeapon()->GetWeaponID() );
			}
			else
			{
				event->SetInt( "weaponid", 0 );
			}
		}
		else
		{
			// hurt by world
			event->SetInt( "attacker_player", 0 );
			event->SetInt( "weaponid", 0 );
		}

		gameeventmanager->FireEvent( event );
	}

	int result = BaseClass::OnTakeDamage_Alive( info );

	// emit injury outputs
	float healthPercentage = (float)GetHealth() / (float)GetMaxHealth();

	if ( m_lastHealthPercentage > 0.9f && healthPercentage < 0.9f )
	{
		m_outputOnHealthBelow90Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.8f && healthPercentage < 0.8f )
	{
		m_outputOnHealthBelow80Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.7f && healthPercentage < 0.7f )
	{
		m_outputOnHealthBelow70Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.6f && healthPercentage < 0.6f )
	{
		m_outputOnHealthBelow60Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.5f && healthPercentage < 0.5f )
	{
		m_outputOnHealthBelow50Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.4f && healthPercentage < 0.4f )
	{
		m_outputOnHealthBelow40Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.3f && healthPercentage < 0.3f )
	{
		m_outputOnHealthBelow30Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.2f && healthPercentage < 0.2f )
	{
		m_outputOnHealthBelow20Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.1f && healthPercentage < 0.1f )
	{
		m_outputOnHealthBelow10Percent.FireOutput( this, this );
	}

	m_lastHealthPercentage = healthPercentage;

	// Let attacker react to the damage they dealt
	CTFPlayer *pAttacker = ToTFPlayer( rawInfo.GetAttacker() );
	if ( pAttacker )
	{
		pAttacker->OnDealtDamage( this, info );

		CTF_GameStats.Event_BossDamage( pAttacker, info.GetDamage() );
	}

	return result;
}


//--------------------------------------------------------------------------------------
void CTFBaseBoss::InputEnable( inputdata_t &inputdata )
{
	m_isEnabled = true;
}


//--------------------------------------------------------------------------------------
void CTFBaseBoss::InputDisable( inputdata_t &inputdata )
{
	m_isEnabled = false;
}


//------------------------------------------------------------------------------
void CTFBaseBoss::InputSetSpeed( inputdata_t &inputdata )
{
	m_speed = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: Set the health of the boss
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputSetHealth( inputdata_t &inputdata )
{
	m_iHealth = inputdata.value.Int();
	SetHealth( m_iHealth );
}

//-----------------------------------------------------------------------------
// Purpose: Set the max health of the boss
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputSetMaxHealth( inputdata_t &inputdata )
{
	m_iMaxHealth = inputdata.value.Int();
	SetMaxHealth( m_iMaxHealth );
}

//-----------------------------------------------------------------------------
// Purpose: Add health to the boss
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputAddHealth( inputdata_t &inputdata )
{
	int iHealth = inputdata.value.Int();
	SetHealth( MIN( GetMaxHealth(), GetHealth() + iHealth ) );
}

//-----------------------------------------------------------------------------
// Purpose: Remove health from the boss
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputRemoveHealth( inputdata_t &inputdata )
{
	int iDamage = inputdata.value.Int();

	SetHealth( GetHealth() - iDamage );
	if ( GetHealth() <= 0 )
	{
		CTakeDamageInfo info( inputdata.pCaller, inputdata.pActivator, vec3_origin, GetAbsOrigin(), iDamage, DMG_GENERIC, TF_DMG_CUSTOM_NONE );
		Event_Killed( info );
	}
}

//-----------------------------------------------------------------------------
void CTFBaseBoss::InputSetStepHeight( inputdata_t &inputdata )
{
	if ( m_locomotor )
	{
		m_locomotor->SetStepHeight( inputdata.value.Float() );
	}
}

//-----------------------------------------------------------------------------
void CTFBaseBoss::InputSetMaxJumpHeight( inputdata_t &inputdata )
{
	if ( m_locomotor )
	{
		m_locomotor->SetMaxJumpHeight( inputdata.value.Float() );
	}
}
