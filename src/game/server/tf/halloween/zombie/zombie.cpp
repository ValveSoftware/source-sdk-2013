//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"
#include "NextBot/Path/NextBotChasePath.h"
#include "particle_parse.h"

#include "zombie.h"
#include "zombie_behavior/zombie_spawn.h"

#include "halloween/tf_weapon_spellbook.h"

#define SKELETON_MODEL "models/bots/skeleton_sniper/skeleton_sniper.mdl"
#define SKELETON_KING_MODEL "models/bots/skeleton_sniper_boss/skeleton_sniper_boss.mdl"
#define SKELETON_KING_CROWN_MODEL "models/player/items/demo/crown.mdl"

ConVar tf_max_active_zombie( "tf_max_active_zombie", "30", FCVAR_CHEAT );


//-----------------------------------------------------------------------------------------------------
// NPC Zombie versions of the players
//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_zombie, CZombie );

IMPLEMENT_SERVERCLASS_ST( CZombie, DT_Zombie )
	SendPropFloat( SENDINFO( m_flHeadScale ) ),
END_SEND_TABLE()

IMPLEMENT_AUTO_LIST( IZombieAutoList );


static const char *s_skeletonHatModels[] =
{
	"models/player/items/all_class/skull_scout.mdl",
	"models/workshop/player/items/scout/hw2013_boston_bandy_mask/hw2013_boston_bandy_mask.mdl",
	"models/workshop/player/items/demo/hw2013_blackguards_bicorn/hw2013_blackguards_bicorn.mdl",
	"models/player/items/heavy/heavy_big_chief.mdl",
};

BEGIN_DATADESC( CZombie )
	DEFINE_OUTPUT( m_OnDeath, "OnDeath" ),
END_DATADESC()


//-----------------------------------------------------------------------------------------------------
CZombie::CZombie()
{
	m_intention = new CZombieIntention( this );
	m_locomotor = new CZombieLocomotion( this );
	m_body = new CHeadlessHatmanBody( this );

	m_nType = SKELETON_NORMAL;

	m_flHeadScale = 1.f;

	m_flAttackRange = 50.f;
	m_flAttackDamage = 30.f;

	m_bSpy = false;
	m_bForceSuicide = false;
	m_bDeathOutputFired = false;
}


//-----------------------------------------------------------------------------------------------------
CZombie::~CZombie()
{
	if ( m_intention )
		delete m_intention;

	if ( m_locomotor )
		delete m_locomotor;

	if ( m_body )
		delete m_body;
}


void CZombie::PrecacheZombie()
{
	/*PrecacheModel( "models/player/items/scout/scout_zombie.mdl" );
	PrecacheModel( "models/player/items/sniper/sniper_zombie.mdl" );
	PrecacheModel( "models/player/items/soldier/soldier_zombie.mdl" );
	PrecacheModel( "models/player/items/demo/demo_zombie.mdl" );
	PrecacheModel( "models/player/items/medic/medic_zombie.mdl" );
	PrecacheModel( "models/player/items/heavy/heavy_zombie.mdl" );
	PrecacheModel( "models/player/items/pyro/pyro_zombie.mdl" );
	PrecacheModel( "models/player/items/spy/spy_zombie.mdl" );
	PrecacheModel( "models/player/items/engineer/engineer_zombie.mdl" );*/

	int nSkeletonModel = PrecacheModel( SKELETON_MODEL );
	PrecacheGibsForModel( nSkeletonModel );

	int nSkeletonKingModel = PrecacheModel( SKELETON_KING_MODEL );
	PrecacheGibsForModel( nSkeletonKingModel );

	PrecacheModel( SKELETON_KING_CROWN_MODEL );

	if( TFGameRules()->GetHalloweenScenario() == CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY )
	{
		for ( int i=0; i<ARRAYSIZE( s_skeletonHatModels ) ; ++i )
		{
			PrecacheModel( s_skeletonHatModels[i] );
		}
	}

	PrecacheParticleSystem( "bomibomicon_ring" );
	PrecacheParticleSystem( "spell_pumpkin_mirv_goop_red" );
	PrecacheParticleSystem( "spell_pumpkin_mirv_goop_blue" );
	PrecacheParticleSystem( "spell_skeleton_goop_green" );

	PrecacheScriptSound( "Halloween.skeleton_break" );
	PrecacheScriptSound( "Halloween.skeleton_laugh_small" );
	PrecacheScriptSound( "Halloween.skeleton_laugh_medium" );
	PrecacheScriptSound( "Halloween.skeleton_laugh_giant" );
}


