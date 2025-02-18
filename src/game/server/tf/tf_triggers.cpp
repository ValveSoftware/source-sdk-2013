//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Team Fortress specific special triggers
//
//===========================================================================//

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "entityapi.h"
#include "entitylist.h"
#include "saverestore_utlvector.h"
#include "tf_player.h"
#include "triggers.h"
#include "tf_triggers.h"
#include "tf_weapon_compound_bow.h"
#include "doors.h"
#include "bot/tf_bot.h"
#include "trigger_area_capture.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CTriggerStun )

	// Function Pointers
	DEFINE_FUNCTION( StunThink ),

	// Fields

	DEFINE_KEYFIELD( m_flTriggerDelay, FIELD_FLOAT, "trigger_delay" ),
	DEFINE_KEYFIELD( m_flStunDuration, FIELD_FLOAT, "stun_duration" ),
	DEFINE_KEYFIELD( m_flMoveSpeedReduction, FIELD_FLOAT, "move_speed_reduction" ),
	DEFINE_KEYFIELD( m_iStunType, FIELD_INTEGER, "stun_type"  ),
	DEFINE_KEYFIELD( m_bStunEffects, FIELD_INTEGER, "stun_effects"  ),

	DEFINE_UTLVECTOR( m_stunEntities, FIELD_EHANDLE ),

	// Outputs
	DEFINE_OUTPUT( m_OnStunPlayer, "OnStunPlayer" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( trigger_stun, CTriggerStun );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerStun::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: When touched, a stun trigger applies its stunflags to the other for a duration.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
bool CTriggerStun::StunEntity( CBaseEntity *pOther )
{
	if ( !pOther->m_takedamage || !PassesTriggerFilters(pOther) )
		return false;

	CTFPlayer* pTFPlayer = ToTFPlayer( pOther );
	if ( !pTFPlayer )
		return false;

	int iStunFlags = TF_STUN_MOVEMENT;
	switch ( m_iStunType )
	{
	case 0:
		// Movement Only
		break;
	case 1:
		// Controls
		iStunFlags |= TF_STUN_CONTROLS;
		break;
	case 2:
		// Loser State
		iStunFlags |= TF_STUN_LOSER_STATE;
		break;
	}

	if ( !m_bStunEffects )
	{
		iStunFlags |= TF_STUN_NO_EFFECTS;
	}

	iStunFlags |= TF_STUN_BY_TRIGGER;

	pTFPlayer->m_Shared.StunPlayer( m_flStunDuration, m_flMoveSpeedReduction, iStunFlags, NULL );

	m_OnStunPlayer.FireOutput( pOther, this );
	m_stunEntities.AddToTail( EHANDLE(pOther) );

	return true;
}

void CTriggerStun::StunThink()
{
	// If I stun anyone, think again.
	if ( StunAllTouchers( 0.5 ) <= 0 )
	{
		SetThink(NULL);
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.5f );
	}
}

void CTriggerStun::EndTouch( CBaseEntity *pOther )
{
	if ( PassesTriggerFilters(pOther) )
	{
		EHANDLE hOther;
		hOther = pOther;

		// If this guy has never been stunned...
		if ( !m_stunEntities.HasElement( hOther ) )
		{
			StunEntity( pOther );
		}
	}
	BaseClass::EndTouch( pOther );
}

int CTriggerStun::StunAllTouchers( float dt )
{
	m_flLastStunTime = gpGlobals->curtime;
	m_stunEntities.RemoveAll();

	int stunCount = 0;
	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			CBaseEntity *pTouch = link->entityTouched;
			if ( pTouch )
			{
				if ( StunEntity( pTouch ) )
				{
					stunCount++;
				}
			}
		}
	}

	return stunCount;
}

void CTriggerStun::Touch( CBaseEntity *pOther )
{
	if ( m_pfnThink == NULL )
	{
		SetThink( &CTriggerStun::StunThink );
		SetNextThink( gpGlobals->curtime + m_flTriggerDelay );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Ignites the arrows of any bow carried by a player who touches this trigger
//-----------------------------------------------------------------------------
class CTriggerIgniteArrows : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerIgniteArrows, CBaseTrigger );

	void Spawn( void );
	void Touch( CBaseEntity *pOther );

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CTriggerIgniteArrows )
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_ignite_arrows, CTriggerIgniteArrows );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerIgniteArrows::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}

//-----------------------------------------------------------------------------
// Purpose: Ignites the arrows of any bow carried by a player who touches this trigger
//-----------------------------------------------------------------------------
void CTriggerIgniteArrows::Touch( CBaseEntity *pOther )
{
	if (!PassesTriggerFilters(pOther))
		return;

	if ( !pOther->IsPlayer() )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	// Ignore non-snipers
	if ( !pPlayer || !pPlayer->IsPlayerClass(TF_CLASS_SNIPER) )
		return;

	// Make sure they're looking at the origin
	Vector vecPos, vecForward, vecUp, vecRight;
	pPlayer->EyePositionAndVectors( &vecPos, &vecForward, &vecUp, &vecRight );
	Vector vTargetDir = GetAbsOrigin() - vecPos;
	VectorNormalize(vTargetDir);
	float fDotPr = DotProduct(vecForward,vTargetDir);
	if (fDotPr < 0.95)
		return;

	// Does he have the bow?
	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
	if ( pWpn && pWpn->GetWeaponID() == TF_WEAPON_COMPOUND_BOW )
	{
		CTFCompoundBow *pBow = static_cast<CTFCompoundBow*>( pWpn );
		pBow->SetArrowAlight( true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Trigger that controls door speed by specified time
//-----------------------------------------------------------------------------
class CTriggerTimerDoor : public CTriggerAreaCapture
{
public:
	DECLARE_CLASS( CTriggerTimerDoor, CTriggerAreaCapture );
	DECLARE_DATADESC();

	virtual void Spawn( void ) OVERRIDE;
	virtual void StartTouch( CBaseEntity *pOther ) OVERRIDE;

	virtual void OnStartCapture( int iTeam ) OVERRIDE;
	virtual void OnEndCapture( int iTeam ) OVERRIDE;

protected:
	virtual bool CaptureModeScalesWithPlayers() const OVERRIDE { return false; }

private:
	CHandle<CBaseDoor>	m_hDoor;	//the door that we are linked to!

	string_t m_iszDoorName;
};

BEGIN_DATADESC( CTriggerTimerDoor )
	DEFINE_KEYFIELD( m_iszDoorName, FIELD_STRING, "door_name" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_timer_door, CTriggerTimerDoor );

#define GATE_THINK_TIME 0.1f


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerTimerDoor::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}

//-----------------------------------------------------------------------------
// Purpose: Bot enters the trigger, open the door
//-----------------------------------------------------------------------------
void CTriggerTimerDoor::StartTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
		return;

	if (!PassesTriggerFilters(pOther))
		return;

	if ( !m_hDoor )
	{
		m_hDoor = dynamic_cast< CBaseDoor* >( gEntList.FindEntityByName(NULL, m_iszDoorName ) );
		if ( !m_hDoor )
		{
			Warning( "trigger_bot_gate failed to find \"%s\" door entity", STRING( m_iszDoorName ) );
			return;
		}
		else
		{
			float flDoorTravelDistance = ( m_hDoor->m_vecPosition2 - m_hDoor->m_vecPosition1 ).Length();
			m_hDoor->m_flSpeed = flDoorTravelDistance / GetCapTime();
		}
	}

	BaseClass::StartTouch( pOther );
}


//-----------------------------------------------------------------------------
// Purpose: Bot starts opening the door
//-----------------------------------------------------------------------------
void CTriggerTimerDoor::OnStartCapture( int iTeam )
{
	BaseClass::OnStartCapture( iTeam );

	if ( FStrEq( gpGlobals->mapname.ToCStr(), "mvm_mannhattan" ) )
	{
		TFGameRules()->RandomPlayersSpeakConceptIfAllowed( MP_CONCEPT_MANNHATTAN_GATE_ATK, 1, TF_TEAM_RED );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Bot finishes opening the door
//-----------------------------------------------------------------------------
void CTriggerTimerDoor::OnEndCapture( int iTeam )
{
	BaseClass::OnEndCapture( iTeam );

	if ( FStrEq( gpGlobals->mapname.ToCStr(), "mvm_mannhattan" ) )
	{
		TFGameRules()->RandomPlayersSpeakConceptIfAllowed( MP_CONCEPT_MANNHATTAN_GATE_TAKE, 1, TF_TEAM_RED );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Trigger that adds tag to bots
//-----------------------------------------------------------------------------
class CTriggerBotTag : public CBaseTrigger
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTriggerBotTag, CBaseTrigger );

	virtual void Spawn( void );

	virtual void Touch( CBaseEntity *pOther );

private:
	string_t m_iszTags;

	bool m_bAdd;
	CUtlStringList m_tags;
};

BEGIN_DATADESC( CTriggerBotTag )
	DEFINE_KEYFIELD( m_iszTags, FIELD_STRING, "tags" ),
	DEFINE_KEYFIELD( m_bAdd, FIELD_BOOLEAN, "add" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_bot_tag, CTriggerBotTag );

void CTriggerBotTag::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();

	m_tags.RemoveAll();
	// chop space-delimited string into individual tokens
	const char *tags = STRING( m_iszTags );
	if ( tags )
	{
		CSplitString splitStrings( tags, " " );
		for( int i=0; i<splitStrings.Count(); ++i )
		{
			m_tags.CopyAndAddToTail( splitStrings.Element( i ) );
		}
	}
}


void CTriggerBotTag::Touch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
	{
		return;
	}

	if ( !pOther->IsPlayer() )
	{
		return;
	}

	CTFBot *pBot = ToTFBot( pOther );
	if ( !pBot )
	{
		return;
	}

	if ( m_bAdd )
	{
		for ( int i=0; i<m_tags.Count(); ++i )
		{
			pBot->AddTag( m_tags[i] );
		}
	}
	else
	{
		for ( int i=0; i<m_tags.Count(); ++i )
		{
			pBot->RemoveTag( m_tags[i] );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Trigger that adds a condition to players
//-----------------------------------------------------------------------------
class CTriggerAddTFPlayerCondition : public CBaseTrigger
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTriggerAddTFPlayerCondition, CBaseTrigger );

	virtual void Spawn( void );

	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );

private:
	ETFCond m_nCondition;
	
	float m_flDuration;
};

BEGIN_DATADESC( CTriggerAddTFPlayerCondition )
	DEFINE_KEYFIELD( m_nCondition, FIELD_INTEGER, "condition" ),
	DEFINE_KEYFIELD( m_flDuration, FIELD_FLOAT, "duration" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_add_tf_player_condition, CTriggerAddTFPlayerCondition );

void CTriggerAddTFPlayerCondition::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();
}