//-----------------------------------------------------------------------------------------------------
void CZombie::Precache()
{
	BaseClass::Precache();

	// These are player models which are already precached...

	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	PrecacheZombie();

	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}


//-----------------------------------------------------------------------------------------------------
void CZombie::Spawn( void )
{
	Precache();

	/*int which = RandomInt( TF_CLASS_SCOUT, TF_CLASS_ENGINEER );
	const char *name = g_aRawPlayerClassNamesShort[ which ];

	if ( FStrEq( name, "spy" ) )
	{
		m_bSpy = true;
	}*/

	//SetModel( CFmtStr( "models/player/%s.mdl", name ) );

	SetModel( SKELETON_MODEL );

	BaseClass::Spawn();

	const int health = 50;
	SetHealth( health );
	SetMaxHealth( health );
	AddFlag( FL_NPC );
	
	QAngle qAngle = vec3_angle;
	qAngle[YAW] = RandomFloat( 0, 360 );
	SetAbsAngles( qAngle );

	// Spawn Pos
	GetBodyInterface()->StartActivity( ACT_TRANSITION );

	//int iSkinIndex = GetTeamNumber() == TF_TEAM_RED ? 0 : 1;

	//m_zombieParts = (CBaseAnimating *)CreateEntityByName( "prop_dynamic" );
	//if ( m_zombieParts )
	//{
	//	m_zombieParts->SetModel( CFmtStr( "models/player/items/%s/%s_zombie.mdl", name, name ) );
	//	m_zombieParts->m_nSkin = iSkinIndex;

	//	// bonemerge into our model
	//	m_zombieParts->FollowEntity( this, true );
	//}

	//if ( m_bSpy )
	//{
	//	// Spy has a bunch of extra skins used to adjust the mask
	//	iSkinIndex += 22;
	//}
	//else
	//{
	//	// 4: red zombie
	//	// 5: blue zombie
	//	// 6: red zombie invuln
	//	// 7: blue zombie invuln
	//	iSkinIndex += 4;
	//}

	switch ( GetTeamNumber() )
	{
	case TF_TEAM_RED:
		m_nSkin = 0;
		break;
	case TF_TEAM_BLUE:
		m_nSkin = 1;
		break;
	default:
		{
			m_nSkin = 2;
			// make sure I'm on TF_TEAM_HALLOWEEN
			ChangeTeam( TF_TEAM_HALLOWEEN );
		}
	}

	// force kill oldest skeletons in the level (except skeleton king) to keep the number of skeletons under the max active
	int nForceKill = IZombieAutoList::AutoList().Count() - tf_max_active_zombie.GetInt();
	for ( int i=0; i<IZombieAutoList::AutoList().Count() && nForceKill > 0; ++i )
	{
		CZombie *pZombie = static_cast< CZombie* >( IZombieAutoList::AutoList()[i] );
		if ( pZombie->GetSkeletonType() != SKELETON_KING )
		{
			pZombie->ForceSuicide();
			nForceKill--;
		}
	}
	Assert( nForceKill <= 0 );
}