void CTriggerAddTFPlayerCondition::StartTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
	{
		return;
	}

	if ( !PassesTriggerFilters(pOther) )
	{
		return;
	}

	if ( !pOther->IsPlayer() )
	{
		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
	{
		return;
	}

	if ( m_nCondition != TF_COND_INVALID )
	{
		pPlayer->m_Shared.AddCond( m_nCondition, m_flDuration );
		BaseClass::StartTouch( pOther );
	}
	else
	{
		Warning( "Invalid Condition ID [%d] in trigger %s\n", m_nCondition, GetEntityName().ToCStr() );
	}
}

void CTriggerAddTFPlayerCondition::EndTouch( CBaseEntity *pOther )
{
	BaseClass::EndTouch( pOther );

	if ( m_flDuration != PERMANENT_CONDITION )
		return;

	if ( m_bDisabled )
		return;

	if ( !PassesTriggerFilters(pOther) )
		return;

	if ( !pOther->IsPlayer() )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
		return;

	if ( m_nCondition != TF_COND_INVALID )
	{
		pPlayer->m_Shared.RemoveCond( m_nCondition );
	}
	else
	{
		Warning( "Invalid Condition ID [%d] in trigger %s\n", m_nCondition, GetEntityName().ToCStr() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: CTriggerPlayerRespawnOverride
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CTriggerPlayerRespawnOverride )
	DEFINE_KEYFIELD( m_flRespawnTime, FIELD_FLOAT, "RespawnTime" ),
	DEFINE_KEYFIELD( m_strRespawnEnt, FIELD_STRING, "RespawnName" ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRespawnTime", InputSetRespawnTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRespawnName", InputSetRespawnName ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_player_respawn_override, CTriggerPlayerRespawnOverride );

IMPLEMENT_AUTO_LIST( ITriggerPlayerRespawnOverride );

//-----------------------------------------------------------------------------
// Purpose: Trigger that ignites players
//-----------------------------------------------------------------------------
class CTriggerIgnite : public CBaseTrigger
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTriggerIgnite, CBaseTrigger );

	CTriggerIgnite();

	virtual void Spawn( void );
	virtual void Precache( void );

	virtual void StartTouch( CBaseEntity *pOther );

	void BurnThink();

private:
	void IgniteEntity( CBaseEntity *pOther );
	int BurnEntities();

	float m_flBurnDuration;
	float m_flDamagePercentPerSecond;
	string_t m_iszIgniteParticleName;
	string_t m_iszIgniteSoundName;

	float m_flLastBurnTime;
};