//-----------------------------------------------------------------------------------------------------
int CZombie::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( info.GetAttacker() && info.GetAttacker()->GetTeamNumber() == GetTeamNumber() )
		return 0;

	if ( !IsPlayingGesture( ACT_MP_GESTURE_FLINCH_CHEST ) )
	{
		AddGesture( ACT_MP_GESTURE_FLINCH_CHEST );
	}

	const char* pszEffectName;
	if ( GetTeamNumber() == TF_TEAM_HALLOWEEN )
	{
		pszEffectName = "spell_skeleton_goop_green";
	}
	else
	{
		pszEffectName = GetTeamNumber() == TF_TEAM_RED ? "spell_pumpkin_mirv_goop_red" : "spell_pumpkin_mirv_goop_blue";
	}

	if (info.GetAttacker() && info.GetAttacker()->IsPlayer())
	{
		int idx = m_vecRecentDamagers.FindPredicate([&info]( const RecentDamager_t& recent)
		{
			return recent.m_hEnt == info.GetAttacker();
		} );

		if (idx == m_vecRecentDamagers.InvalidIndex())
		{
			idx = m_vecRecentDamagers.AddToTail();
		}

		RecentDamager_t& recentDamager = m_vecRecentDamagers[ idx ];
		recentDamager.m_flDamageTime = gpGlobals->curtime;
		recentDamager.m_hEnt = info.GetAttacker();
	}

	DispatchParticleEffect( pszEffectName, info.GetDamagePosition(), GetAbsAngles() );

	return BaseClass::OnTakeDamage_Alive( info );
}


//-----------------------------------------------------------------------------------------------------
void CZombie::Event_Killed( const CTakeDamageInfo &info )
{
	EmitSound( "Halloween.skeleton_break" );


	if (info.GetAttacker() && info.GetAttacker()->IsPlayer())
	{
		CTFPlayer *pPlayerAttacker = ToTFPlayer(info.GetAttacker());
		if (pPlayerAttacker)
		{
			if (TFGameRules() && TFGameRules()->IsHalloweenScenario(CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER))
			{
				pPlayerAttacker->AwardAchievement(ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_SKELETON_GRIND);

				IGameEvent *pEvent = gameeventmanager->CreateEvent("halloween_skeleton_killed");
				if (pEvent)
				{
					pEvent->SetInt("player", pPlayerAttacker->GetUserID());
					gameeventmanager->FireEvent(pEvent, true);
				}
			}
		}
	}

	for (const RecentDamager_t& recent : m_vecRecentDamagers)
	{
		if ( gpGlobals->curtime - recent.m_flDamageTime > TF_TIME_ASSIST_KILL)
			continue;

		CTFPlayer* pPlayerAttacker = ToTFPlayer(recent.m_hEnt);

		IGameEvent *pEvent = gameeventmanager->CreateEvent(GetSkeletonType() == SKELETON_KING ? "skeleton_king_killed_quest" : "skeleton_killed_quest");
		if (pEvent)
		{
			pEvent->SetInt("player", pPlayerAttacker->GetUserID());
			gameeventmanager->FireEvent(pEvent, true);
		}
	}

	m_vecRecentDamagers.Purge();

	FireDeathOutput( info.GetInflictor() );
	
	BaseClass::Event_Killed( info );
}