BEGIN_DATADESC( CTriggerIgnite )
	DEFINE_KEYFIELD( m_flBurnDuration,				FIELD_FLOAT,	"burn_duration" ),
	DEFINE_KEYFIELD( m_flDamagePercentPerSecond,	FIELD_FLOAT,	"damage_percent_per_second" ),
	DEFINE_KEYFIELD( m_iszIgniteParticleName,		FIELD_STRING,	"ignite_particle_name" ),
	DEFINE_KEYFIELD( m_iszIgniteSoundName,			FIELD_STRING,	"ignite_sound_name" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_ignite, CTriggerIgnite );

#define BURN_INTERVAL 0.1f


CTriggerIgnite::CTriggerIgnite()
{
	m_flBurnDuration = 1.f;
	m_flDamagePercentPerSecond = 10.f;
	m_flLastBurnTime = 0.f;
}


void CTriggerIgnite::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}


void CTriggerIgnite::Precache( void )
{
	BaseClass::Precache();

	const char *pszParticleName = STRING( m_iszIgniteParticleName );
	if ( pszParticleName && *pszParticleName )
	{
		PrecacheParticleSystem( pszParticleName );
	}

	const char *pszSoundName = STRING( m_iszIgniteSoundName );
	if ( pszSoundName && *pszSoundName )
	{
		PrecacheScriptSound( pszSoundName );
	}
}


void CTriggerIgnite::BurnThink()
{
	if ( BurnEntities() > 0 )
	{
		SetNextThink( gpGlobals->curtime + BURN_INTERVAL );
	}
	else
	{
		SetThink( NULL );
	}
}


void CTriggerIgnite::StartTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
	{
		return;
	}

	if ( !PassesTriggerFilters(pOther) )
	{
		return;
	}

	IgniteEntity( pOther );

	BaseClass::StartTouch( pOther );

	if ( m_pfnThink == NULL )
	{
		m_flLastBurnTime = gpGlobals->curtime;
		SetThink( &CTriggerIgnite::BurnThink );
		SetNextThink( gpGlobals->curtime + BURN_INTERVAL );
	}
}


void CTriggerIgnite::IgniteEntity( CBaseEntity *pOther )
{
	Vector vecEffectPos = pOther->GetAbsOrigin();
	const char *pszParticleName = STRING( m_iszIgniteParticleName );
	if ( pszParticleName && *pszParticleName )
	{
		DispatchParticleEffect( pszParticleName, vecEffectPos, vec3_angle );
	}

	const char *pszSoundName = STRING( m_iszIgniteSoundName );
	if ( pszSoundName && *pszSoundName )
	{
		CSoundParameters params;
		if ( CBaseEntity::GetParametersForSound( pszSoundName, params, NULL ) )
		{
			CPASAttenuationFilter soundFilter( vecEffectPos, params.soundlevel );
			EmitSound_t ep( params );
			ep.m_pOrigin = &vecEffectPos;
			EmitSound( soundFilter, entindex(), ep );
		}
	}

	if ( pOther->IsPlayer() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pOther );
		if ( pTFPlayer && !pTFPlayer->m_Shared.IsInvulnerable() && !pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) )
		{
			pTFPlayer->m_Shared.SelfBurn( m_flBurnDuration );
		}
	}
	else
	{
		UTIL_Remove( pOther );
	}
}


int CTriggerIgnite::BurnEntities()
{
	if ( m_hTouchingEntities.IsEmpty() )
		return 0;

	float flDT = gpGlobals->curtime - m_flLastBurnTime;
	m_flLastBurnTime = gpGlobals->curtime;
	int nBurn = 0;
	FOR_EACH_VEC( m_hTouchingEntities, i )
	{
		CBaseEntity *pEnt = m_hTouchingEntities[i];
		if ( pEnt && pEnt->IsPlayer() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( pEnt );
			if ( pTFPlayer )
			{
				float flDamageScale = m_flDamagePercentPerSecond * 0.01f;
				float flDamage = flDT * flDamageScale * pTFPlayer->GetMaxHealth();
				CTakeDamageInfo info( this, this, flDamage, DMG_BURN );
				if ( !pTFPlayer->m_Shared.IsInvulnerable() && !pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) ) // if player enters trigger invuln, we need to ignite them when it wears off. We also don't want to ignite an already burning player
				{
					IgniteEntity( pTFPlayer );
				}

				pTFPlayer->TakeDamage( info );
				nBurn++;
			}
		}
	}

	return nBurn;
}


//-----------------------------------------------------------------------------
// Purpose: Trigger that spawn particles on entities
//-----------------------------------------------------------------------------
class CTriggerParticle : public CBaseTrigger
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTriggerParticle, CBaseTrigger );

	CTriggerParticle();

	virtual void Spawn( void );
	virtual void Precache( void );

	virtual void StartTouch( CBaseEntity *pOther );

private:
	string_t m_iszParticleName;
	string_t m_iszAttachmentName;
	ParticleAttachment_t m_nAttachType;
};

BEGIN_DATADESC( CTriggerParticle )
	DEFINE_KEYFIELD( m_iszParticleName,		FIELD_STRING,	"particle_name" ),
	DEFINE_KEYFIELD( m_iszAttachmentName,	FIELD_STRING,	"attachment_name" ),
	DEFINE_KEYFIELD( m_nAttachType,			FIELD_INTEGER,	"attachment_type" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_particle, CTriggerParticle );


CTriggerParticle::CTriggerParticle()
{
	m_nAttachType = PATTACH_ABSORIGIN;
}


void CTriggerParticle::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}


void CTriggerParticle::Precache( void )
{
	BaseClass::Precache();

	const char *pszParticleName = STRING( m_iszParticleName );
	if ( pszParticleName && *pszParticleName )
	{
		PrecacheParticleSystem( pszParticleName );
	}
}


void CTriggerParticle::StartTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
	{
		return;
	}

	if ( !PassesTriggerFilters(pOther) )
	{
		return;
	}

	BaseClass::StartTouch( pOther );

	const char *pszParticleName = STRING( m_iszParticleName );
	const char *pszAttachmentName = STRING( m_iszAttachmentName );
	int iAttachment = -1;
	if ( pszAttachmentName && *pszAttachmentName )
	{
		iAttachment = pOther->GetBaseAnimating()->LookupAttachment( pszAttachmentName );
	}

	DispatchParticleEffect( pszParticleName, m_nAttachType, pOther, iAttachment );
}


class CTriggerRemoveTFPlayerCondition : public CBaseTrigger
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTriggerRemoveTFPlayerCondition, CBaseTrigger );

	virtual void Spawn( void );

	virtual void StartTouch( CBaseEntity *pOther );

private:
	ETFCond m_nCondition;
};