//-----------------------------------------------------------------------------------------------------
void CZombie::UpdateOnRemove()
{
	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( m_nSkin );
	MessageEnd();

	UTIL_Remove( m_hHat );

	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------------------------------
void CZombie::FireDeathOutput( CBaseEntity *pCulprit )
{
	// only fire this once
	if ( m_bDeathOutputFired )
		return;
	
	m_bDeathOutputFired = true;
	m_OnDeath.FireOutput( pCulprit, this );
}


//---------------------------------------------------------------------------------------------
/*static*/ CZombie* CZombie::SpawnAtPos( const Vector& vSpawnPos, float flLifeTime /*= 0.f*/, int nTeam /*= TF_TEAM_HALLOWEEN*/, CBaseEntity *pOwner /*= NULL*/, SkeletonType_t nSkeletonType /*= SKELETON_NORMAL*/ )
{
	CZombie *pZombie = (CZombie *)CreateEntityByName( "tf_zombie" );
	if ( pZombie )
	{
		pZombie->ChangeTeam( nTeam );

		DispatchSpawn( pZombie );

		pZombie->SetAbsOrigin( vSpawnPos );
		pZombie->SetOwnerEntity( pOwner );

		if ( flLifeTime > 0.f )
		{
			pZombie->StartLifeTimer( flLifeTime );
		}

		pZombie->SetSkeletonType( nSkeletonType );
	}

	return pZombie;
}


bool CZombie::ShouldSuicide() const
{
	// out of life time
	if ( m_lifeTimer.HasStarted() && m_lifeTimer.IsElapsed() )
		return true;

	// owner changed team
	if ( GetOwnerEntity() && GetOwnerEntity()->GetTeamNumber() != GetTeamNumber() )
		return true;

	return m_bForceSuicide;
}


//-----------------------------------------------------------------------------------------------------
void CZombie::SetSkeletonType( SkeletonType_t nType )
{
	m_nType = nType;
	// Skeleton King?
	if ( nType == SKELETON_KING )
	{
		SetModel( SKELETON_KING_MODEL );
		SetModelScale( 2.f );

		const int health = 1000;
		SetHealth( health );
		SetMaxHealth( health );

		m_flAttackRange = 100.f;
		m_flAttackDamage = 100.f;

		AddHat( SKELETON_KING_CROWN_MODEL );
	}
	else if ( nType == SKELETON_MINI )
	{
		SetModel( SKELETON_MODEL );
		SetModelScale( 0.5f );
		m_flHeadScale = 3.f;

		if( TFGameRules()->GetHalloweenScenario() == CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY )
		{
			int iModelIndex = RandomInt( 0, ARRAYSIZE( s_skeletonHatModels ) - 1 );
			const char *pszHat = s_skeletonHatModels[ iModelIndex ];
			AddHat( pszHat );
		}

		m_flAttackRange = 40.f;
		m_flAttackDamage = 20.f;
	}
}


//-----------------------------------------------------------------------------------------------------
void CZombie::AddHat( const char *pszModel )
{
	if ( !m_hHat )
	{
		int iHead = LookupBone( "bip_head" );
		Assert( iHead != -1 );
		if ( iHead != -1 )
		{
			m_hHat = (CBaseAnimating *)CreateEntityByName( "prop_dynamic" );
			if ( m_hHat )
			{
				m_hHat->SetModel( pszModel );

				Vector pos;
				QAngle angles;
				GetBonePosition( iHead, pos, angles );
				m_hHat->SetAbsOrigin( pos );
				m_hHat->SetAbsAngles( angles );
				m_hHat->FollowEntity( this, true );
			}
		}
	}
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CZombieBehavior : public Action< CZombie >
{
public:
	virtual Action< CZombie > *InitialContainedAction( CZombie *me )	
	{
		return new CZombieSpawn;
	}

	virtual ActionResult< CZombie >	OnStart( CZombie *me, Action< CZombie > *priorAction )
	{
		return Continue();
	}

	virtual ActionResult< CZombie >	Update( CZombie *me, float interval )
	{
		bool bDead = !me->IsAlive();
		if ( !bDead && me->ShouldSuicide() )
		{
			me->FireDeathOutput( me );
			bDead = true;
		}

		if ( bDead )
		{
			UTIL_Remove( me );
			return Done();
		}

		if ( ShouldLaugh( me ) )
		{
			Laugh( me );
		}

		return Continue();
	}

	virtual EventDesiredResult< CZombie > OnKilled( CZombie *me, const CTakeDamageInfo &info )
	{
		// bonemerged models don't ragdoll
		//UTIL_Remove( me->m_zombieParts );

		if ( info.GetAttacker() && dynamic_cast< CBaseCombatCharacter* >( info.GetAttacker() ) )
		{
			if ( me->GetSkeletonType() == CZombie::SKELETON_NORMAL )
			{
				// normal skeleton spawns 3 mini skeletons
				CBaseCombatCharacter* pOwner = dynamic_cast< CBaseCombatCharacter* >( me->GetOwnerEntity() );
				pOwner = pOwner ? pOwner : me;
				for ( int i=0; i<3; ++i )
				{
					CreateSpellSpawnZombie( pOwner, me->GetAbsOrigin(), 2 );
				}
			}
			else if ( me->GetSkeletonType() == CZombie::SKELETON_KING )
			{
				// skeleton king drops rare spell
				TFGameRules()->DropSpellPickup( me->GetAbsOrigin(), 1 );
			}
		}

		UTIL_Remove( me );

		return TryDone();
	}

	virtual const char *GetName( void ) const	{ return "ZombieBehavior"; }		// return name of this action

private:

	bool ShouldLaugh( CZombie *me )
	{
		if ( !m_laughTimer.HasStarted() )
		{
			switch ( me->GetSkeletonType() )
			{
			case CZombie::SKELETON_KING:
				{
					m_laughTimer.Start( RandomFloat( 6.f, 7.f ) );
					break;
				}
			case CZombie::SKELETON_MINI:
				{
					m_laughTimer.Start( RandomFloat( 2.f, 3.f ) );
					break;
				}
			default:
				{
					m_laughTimer.Start( RandomFloat( 4.f, 5.f ) );
				}
			}

			return false;
		}

		if ( m_laughTimer.HasStarted() && m_laughTimer.IsElapsed() )
		{
			m_laughTimer.Invalidate();
			return true;
		}

		return false;
	}

	void Laugh( CZombie *me )
	{
		const char *pszSoundName;
		switch ( me->GetSkeletonType() )
		{
		case CZombie::SKELETON_KING:
			{
				pszSoundName = "Halloween.skeleton_laugh_giant";
				break;
			}
		case CZombie::SKELETON_MINI:
			{
				pszSoundName = "Halloween.skeleton_laugh_small";
				break;
			}
		default:
			{
				pszSoundName = "Halloween.skeleton_laugh_medium";
			}
		}

		me->EmitSound( pszSoundName );
	}

	CountdownTimer m_laughTimer;
};


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CZombieIntention::CZombieIntention( CZombie *me ) : IIntention( me )
{ 
	m_behavior = new Behavior< CZombie >( new CZombieBehavior ); 
}

CZombieIntention::~CZombieIntention()
{
	delete m_behavior;
}

void CZombieIntention::Reset( void )
{ 
	delete m_behavior; 
	m_behavior = new Behavior< CZombie >( new CZombieBehavior );
}

void CZombieIntention::Update( void )
{
	m_behavior->Update( static_cast< CZombie * >( GetBot() ), GetUpdateInterval() ); 
}

// is this a place we can be?
QueryResultType CZombieIntention::IsPositionAllowed( const INextBot *meBot, const Vector &pos ) const
{
	return ANSWER_YES;
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
float CZombieLocomotion::GetRunSpeed( void ) const
{
	return 300.f;
}


//---------------------------------------------------------------------------------------------
// if delta Z is greater than this, we have to jump to get up
float CZombieLocomotion::GetStepHeight( void ) const
{
	return 18.0f;
}


//---------------------------------------------------------------------------------------------
// return maximum height of a jump
float CZombieLocomotion::GetMaxJumpHeight( void ) const
{
	return 18.0f;
}


//---------------------------------------------------------------------------------------------
// Return max rate of yaw rotation
float CZombieLocomotion::GetMaxYawRate( void ) const
{
	return 200.0f;
}


//---------------------------------------------------------------------------------------------
bool CZombieLocomotion::ShouldCollideWith( const CBaseEntity *object ) const
{
	return false;
}