BEGIN_DATADESC( CTriggerRemoveTFPlayerCondition )
	DEFINE_KEYFIELD( m_nCondition, FIELD_INTEGER, "condition" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_remove_tf_player_condition, CTriggerRemoveTFPlayerCondition );


void CTriggerRemoveTFPlayerCondition::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();
}


void CTriggerRemoveTFPlayerCondition::StartTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
	{
		return;
	}

	if ( !PassesTriggerFilters(pOther) )
	{
		return;
	}

	if ( !pOther->IsPlayer() )
	{
		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
	{
		return;
	}

	if ( m_nCondition != TF_COND_INVALID )
	{
		pPlayer->m_Shared.RemoveCond( m_nCondition );

		// Hack for Bank until we re-address this for parachuting MvM bots
		CTFBot *pTFBot = dynamic_cast< CTFBot* >( pPlayer );
		if ( pTFBot )
		{
			pTFBot->ClearLastKnownArea();
		}
	}
	else
	{
		pPlayer->m_Shared.RemoveAllCond();
	}

	BaseClass::StartTouch( pOther );
}


class CTriggerAddOrRemoveTFPlayerAttributes : public CBaseTrigger
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTriggerAddOrRemoveTFPlayerAttributes, CBaseTrigger );

	virtual void Spawn( void );

	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );

private:
	bool m_bRemove;
	string_t m_iszAttributeName;
	float m_flAttributeValue;
	float m_flDuration;
	bool m_bValidAttribute;
};


BEGIN_DATADESC( CTriggerAddOrRemoveTFPlayerAttributes )
	DEFINE_KEYFIELD( m_bRemove, FIELD_BOOLEAN, "add_or_remove" ),
	DEFINE_KEYFIELD( m_iszAttributeName, FIELD_STRING, "attribute_name" ),
	DEFINE_KEYFIELD( m_flAttributeValue, FIELD_FLOAT, "value" ),
	DEFINE_KEYFIELD( m_flDuration, FIELD_FLOAT, "duration" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_add_or_remove_tf_player_attributes, CTriggerAddOrRemoveTFPlayerAttributes );


void CTriggerAddOrRemoveTFPlayerAttributes::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();
	
	m_bValidAttribute = false;
	const char *pszAttrName = STRING( m_iszAttributeName );
	if ( pszAttrName && *pszAttrName )
	{
		CSchemaAttributeDefHandle pAttrTest( pszAttrName );
		if ( pAttrTest )
		{
			m_bValidAttribute = true;
		}
		else
		{
			Warning( "Invalid attribute name '%s' from trigger trigger_add_or_remove_tf_player_attributes name '%s'\n", pszAttrName, STRING( GetEntityName() ) );
		}
	}
}


void CTriggerAddOrRemoveTFPlayerAttributes::StartTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
	{
		return;
	}

	if ( !m_bValidAttribute )
	{
		return;
	}

	if ( !PassesTriggerFilters(pOther) )
	{
		return;
	}

	if ( !pOther->IsPlayer() )
	{
		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
	{
		return;
	}

	const char *pszAttrName = STRING( m_iszAttributeName );
	if ( m_bRemove )
	{
		pPlayer->RemoveCustomAttribute( pszAttrName );
	}
	else
	{
		pPlayer->AddCustomAttribute( pszAttrName, m_flAttributeValue, m_flDuration ); 
	}

	BaseClass::StartTouch( pOther );
}


void CTriggerAddOrRemoveTFPlayerAttributes::EndTouch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
	{
		return;
	}

	if ( !m_bValidAttribute )
	{
		return;
	}

	// only run auto remove on added attribute
	if ( m_bRemove )
	{
		return;
	}

	// ignore timer attribute. it'll remove itself
	if ( m_flDuration > 0.f )
	{
		return;
	}

	if ( !PassesTriggerFilters(pOther) )
	{
		return;
	}

	if ( !pOther->IsPlayer() )
	{
		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
	{
		return;
	}

	// remove permanent attribute on exist trigger
	const char *pszAttrName = STRING( m_iszAttributeName );
	pPlayer->RemoveCustomAttribute( pszAttrName );

	BaseClass::EndTouch( pOther );
}
