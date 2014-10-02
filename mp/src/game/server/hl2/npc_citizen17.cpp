//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The downtrodden citizens of City 17.
//
//=============================================================================//

#include "cbase.h"

#include "npc_citizen17.h"

#include "ammodef.h"
#include "globalstate.h"
#include "soundent.h"
#include "BasePropDoor.h"
#include "weapon_rpg.h"
#include "hl2_player.h"
#include "items.h"


#ifdef HL2MP
#include "hl2mp/weapon_crowbar.h"
#else
#include "weapon_crowbar.h"
#endif

#include "eventqueue.h"

#include "ai_squad.h"
#include "ai_pathfinder.h"
#include "ai_route.h"
#include "ai_hint.h"
#include "ai_interactions.h"
#include "ai_looktarget.h"
#include "sceneentity.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INSIGNIA_MODEL "models/chefhat.mdl"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define CIT_INSPECTED_DELAY_TIME 120  //How often I'm allowed to be inspected

extern ConVar sk_healthkit;
extern ConVar sk_healthvial;

const int MAX_PLAYER_SQUAD = 4;

ConVar	sk_citizen_health				( "sk_citizen_health",					"0");
ConVar	sk_citizen_heal_player			( "sk_citizen_heal_player",				"25");
ConVar	sk_citizen_heal_player_delay	( "sk_citizen_heal_player_delay",		"25");
ConVar	sk_citizen_giveammo_player_delay( "sk_citizen_giveammo_player_delay",	"10");
ConVar	sk_citizen_heal_player_min_pct	( "sk_citizen_heal_player_min_pct",		"0.60");
ConVar	sk_citizen_heal_player_min_forced( "sk_citizen_heal_player_min_forced",		"10.0");
ConVar	sk_citizen_heal_ally			( "sk_citizen_heal_ally",				"30");
ConVar	sk_citizen_heal_ally_delay		( "sk_citizen_heal_ally_delay",			"20");
ConVar	sk_citizen_heal_ally_min_pct	( "sk_citizen_heal_ally_min_pct",		"0.90");
ConVar	sk_citizen_player_stare_time	( "sk_citizen_player_stare_time",		"1.0" );
ConVar  sk_citizen_player_stare_dist	( "sk_citizen_player_stare_dist",		"72" );
ConVar	sk_citizen_stare_heal_time		( "sk_citizen_stare_heal_time",			"5" );

ConVar	g_ai_citizen_show_enemy( "g_ai_citizen_show_enemy", "0" );

ConVar	npc_citizen_insignia( "npc_citizen_insignia", "0" );
ConVar	npc_citizen_squad_marker( "npc_citizen_squad_marker", "0" );
ConVar	npc_citizen_explosive_resist( "npc_citizen_explosive_resist", "0" );
ConVar	npc_citizen_auto_player_squad( "npc_citizen_auto_player_squad", "1" );
ConVar	npc_citizen_auto_player_squad_allow_use( "npc_citizen_auto_player_squad_allow_use", "0" );


ConVar	npc_citizen_dont_precache_all( "npc_citizen_dont_precache_all", "0" );


ConVar  npc_citizen_medic_emit_sound("npc_citizen_medic_emit_sound", "1" );
#ifdef HL2_EPISODIC
// todo: bake these into pound constants (for now they're not just for tuning purposes)
ConVar  npc_citizen_heal_chuck_medkit("npc_citizen_heal_chuck_medkit" , "1" , FCVAR_ARCHIVE, "Set to 1 to use new experimental healthkit-throwing medic.");
ConVar npc_citizen_medic_throw_style( "npc_citizen_medic_throw_style", "1", FCVAR_ARCHIVE, "Set to 0 for a lobbier trajectory" );
ConVar npc_citizen_medic_throw_speed( "npc_citizen_medic_throw_speed", "650" );
ConVar	sk_citizen_heal_toss_player_delay("sk_citizen_heal_toss_player_delay", "26", FCVAR_NONE, "how long between throwing healthkits" );


#define MEDIC_THROW_SPEED npc_citizen_medic_throw_speed.GetFloat()
#define USE_EXPERIMENTAL_MEDIC_CODE() (npc_citizen_heal_chuck_medkit.GetBool() && NameMatches("griggs"))
#endif

ConVar player_squad_autosummon_time( "player_squad_autosummon_time", "5" );
ConVar player_squad_autosummon_move_tolerance( "player_squad_autosummon_move_tolerance", "20" );
ConVar player_squad_autosummon_player_tolerance( "player_squad_autosummon_player_tolerance", "10" );
ConVar player_squad_autosummon_time_after_combat( "player_squad_autosummon_time_after_combat", "8" );
ConVar player_squad_autosummon_debug( "player_squad_autosummon_debug", "0" );

#define ShouldAutosquad() (npc_citizen_auto_player_squad.GetBool())

enum SquadSlot_T
{
	SQUAD_SLOT_CITIZEN_RPG1	= LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_CITIZEN_RPG2,
};

const float HEAL_MOVE_RANGE = 30*12;
const float HEAL_TARGET_RANGE = 120; // 10 feet
#ifdef HL2_EPISODIC
const float HEAL_TOSS_TARGET_RANGE = 480; // 40 feet when we are throwing medkits 
const float HEAL_TARGET_RANGE_Z = 72; // a second check that Gordon isn't too far above us -- 6 feet
#endif

// player must be at least this distance away from an enemy before we fire an RPG at him
const float RPG_SAFE_DISTANCE = CMissile::EXPLOSION_RADIUS + 64.0;

// Animation events
int AE_CITIZEN_GET_PACKAGE;
int AE_CITIZEN_HEAL;

//-------------------------------------
//-------------------------------------

ConVar	ai_follow_move_commands( "ai_follow_move_commands", "1" );
ConVar	ai_citizen_debug_commander( "ai_citizen_debug_commander", "1" );
#define DebuggingCommanderMode() (ai_citizen_debug_commander.GetBool() && (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))

//-----------------------------------------------------------------------------
// Citizen expressions for the citizen expression types
//-----------------------------------------------------------------------------
#define STATES_WITH_EXPRESSIONS		3		// Idle, Alert, Combat
#define EXPRESSIONS_PER_STATE		1

char *szExpressionTypes[CIT_EXP_LAST_TYPE] =
{
	"Unassigned",
	"Scared",
	"Normal",
	"Angry"
};

struct citizen_expression_list_t
{
	char *szExpressions[EXPRESSIONS_PER_STATE];
};
// Scared
citizen_expression_list_t ScaredExpressions[STATES_WITH_EXPRESSIONS] =
{
	{ { "scenes/Expressions/citizen_scared_idle_01.vcd" } },
	{ { "scenes/Expressions/citizen_scared_alert_01.vcd" } },
	{ { "scenes/Expressions/citizen_scared_combat_01.vcd" } },
};
// Normal
citizen_expression_list_t NormalExpressions[STATES_WITH_EXPRESSIONS] =
{
	{ { "scenes/Expressions/citizen_normal_idle_01.vcd" } },
	{ { "scenes/Expressions/citizen_normal_alert_01.vcd" } },
	{ { "scenes/Expressions/citizen_normal_combat_01.vcd" } },
};
// Angry
citizen_expression_list_t AngryExpressions[STATES_WITH_EXPRESSIONS] =
{
	{ { "scenes/Expressions/citizen_angry_idle_01.vcd" } },
	{ { "scenes/Expressions/citizen_angry_alert_01.vcd" } },
	{ { "scenes/Expressions/citizen_angry_combat_01.vcd" } },
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define COMMAND_POINT_CLASSNAME "info_target_command_point"

class CCommandPoint : public CPointEntity
{
	DECLARE_CLASS( CCommandPoint, CPointEntity );
public:
	CCommandPoint()
		: m_bNotInTransition(false)
	{
		if ( ++gm_nCommandPoints > 1 )
			DevMsg( "WARNING: More than one citizen command point present\n" );
	}

	~CCommandPoint()
	{
		--gm_nCommandPoints;
	}

	int ObjectCaps()
	{
		int caps = ( BaseClass::ObjectCaps() | FCAP_NOTIFY_ON_TRANSITION );

		if ( m_bNotInTransition )
			caps |= FCAP_DONT_SAVE;

		return caps;
	}

	void InputOutsideTransition( inputdata_t &inputdata )
	{
		if ( !AI_IsSinglePlayer() )
			return;

		m_bNotInTransition = true;

		CAI_Squad *pPlayerAISquad = g_AI_SquadManager.FindSquad(AllocPooledString(PLAYER_SQUADNAME));

		if ( pPlayerAISquad )
		{
			AISquadIter_t iter;
			for ( CAI_BaseNPC *pAllyNpc = pPlayerAISquad->GetFirstMember(&iter); pAllyNpc; pAllyNpc = pPlayerAISquad->GetNextMember(&iter) )
			{
				if ( pAllyNpc->GetCommandGoal() != vec3_invalid )
				{
					bool bHadGag = pAllyNpc->HasSpawnFlags(SF_NPC_GAG);

					pAllyNpc->AddSpawnFlags(SF_NPC_GAG);
					pAllyNpc->TargetOrder( UTIL_GetLocalPlayer(), &pAllyNpc, 1 );
					if ( !bHadGag )
						pAllyNpc->RemoveSpawnFlags(SF_NPC_GAG);
				}
			}
		}
	}
	DECLARE_DATADESC();

private:
	bool m_bNotInTransition; // does not need to be saved. If this is ever not default, the object is not being saved.
	static int gm_nCommandPoints;
};

int CCommandPoint::gm_nCommandPoints;

LINK_ENTITY_TO_CLASS( info_target_command_point, CCommandPoint );
BEGIN_DATADESC( CCommandPoint )
	
//	DEFINE_FIELD( m_bNotInTransition,	FIELD_BOOLEAN ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"OutsideTransition",	InputOutsideTransition ),

END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

class CMattsPipe : public CWeaponCrowbar
{
	DECLARE_CLASS( CMattsPipe, CWeaponCrowbar );

	const char *GetWorldModel() const	{ return "models/props_canal/mattpipe.mdl"; }
	void SetPickupTouch( void )	{	/* do nothing */ }
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//---------------------------------------------------------
// Citizen models
//---------------------------------------------------------

static const char *g_ppszRandomHeads[] = 
{
	"male_01.mdl",
	"male_02.mdl",
	"female_01.mdl",
	"male_03.mdl",
	"female_02.mdl",
	"male_04.mdl",
	"female_03.mdl",
	"male_05.mdl",
	"female_04.mdl",
	"male_06.mdl",
	"female_06.mdl",
	"male_07.mdl",
	"female_07.mdl",
	"male_08.mdl",
	"male_09.mdl",
};

static const char *g_ppszModelLocs[] =
{
	"Group01",
	"Group01",
	"Group02",
	"Group03%s",
};

#define IsExcludedHead( type, bMedic, iHead) false // see XBox codeline for an implementation


//---------------------------------------------------------
// Citizen activities
//---------------------------------------------------------

int ACT_CIT_HANDSUP;
int	ACT_CIT_BLINDED;		// Blinded by scanner photo
int ACT_CIT_SHOWARMBAND;
int ACT_CIT_HEAL;
int	ACT_CIT_STARTLED;		// Startled by sneaky scanner

//---------------------------------------------------------

LINK_ENTITY_TO_CLASS( npc_citizen, CNPC_Citizen );

//---------------------------------------------------------

BEGIN_DATADESC( CNPC_Citizen )

	DEFINE_CUSTOM_FIELD( m_nInspectActivity,		ActivityDataOps() ),
	DEFINE_FIELD( 		m_flNextFearSoundTime, 		FIELD_TIME ),
	DEFINE_FIELD( 		m_flStopManhackFlinch, 		FIELD_TIME ),
	DEFINE_FIELD( 		m_fNextInspectTime, 		FIELD_TIME ),
	DEFINE_FIELD( 		m_flPlayerHealTime, 		FIELD_TIME ),
	DEFINE_FIELD(		m_flNextHealthSearchTime,	FIELD_TIME ),
	DEFINE_FIELD( 		m_flAllyHealTime, 			FIELD_TIME ),
//						gm_PlayerSquadEvaluateTimer
//						m_AssaultBehavior
//						m_FollowBehavior
//						m_StandoffBehavior
//						m_LeadBehavior
//						m_FuncTankBehavior
	DEFINE_FIELD( 		m_flPlayerGiveAmmoTime, 	FIELD_TIME ),
	DEFINE_KEYFIELD(	m_iszAmmoSupply, 			FIELD_STRING,	"ammosupply" ),
	DEFINE_KEYFIELD(	m_iAmmoAmount, 				FIELD_INTEGER,	"ammoamount" ),
	DEFINE_FIELD( 		m_bRPGAvoidPlayer, 			FIELD_BOOLEAN ),
	DEFINE_FIELD( 		m_bShouldPatrol, 			FIELD_BOOLEAN ),
	DEFINE_FIELD( 		m_iszOriginalSquad, 		FIELD_STRING ),
	DEFINE_FIELD( 		m_flTimeJoinedPlayerSquad,	FIELD_TIME ),
	DEFINE_FIELD( 		m_bWasInPlayerSquad, FIELD_BOOLEAN ),
	DEFINE_FIELD( 		m_flTimeLastCloseToPlayer,	FIELD_TIME ),
	DEFINE_EMBEDDED(	m_AutoSummonTimer ),
	DEFINE_FIELD(		m_vAutoSummonAnchor, FIELD_POSITION_VECTOR ),
	DEFINE_KEYFIELD(	m_Type, 					FIELD_INTEGER,	"citizentype" ),
	DEFINE_KEYFIELD(	m_ExpressionType,			FIELD_INTEGER,	"expressiontype" ),
	DEFINE_FIELD(		m_iHead,					FIELD_INTEGER ),
	DEFINE_FIELD(		m_flTimePlayerStare,		FIELD_TIME ),
	DEFINE_FIELD(		m_flTimeNextHealStare,		FIELD_TIME ),
	DEFINE_FIELD( 		m_hSavedFollowGoalEnt,		FIELD_EHANDLE ),
	DEFINE_KEYFIELD(	m_bNotifyNavFailBlocked,	FIELD_BOOLEAN, "notifynavfailblocked" ),
	DEFINE_KEYFIELD(	m_bNeverLeavePlayerSquad,	FIELD_BOOLEAN, "neverleaveplayersquad" ),
	DEFINE_KEYFIELD(	m_iszDenyCommandConcept,	FIELD_STRING, "denycommandconcept" ),

	DEFINE_OUTPUT(		m_OnJoinedPlayerSquad,	"OnJoinedPlayerSquad" ),
	DEFINE_OUTPUT(		m_OnLeftPlayerSquad,	"OnLeftPlayerSquad" ),
	DEFINE_OUTPUT(		m_OnFollowOrder,		"OnFollowOrder" ),
	DEFINE_OUTPUT(		m_OnStationOrder,		"OnStationOrder" ),
	DEFINE_OUTPUT(		m_OnPlayerUse,			"OnPlayerUse" ),
	DEFINE_OUTPUT(		m_OnNavFailBlocked,		"OnNavFailBlocked" ),

	DEFINE_INPUTFUNC( FIELD_VOID,	"RemoveFromPlayerSquad", InputRemoveFromPlayerSquad ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"StartPatrolling",	InputStartPatrolling ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"StopPatrolling",	InputStopPatrolling ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetCommandable",	InputSetCommandable ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetMedicOn",	InputSetMedicOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetMedicOff",	InputSetMedicOff ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetAmmoResupplierOn",	InputSetAmmoResupplierOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetAmmoResupplierOff",	InputSetAmmoResupplierOff ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SpeakIdleResponse", InputSpeakIdleResponse ),

#if HL2_EPISODIC
	DEFINE_INPUTFUNC( FIELD_VOID,   "ThrowHealthKit", InputForceHealthKitToss ),
#endif

	DEFINE_USEFUNC( CommanderUse ),
	DEFINE_USEFUNC( SimpleUse ),

END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CSimpleSimTimer CNPC_Citizen::gm_PlayerSquadEvaluateTimer;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CNPC_Citizen::CreateBehaviors()
{
	BaseClass::CreateBehaviors();
	AddBehavior( &m_FuncTankBehavior );
	
	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::Precache()
{
	SelectModel();
	SelectExpressionType();

	if ( !npc_citizen_dont_precache_all.GetBool() )
		PrecacheAllOfType( m_Type );
	else
		PrecacheModel( STRING( GetModelName() ) );

	if ( NameMatches( "matt" ) )
		PrecacheModel( "models/props_canal/mattpipe.mdl" );

	PrecacheModel( INSIGNIA_MODEL );

	PrecacheScriptSound( "NPC_Citizen.FootstepLeft" );
	PrecacheScriptSound( "NPC_Citizen.FootstepRight" );
	PrecacheScriptSound( "NPC_Citizen.Die" );

	PrecacheInstancedScene( "scenes/Expressions/CitizenIdle.vcd" );
	PrecacheInstancedScene( "scenes/Expressions/CitizenAlert_loop.vcd" );
	PrecacheInstancedScene( "scenes/Expressions/CitizenCombat_loop.vcd" );

	for ( int i = 0; i < STATES_WITH_EXPRESSIONS; i++ )
	{
		for ( int j = 0; j < ARRAYSIZE(ScaredExpressions[i].szExpressions); j++ )
		{
			PrecacheInstancedScene( ScaredExpressions[i].szExpressions[j] );
		}
		for ( int j = 0; j < ARRAYSIZE(NormalExpressions[i].szExpressions); j++ )
		{
			PrecacheInstancedScene( NormalExpressions[i].szExpressions[j] );
		}
		for ( int j = 0; j < ARRAYSIZE(AngryExpressions[i].szExpressions); j++ )
		{
			PrecacheInstancedScene( AngryExpressions[i].szExpressions[j] );
		}
	}

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::PrecacheAllOfType( CitizenType_t type )
{
	if ( m_Type == CT_UNIQUE )
		return;

	int nHeads = ARRAYSIZE( g_ppszRandomHeads );
	int i;
	for ( i = 0; i < nHeads; ++i )
	{
		if ( !IsExcludedHead( type, false, i ) )
		{
			PrecacheModel( CFmtStr( "models/Humans/%s/%s", (const char *)(CFmtStr(g_ppszModelLocs[m_Type], "")), g_ppszRandomHeads[i] ) );
		}
	}

	if ( m_Type == CT_REBEL )
	{
		for ( i = 0; i < nHeads; ++i )
		{
			if ( !IsExcludedHead( type, true, i ) )
			{
				PrecacheModel( CFmtStr( "models/Humans/%s/%s", (const char *)(CFmtStr(g_ppszModelLocs[m_Type], "m")), g_ppszRandomHeads[i] ) );
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::Spawn()
{
	BaseClass::Spawn();

#ifdef _XBOX
	// Always fade the corpse
	AddSpawnFlags( SF_NPC_FADE_CORPSE );
#endif // _XBOX

	if ( ShouldAutosquad() )
	{
		if ( m_SquadName == GetPlayerSquadName() )
		{
			CAI_Squad *pPlayerSquad = g_AI_SquadManager.FindSquad( GetPlayerSquadName() );
			if ( pPlayerSquad && pPlayerSquad->NumMembers() >= MAX_PLAYER_SQUAD )
				m_SquadName = NULL_STRING;
		}
		gm_PlayerSquadEvaluateTimer.Force();
	}

	if ( IsAmmoResupplier() )
		m_nSkin = 2;
	
	m_bRPGAvoidPlayer = false;

	m_bShouldPatrol = false;
	m_iHealth = sk_citizen_health.GetFloat();
	
	// Are we on a train? Used in trainstation to have NPCs on trains.
	if ( GetMoveParent() && FClassnameIs( GetMoveParent(), "func_tracktrain" ) )
	{
		CapabilitiesRemove( bits_CAP_MOVE_GROUND );
		SetMoveType( MOVETYPE_NONE );
		if ( NameMatches("citizen_train_2") )
		{
			SetSequenceByName( "d1_t01_TrainRide_Sit_Idle" );
			SetIdealActivity( ACT_DO_NOT_DISTURB );
		}
		else
		{
			SetSequenceByName( "d1_t01_TrainRide_Stand" );
			SetIdealActivity( ACT_DO_NOT_DISTURB );
		}
	}

	m_flStopManhackFlinch = -1;

	m_iszIdleExpression = MAKE_STRING("scenes/expressions/citizenidle.vcd");
	m_iszAlertExpression = MAKE_STRING("scenes/expressions/citizenalert_loop.vcd");
	m_iszCombatExpression = MAKE_STRING("scenes/expressions/citizencombat_loop.vcd");

	m_iszOriginalSquad = m_SquadName;

	m_flNextHealthSearchTime = gpGlobals->curtime;

	CWeaponRPG *pRPG = dynamic_cast<CWeaponRPG*>(GetActiveWeapon());
	if ( pRPG )
	{
		CapabilitiesRemove( bits_CAP_USE_SHOT_REGULATOR );
		pRPG->StopGuiding();
	}

	m_flTimePlayerStare = FLT_MAX;

	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );

	NPCInit();

	SetUse( &CNPC_Citizen::CommanderUse );
	Assert( !ShouldAutosquad() || !IsInPlayerSquad() );

	m_bWasInPlayerSquad = IsInPlayerSquad();

	// Use render bounds instead of human hull for guys sitting in chairs, etc.
	m_ActBusyBehavior.SetUseRenderBounds( HasSpawnFlags( SF_CITIZEN_USE_RENDER_BOUNDS ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::PostNPCInit()
{
	if ( !gEntList.FindEntityByClassname( NULL, COMMAND_POINT_CLASSNAME ) )
	{
		CreateEntityByName( COMMAND_POINT_CLASSNAME );
	}
	
	if ( IsInPlayerSquad() )
	{
		if ( m_pSquad->NumMembers() > MAX_PLAYER_SQUAD )
			DevMsg( "Error: Spawning citizen in player squad but exceeds squad limit of %d members\n", MAX_PLAYER_SQUAD );

		FixupPlayerSquad();
	}
	else
	{
		if ( ( m_spawnflags & SF_CITIZEN_FOLLOW ) && AI_IsSinglePlayer() )
		{
			m_FollowBehavior.SetFollowTarget( UTIL_GetLocalPlayer() );
			m_FollowBehavior.SetParameters( AIF_SIMPLE );
		}
	}

	BaseClass::PostNPCInit();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct HeadCandidate_t
{
	int iHead;
	int nHeads;

	static int __cdecl Sort( const HeadCandidate_t *pLeft, const HeadCandidate_t *pRight )
	{
		return ( pLeft->nHeads - pRight->nHeads );
	}
};

void CNPC_Citizen::SelectModel()
{
	// If making reslists, precache everything!!!
	static bool madereslists = false;

	if ( CommandLine()->CheckParm("-makereslists") && !madereslists )
	{
		madereslists = true;

		PrecacheAllOfType( CT_DOWNTRODDEN );
		PrecacheAllOfType( CT_REFUGEE );
		PrecacheAllOfType( CT_REBEL );
	}

	const char *pszModelName = NULL;

	if ( m_Type == CT_DEFAULT )
	{
		struct CitizenTypeMapping
		{
			const char *pszMapTag;
			CitizenType_t type;
		};

		static CitizenTypeMapping CitizenTypeMappings[] = 
		{
			{ "trainstation",	CT_DOWNTRODDEN	},
			{ "canals",			CT_REFUGEE		},
			{ "town",			CT_REFUGEE		},
			{ "coast",			CT_REFUGEE		},
			{ "prison",			CT_DOWNTRODDEN	},
			{ "c17",			CT_REBEL		},
			{ "citadel",		CT_DOWNTRODDEN	},
		};

		char szMapName[256];
		Q_strncpy(szMapName, STRING(gpGlobals->mapname), sizeof(szMapName) );
		Q_strlower(szMapName);

		for ( int i = 0; i < ARRAYSIZE(CitizenTypeMappings); i++ )
		{
			if ( Q_stristr( szMapName, CitizenTypeMappings[i].pszMapTag ) )
			{
				m_Type = CitizenTypeMappings[i].type;
				break;
			}
		}

		if ( m_Type == CT_DEFAULT )
			m_Type = CT_DOWNTRODDEN;
	}

	if( HasSpawnFlags( SF_CITIZEN_RANDOM_HEAD | SF_CITIZEN_RANDOM_HEAD_MALE | SF_CITIZEN_RANDOM_HEAD_FEMALE ) || GetModelName() == NULL_STRING )
	{
		Assert( m_iHead == -1 );
		char gender = ( HasSpawnFlags( SF_CITIZEN_RANDOM_HEAD_MALE ) ) ? 'm' : 
					  ( HasSpawnFlags( SF_CITIZEN_RANDOM_HEAD_FEMALE ) ) ? 'f' : 0;

		RemoveSpawnFlags( SF_CITIZEN_RANDOM_HEAD | SF_CITIZEN_RANDOM_HEAD_MALE | SF_CITIZEN_RANDOM_HEAD_FEMALE );
		if( HasSpawnFlags( SF_NPC_START_EFFICIENT ) )
		{
			SetModelName( AllocPooledString("models/humans/male_cheaple.mdl" ) );
			return;
		}
		else
		{
			// Count the heads
			int headCounts[ARRAYSIZE(g_ppszRandomHeads)] = { 0 };
			int i;

			for ( i = 0; i < g_AI_Manager.NumAIs(); i++ )
			{
				CNPC_Citizen *pCitizen = dynamic_cast<CNPC_Citizen *>(g_AI_Manager.AccessAIs()[i]);
				if ( pCitizen && pCitizen != this && pCitizen->m_iHead >= 0 && pCitizen->m_iHead < ARRAYSIZE(g_ppszRandomHeads) )
				{
					headCounts[pCitizen->m_iHead]++;
				}
			}

			// Find all candidates
			CUtlVectorFixed<HeadCandidate_t, ARRAYSIZE(g_ppszRandomHeads)> candidates;

			for ( i = 0; i < ARRAYSIZE(g_ppszRandomHeads); i++ )
			{
				if ( !gender || g_ppszRandomHeads[i][0] == gender )
				{
					if ( !IsExcludedHead( m_Type, IsMedic(), i ) )
					{
						HeadCandidate_t candidate = { i, headCounts[i] };
						candidates.AddToTail( candidate );
					}
				}
			}

			Assert( candidates.Count() );
			candidates.Sort( &HeadCandidate_t::Sort );

			int iSmallestCount = candidates[0].nHeads;
			int iLimit;

			for ( iLimit = 0; iLimit < candidates.Count(); iLimit++ )
			{
				if ( candidates[iLimit].nHeads > iSmallestCount )
					break;
			}

			m_iHead = candidates[random->RandomInt( 0, iLimit - 1 )].iHead;
			pszModelName = g_ppszRandomHeads[m_iHead];
			SetModelName(NULL_STRING);
		}
	}

	Assert( pszModelName || GetModelName() != NULL_STRING );

	if ( !pszModelName )
	{
		if ( GetModelName() == NULL_STRING )
			return;
		pszModelName = strrchr(STRING(GetModelName()), '/' );
		if ( !pszModelName )
			pszModelName = STRING(GetModelName());
		else
		{
			pszModelName++;
			if ( m_iHead == -1 )
			{
				for ( int i = 0; i < ARRAYSIZE(g_ppszRandomHeads); i++ )
				{
					if ( Q_stricmp( g_ppszRandomHeads[i], pszModelName ) == 0 )
					{
						m_iHead = i;
						break;
					}
				}
			}
		}
		if ( !*pszModelName )
			return;
	}

	// Unique citizen models are left alone
	if ( m_Type != CT_UNIQUE )
	{
		SetModelName( AllocPooledString( CFmtStr( "models/Humans/%s/%s", (const char *)(CFmtStr(g_ppszModelLocs[ m_Type ], ( IsMedic() ) ? "m" : "" )), pszModelName ) ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Citizen::SelectExpressionType()
{
	// If we've got a mapmaker assigned type, leave it alone
	if ( m_ExpressionType != CIT_EXP_UNASSIGNED )
		return;

	switch ( m_Type )
	{
	case CT_DOWNTRODDEN:
		m_ExpressionType = (CitizenExpressionTypes_t)RandomInt( CIT_EXP_SCARED, CIT_EXP_NORMAL );
		break;
	case CT_REFUGEE:
		m_ExpressionType = (CitizenExpressionTypes_t)RandomInt( CIT_EXP_SCARED, CIT_EXP_NORMAL );
		break;
	case CT_REBEL:
		m_ExpressionType = (CitizenExpressionTypes_t)RandomInt( CIT_EXP_SCARED, CIT_EXP_ANGRY );
		break;

	case CT_DEFAULT:
	case CT_UNIQUE:
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::FixupMattWeapon()
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon && pWeapon->ClassMatches( "weapon_crowbar" ) && NameMatches( "matt" ) )
	{
		Weapon_Drop( pWeapon );
		UTIL_Remove( pWeapon );
		pWeapon = (CBaseCombatWeapon *)CREATE_UNSAVED_ENTITY( CMattsPipe, "weapon_crowbar" );
		pWeapon->SetName( AllocPooledString( "matt_weapon" ) );
		DispatchSpawn( pWeapon );

#ifdef DEBUG
		extern bool g_bReceivedChainedActivate;
		g_bReceivedChainedActivate = false;
#endif
		pWeapon->Activate();
		Weapon_Equip( pWeapon );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CNPC_Citizen::Activate()
{
	BaseClass::Activate();
	FixupMattWeapon();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::OnRestore()
{
	gm_PlayerSquadEvaluateTimer.Force();

	BaseClass::OnRestore();

	if ( !gEntList.FindEntityByClassname( NULL, COMMAND_POINT_CLASSNAME ) )
	{
		CreateEntityByName( COMMAND_POINT_CLASSNAME );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
string_t CNPC_Citizen::GetModelName() const
{
	string_t iszModelName = BaseClass::GetModelName();

	//
	// If the model refers to an obsolete model, pretend it was blank
	// so that we pick the new default model.
	//
	if (!Q_strnicmp(STRING(iszModelName), "models/c17_", 11) ||
		!Q_strnicmp(STRING(iszModelName), "models/male", 11) ||
		!Q_strnicmp(STRING(iszModelName), "models/female", 13) ||
		!Q_strnicmp(STRING(iszModelName), "models/citizen", 14))
	{
		return NULL_STRING;
	}

	return iszModelName;
}

//-----------------------------------------------------------------------------
// Purpose: Overridden to switch our behavior between passive and rebel. We
//			become combative after Gordon becomes a criminal.
//-----------------------------------------------------------------------------
Class_T	CNPC_Citizen::Classify()
{
	if (GlobalEntity_GetState("gordon_precriminal") == GLOBAL_ON)
		return CLASS_CITIZEN_PASSIVE;

	if (GlobalEntity_GetState("citizens_passive") == GLOBAL_ON)
		return CLASS_CITIZEN_PASSIVE;

	return CLASS_PLAYER_ALLY;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldAlwaysThink() 
{ 
	return ( BaseClass::ShouldAlwaysThink() || IsInPlayerSquad() ); 
}
	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define CITIZEN_FOLLOWER_DESERT_FUNCTANK_DIST	45.0f*12.0f
bool CNPC_Citizen::ShouldBehaviorSelectSchedule( CAI_BehaviorBase *pBehavior )
{
	if( pBehavior == &m_FollowBehavior )
	{
		// Suppress follow behavior if I have a func_tank and the func tank is near
		// what I'm supposed to be following.
		if( m_FuncTankBehavior.CanSelectSchedule() )
		{
			// Is the tank close to the follow target?
			Vector vecTank = m_FuncTankBehavior.GetFuncTank()->WorldSpaceCenter();
			Vector vecFollowGoal = m_FollowBehavior.GetFollowGoalInfo().position;

			float flTankDistSqr = (vecTank - vecFollowGoal).LengthSqr();
			float flAllowDist = m_FollowBehavior.GetFollowGoalInfo().followPointTolerance * 2.0f;
			float flAllowDistSqr = flAllowDist * flAllowDist;
			if( flTankDistSqr < flAllowDistSqr )
			{
				// Deny follow behavior so the tank can go.
				return false;
			}
		}
	}
	else if( IsInPlayerSquad() && pBehavior == &m_FuncTankBehavior && m_FuncTankBehavior.IsMounted() )
	{
		if( m_FollowBehavior.GetFollowTarget() )
		{
			Vector vecFollowGoal = m_FollowBehavior.GetFollowTarget()->GetAbsOrigin();
			if( vecFollowGoal.DistToSqr( GetAbsOrigin() ) > Square(CITIZEN_FOLLOWER_DESERT_FUNCTANK_DIST) )
			{
				return false;
			}
		}
	}

	return BaseClass::ShouldBehaviorSelectSchedule( pBehavior );
}

void CNPC_Citizen::OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior )
{
	if ( pNewBehavior == &m_FuncTankBehavior )
	{
		m_bReadinessCapable = false;
	}
	else if ( pOldBehavior == &m_FuncTankBehavior )
	{
		m_bReadinessCapable = IsReadinessCapable();
	}

	BaseClass::OnChangeRunningBehavior( pOldBehavior, pNewBehavior );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::GatherConditions()
{
	BaseClass::GatherConditions();

	if( IsInPlayerSquad() && hl2_episodic.GetBool() )
	{
		// Leave the player squad if someone has made me neutral to player.
		if( IRelationType(UTIL_GetLocalPlayer()) == D_NU )
		{
			RemoveFromPlayerSquad();
		}
	}

	if ( !SpokeConcept( TLK_JOINPLAYER ) && IsRunningScriptedSceneWithSpeech( this, true ) )
	{
		SetSpokeConcept( TLK_JOINPLAYER, NULL );
		for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
			CAI_BaseNPC *pNpc = g_AI_Manager.AccessAIs()[i];
			if ( pNpc != this && pNpc->GetClassname() == GetClassname() && pNpc->GetAbsOrigin().DistToSqr( GetAbsOrigin() ) < Square( 15*12 ) && FVisible( pNpc ) )
			{
				(assert_cast<CNPC_Citizen *>(pNpc))->SetSpokeConcept( TLK_JOINPLAYER, NULL );
			}
		}
	}

	if( ShouldLookForHealthItem() )
	{
		if( FindHealthItem( GetAbsOrigin(), Vector( 240, 240, 240 ) ) )
			SetCondition( COND_HEALTH_ITEM_AVAILABLE );
		else
			ClearCondition( COND_HEALTH_ITEM_AVAILABLE );

		m_flNextHealthSearchTime = gpGlobals->curtime + 4.0;
	}

	// If the player is standing near a medic and can see the medic, 
	// assume the player is 'staring' and wants health.
	if( CanHeal() )
	{
		CBasePlayer *pPlayer = AI_GetSinglePlayer();

		if ( !pPlayer )
		{
			m_flTimePlayerStare = FLT_MAX;
			return;
		}

		float flDistSqr = ( GetAbsOrigin() - pPlayer->GetAbsOrigin() ).Length2DSqr();
		float flStareDist = sk_citizen_player_stare_dist.GetFloat();
		float flPlayerDamage = pPlayer->GetMaxHealth() - pPlayer->GetHealth();

		if( pPlayer->IsAlive() && flPlayerDamage > 0 && (flDistSqr <= flStareDist * flStareDist) && pPlayer->FInViewCone( this ) && pPlayer->FVisible( this ) )
		{
			if( m_flTimePlayerStare == FLT_MAX )
			{
				// Player wasn't looking at me at last think. He started staring now.
				m_flTimePlayerStare = gpGlobals->curtime;
			}

			// Heal if it's been long enough since last time I healed a staring player.
			if( gpGlobals->curtime - m_flTimePlayerStare >= sk_citizen_player_stare_time.GetFloat() && gpGlobals->curtime > m_flTimeNextHealStare && !IsCurSchedule( SCHED_CITIZEN_HEAL ) )
			{
				if ( ShouldHealTarget( pPlayer, true ) )
				{
					SetCondition( COND_CIT_PLAYERHEALREQUEST );
				}
				else
				{
					m_flTimeNextHealStare = gpGlobals->curtime + sk_citizen_stare_heal_time.GetFloat() * .5f;
					ClearCondition( COND_CIT_PLAYERHEALREQUEST );
				}
			}

#ifdef HL2_EPISODIC
			// Heal if I'm on an assault. The player hasn't had time to stare at me.
			if( m_AssaultBehavior.IsRunning() && IsMoving() )
			{
				SetCondition( COND_CIT_PLAYERHEALREQUEST );
			}
#endif
		}
		else
		{
			m_flTimePlayerStare = FLT_MAX;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::PredictPlayerPush()
{
	if ( !AI_IsSinglePlayer() )
		return;

	if ( HasCondition( COND_CIT_PLAYERHEALREQUEST ) )
		return;

	bool bHadPlayerPush = HasCondition( COND_PLAYER_PUSHING );

	BaseClass::PredictPlayerPush();

	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	if ( !bHadPlayerPush && HasCondition( COND_PLAYER_PUSHING ) && 
		 pPlayer->FInViewCone( this ) && CanHeal() )
	{
		if ( ShouldHealTarget( pPlayer, true ) )
		{
			ClearCondition( COND_PLAYER_PUSHING );
			SetCondition( COND_CIT_PLAYERHEALREQUEST );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::PrescheduleThink()
{
	BaseClass::PrescheduleThink();

	UpdatePlayerSquad();
	UpdateFollowCommandPoint();

	if ( !npc_citizen_insignia.GetBool() && npc_citizen_squad_marker.GetBool() && IsInPlayerSquad() )
	{
		Vector mins = WorldAlignMins() * .5 + GetAbsOrigin();
		Vector maxs = WorldAlignMaxs() * .5 + GetAbsOrigin();
		
		float rMax = 255;
		float gMax = 255;
		float bMax = 255;

		float rMin = 255;
		float gMin = 128;
		float bMin = 0;

		const float TIME_FADE = 1.0;
		float timeInSquad = gpGlobals->curtime - m_flTimeJoinedPlayerSquad;
		timeInSquad = MIN( TIME_FADE, MAX( timeInSquad, 0 ) );

		float fade = ( 1.0 - timeInSquad / TIME_FADE );

		float r = rMin + ( rMax - rMin ) * fade;
		float g = gMin + ( gMax - gMin ) * fade;
		float b = bMin + ( bMax - bMin ) * fade;

		// THIS IS A PLACEHOLDER UNTIL WE HAVE A REAL DESIGN & ART -- DO NOT REMOVE
		NDebugOverlay::Line( Vector( mins.x, GetAbsOrigin().y, GetAbsOrigin().z+1 ), Vector( maxs.x, GetAbsOrigin().y, GetAbsOrigin().z+1 ), r, g, b, false, .11 );
		NDebugOverlay::Line( Vector( GetAbsOrigin().x, mins.y, GetAbsOrigin().z+1 ), Vector( GetAbsOrigin().x, maxs.y, GetAbsOrigin().z+1 ), r, g, b, false, .11 );
	}
	if( GetEnemy() && g_ai_citizen_show_enemy.GetBool() )
	{
		NDebugOverlay::Line( EyePosition(), GetEnemy()->EyePosition(), 255, 0, 0, false, .1 );
	}
	
	if ( DebuggingCommanderMode() )
	{
		if ( HaveCommandGoal() )
		{
			CBaseEntity *pCommandPoint = gEntList.FindEntityByClassname( NULL, COMMAND_POINT_CLASSNAME );
			
			if ( pCommandPoint )
			{
				NDebugOverlay::Cross3D(pCommandPoint->GetAbsOrigin(), 16, 0, 255, 255, false, 0.1 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_Citizen::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	if ( IsCurSchedule( SCHED_IDLE_STAND ) || IsCurSchedule( SCHED_ALERT_STAND ) )
	{
		SetCustomInterruptCondition( COND_CIT_START_INSPECTION );
	}

	if ( IsMedic() && IsCustomInterruptConditionSet( COND_HEAR_MOVE_AWAY ) )
	{
		if( !IsCurSchedule(SCHED_RELOAD, false) )
		{
			// Since schedule selection code prioritizes reloading over requests to heal
			// the player, we must prevent this condition from breaking the reload schedule.
			SetCustomInterruptCondition( COND_CIT_PLAYERHEALREQUEST );
		}

		SetCustomInterruptCondition( COND_CIT_COMMANDHEAL );
	}

	if( !IsCurSchedule( SCHED_NEW_WEAPON ) )
	{
		SetCustomInterruptCondition( COND_RECEIVED_ORDERS );
	}

	if( GetCurSchedule()->HasInterrupt( COND_IDLE_INTERRUPT ) )
	{
		SetCustomInterruptCondition( COND_BETTER_WEAPON_AVAILABLE );
	}

#ifdef HL2_EPISODIC
	if( IsMedic() && m_AssaultBehavior.IsRunning() )
	{
		if( !IsCurSchedule(SCHED_RELOAD, false) )
		{
			SetCustomInterruptCondition( COND_CIT_PLAYERHEALREQUEST );
		}

		SetCustomInterruptCondition( COND_CIT_COMMANDHEAL );
	}
#else
	if( IsMedic() && m_AssaultBehavior.IsRunning() && !IsMoving() )
	{
		if( !IsCurSchedule(SCHED_RELOAD, false) )
		{
			SetCustomInterruptCondition( COND_CIT_PLAYERHEALREQUEST );
		}

		SetCustomInterruptCondition( COND_CIT_COMMANDHEAL );
	}
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::FInViewCone( CBaseEntity *pEntity )
{
#if 0
	if ( IsMortar( pEntity ) )
	{
		// @TODO (toml 11-20-03): do this only if have heard mortar shell recently and it's active
		return true;
	}
#endif
	return BaseClass::FInViewCone( pEntity );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	switch( failedSchedule )
	{
	case SCHED_NEW_WEAPON:
		// If failed trying to pick up a weapon, try again in one second. This is because other AI code
		// has put this off for 10 seconds under the assumption that the citizen would be able to 
		// pick up the weapon that they found. 
		m_flNextWeaponSearchTime = gpGlobals->curtime + 1.0f;
		break;

	case SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK:
	case SCHED_MOVE_TO_WEAPON_RANGE:
		if( !IsMortar( GetEnemy() ) )
		{
			if ( GetActiveWeapon() && ( GetActiveWeapon()->CapabilitiesGet() & bits_CAP_WEAPON_RANGE_ATTACK1 ) && random->RandomInt( 0, 1 ) && HasCondition(COND_SEE_ENEMY) && !HasCondition ( COND_NO_PRIMARY_AMMO ) )
				return TranslateSchedule( SCHED_RANGE_ATTACK1 );

			return SCHED_STANDOFF;
		}
		break;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectSchedule()
{
	// If we can't move, we're on a train, and should be sitting.
	if ( GetMoveType() == MOVETYPE_NONE )
	{
		// For now, we're only ever parented to trains. If you hit this assert, you've parented a citizen
		// to something else, and now we need to figure out a better system.
		Assert( GetMoveParent() && FClassnameIs( GetMoveParent(), "func_tracktrain" ) );
		return SCHED_CITIZEN_SIT_ON_TRAIN;
	}

	CWeaponRPG *pRPG = dynamic_cast<CWeaponRPG*>(GetActiveWeapon());
	if ( pRPG && pRPG->IsGuiding() )
	{
		DevMsg( "Citizen in select schedule but RPG is guiding?\n");
		pRPG->StopGuiding();
	}
	
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectSchedulePriorityAction()
{
	int schedule = SelectScheduleHeal();
	if ( schedule != SCHED_NONE )
		return schedule;

	schedule = BaseClass::SelectSchedulePriorityAction();
	if ( schedule != SCHED_NONE )
		return schedule;

	schedule = SelectScheduleRetrieveItem();
	if ( schedule != SCHED_NONE )
		return schedule;

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Determine if citizen should perform heal action.
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectScheduleHeal()
{
	// episodic medics may toss the healthkits rather than poke you with them
#if HL2_EPISODIC

	if ( CanHeal() )
	{
		CBaseEntity *pEntity = PlayerInRange( GetLocalOrigin(), HEAL_TOSS_TARGET_RANGE );
		if ( pEntity )
		{
			if ( USE_EXPERIMENTAL_MEDIC_CODE() && IsMedic() )
			{
				// use the new heal toss algorithm
				if ( ShouldHealTossTarget( pEntity, HasCondition( COND_CIT_PLAYERHEALREQUEST ) ) )
				{
					SetTarget( pEntity );
					return SCHED_CITIZEN_HEAL_TOSS;
				}
			}
			else if ( PlayerInRange( GetLocalOrigin(), HEAL_MOVE_RANGE ) )
			{
				// use old mechanism for ammo
				if ( ShouldHealTarget( pEntity, HasCondition( COND_CIT_PLAYERHEALREQUEST ) ) )
				{
					SetTarget( pEntity );
					return SCHED_CITIZEN_HEAL;
				}
			}

		}
		
		if ( m_pSquad )
		{
			pEntity = NULL;
			float distClosestSq = HEAL_MOVE_RANGE*HEAL_MOVE_RANGE;
			float distCurSq;
			
			AISquadIter_t iter;
			CAI_BaseNPC *pSquadmate = m_pSquad->GetFirstMember( &iter );
			while ( pSquadmate )
			{
				if ( pSquadmate != this )
				{
					distCurSq = ( GetAbsOrigin() - pSquadmate->GetAbsOrigin() ).LengthSqr();
					if ( distCurSq < distClosestSq && ShouldHealTarget( pSquadmate ) )
					{
						distClosestSq = distCurSq;
						pEntity = pSquadmate;
					}
				}

				pSquadmate = m_pSquad->GetNextMember( &iter );
			}
			
			if ( pEntity )
			{
				SetTarget( pEntity );
				return SCHED_CITIZEN_HEAL;
			}
		}
	}
	else
	{
		if ( HasCondition( COND_CIT_PLAYERHEALREQUEST ) )
			DevMsg( "Would say: sorry, need to recharge\n" );
	}
	
	return SCHED_NONE;

#else

	if ( CanHeal() )
	{
		CBaseEntity *pEntity = PlayerInRange( GetLocalOrigin(), HEAL_MOVE_RANGE );
		if ( pEntity && ShouldHealTarget( pEntity, HasCondition( COND_CIT_PLAYERHEALREQUEST ) ) )
		{
			SetTarget( pEntity );
			return SCHED_CITIZEN_HEAL;
		}

		if ( m_pSquad )
		{
			pEntity = NULL;
			float distClosestSq = HEAL_MOVE_RANGE*HEAL_MOVE_RANGE;
			float distCurSq;

			AISquadIter_t iter;
			CAI_BaseNPC *pSquadmate = m_pSquad->GetFirstMember( &iter );
			while ( pSquadmate )
			{
				if ( pSquadmate != this )
				{
					distCurSq = ( GetAbsOrigin() - pSquadmate->GetAbsOrigin() ).LengthSqr();
					if ( distCurSq < distClosestSq && ShouldHealTarget( pSquadmate ) )
					{
						distClosestSq = distCurSq;
						pEntity = pSquadmate;
					}
				}

				pSquadmate = m_pSquad->GetNextMember( &iter );
			}

			if ( pEntity )
			{
				SetTarget( pEntity );
				return SCHED_CITIZEN_HEAL;
			}
		}
	}
	else
	{
		if ( HasCondition( COND_CIT_PLAYERHEALREQUEST ) )
			DevMsg( "Would say: sorry, need to recharge\n" );
	}

	return SCHED_NONE;

#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectScheduleRetrieveItem()
{
	if ( HasCondition(COND_BETTER_WEAPON_AVAILABLE) )
	{
		CBaseHLCombatWeapon *pWeapon = dynamic_cast<CBaseHLCombatWeapon *>(Weapon_FindUsable( WEAPON_SEARCH_DELTA ));
		if ( pWeapon )
		{
			m_flNextWeaponSearchTime = gpGlobals->curtime + 10.0;
			// Now lock the weapon for several seconds while we go to pick it up.
			pWeapon->Lock( 10.0, this );
			SetTarget( pWeapon );
			return SCHED_NEW_WEAPON;
		}
	}

	if( HasCondition(COND_HEALTH_ITEM_AVAILABLE) )
	{
		if( !IsInPlayerSquad() )
		{
			// Been kicked out of the player squad since the time I located the health.
			ClearCondition( COND_HEALTH_ITEM_AVAILABLE );
		}
		else
		{
			CBaseEntity *pBase = FindHealthItem(m_FollowBehavior.GetFollowTarget()->GetAbsOrigin(), Vector( 120, 120, 120 ) );
			CItem *pItem = dynamic_cast<CItem *>(pBase);

			if( pItem )
			{
				SetTarget( pItem );
				return SCHED_GET_HEALTHKIT;
			}
		}
	}
	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectScheduleNonCombat()
{
	if ( m_NPCState == NPC_STATE_IDLE )
	{
		// Handle being inspected by the scanner
		if ( HasCondition( COND_CIT_START_INSPECTION ) )
		{
			ClearCondition( COND_CIT_START_INSPECTION );
			return SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY;
		}
	}
	
	ClearCondition( COND_CIT_START_INSPECTION );

	if ( m_bShouldPatrol )
		return SCHED_CITIZEN_PATROL;
	
	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectScheduleManhackCombat()
{
	if ( m_NPCState == NPC_STATE_COMBAT && IsManhackMeleeCombatant() )
	{
		if ( !HasCondition( COND_CAN_MELEE_ATTACK1 ) )
		{
			float distSqEnemy = ( GetEnemy()->GetAbsOrigin() - EyePosition() ).LengthSqr();
			if ( distSqEnemy < 48.0*48.0 &&
				 ( ( GetEnemy()->GetAbsOrigin() + GetEnemy()->GetSmoothedVelocity() * .1 ) - EyePosition() ).LengthSqr() < distSqEnemy )
				return SCHED_COWER;

			int iRoll = random->RandomInt( 1, 4 );
			if ( iRoll == 1 )
				return SCHED_BACK_AWAY_FROM_ENEMY;
			else if ( iRoll == 2 )
				return SCHED_CHASE_ENEMY;
		}
	}
	
	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::SelectScheduleCombat()
{
	int schedule = SelectScheduleManhackCombat();
	if ( schedule != SCHED_NONE )
		return schedule;
		
	return BaseClass::SelectScheduleCombat();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldDeferToFollowBehavior()
{
#if 0
	if ( HaveCommandGoal() )
		return false;
#endif
		
	return BaseClass::ShouldDeferToFollowBehavior();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::TranslateSchedule( int scheduleType ) 
{
	CBasePlayer *pLocalPlayer = AI_GetSinglePlayer();

	switch( scheduleType )
	{
	case SCHED_IDLE_STAND:
	case SCHED_ALERT_STAND:
		if( m_NPCState != NPC_STATE_COMBAT && pLocalPlayer && !pLocalPlayer->IsAlive() && CanJoinPlayerSquad() )
		{
			// Player is dead! 
			float flDist;
			flDist = ( pLocalPlayer->GetAbsOrigin() - GetAbsOrigin() ).Length();

			if( flDist < 50 * 12 )
			{
				AddSpawnFlags( SF_CITIZEN_NOT_COMMANDABLE );
				return SCHED_CITIZEN_MOURN_PLAYER;
			}
		}
		break;

	case SCHED_ESTABLISH_LINE_OF_FIRE:
	case SCHED_MOVE_TO_WEAPON_RANGE:
		if( !IsMortar( GetEnemy() ) && HaveCommandGoal() )
		{
			if ( GetActiveWeapon() && ( GetActiveWeapon()->CapabilitiesGet() & bits_CAP_WEAPON_RANGE_ATTACK1 ) && random->RandomInt( 0, 1 ) && HasCondition(COND_SEE_ENEMY) && !HasCondition ( COND_NO_PRIMARY_AMMO ) )
				return TranslateSchedule( SCHED_RANGE_ATTACK1 );

			return SCHED_STANDOFF;
		}
		break;

	case SCHED_CHASE_ENEMY:
		if( !IsMortar( GetEnemy() ) && HaveCommandGoal() )
		{
			return SCHED_STANDOFF;
		}
		break;

	case SCHED_RANGE_ATTACK1:
		// If we have an RPG, we use a custom schedule for it
		if ( !IsMortar( GetEnemy() ) && GetActiveWeapon() && FClassnameIs( GetActiveWeapon(), "weapon_rpg" ) )
		{
			if ( GetEnemy() && GetEnemy()->ClassMatches( "npc_strider" ) )
			{
				if (OccupyStrategySlotRange( SQUAD_SLOT_CITIZEN_RPG1, SQUAD_SLOT_CITIZEN_RPG2 ) )
				{
					return SCHED_CITIZEN_STRIDER_RANGE_ATTACK1_RPG;
				}
				else
				{
					return SCHED_STANDOFF;
				}
			}
			else
			{
				CBasePlayer *pPlayer = AI_GetSinglePlayer();
				if ( pPlayer && GetEnemy() && ( ( GetEnemy()->GetAbsOrigin() - 
					pPlayer->GetAbsOrigin() ).LengthSqr() < RPG_SAFE_DISTANCE * RPG_SAFE_DISTANCE ) )
				{
					// Don't fire our RPG at an enemy too close to the player
					return SCHED_STANDOFF;
				}
				else
				{
					return SCHED_CITIZEN_RANGE_ATTACK1_RPG;
				}
			}
		}
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldAcceptGoal( CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal )
{
	if ( BaseClass::ShouldAcceptGoal( pBehavior, pGoal ) )
	{
		CAI_FollowBehavior *pFollowBehavior = dynamic_cast<CAI_FollowBehavior *>(pBehavior );
		if ( pFollowBehavior )
		{
			if ( IsInPlayerSquad() )
			{
				m_hSavedFollowGoalEnt = (CAI_FollowGoal *)pGoal;
				return false;
			}
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::OnClearGoal( CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal )
{
	if ( m_hSavedFollowGoalEnt == pGoal )
		m_hSavedFollowGoalEnt = NULL;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_CIT_PLAY_INSPECT_SEQUENCE:
		SetIdealActivity( (Activity) m_nInspectActivity );
		break;

	case TASK_CIT_SIT_ON_TRAIN:
		if ( NameMatches("citizen_train_2") )
		{
			SetSequenceByName( "d1_t01_TrainRide_Sit_Idle" );
			SetIdealActivity( ACT_DO_NOT_DISTURB );
		}
		else
		{
			SetSequenceByName( "d1_t01_TrainRide_Stand" );
			SetIdealActivity( ACT_DO_NOT_DISTURB );
		}
		break;

	case TASK_CIT_LEAVE_TRAIN:
		if ( NameMatches("citizen_train_2") )
		{
			SetSequenceByName( "d1_t01_TrainRide_Sit_Exit" );
			SetIdealActivity( ACT_DO_NOT_DISTURB );
		}
		else
		{
			SetSequenceByName( "d1_t01_TrainRide_Stand_Exit" );
			SetIdealActivity( ACT_DO_NOT_DISTURB );
		}
		break;
		
	case TASK_CIT_HEAL:
#if HL2_EPISODIC
	case TASK_CIT_HEAL_TOSS:
#endif
		if ( IsMedic() )
		{
			if ( GetTarget() && GetTarget()->IsPlayer() && GetTarget()->m_iMaxHealth == GetTarget()->m_iHealth )
			{
				// Doesn't need us anymore
				TaskComplete();
				break;
			}

			Speak( TLK_HEAL );
		}
		else if ( IsAmmoResupplier() )
		{
			Speak( TLK_GIVEAMMO );
		}
		SetIdealActivity( (Activity)ACT_CIT_HEAL );
		break;
	
	case TASK_CIT_RPG_AUGER:
		m_bRPGAvoidPlayer = false;
		SetWait( 15.0 ); // maximum time auger before giving up
		break;

	case TASK_CIT_SPEAK_MOURNING:
		if ( !IsSpeaking() && CanSpeakAfterMyself() )
		{
 			//CAI_AllySpeechManager *pSpeechManager = GetAllySpeechManager();

			//if ( pSpeechManager-> )

			Speak(TLK_PLDEAD);
		}
		TaskComplete();
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
		case TASK_WAIT_FOR_MOVEMENT:
		{
			if ( IsManhackMeleeCombatant() )
			{
				AddFacingTarget( GetEnemy(), 1.0, 0.5 );
			}

			BaseClass::RunTask( pTask );
			break;
		}

		case TASK_MOVE_TO_TARGET_RANGE:
		{
			// If we're moving to heal a target, and the target dies, stop
			if ( IsCurSchedule( SCHED_CITIZEN_HEAL ) && (!GetTarget() || !GetTarget()->IsAlive()) )
			{
				TaskFail(FAIL_NO_TARGET);
				return;
			}

			BaseClass::RunTask( pTask );
			break;
		}

		case TASK_CIT_PLAY_INSPECT_SEQUENCE:
		{
			AutoMovement();
			
			if ( IsSequenceFinished() )
			{
				TaskComplete();
			}
			break;
		}
		case TASK_CIT_SIT_ON_TRAIN:
		{
			// If we were on a train, but we're not anymore, enable movement
			if ( !GetMoveParent() )
			{
				SetMoveType( MOVETYPE_STEP );
				CapabilitiesAdd( bits_CAP_MOVE_GROUND );
				TaskComplete();
			}
			break;
		}

		case TASK_CIT_LEAVE_TRAIN:
		{
			if ( IsSequenceFinished() )
			{
				SetupVPhysicsHull();
				TaskComplete();
			}
			break;
		}

		case TASK_CIT_HEAL:
			if ( IsSequenceFinished() )
			{
				TaskComplete();
			}
			else if (!GetTarget())
			{
				// Our heal target was killed or deleted somehow.
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				if ( ( GetTarget()->GetAbsOrigin() - GetAbsOrigin() ).Length2D() > HEAL_MOVE_RANGE/2 )
					TaskComplete();

				GetMotor()->SetIdealYawToTargetAndUpdate( GetTarget()->GetAbsOrigin() );
			}
			break;


#if HL2_EPISODIC
		case TASK_CIT_HEAL_TOSS:
			if ( IsSequenceFinished() )
			{
				TaskComplete();
			}
			else if (!GetTarget())
			{
				// Our heal target was killed or deleted somehow.
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				GetMotor()->SetIdealYawToTargetAndUpdate( GetTarget()->GetAbsOrigin() );
			}
			break;

#endif

		case TASK_CIT_RPG_AUGER:
			{
				// Keep augering until the RPG has been destroyed
				CWeaponRPG *pRPG = dynamic_cast<CWeaponRPG*>(GetActiveWeapon());
				if ( !pRPG )
				{
					TaskFail( FAIL_ITEM_NO_FIND );
					return;
				}

				// Has the RPG detonated?
				if ( !pRPG->GetMissile() )
				{
					pRPG->StopGuiding();
					TaskComplete();
					return;
				}

				Vector vecLaserPos = pRPG->GetNPCLaserPosition();

				if ( !m_bRPGAvoidPlayer )
				{
					// Abort if we've lost our enemy
					if ( !GetEnemy() )
					{
						pRPG->StopGuiding();
						TaskFail( FAIL_NO_ENEMY );
						return;
					}

					// Is our enemy occluded?
					if ( HasCondition( COND_ENEMY_OCCLUDED ) )
					{
						// Turn off the laserdot, but don't stop augering
						pRPG->StopGuiding();
						return;
					}
					else if ( pRPG->IsGuiding() == false )
					{
						pRPG->StartGuiding();
					}

					Vector vecEnemyPos = GetEnemy()->BodyTarget(GetAbsOrigin(), false);
					CBasePlayer *pPlayer = AI_GetSinglePlayer();
					if ( pPlayer && ( ( vecEnemyPos - pPlayer->GetAbsOrigin() ).LengthSqr() < RPG_SAFE_DISTANCE * RPG_SAFE_DISTANCE ) )
					{
						m_bRPGAvoidPlayer = true;
						Speak( TLK_WATCHOUT );
					}
					else
					{
						// Pull the laserdot towards the target
						Vector vecToTarget = (vecEnemyPos - vecLaserPos);
						float distToMove = VectorNormalize( vecToTarget );
						if ( distToMove > 90 )
							distToMove = 90;
						vecLaserPos += vecToTarget * distToMove;
					}
				}

				if ( m_bRPGAvoidPlayer )
				{
					// Pull the laserdot up
					vecLaserPos.z += 90;
				}

				if ( IsWaitFinished() )
				{
					pRPG->StopGuiding();
					TaskFail( FAIL_NO_SHOOT );
					return;
				}
				// Add imprecision to avoid obvious robotic perfection stationary targets
				float imprecision = 18*sin(gpGlobals->curtime);
				vecLaserPos.x += imprecision;
				vecLaserPos.y += imprecision;
				vecLaserPos.z += imprecision;
				pRPG->UpdateNPCLaserPosition( vecLaserPos );
			}
			break;

		default:
			BaseClass::RunTask( pTask );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
//-----------------------------------------------------------------------------
void CNPC_Citizen::TaskFail( AI_TaskFailureCode_t code )
{
	// If our heal task has failed, push out the heal time
	if ( IsCurSchedule( SCHED_CITIZEN_HEAL ) )
	{
		m_flPlayerHealTime 	= gpGlobals->curtime + sk_citizen_heal_ally_delay.GetFloat();
	}

	if( code == FAIL_NO_ROUTE_BLOCKED && m_bNotifyNavFailBlocked )
	{
		m_OnNavFailBlocked.FireOutput( this, this );
	}

	BaseClass::TaskFail( code );
}

//-----------------------------------------------------------------------------
// Purpose: Override base class activiites
//-----------------------------------------------------------------------------
Activity CNPC_Citizen::NPC_TranslateActivity( Activity activity )
{
	if ( activity == ACT_MELEE_ATTACK1 )
	{
		return ACT_MELEE_ATTACK_SWING;
	}

	// !!!HACK - Citizens don't have the required animations for shotguns, 
	// so trick them into using the rifle counterparts for now (sjb)
	if ( activity == ACT_RUN_AIM_SHOTGUN )
		return ACT_RUN_AIM_RIFLE;
	if ( activity == ACT_WALK_AIM_SHOTGUN )
		return ACT_WALK_AIM_RIFLE;
	if ( activity == ACT_IDLE_ANGRY_SHOTGUN )
		return ACT_IDLE_ANGRY_SMG1;
	if ( activity == ACT_RANGE_ATTACK_SHOTGUN_LOW )
		return ACT_RANGE_ATTACK_SMG1_LOW;

	return BaseClass::NPC_TranslateActivity( activity );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Citizen::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_CITIZEN_GET_PACKAGE )
	{
		// Give the citizen a package
		CBaseCombatWeapon *pWeapon = Weapon_Create( "weapon_citizenpackage" );
		if ( pWeapon )
		{
			// If I have a name, make my weapon match it with "_weapon" appended
			if ( GetEntityName() != NULL_STRING )
			{
				pWeapon->SetName( AllocPooledString(UTIL_VarArgs("%s_weapon", STRING(GetEntityName()) )) );
			}
			Weapon_Equip( pWeapon );
		}
		return;
	}
	else if ( pEvent->event == AE_CITIZEN_HEAL )
	{
		// Heal my target (if within range)
#if HL2_EPISODIC
		if ( USE_EXPERIMENTAL_MEDIC_CODE() && IsMedic() )
		{
			CBaseCombatCharacter *pTarget = dynamic_cast<CBaseCombatCharacter *>( GetTarget() );
			Assert(pTarget);
			if ( pTarget )
			{
				m_flPlayerHealTime 	= gpGlobals->curtime + sk_citizen_heal_toss_player_delay.GetFloat();;
				TossHealthKit( pTarget, Vector(48.0f, 0.0f, 0.0f)  );
			}
		}
		else
		{
			Heal();
		}
#else
		Heal();
#endif
		return;
	}

	switch( pEvent->event )
	{
	case NPC_EVENT_LEFTFOOT:
		{
			EmitSound( "NPC_Citizen.FootstepLeft", pEvent->eventtime );
		}
		break;

	case NPC_EVENT_RIGHTFOOT:
		{
			EmitSound( "NPC_Citizen.FootstepRight", pEvent->eventtime );
		}
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Citizen::PickupItem( CBaseEntity *pItem )
{
	Assert( pItem != NULL );
	if( FClassnameIs( pItem, "item_healthkit" ) )
	{
		if ( TakeHealth( sk_healthkit.GetFloat(), DMG_GENERIC ) )
		{
			RemoveAllDecals();
			UTIL_Remove( pItem );
		}
	}
	else if( FClassnameIs( pItem, "item_healthvial" ) )
	{
		if ( TakeHealth( sk_healthvial.GetFloat(), DMG_GENERIC ) )
		{
			RemoveAllDecals();
			UTIL_Remove( pItem );
		}
	}
	else
	{
		DevMsg("Citizen doesn't know how to pick up %s!\n", pItem->GetClassname() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Citizen::IgnorePlayerPushing( void )
{
	// If the NPC's on a func_tank that the player cannot man, ignore player pushing
	if ( m_FuncTankBehavior.IsMounted() )
	{
		CFuncTank *pTank = m_FuncTankBehavior.GetFuncTank();
		if ( pTank && !pTank->IsControllable() )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return a random expression for the specified state to play over 
//			the state's expression loop.
//-----------------------------------------------------------------------------
const char *CNPC_Citizen::SelectRandomExpressionForState( NPC_STATE state )
{
	// Hacky remap of NPC states to expression states that we care about
	int iExpressionState = 0;
	switch ( state )
	{
	case NPC_STATE_IDLE:
		iExpressionState = 0;
		break;

	case NPC_STATE_ALERT:
		iExpressionState = 1;
		break;

	case NPC_STATE_COMBAT:
		iExpressionState = 2;
		break;

	default:
		// An NPC state we don't have expressions for
		return NULL;
	}

	// Now pick the right one for our expression type
	switch ( m_ExpressionType )
	{
	case CIT_EXP_SCARED:
		{
			int iRandom = RandomInt( 0, ARRAYSIZE(ScaredExpressions[iExpressionState].szExpressions)-1 );
			return ScaredExpressions[iExpressionState].szExpressions[iRandom];
		}

	case CIT_EXP_NORMAL:
		{
			int iRandom = RandomInt( 0, ARRAYSIZE(NormalExpressions[iExpressionState].szExpressions)-1 );
			return NormalExpressions[iExpressionState].szExpressions[iRandom];
		}

	case CIT_EXP_ANGRY:
		{
			int iRandom = RandomInt( 0, ARRAYSIZE(AngryExpressions[iExpressionState].szExpressions)-1 );
			return AngryExpressions[iExpressionState].szExpressions[iRandom];
		}

	default:
		break;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::SimpleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Under these conditions, citizens will refuse to go with the player.
	// Robin: NPCs should always respond to +USE even if someone else has the semaphore.
	m_bDontUseSemaphore = true;

	// First, try to speak the +USE concept
	if ( !SelectPlayerUseSpeech() )
	{
		if ( HasSpawnFlags(SF_CITIZEN_NOT_COMMANDABLE) || IRelationType( pActivator ) == D_NU )
		{
			// If I'm denying commander mode because a level designer has made that decision,
			// then fire this output in case they've hooked it to an event.
			m_OnDenyCommanderUse.FireOutput( this, this );
		}
	}

	m_bDontUseSemaphore = false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::OnBeginMoveAndShoot()
{
	if ( BaseClass::OnBeginMoveAndShoot() )
	{
		if( m_iMySquadSlot == SQUAD_SLOT_ATTACK1 || m_iMySquadSlot == SQUAD_SLOT_ATTACK2 )
			return true; // already have the slot I need

		if( m_iMySquadSlot == SQUAD_SLOT_NONE && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::OnEndMoveAndShoot()
{
	VacateStrategySlot();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::LocateEnemySound()
{
#if 0
	if ( !GetEnemy() )
		return;

	float flZDiff = GetLocalOrigin().z - GetEnemy()->GetLocalOrigin().z;

	if( flZDiff < -128 )
	{
		EmitSound( "NPC_Citizen.UpThere" );
	}
	else if( flZDiff > 128 )
	{
		EmitSound( "NPC_Citizen.DownThere" );
	}
	else
	{
		EmitSound( "NPC_Citizen.OverHere" );
	}
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::IsManhackMeleeCombatant()
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	CBaseEntity *pEnemy = GetEnemy();
	return ( pEnemy && pWeapon && pEnemy->Classify() == CLASS_MANHACK && pWeapon->ClassMatches( "weapon_crowbar" ) );
}

//-----------------------------------------------------------------------------
// Purpose: Return the actual position the NPC wants to fire at when it's trying
//			to hit it's current enemy.
//-----------------------------------------------------------------------------
Vector CNPC_Citizen::GetActualShootPosition( const Vector &shootOrigin )
{
	Vector vecTarget = BaseClass::GetActualShootPosition( shootOrigin );

	CWeaponRPG *pRPG = dynamic_cast<CWeaponRPG*>(GetActiveWeapon());
	// If we're firing an RPG at a gunship, aim off to it's side, because we'll auger towards it.
	if ( pRPG && GetEnemy() )
	{
		if ( FClassnameIs( GetEnemy(), "npc_combinegunship" ) )
		{
			Vector vecRight;
			GetVectors( NULL, &vecRight, NULL );
			// Random height
			vecRight.z = 0;

			// Find a clear shot by checking for clear shots around it
			float flShotOffsets[] =
			{
				512,
				-512,
				128,
				-128
			};
			for ( int i = 0; i < ARRAYSIZE(flShotOffsets); i++ )
			{
				Vector vecTest = vecTarget + (vecRight * flShotOffsets[i]);
				// Add some random height to it
				vecTest.z += RandomFloat( -512, 512 );
				trace_t tr;
				AI_TraceLine( shootOrigin, vecTest, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

				// If we can see the point, it's a clear shot
				if ( tr.fraction == 1.0 && tr.m_pEnt != GetEnemy() )
				{
					pRPG->SetNPCLaserPosition( vecTest );
					return vecTest;
				}
			}
		}
		else
		{
			pRPG->SetNPCLaserPosition( vecTarget );
		}

	}

	return vecTarget;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	if ( pNewWeapon )
	{
		GetShotRegulator()->SetParameters( pNewWeapon->GetMinBurst(), pNewWeapon->GetMaxBurst(), pNewWeapon->GetMinRestTime(), pNewWeapon->GetMaxRestTime() );
	}
	BaseClass::OnChangeActiveWeapon( pOldWeapon, pNewWeapon );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define SHOTGUN_DEFER_SEARCH_TIME	20.0f
#define OTHER_DEFER_SEARCH_TIME		FLT_MAX
bool CNPC_Citizen::ShouldLookForBetterWeapon()
{
	if ( BaseClass::ShouldLookForBetterWeapon() )
	{
		if ( IsInPlayerSquad() && (GetActiveWeapon()&&IsMoving()) && ( m_FollowBehavior.GetFollowTarget() && m_FollowBehavior.GetFollowTarget()->IsPlayer() ) )
		{
			// For citizens in the player squad, you must be unarmed, or standing still (if armed) in order to 
			// divert attention to looking for a new weapon.
			return false;
		}

		if ( GetActiveWeapon() && IsMoving() )
			return false;

		if ( GlobalEntity_GetState("gordon_precriminal") == GLOBAL_ON )
		{
			// This stops the NPC looking altogether.
			m_flNextWeaponSearchTime = FLT_MAX;
			return false;
		}

#ifdef DBGFLAG_ASSERT
		// Cached off to make sure you change this if you ask the code to defer.
		float flOldWeaponSearchTime = m_flNextWeaponSearchTime;
#endif

		CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		if( pWeapon )
		{
			bool bDefer = false;

			if( FClassnameIs( pWeapon, "weapon_ar2" ) )
			{
				// Content to keep this weapon forever
				m_flNextWeaponSearchTime = OTHER_DEFER_SEARCH_TIME;
				bDefer = true;
			}
			else if( FClassnameIs( pWeapon, "weapon_rpg" ) )
			{
				// Content to keep this weapon forever
				m_flNextWeaponSearchTime = OTHER_DEFER_SEARCH_TIME;
				bDefer = true;
			}
			else if( FClassnameIs( pWeapon, "weapon_shotgun" ) )
			{
				// Shotgunners do not defer their weapon search indefinitely.
				// If more than one citizen in the squad has a shotgun, we force
				// some of them to trade for another weapon.
				if( NumWeaponsInSquad("weapon_shotgun") > 1 )
				{
					// Check for another weapon now. If I don't find one, this code will
					// retry in 2 seconds or so.
					bDefer = false;
				}
				else
				{
					// I'm the only shotgunner in the group right now, so I'll check
					// again in 3 0seconds or so. This code attempts to distribute
					// the desire to reduce shotguns amongst squadmates so that all 
					// shotgunners do not discard their weapons when they suddenly realize
					// the squad has too many.
					if( random->RandomInt( 0, 1 ) == 0 )
					{
						m_flNextWeaponSearchTime = gpGlobals->curtime + SHOTGUN_DEFER_SEARCH_TIME;
					}
					else
					{
						m_flNextWeaponSearchTime = gpGlobals->curtime + SHOTGUN_DEFER_SEARCH_TIME + 10.0f;
					}

					bDefer = true;
				}
			}

			if( bDefer )
			{
				// I'm happy with my current weapon. Don't search now.
				// If you ask the code to defer, you must have set m_flNextWeaponSearchTime to when
				// you next want to try to search.
				Assert( m_flNextWeaponSearchTime != flOldWeaponSearchTime );
				return false;
			}
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if( (info.GetDamageType() & DMG_BURN) && (info.GetDamageType() & DMG_DIRECT) )
	{
#define CITIZEN_SCORCH_RATE		6
#define CITIZEN_SCORCH_FLOOR	75

		Scorch( CITIZEN_SCORCH_RATE, CITIZEN_SCORCH_FLOOR );
	}

	CTakeDamageInfo newInfo = info;

	if( IsInSquad() && (info.GetDamageType() & DMG_BLAST) && info.GetInflictor() )
	{
		if( npc_citizen_explosive_resist.GetBool() )
		{
			// Blast damage. If this kills a squad member, give the 
			// remaining citizens a resistance bonus to this inflictor
			// to try to avoid having the entire squad wiped out by a
			// single explosion.
			if( m_pSquad->IsSquadInflictor( info.GetInflictor() ) )
			{
				newInfo.ScaleDamage( 0.5 );
			}
			else
			{
				// If this blast is going to kill me, designate the inflictor
				// so that the rest of the squad can enjoy a damage resist.
				if( info.GetDamage() >= GetHealth() )
				{
					m_pSquad->SetSquadInflictor( info.GetInflictor() );
				}
			}
		}
	}

	return BaseClass::OnTakeDamage_Alive( newInfo );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::IsCommandable() 
{
	return ( !HasSpawnFlags(SF_CITIZEN_NOT_COMMANDABLE) && IsInPlayerSquad() );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::IsPlayerAlly( CBasePlayer *pPlayer )											
{ 
	if ( Classify() == CLASS_CITIZEN_PASSIVE && GlobalEntity_GetState("gordon_precriminal") == GLOBAL_ON )
	{
		// Robin: Citizens use friendly speech semaphore in trainstation
		return true;
	}

	return BaseClass::IsPlayerAlly( pPlayer );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::CanJoinPlayerSquad()
{
	if ( !AI_IsSinglePlayer() )
		return false;

	if ( m_NPCState == NPC_STATE_SCRIPT || m_NPCState == NPC_STATE_PRONE )
		return false;

	if ( HasSpawnFlags(SF_CITIZEN_NOT_COMMANDABLE) )
		return false;

	if ( IsInAScript() )
		return false;

	// Don't bother people who don't want to be bothered
	if ( !CanBeUsedAsAFriend() )
		return false;

	if ( IRelationType( UTIL_GetLocalPlayer() ) != D_LI )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::WasInPlayerSquad()
{
	return m_bWasInPlayerSquad;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::HaveCommandGoal() const			
{	
	if (GetCommandGoal() != vec3_invalid)
		return true;
	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::IsCommandMoving()
{
	if ( AI_IsSinglePlayer() && IsInPlayerSquad() )
	{
		if ( m_FollowBehavior.GetFollowTarget() == UTIL_GetLocalPlayer() ||
			 IsFollowingCommandPoint() )
		{
			return ( m_FollowBehavior.IsMovingToFollowTarget() );
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldAutoSummon()
{
	if ( !AI_IsSinglePlayer() || !IsFollowingCommandPoint() || !IsInPlayerSquad() )
		return false;

	CHL2_Player *pPlayer = (CHL2_Player *)UTIL_GetLocalPlayer();
	
	float distMovedSq = ( pPlayer->GetAbsOrigin() - m_vAutoSummonAnchor ).LengthSqr();
	float moveTolerance = player_squad_autosummon_move_tolerance.GetFloat() * 12;
	const Vector &vCommandGoal = GetCommandGoal();

	if ( distMovedSq < Square(moveTolerance * 10) && (GetAbsOrigin() - vCommandGoal).LengthSqr() > Square(10*12) && IsCommandMoving() )
	{
		m_AutoSummonTimer.Set( player_squad_autosummon_time.GetFloat() );
		if ( player_squad_autosummon_debug.GetBool() )
			DevMsg( "Waiting for arrival before initiating autosummon logic\n");
	}
	else if ( m_AutoSummonTimer.Expired() )
	{
		bool bSetFollow = false;
		bool bTestEnemies = true;
		
		// Auto summon unconditionally if a significant amount of time has passed
		if ( gpGlobals->curtime - m_AutoSummonTimer.GetNext() > player_squad_autosummon_time.GetFloat() * 2 )
		{
			bSetFollow = true;
			if ( player_squad_autosummon_debug.GetBool() )
				DevMsg( "Auto summoning squad: long time (%f)\n", ( gpGlobals->curtime - m_AutoSummonTimer.GetNext() ) + player_squad_autosummon_time.GetFloat() );
		}
			
		// Player must move for autosummon
		if ( distMovedSq > Square(12) )
		{
			bool bCommandPointIsVisible = pPlayer->FVisible( vCommandGoal + pPlayer->GetViewOffset() );

			// Auto summon if the player is close by the command point
			if ( !bSetFollow && bCommandPointIsVisible && distMovedSq > Square(24) )
			{
				float closenessTolerance = player_squad_autosummon_player_tolerance.GetFloat() * 12;
				if ( (pPlayer->GetAbsOrigin() - vCommandGoal).LengthSqr() < Square( closenessTolerance ) &&
					 ((m_vAutoSummonAnchor - vCommandGoal).LengthSqr() > Square( closenessTolerance )) )
				{
					bSetFollow = true;
					if ( player_squad_autosummon_debug.GetBool() )
						DevMsg( "Auto summoning squad: player close to command point (%f)\n", (GetAbsOrigin() - vCommandGoal).Length() );
				}
			}
			
			// Auto summon if moved a moderate distance and can't see command point, or moved a great distance
			if ( !bSetFollow )
			{
				if ( distMovedSq > Square( moveTolerance * 2 ) )
				{
					bSetFollow = true;
					bTestEnemies = ( distMovedSq < Square( moveTolerance * 10 ) );
					if ( player_squad_autosummon_debug.GetBool() )
						DevMsg( "Auto summoning squad: player very far from anchor (%f)\n", sqrt(distMovedSq) );
				}
				else if ( distMovedSq > Square( moveTolerance ) )
				{
					if ( !bCommandPointIsVisible )
					{
						bSetFollow = true;
						if ( player_squad_autosummon_debug.GetBool() )
							DevMsg( "Auto summoning squad: player far from anchor (%f)\n", sqrt(distMovedSq) );
					}
				}
			}
		}
		
		// Auto summon only if there are no readily apparent enemies
		if ( bSetFollow && bTestEnemies )
		{
			for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
			{
				CAI_BaseNPC *pNpc = g_AI_Manager.AccessAIs()[i];
				float timeSinceCombatTolerance = player_squad_autosummon_time_after_combat.GetFloat();
				
				if ( pNpc->IsInPlayerSquad() )
				{
					if ( gpGlobals->curtime - pNpc->GetLastAttackTime() > timeSinceCombatTolerance || 
						 gpGlobals->curtime - pNpc->GetLastDamageTime() > timeSinceCombatTolerance )
						continue;
				}
				else if ( pNpc->GetEnemy() )
				{
					CBaseEntity *pNpcEnemy = pNpc->GetEnemy();
					if ( !IsSniper( pNpc ) && ( gpGlobals->curtime - pNpc->GetEnemyLastTimeSeen() ) > timeSinceCombatTolerance )
						continue;

					if ( pNpcEnemy == pPlayer )
					{
						if ( pNpc->CanBeAnEnemyOf( pPlayer ) )
						{
							bSetFollow = false;
							break;
						}
					}
					else if ( pNpcEnemy->IsNPC() && ( pNpcEnemy->MyNPCPointer()->GetSquad() == GetSquad() || pNpcEnemy->Classify() == CLASS_PLAYER_ALLY_VITAL ) )
					{
						if ( pNpc->CanBeAnEnemyOf( this ) )
						{
							bSetFollow = false;
							break;
						}
					}
				}
			}
			if ( !bSetFollow && player_squad_autosummon_debug.GetBool() )
				DevMsg( "Auto summon REVOKED: Combat recent \n");
		}
		
		return bSetFollow;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Is this entity something that the citizen should interact with (return true)
// or something that he should try to get close to (return false)
//-----------------------------------------------------------------------------
bool CNPC_Citizen::IsValidCommandTarget( CBaseEntity *pTarget )
{
	return false;
}

//-----------------------------------------------------------------------------
bool CNPC_Citizen::SpeakCommandResponse( AIConcept_t concept, const char *modifiers )
{
	return SpeakIfAllowed( concept, 
						   CFmtStr( "numselected:%d,"
									"useradio:%d%s",
									( GetSquad() ) ? GetSquad()->NumMembers() : 1,
									ShouldSpeakRadio( AI_GetSinglePlayer() ),
									( modifiers ) ? CFmtStr(",%s", modifiers).operator const char *() : "" ) );
}

//-----------------------------------------------------------------------------
// Purpose: return TRUE if the commander mode should try to give this order
//			to more people. return FALSE otherwise. For instance, we don't
//			try to send all 3 selectedcitizens to pick up the same gun.
//-----------------------------------------------------------------------------
bool CNPC_Citizen::TargetOrder( CBaseEntity *pTarget, CAI_BaseNPC **Allies, int numAllies )
{
	if ( pTarget->IsPlayer() )
	{
		// I'm the target! Toggle follow!
		if( m_FollowBehavior.GetFollowTarget() != pTarget )
		{
			ClearFollowTarget();
			SetCommandGoal( vec3_invalid );

			// Turn follow on!
			m_AssaultBehavior.Disable();
			m_FollowBehavior.SetFollowTarget( pTarget );
			m_FollowBehavior.SetParameters( AIF_SIMPLE );			
			SpeakCommandResponse( TLK_STARTFOLLOW );

			m_OnFollowOrder.FireOutput( this, this );
		}
		else if ( m_FollowBehavior.GetFollowTarget() == pTarget )
		{
			// Stop following.
			m_FollowBehavior.SetFollowTarget( NULL );
			SpeakCommandResponse( TLK_STOPFOLLOW );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Turn off following before processing a move order.
//-----------------------------------------------------------------------------
void CNPC_Citizen::MoveOrder( const Vector &vecDest, CAI_BaseNPC **Allies, int numAllies )
{
	if ( !AI_IsSinglePlayer() )
		return;

	if( hl2_episodic.GetBool() && m_iszDenyCommandConcept != NULL_STRING )
	{
		SpeakCommandResponse( STRING(m_iszDenyCommandConcept) );
		return;
	}

	CHL2_Player *pPlayer = (CHL2_Player *)UTIL_GetLocalPlayer();

	m_AutoSummonTimer.Set( player_squad_autosummon_time.GetFloat() );
	m_vAutoSummonAnchor = pPlayer->GetAbsOrigin();

	if( m_StandoffBehavior.IsRunning() )
	{
		m_StandoffBehavior.SetStandoffGoalPosition( vecDest );
	}

	// If in assault, cancel and move.
	if( m_AssaultBehavior.HasHitRallyPoint() && !m_AssaultBehavior.HasHitAssaultPoint() )
	{
		m_AssaultBehavior.Disable();
		ClearSchedule( "Moving from rally point to assault point" );
	}

	bool spoke = false;

	CAI_BaseNPC *pClosest = NULL;
	float closestDistSq = FLT_MAX;

	for( int i = 0 ; i < numAllies ; i++ )
	{
		if( Allies[i]->IsInPlayerSquad() )
		{
			Assert( Allies[i]->IsCommandable() );
			float distSq = ( pPlayer->GetAbsOrigin() - Allies[i]->GetAbsOrigin() ).LengthSqr();
			if( distSq < closestDistSq )
			{
				pClosest = Allies[i];
				closestDistSq = distSq;
			}
		}
	}

	if( m_FollowBehavior.GetFollowTarget() && !IsFollowingCommandPoint() )
	{
		ClearFollowTarget();
#if 0
		if ( ( pPlayer->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr() < Square( 180 ) &&
			 ( ( vecDest - pPlayer->GetAbsOrigin() ).LengthSqr() < Square( 120 ) || 
			   ( vecDest - GetAbsOrigin() ).LengthSqr() < Square( 120 ) ) )
		{
			if ( pClosest == this )
				SpeakIfAllowed( TLK_STOPFOLLOW );
			spoke = true;
		}
#endif
	}

	if ( !spoke && pClosest == this )
	{
		float destDistToPlayer = ( vecDest - pPlayer->GetAbsOrigin() ).Length();
		float destDistToClosest = ( vecDest - GetAbsOrigin() ).Length();
		CFmtStr modifiers( "commandpoint_dist_to_player:%.0f,"
						   "commandpoint_dist_to_npc:%.0f",
						   destDistToPlayer,
						   destDistToClosest );

		SpeakCommandResponse( TLK_COMMANDED, modifiers );
	}

	m_OnStationOrder.FireOutput( this, this );

	BaseClass::MoveOrder( vecDest, Allies, numAllies );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::OnMoveOrder()
{
	SetReadinessLevel( AIRL_STIMULATED, false, false );
	BaseClass::OnMoveOrder();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::CommanderUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_OnPlayerUse.FireOutput( pActivator, pCaller );

	// Under these conditions, citizens will refuse to go with the player.
	// Robin: NPCs should always respond to +USE even if someone else has the semaphore.
	if ( !AI_IsSinglePlayer() || !CanJoinPlayerSquad() )
	{
		SimpleUse( pActivator, pCaller, useType, value );
		return;
	}
	
	if ( pActivator == UTIL_GetLocalPlayer() )
	{
		// Don't say hi after you've been addressed by the player
		SetSpokeConcept( TLK_HELLO, NULL );	

		if ( npc_citizen_auto_player_squad_allow_use.GetBool() )
		{
			if ( !ShouldAutosquad() )
				TogglePlayerSquadState();
			else if ( !IsInPlayerSquad() && npc_citizen_auto_player_squad_allow_use.GetBool() )
				AddToPlayerSquad();
		}
		else if ( GetCurSchedule() && ConditionInterruptsCurSchedule( COND_IDLE_INTERRUPT ) )
		{
			if ( SpeakIfAllowed( TLK_QUESTION, NULL, true ) )
			{
				if ( random->RandomInt( 1, 4 ) < 4 )
				{
					CBaseEntity *pRespondant = FindSpeechTarget( AIST_NPCS );
					if ( pRespondant )
					{
						g_EventQueue.AddEvent( pRespondant, "SpeakIdleResponse", ( GetTimeSpeechComplete() - gpGlobals->curtime ) + .2, this, this );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldSpeakRadio( CBaseEntity *pListener )
{
	if ( !pListener )
		return false;

	const float		radioRange = 384 * 384;
	Vector			vecDiff;

	vecDiff = WorldSpaceCenter() - pListener->WorldSpaceCenter();

	if( vecDiff.LengthSqr() > radioRange )
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::OnMoveToCommandGoalFailed()
{
	// Clear the goal.
	SetCommandGoal( vec3_invalid );

	// Announce failure.
	SpeakCommandResponse( TLK_COMMAND_FAILED );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::AddToPlayerSquad()
{
	Assert( !IsInPlayerSquad() );

	AddToSquad( AllocPooledString(PLAYER_SQUADNAME) );
	m_hSavedFollowGoalEnt = m_FollowBehavior.GetFollowGoal();
	m_FollowBehavior.SetFollowGoalDirect( NULL );

	FixupPlayerSquad();

	SetCondition( COND_PLAYER_ADDED_TO_SQUAD );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::RemoveFromPlayerSquad()
{
	Assert( IsInPlayerSquad() );

	ClearFollowTarget();
	ClearCommandGoal();
	if ( m_iszOriginalSquad != NULL_STRING && strcmp( STRING( m_iszOriginalSquad ), PLAYER_SQUADNAME ) != 0 )
		AddToSquad( m_iszOriginalSquad );
	else
		RemoveFromSquad();
	
	if ( m_hSavedFollowGoalEnt )
		m_FollowBehavior.SetFollowGoal( m_hSavedFollowGoalEnt );

	SetCondition( COND_PLAYER_REMOVED_FROM_SQUAD );

	// Don't evaluate the player squad for 2 seconds. 
	gm_PlayerSquadEvaluateTimer.Set( 2.0 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::TogglePlayerSquadState()
{
	if ( !AI_IsSinglePlayer() )
		return;

	if ( !IsInPlayerSquad() )
	{
		AddToPlayerSquad();

		if ( HaveCommandGoal() )
		{
			SpeakCommandResponse( TLK_COMMANDED );
		}
		else if ( m_FollowBehavior.GetFollowTarget() == UTIL_GetLocalPlayer() )
		{
			SpeakCommandResponse( TLK_STARTFOLLOW );
		}
	}
	else
	{
		SpeakCommandResponse( TLK_STOPFOLLOW );
		RemoveFromPlayerSquad();
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct SquadCandidate_t
{
	CNPC_Citizen *pCitizen;
	bool		  bIsInSquad;
	float		  distSq;
	int			  iSquadIndex;
};

void CNPC_Citizen::UpdatePlayerSquad()
{
	if ( !AI_IsSinglePlayer() )
		return;

	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	if ( ( pPlayer->GetAbsOrigin().AsVector2D() - GetAbsOrigin().AsVector2D() ).LengthSqr() < Square(20*12) )
		m_flTimeLastCloseToPlayer = gpGlobals->curtime;

	if ( !gm_PlayerSquadEvaluateTimer.Expired() )
		return;

	gm_PlayerSquadEvaluateTimer.Set( 2.0 );

	// Remove stragglers
	CAI_Squad *pPlayerSquad = g_AI_SquadManager.FindSquad( MAKE_STRING( PLAYER_SQUADNAME ) );
	if ( pPlayerSquad )
	{
		CUtlVectorFixed<CNPC_Citizen *, MAX_PLAYER_SQUAD> squadMembersToRemove;
		AISquadIter_t iter;

		for ( CAI_BaseNPC *pPlayerSquadMember = pPlayerSquad->GetFirstMember(&iter); pPlayerSquadMember; pPlayerSquadMember = pPlayerSquad->GetNextMember(&iter) )
		{
			if ( pPlayerSquadMember->GetClassname() != GetClassname() )
				continue;

			CNPC_Citizen *pCitizen = assert_cast<CNPC_Citizen *>(pPlayerSquadMember);

			if ( !pCitizen->m_bNeverLeavePlayerSquad &&
				 pCitizen->m_FollowBehavior.GetFollowTarget() &&
				 !pCitizen->m_FollowBehavior.FollowTargetVisible() && 
				 pCitizen->m_FollowBehavior.GetNumFailedFollowAttempts() > 0 && 
				 gpGlobals->curtime - pCitizen->m_FollowBehavior.GetTimeFailFollowStarted() > 20 &&
				 ( fabsf(( pCitizen->m_FollowBehavior.GetFollowTarget()->GetAbsOrigin().z - pCitizen->GetAbsOrigin().z )) > 196 ||
				   ( pCitizen->m_FollowBehavior.GetFollowTarget()->GetAbsOrigin().AsVector2D() - pCitizen->GetAbsOrigin().AsVector2D() ).LengthSqr() > Square(50*12) ) )
			{
				if ( DebuggingCommanderMode() )
				{
					DevMsg( "Player follower is lost (%d, %f, %d)\n", 
						 pCitizen->m_FollowBehavior.GetNumFailedFollowAttempts(), 
						 gpGlobals->curtime - pCitizen->m_FollowBehavior.GetTimeFailFollowStarted(), 
						 (int)((pCitizen->m_FollowBehavior.GetFollowTarget()->GetAbsOrigin().AsVector2D() - pCitizen->GetAbsOrigin().AsVector2D() ).Length()) );
				}

				squadMembersToRemove.AddToTail( pCitizen );
			}
		}

		for ( int i = 0; i < squadMembersToRemove.Count(); i++ )
		{
			squadMembersToRemove[i]->RemoveFromPlayerSquad();
		}
	}

	// Autosquadding
	const float JOIN_PLAYER_XY_TOLERANCE_SQ = Square(36*12);
	const float UNCONDITIONAL_JOIN_PLAYER_XY_TOLERANCE_SQ = Square(12*12);
	const float UNCONDITIONAL_JOIN_PLAYER_Z_TOLERANCE = 5*12;
	const float SECOND_TIER_JOIN_DIST_SQ = Square(48*12);
	if ( pPlayer && ShouldAutosquad() && !(pPlayer->GetFlags() & FL_NOTARGET ) && pPlayer->IsAlive() )
	{
		CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
		CUtlVector<SquadCandidate_t> candidates;
		const Vector &vPlayerPos = pPlayer->GetAbsOrigin();
		bool bFoundNewGuy = false;
		int i;

		for ( i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
			if ( ppAIs[i]->GetState() == NPC_STATE_DEAD )
				continue;

			if ( ppAIs[i]->GetClassname() != GetClassname() )
				continue;

			CNPC_Citizen *pCitizen = assert_cast<CNPC_Citizen *>(ppAIs[i]);
			int iNew;

			if ( pCitizen->IsInPlayerSquad() )
			{
				iNew = candidates.AddToTail();
				candidates[iNew].pCitizen = pCitizen;
				candidates[iNew].bIsInSquad = true;
				candidates[iNew].distSq = 0;
				candidates[iNew].iSquadIndex = pCitizen->GetSquad()->GetSquadIndex( pCitizen );
			}
			else
			{
				float distSq = (vPlayerPos.AsVector2D() - pCitizen->GetAbsOrigin().AsVector2D()).LengthSqr(); 
				if ( distSq > JOIN_PLAYER_XY_TOLERANCE_SQ && 
					( pCitizen->m_flTimeJoinedPlayerSquad == 0 || gpGlobals->curtime - pCitizen->m_flTimeJoinedPlayerSquad > 60.0 ) && 
					( pCitizen->m_flTimeLastCloseToPlayer == 0 || gpGlobals->curtime - pCitizen->m_flTimeLastCloseToPlayer > 15.0 ) )
					continue;

				if ( !pCitizen->CanJoinPlayerSquad() )
					continue;

				bool bShouldAdd = false;

				if ( pCitizen->HasCondition( COND_SEE_PLAYER ) )
					bShouldAdd = true;
				else
				{
					bool bPlayerVisible = pCitizen->FVisible( pPlayer );
					if ( bPlayerVisible )
					{
						if ( pCitizen->HasCondition( COND_HEAR_PLAYER ) )
							bShouldAdd = true;
						else if ( distSq < UNCONDITIONAL_JOIN_PLAYER_XY_TOLERANCE_SQ && fabsf(vPlayerPos.z - pCitizen->GetAbsOrigin().z) < UNCONDITIONAL_JOIN_PLAYER_Z_TOLERANCE )
							bShouldAdd = true;
					}
				}

				if ( bShouldAdd )
				{
					// @TODO (toml 05-25-04): probably everyone in a squad should be a candidate if one of them sees the player
					AI_Waypoint_t *pPathToPlayer = pCitizen->GetPathfinder()->BuildRoute( pCitizen->GetAbsOrigin(), vPlayerPos, pPlayer, 5*12, NAV_NONE, true );
					GetPathfinder()->UnlockRouteNodes( pPathToPlayer );

					if ( !pPathToPlayer )
						continue;

					CAI_Path tempPath;
					tempPath.SetWaypoints( pPathToPlayer ); // path object will delete waypoints

					iNew = candidates.AddToTail();
					candidates[iNew].pCitizen = pCitizen;
					candidates[iNew].bIsInSquad = false;
					candidates[iNew].distSq = distSq;
					candidates[iNew].iSquadIndex = -1;
					
					bFoundNewGuy = true;
				}
			}
		}
		
		if ( bFoundNewGuy )
		{
			// Look for second order guys
			int initialCount = candidates.Count();
			for ( i = 0; i < initialCount; i++ )
				candidates[i].pCitizen->AddSpawnFlags( SF_CITIZEN_NOT_COMMANDABLE ); // Prevents double-add
			for ( i = 0; i < initialCount; i++ )
			{
				if ( candidates[i].iSquadIndex == -1 )
				{
					for ( int j = 0; j < g_AI_Manager.NumAIs(); j++ )
					{
						if ( ppAIs[j]->GetState() == NPC_STATE_DEAD )
							continue;

						if ( ppAIs[j]->GetClassname() != GetClassname() )
							continue;

						if ( ppAIs[j]->HasSpawnFlags( SF_CITIZEN_NOT_COMMANDABLE ) )
							continue; 

						CNPC_Citizen *pCitizen = assert_cast<CNPC_Citizen *>(ppAIs[j]);

						float distSq = (vPlayerPos - pCitizen->GetAbsOrigin()).Length2DSqr(); 
						if ( distSq > JOIN_PLAYER_XY_TOLERANCE_SQ )
							continue;

						distSq = (candidates[i].pCitizen->GetAbsOrigin() - pCitizen->GetAbsOrigin()).Length2DSqr(); 
						if ( distSq > SECOND_TIER_JOIN_DIST_SQ )
							continue;

						if ( !pCitizen->CanJoinPlayerSquad() )
							continue;

						if ( !pCitizen->FVisible( pPlayer ) )
							continue;

						int iNew = candidates.AddToTail();
						candidates[iNew].pCitizen = pCitizen;
						candidates[iNew].bIsInSquad = false;
						candidates[iNew].distSq = distSq;
						candidates[iNew].iSquadIndex = -1;
						pCitizen->AddSpawnFlags( SF_CITIZEN_NOT_COMMANDABLE ); // Prevents double-add
					}
				}
			}
			for ( i = 0; i < candidates.Count(); i++ )
				candidates[i].pCitizen->RemoveSpawnFlags( SF_CITIZEN_NOT_COMMANDABLE );

			if ( candidates.Count() > MAX_PLAYER_SQUAD )
			{
				candidates.Sort( PlayerSquadCandidateSortFunc );

				for ( i = MAX_PLAYER_SQUAD; i < candidates.Count(); i++ )
				{
					if ( candidates[i].pCitizen->IsInPlayerSquad() )
					{
						candidates[i].pCitizen->RemoveFromPlayerSquad();
					}
				}
			}

			if ( candidates.Count() )
			{
				CNPC_Citizen *pClosest = NULL;
				float closestDistSq = FLT_MAX;
				int nJoined = 0;

				for ( i = 0; i < candidates.Count() && i < MAX_PLAYER_SQUAD; i++ )
				{
					if ( !candidates[i].pCitizen->IsInPlayerSquad() )
					{
						candidates[i].pCitizen->AddToPlayerSquad();
						nJoined++;

						if ( candidates[i].distSq < closestDistSq )
						{
							pClosest = candidates[i].pCitizen;
							closestDistSq = candidates[i].distSq;
						}
					}
				}

				if ( pClosest )
				{
					if ( !pClosest->SpokeConcept( TLK_JOINPLAYER ) )
					{
						pClosest->SpeakCommandResponse( TLK_JOINPLAYER, CFmtStr( "numjoining:%d", nJoined ) );
					}
					else
					{
						pClosest->SpeakCommandResponse( TLK_STARTFOLLOW );
					}

					for ( i = 0; i < candidates.Count() && i < MAX_PLAYER_SQUAD; i++ )
					{
						candidates[i].pCitizen->SetSpokeConcept( TLK_JOINPLAYER, NULL ); 
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Citizen::PlayerSquadCandidateSortFunc( const SquadCandidate_t *pLeft, const SquadCandidate_t *pRight )
{
	// "Bigger" means less approprate 
	CNPC_Citizen *pLeftCitizen = pLeft->pCitizen;
	CNPC_Citizen *pRightCitizen = pRight->pCitizen;

	// Medics are better than anyone
	if ( pLeftCitizen->IsMedic() && !pRightCitizen->IsMedic() )
		return -1;

	if ( !pLeftCitizen->IsMedic() && pRightCitizen->IsMedic() )
		return 1;

	CBaseCombatWeapon *pLeftWeapon = pLeftCitizen->GetActiveWeapon();
	CBaseCombatWeapon *pRightWeapon = pRightCitizen->GetActiveWeapon();
	
	// People with weapons are better than those without
	if ( pLeftWeapon && !pRightWeapon )
		return -1;
		
	if ( !pLeftWeapon && pRightWeapon )
		return 1;
	
	// Existing squad members are better than non-members
	if ( pLeft->bIsInSquad && !pRight->bIsInSquad )
		return -1;

	if ( !pLeft->bIsInSquad && pRight->bIsInSquad )
		return 1;

	// New squad members are better than older ones
	if ( pLeft->bIsInSquad && pRight->bIsInSquad )
		return pRight->iSquadIndex - pLeft->iSquadIndex;

	// Finally, just take the closer
	return (int)(pRight->distSq - pLeft->distSq);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::FixupPlayerSquad()
{
	if ( !AI_IsSinglePlayer() )
		return;

	m_flTimeJoinedPlayerSquad = gpGlobals->curtime;
	m_bWasInPlayerSquad = true;
	if ( m_pSquad->NumMembers() > MAX_PLAYER_SQUAD )
	{
		CAI_BaseNPC *pFirstMember = m_pSquad->GetFirstMember(NULL);
		m_pSquad->RemoveFromSquad( pFirstMember );
		pFirstMember->ClearCommandGoal();

		CNPC_Citizen *pFirstMemberCitizen = dynamic_cast< CNPC_Citizen * >( pFirstMember );
		if ( pFirstMemberCitizen )
		{
			pFirstMemberCitizen->ClearFollowTarget();
		}
		else
		{
			CAI_FollowBehavior *pOldMemberFollowBehavior;
			if ( pFirstMember->GetBehavior( &pOldMemberFollowBehavior ) )
			{
				pOldMemberFollowBehavior->SetFollowTarget( NULL );
			}
		}
	}

	ClearFollowTarget();

	CAI_BaseNPC *pLeader = NULL;
	AISquadIter_t iter;
	for ( CAI_BaseNPC *pAllyNpc = m_pSquad->GetFirstMember(&iter); pAllyNpc; pAllyNpc = m_pSquad->GetNextMember(&iter) )
	{
		if ( pAllyNpc->IsCommandable() )
		{
			pLeader = pAllyNpc;
			break;
		}
	}

	if ( pLeader && pLeader != this )
	{
		const Vector &commandGoal = pLeader->GetCommandGoal();
		if ( commandGoal != vec3_invalid )
		{
			SetCommandGoal( commandGoal );
			SetCondition( COND_RECEIVED_ORDERS ); 
			OnMoveOrder();
		}
		else
		{
			CAI_FollowBehavior *pLeaderFollowBehavior;
			if ( pLeader->GetBehavior( &pLeaderFollowBehavior ) )
			{
				m_FollowBehavior.SetFollowTarget( pLeaderFollowBehavior->GetFollowTarget() );
				m_FollowBehavior.SetParameters( m_FollowBehavior.GetFormation() );
			}

		}
	}
	else
	{
		m_FollowBehavior.SetFollowTarget( UTIL_GetLocalPlayer() );
		m_FollowBehavior.SetParameters( AIF_SIMPLE );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::ClearFollowTarget()
{
	m_FollowBehavior.SetFollowTarget( NULL );
	m_FollowBehavior.SetParameters( AIF_SIMPLE );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::UpdateFollowCommandPoint()
{
	if ( !AI_IsSinglePlayer() )
		return;

	if ( IsInPlayerSquad() )
	{
		if ( HaveCommandGoal() )
		{
			CBaseEntity *pFollowTarget = m_FollowBehavior.GetFollowTarget();
			CBaseEntity *pCommandPoint = gEntList.FindEntityByClassname( NULL, COMMAND_POINT_CLASSNAME );
			
			if( !pCommandPoint )
			{
				DevMsg("**\nVERY BAD THING\nCommand point vanished! Creating a new one\n**\n");
				pCommandPoint = CreateEntityByName( COMMAND_POINT_CLASSNAME );
			}

			if ( pFollowTarget != pCommandPoint )
			{
				pFollowTarget = pCommandPoint;
				m_FollowBehavior.SetFollowTarget( pFollowTarget );
				m_FollowBehavior.SetParameters( AIF_COMMANDER );
			}
			
			if ( ( pCommandPoint->GetAbsOrigin() - GetCommandGoal() ).LengthSqr() > 0.01 )
			{
				UTIL_SetOrigin( pCommandPoint, GetCommandGoal(), false );
			}
		}
		else
		{
			if ( IsFollowingCommandPoint() )
				ClearFollowTarget();
			if ( m_FollowBehavior.GetFollowTarget() != UTIL_GetLocalPlayer() )
			{
				DevMsg( "Expected to be following player, but not\n" );
				m_FollowBehavior.SetFollowTarget( UTIL_GetLocalPlayer() );
				m_FollowBehavior.SetParameters( AIF_SIMPLE );
			}
		}
	}
	else if ( IsFollowingCommandPoint() )
		ClearFollowTarget();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::IsFollowingCommandPoint()
{
	CBaseEntity *pFollowTarget = m_FollowBehavior.GetFollowTarget();
	if ( pFollowTarget )
		return FClassnameIs( pFollowTarget, COMMAND_POINT_CLASSNAME );
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct SquadMemberInfo_t
{
	CNPC_Citizen *	pMember;
	bool			bSeesPlayer;
	float			distSq;
};

int __cdecl SquadSortFunc( const SquadMemberInfo_t *pLeft, const SquadMemberInfo_t *pRight )
{
	if ( pLeft->bSeesPlayer && !pRight->bSeesPlayer )
	{
		return -1;
	}

	if ( !pLeft->bSeesPlayer && pRight->bSeesPlayer )
	{
		return 1;
	}

	return ( pLeft->distSq - pRight->distSq );
}

CAI_BaseNPC *CNPC_Citizen::GetSquadCommandRepresentative()
{
	if ( !AI_IsSinglePlayer() )
		return NULL;

	if ( IsInPlayerSquad() )
	{
		static float lastTime;
		static AIHANDLE hCurrent;

		if ( gpGlobals->curtime - lastTime > 2.0 || !hCurrent || !hCurrent->IsInPlayerSquad() ) // hCurrent will be NULL after level change
		{
			lastTime = gpGlobals->curtime;
			hCurrent = NULL;

			CUtlVectorFixed<SquadMemberInfo_t, MAX_SQUAD_MEMBERS> candidates;
			CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

			if ( pPlayer )
			{
				AISquadIter_t iter;
				for ( CAI_BaseNPC *pAllyNpc = m_pSquad->GetFirstMember(&iter); pAllyNpc; pAllyNpc = m_pSquad->GetNextMember(&iter) )
				{
					if ( pAllyNpc->IsCommandable() && dynamic_cast<CNPC_Citizen *>(pAllyNpc) )
					{
						int i = candidates.AddToTail();
						candidates[i].pMember = (CNPC_Citizen *)(pAllyNpc);
						candidates[i].bSeesPlayer = pAllyNpc->HasCondition( COND_SEE_PLAYER );
						candidates[i].distSq = ( pAllyNpc->GetAbsOrigin() - pPlayer->GetAbsOrigin() ).LengthSqr();
					}
				}

				if ( candidates.Count() > 0 )
				{
					candidates.Sort( SquadSortFunc );
					hCurrent = candidates[0].pMember;
				}
			}
		}

		if ( hCurrent != NULL )
		{
			Assert( dynamic_cast<CNPC_Citizen *>(hCurrent.Get()) && hCurrent->IsInPlayerSquad() );
			return hCurrent;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::SetSquad( CAI_Squad *pSquad )
{
	bool bWasInPlayerSquad = IsInPlayerSquad();

	BaseClass::SetSquad( pSquad );

	if( IsInPlayerSquad() && !bWasInPlayerSquad )
	{
		m_OnJoinedPlayerSquad.FireOutput(this, this);
		if ( npc_citizen_insignia.GetBool() )
			AddInsignia();
	}
	else if ( !IsInPlayerSquad() && bWasInPlayerSquad )
	{
		if ( npc_citizen_insignia.GetBool() )
			RemoveInsignia();
		m_OnLeftPlayerSquad.FireOutput(this, this);
	}
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_Citizen::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	// TODO:  As citizen gets more complex, we will have to only allow
	//		  these interruptions to happen from certain schedules
	if (interactionType ==	g_interactionScannerInspect)
	{
		if ( gpGlobals->curtime > m_fNextInspectTime )
		{
			//SetLookTarget(sourceEnt);

			// Don't let anyone else pick me for a couple seconds
			SetNextScannerInspectTime( gpGlobals->curtime + 5.0 );
			return true;
		}
		return false;
	}
	else if (interactionType ==	g_interactionScannerInspectBegin)
	{
		// Don't inspect me again for a while
		SetNextScannerInspectTime( gpGlobals->curtime + CIT_INSPECTED_DELAY_TIME );
		
		Vector	targetDir = ( sourceEnt->WorldSpaceCenter() - WorldSpaceCenter() );
		VectorNormalize( targetDir );

		// If we're hit from behind, startle
		if ( DotProduct( targetDir, BodyDirection3D() ) < 0 )
		{
			m_nInspectActivity = ACT_CIT_STARTLED;
		}
		else
		{
			// Otherwise we're blinded by the flash
			m_nInspectActivity = ACT_CIT_BLINDED;
		}
		
		SetCondition( COND_CIT_START_INSPECTION );
		return true;
	}
	else if (interactionType ==	g_interactionScannerInspectHandsUp)
	{
		m_nInspectActivity = ACT_CIT_HANDSUP;
		SetSchedule(SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY);
		return true;
	}
	else if (interactionType ==	g_interactionScannerInspectShowArmband)
	{
		m_nInspectActivity = ACT_CIT_SHOWARMBAND;
		SetSchedule(SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY);
		return true;
	}
	else if (interactionType ==	g_interactionScannerInspectDone)
	{
		SetSchedule(SCHED_IDLE_WANDER);
		return true;
	}
	else if (interactionType == g_interactionHitByPlayerThrownPhysObj )
	{
		if ( IsOkToSpeakInResponseToPlayer() )
		{
			Speak( TLK_PLYR_PHYSATK );
		}
		return true;
	}

	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::FValidateHintType( CAI_Hint *pHint )
{
	switch( pHint->HintType() )
	{
	case HINT_WORLD_VISUALLY_INTERESTING:
		return true;
		break;

	default:
		break;
	}

	return BaseClass::FValidateHintType( pHint );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::CanHeal()
{ 
	if ( !IsMedic() && !IsAmmoResupplier() )
		return false;

	if( !hl2_episodic.GetBool() )
	{
		// If I'm not armed, my priority should be to arm myself.
		if ( IsMedic() && !GetActiveWeapon() )
			return false;
	}

	if ( IsInAScript() || (m_NPCState == NPC_STATE_SCRIPT) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldHealTarget( CBaseEntity *pTarget, bool bActiveUse )
{
	Disposition_t disposition;
	
	if ( !pTarget && ( ( disposition = IRelationType( pTarget ) ) != D_LI && disposition != D_NU ) )
		return false;

	// Don't heal if I'm in the middle of talking
	if ( IsSpeaking() )
		return false;

	bool bTargetIsPlayer = pTarget->IsPlayer();

	// Don't heal or give ammo to targets in vehicles
	CBaseCombatCharacter *pCCTarget = pTarget->MyCombatCharacterPointer();
	if ( pCCTarget != NULL && pCCTarget->IsInAVehicle() )
		return false;

	if ( IsMedic() )
	{
		Vector toPlayer = ( pTarget->GetAbsOrigin() - GetAbsOrigin() );
	 	if (( bActiveUse || !HaveCommandGoal() || toPlayer.Length() < HEAL_TARGET_RANGE) 
#ifdef HL2_EPISODIC
			&& fabs(toPlayer.z) < HEAL_TARGET_RANGE_Z
#endif
			)
	 	{
			if ( pTarget->m_iHealth > 0 )
			{
	 			if ( bActiveUse )
				{
					// Ignore heal requests if we're going to heal a tiny amount
					float timeFullHeal = m_flPlayerHealTime;
					float timeRecharge = sk_citizen_heal_player_delay.GetFloat();
					float maximumHealAmount = sk_citizen_heal_player.GetFloat();
					float healAmt = ( maximumHealAmount * ( 1.0 - ( timeFullHeal - gpGlobals->curtime ) / timeRecharge ) );
					if ( healAmt > pTarget->m_iMaxHealth - pTarget->m_iHealth )
						healAmt = pTarget->m_iMaxHealth - pTarget->m_iHealth;
					if ( healAmt < sk_citizen_heal_player_min_forced.GetFloat() )
						return false;

	 				return ( pTarget->m_iMaxHealth > pTarget->m_iHealth );
				}
	 				
				// Are we ready to heal again?
				bool bReadyToHeal = ( ( bTargetIsPlayer && m_flPlayerHealTime <= gpGlobals->curtime ) || 
									  ( !bTargetIsPlayer && m_flAllyHealTime <= gpGlobals->curtime ) );

				// Only heal if we're ready
				if ( bReadyToHeal )
				{
					int requiredHealth;

					if ( bTargetIsPlayer )
						requiredHealth = pTarget->GetMaxHealth() - sk_citizen_heal_player.GetFloat();
					else
						requiredHealth = pTarget->GetMaxHealth() * sk_citizen_heal_player_min_pct.GetFloat();

					if ( ( pTarget->m_iHealth <= requiredHealth ) && IRelationType( pTarget ) == D_LI )
						return true;
				}
			}
		}
	}

	// Only players need ammo
	if ( IsAmmoResupplier() && bTargetIsPlayer )
	{
		if ( m_flPlayerGiveAmmoTime <= gpGlobals->curtime )
		{
			int iAmmoType = GetAmmoDef()->Index( STRING(m_iszAmmoSupply) );
			if ( iAmmoType == -1 )
			{
				DevMsg("ERROR: Citizen attempting to give unknown ammo type (%s)\n", STRING(m_iszAmmoSupply) );
			}
			else
			{
				// Does the player need the ammo we can give him?
				int iMax = GetAmmoDef()->MaxCarry(iAmmoType);
				int iCount = ((CBasePlayer*)pTarget)->GetAmmoCount(iAmmoType);
				if ( !iCount || ((iMax - iCount) >= m_iAmmoAmount) )
				{
					// Only give the player ammo if he has a weapon that uses it
					if ( ((CBasePlayer*)pTarget)->Weapon_GetWpnForAmmo( iAmmoType ) )
						return true;
				}
			}
		}
	}
	return false;
}

#ifdef HL2_EPISODIC
//-----------------------------------------------------------------------------
// Determine if the citizen is in a position to be throwing medkits
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldHealTossTarget( CBaseEntity *pTarget, bool bActiveUse )
{
	Disposition_t disposition;

	Assert( IsMedic() );
	if ( !IsMedic() )
		return false;
	
	if ( !pTarget && ( ( disposition = IRelationType( pTarget ) ) != D_LI && disposition != D_NU ) )
		return false;

	// Don't heal if I'm in the middle of talking
	if ( IsSpeaking() )
		return false;

	bool bTargetIsPlayer = pTarget->IsPlayer();

	// Don't heal or give ammo to targets in vehicles
	CBaseCombatCharacter *pCCTarget = pTarget->MyCombatCharacterPointer();
	if ( pCCTarget != NULL && pCCTarget->IsInAVehicle() )
		return false;

	Vector toPlayer = ( pTarget->GetAbsOrigin() - GetAbsOrigin() );
	if ( bActiveUse || !HaveCommandGoal() || toPlayer.Length() < HEAL_TOSS_TARGET_RANGE )
	{
		if ( pTarget->m_iHealth > 0 )
		{
			if ( bActiveUse )
			{
				// Ignore heal requests if we're going to heal a tiny amount
				float timeFullHeal = m_flPlayerHealTime;
				float timeRecharge = sk_citizen_heal_player_delay.GetFloat();
				float maximumHealAmount = sk_citizen_heal_player.GetFloat();
				float healAmt = ( maximumHealAmount * ( 1.0 - ( timeFullHeal - gpGlobals->curtime ) / timeRecharge ) );
				if ( healAmt > pTarget->m_iMaxHealth - pTarget->m_iHealth )
					healAmt = pTarget->m_iMaxHealth - pTarget->m_iHealth;
				if ( healAmt < sk_citizen_heal_player_min_forced.GetFloat() )
					return false;

				return ( pTarget->m_iMaxHealth > pTarget->m_iHealth );
			}

			// Are we ready to heal again?
			bool bReadyToHeal = ( ( bTargetIsPlayer && m_flPlayerHealTime <= gpGlobals->curtime ) || 
				( !bTargetIsPlayer && m_flAllyHealTime <= gpGlobals->curtime ) );

			// Only heal if we're ready
			if ( bReadyToHeal )
			{
				int requiredHealth;

				if ( bTargetIsPlayer )
					requiredHealth = pTarget->GetMaxHealth() - sk_citizen_heal_player.GetFloat();
				else
					requiredHealth = pTarget->GetMaxHealth() * sk_citizen_heal_player_min_pct.GetFloat();

				if ( ( pTarget->m_iHealth <= requiredHealth ) && IRelationType( pTarget ) == D_LI )
					return true;
			}
		}
	}
	
	return false;
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::Heal()
{
	if ( !CanHeal() )
		  return;

	CBaseEntity *pTarget = GetTarget();

	Vector target = pTarget->GetAbsOrigin() - GetAbsOrigin();
	if ( target.Length() > HEAL_TARGET_RANGE * 2 )
		return;

	// Don't heal a player that's staring at you until a few seconds have passed.
	m_flTimeNextHealStare = gpGlobals->curtime + sk_citizen_stare_heal_time.GetFloat();

	if ( IsMedic() )
	{
		float timeFullHeal;
		float timeRecharge;
		float maximumHealAmount;
		if ( pTarget->IsPlayer() )
		{
			timeFullHeal 		= m_flPlayerHealTime;
			timeRecharge 		= sk_citizen_heal_player_delay.GetFloat();
			maximumHealAmount 	= sk_citizen_heal_player.GetFloat();
			m_flPlayerHealTime 	= gpGlobals->curtime + timeRecharge;
		}
		else
		{
			timeFullHeal 		= m_flAllyHealTime;
			timeRecharge 		= sk_citizen_heal_ally_delay.GetFloat();
			maximumHealAmount 	= sk_citizen_heal_ally.GetFloat();
			m_flAllyHealTime 	= gpGlobals->curtime + timeRecharge;
		}
		
		float healAmt = ( maximumHealAmount * ( 1.0 - ( timeFullHeal - gpGlobals->curtime ) / timeRecharge ) );
		
		if ( healAmt > maximumHealAmount )
			healAmt = maximumHealAmount;
		else
			healAmt = RoundFloatToInt( healAmt );
		
		if ( healAmt > 0 )
		{
			if ( pTarget->IsPlayer() && npc_citizen_medic_emit_sound.GetBool() )
			{
				CPASAttenuationFilter filter( pTarget, "HealthKit.Touch" );
				EmitSound( filter, pTarget->entindex(), "HealthKit.Touch" );
			}

			pTarget->TakeHealth( healAmt, DMG_GENERIC );
			pTarget->RemoveAllDecals();
		}
	}

	if ( IsAmmoResupplier() )
	{
		// Non-players don't use ammo
		if ( pTarget->IsPlayer() )
		{
			int iAmmoType = GetAmmoDef()->Index( STRING(m_iszAmmoSupply) );
			if ( iAmmoType == -1 )
			{
				DevMsg("ERROR: Citizen attempting to give unknown ammo type (%s)\n", STRING(m_iszAmmoSupply) );
			}
			else
			{
				((CBasePlayer*)pTarget)->GiveAmmo( m_iAmmoAmount, iAmmoType, false );
			}

			m_flPlayerGiveAmmoTime = gpGlobals->curtime + sk_citizen_giveammo_player_delay.GetFloat();
		}
	}
}



#if HL2_EPISODIC
//-----------------------------------------------------------------------------
// Like Heal(), but tosses a healthkit in front of the player rather than just juicing him up.
//-----------------------------------------------------------------------------
void	CNPC_Citizen::TossHealthKit(CBaseCombatCharacter *pThrowAt, const Vector &offset)
{
	Assert( pThrowAt );

	Vector forward, right, up;
	GetVectors( &forward, &right, &up );
	Vector medKitOriginPoint = WorldSpaceCenter() + ( forward * 20.0f );
	Vector destinationPoint;
	// this doesn't work without a moveparent: pThrowAt->ComputeAbsPosition( offset, &destinationPoint );
	VectorTransform( offset, pThrowAt->EntityToWorldTransform(), destinationPoint );
	// flatten out any z change due to player looking up/down
	destinationPoint.z = pThrowAt->EyePosition().z;

	Vector tossVelocity;

	if (npc_citizen_medic_throw_style.GetInt() == 0)
	{
		CTraceFilterSkipTwoEntities tracefilter( this, pThrowAt, COLLISION_GROUP_NONE );
		tossVelocity = VecCheckToss( this, &tracefilter, medKitOriginPoint, destinationPoint, 0.233f, 1.0f, false );
	}
	else
	{
		tossVelocity = VecCheckThrow( this, medKitOriginPoint, destinationPoint, MEDIC_THROW_SPEED, 1.0f );

		if (vec3_origin == tossVelocity)
		{
			// if out of range, just throw it as close as I can
			tossVelocity = destinationPoint - medKitOriginPoint;

			// rotate upwards against gravity
			float len = VectorLength(tossVelocity);
			tossVelocity *= (MEDIC_THROW_SPEED / len);
			tossVelocity.z += 0.57735026918962576450914878050196 * MEDIC_THROW_SPEED;
		}
	}

	// create a healthkit and toss it into the world
	CBaseEntity *pHealthKit = CreateEntityByName( "item_healthkit" );
	Assert(pHealthKit);
	if (pHealthKit)
	{
		pHealthKit->SetAbsOrigin( medKitOriginPoint );
		pHealthKit->SetOwnerEntity( this );
		// pHealthKit->SetAbsVelocity( tossVelocity );
		DispatchSpawn( pHealthKit );

		{
			IPhysicsObject *pPhysicsObject = pHealthKit->VPhysicsGetObject();
			Assert( pPhysicsObject );
			if ( pPhysicsObject )
			{
				unsigned int cointoss = random->RandomInt(0,0xFF); // int bits used for bools

				// some random precession
				Vector angDummy(random->RandomFloat(-200,200), random->RandomFloat(-200,200), 
					cointoss & 0x01 ? random->RandomFloat(200,600) : -1.0f * random->RandomFloat(200,600));
				pPhysicsObject->SetVelocity( &tossVelocity, &angDummy );
			}
		}
	}
	else
	{
		Warning("Citizen tried to heal but could not spawn item_healthkit!\n");
	}

}

//-----------------------------------------------------------------------------
// cause an immediate call to TossHealthKit with some default numbers
//-----------------------------------------------------------------------------
void	CNPC_Citizen::InputForceHealthKitToss( inputdata_t &inputdata )
{
	TossHealthKit( UTIL_GetLocalPlayer(), Vector(48.0f, 0.0f, 0.0f)  );
}

#endif



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldLookForHealthItem()
{
	// Definitely do not take health if not in the player's squad.
	if( !IsInPlayerSquad() )
		return false;

	if( gpGlobals->curtime < m_flNextHealthSearchTime )
		return false;

	// I'm fully healthy.
	if( GetHealth() >= GetMaxHealth() )
		return false;

	// Player is hurt, don't steal his health.
	if( AI_IsSinglePlayer() && UTIL_GetLocalPlayer()->GetHealth() <= UTIL_GetLocalPlayer()->GetHealth() * 0.75f )
		return false;

	// Wait till you're standing still.
	if( IsMoving() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::InputStartPatrolling( inputdata_t &inputdata )
{
	m_bShouldPatrol = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::InputStopPatrolling( inputdata_t &inputdata )
{
	m_bShouldPatrol = false;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Citizen::OnGivenWeapon( CBaseCombatWeapon *pNewWeapon )
{
	FixupMattWeapon();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Citizen::InputSetCommandable( inputdata_t &inputdata )
{
	RemoveSpawnFlags( SF_CITIZEN_NOT_COMMANDABLE );
	gm_PlayerSquadEvaluateTimer.Force();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Citizen::InputSetMedicOn( inputdata_t &inputdata )
{
	AddSpawnFlags( SF_CITIZEN_MEDIC );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Citizen::InputSetMedicOff( inputdata_t &inputdata )
{
	RemoveSpawnFlags( SF_CITIZEN_MEDIC );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Citizen::InputSetAmmoResupplierOn( inputdata_t &inputdata )
{
	AddSpawnFlags( SF_CITIZEN_AMMORESUPPLIER );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Citizen::InputSetAmmoResupplierOff( inputdata_t &inputdata )
{
	RemoveSpawnFlags( SF_CITIZEN_AMMORESUPPLIER );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Citizen::InputSpeakIdleResponse( inputdata_t &inputdata )
{
	SpeakIfAllowed( TLK_ANSWER, NULL, true );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Citizen::DeathSound( const CTakeDamageInfo &info )
{
	// Sentences don't play on dead NPCs
	SentenceStop();

	EmitSound( "NPC_Citizen.Die" );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Citizen::FearSound( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Citizen::UseSemaphore( void )
{
	// Ignore semaphore if we're told to work outside it
	if ( HasSpawnFlags(SF_CITIZEN_IGNORE_SEMAPHORE) )
		return false;

	return BaseClass::UseSemaphore();
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_citizen, CNPC_Citizen )

	DECLARE_TASK( TASK_CIT_HEAL )
	DECLARE_TASK( TASK_CIT_RPG_AUGER )
	DECLARE_TASK( TASK_CIT_PLAY_INSPECT_SEQUENCE )
	DECLARE_TASK( TASK_CIT_SIT_ON_TRAIN )
	DECLARE_TASK( TASK_CIT_LEAVE_TRAIN )
	DECLARE_TASK( TASK_CIT_SPEAK_MOURNING )
#if HL2_EPISODIC
	DECLARE_TASK( TASK_CIT_HEAL_TOSS )
#endif

	DECLARE_ACTIVITY( ACT_CIT_HANDSUP )
	DECLARE_ACTIVITY( ACT_CIT_BLINDED )
	DECLARE_ACTIVITY( ACT_CIT_SHOWARMBAND )
	DECLARE_ACTIVITY( ACT_CIT_HEAL )
	DECLARE_ACTIVITY( ACT_CIT_STARTLED )

	DECLARE_CONDITION( COND_CIT_PLAYERHEALREQUEST )
	DECLARE_CONDITION( COND_CIT_COMMANDHEAL )
	DECLARE_CONDITION( COND_CIT_START_INSPECTION )

	//Events
	DECLARE_ANIMEVENT( AE_CITIZEN_GET_PACKAGE )
	DECLARE_ANIMEVENT( AE_CITIZEN_HEAL )

	//=========================================================
	// > SCHED_SCI_HEAL
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_HEAL,

		"	Tasks"
		"		TASK_GET_PATH_TO_TARGET				0"
		"		TASK_MOVE_TO_TARGET_RANGE			50"
		"		TASK_STOP_MOVING					0"
		"		TASK_FACE_IDEAL						0"
//		"		TASK_SAY_HEAL						0"
//		"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_ARM"
		"		TASK_CIT_HEAL							0"
//		"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_DISARM"
		"	"
		"	Interrupts"
	)

#if HL2_EPISODIC
	//=========================================================
	// > SCHED_CITIZEN_HEAL_TOSS
	// this is for the episodic behavior where the citizen hurls the medkit
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_HEAL_TOSS,

	"	Tasks"
//  "		TASK_GET_PATH_TO_TARGET				0"
//  "		TASK_MOVE_TO_TARGET_RANGE			50"
	"		TASK_STOP_MOVING					0"
	"		TASK_FACE_IDEAL						0"
//	"		TASK_SAY_HEAL						0"
//	"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_ARM"
	"		TASK_CIT_HEAL_TOSS							0"
//	"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_DISARM"
	"	"
	"	Interrupts"
	)
#endif

	//=========================================================
	// > SCHED_CITIZEN_RANGE_ATTACK1_RPG
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_RANGE_ATTACK1_RPG,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_ANNOUNCE_ATTACK		1"	// 1 = primary attack
		"		TASK_RANGE_ATTACK1			0"
		"		TASK_CIT_RPG_AUGER			1"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_CITIZEN_RANGE_ATTACK1_RPG
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_STRIDER_RANGE_ATTACK1_RPG,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_ANNOUNCE_ATTACK		1"	// 1 = primary attack
		"		TASK_WAIT					1"
		"		TASK_RANGE_ATTACK1			0"
		"		TASK_CIT_RPG_AUGER			1"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
		""
		"	Interrupts"
	)


	//=========================================================
	// > SCHED_CITIZEN_PATROL
	//=========================================================
	DEFINE_SCHEDULE	
	(
		SCHED_CITIZEN_PATROL,
		  
		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_WANDER						901024"		// 90 to 1024 units
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT						3"
		"		TASK_WAIT_RANDOM				3"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_CITIZEN_PATROL" // keep doing it
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_NEW_ENEMY"
	)

	DEFINE_SCHEDULE	
	(
		SCHED_CITIZEN_MOURN_PLAYER,
		  
		"	Tasks"
		"		TASK_GET_PATH_TO_PLAYER		0"
		"		TASK_RUN_PATH_WITHIN_DIST	180"
		"		TASK_WAIT_FOR_MOVEMENT		0"
		"		TASK_STOP_MOVING			0"
		"		TASK_TARGET_PLAYER			0"
		"		TASK_FACE_TARGET			0"
		"		TASK_CIT_SPEAK_MOURNING		0"
		"		TASK_SUGGEST_STATE			STATE:IDLE"
		""
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_NEW_ENEMY"
	)

	DEFINE_SCHEDULE	
	(
		SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY,
		  
		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_CIT_PLAY_INSPECT_SEQUENCE	0"	// Play the sequence the scanner requires
		"		TASK_WAIT						2"
		""
		"	Interrupts"
		"		"
	)

	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_SIT_ON_TRAIN,

		"	Tasks"
		"		TASK_CIT_SIT_ON_TRAIN		0"
		"		TASK_WAIT_RANDOM			1"
		"		TASK_CIT_LEAVE_TRAIN		0"
		""
		"	Interrupts"
	)

AI_END_CUSTOM_NPC()



//==================================================================================================================
// CITIZEN PLAYER-RESPONSE SYSTEM
//
// NOTE: This system is obsolete, and left here for legacy support.
//		 It has been superseded by the ai_eventresponse system.
//
//==================================================================================================================
CHandle<CCitizenResponseSystem>	g_pCitizenResponseSystem = NULL;

CCitizenResponseSystem	*GetCitizenResponse()
{
	return g_pCitizenResponseSystem;
}

char *CitizenResponseConcepts[MAX_CITIZEN_RESPONSES] = 
{
	"TLK_CITIZEN_RESPONSE_SHOT_GUNSHIP",
	"TLK_CITIZEN_RESPONSE_KILLED_GUNSHIP",
	"TLK_VITALNPC_DIED",
};

LINK_ENTITY_TO_CLASS( ai_citizen_response_system, CCitizenResponseSystem );

BEGIN_DATADESC( CCitizenResponseSystem )
	DEFINE_ARRAY( m_flResponseAddedTime, FIELD_FLOAT, MAX_CITIZEN_RESPONSES ),
	DEFINE_FIELD( m_flNextResponseTime, FIELD_FLOAT ),

	DEFINE_INPUTFUNC( FIELD_VOID,	"ResponseVitalNPC",	InputResponseVitalNPC ),

	DEFINE_THINKFUNC( ResponseThink ),
END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCitizenResponseSystem::Spawn()
{
	if ( g_pCitizenResponseSystem )
	{
		Warning("Multiple citizen response systems in level.\n");
		UTIL_Remove( this );
		return;
	}
	g_pCitizenResponseSystem = this;

	// Invisible, non solid.
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );
	SetThink( &CCitizenResponseSystem::ResponseThink );

	m_flNextResponseTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCitizenResponseSystem::OnRestore()
{
	BaseClass::OnRestore();

	g_pCitizenResponseSystem = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCitizenResponseSystem::AddResponseTrigger( citizenresponses_t	iTrigger )
{
	m_flResponseAddedTime[ iTrigger ] = gpGlobals->curtime;

	SetNextThink( gpGlobals->curtime + 0.1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCitizenResponseSystem::InputResponseVitalNPC( inputdata_t &inputdata )
{
	AddResponseTrigger( CR_VITALNPC_DIED );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCitizenResponseSystem::ResponseThink()
{
	bool bStayActive = false;
	if ( AI_IsSinglePlayer() )
	{
		for ( int i = 0; i < MAX_CITIZEN_RESPONSES; i++ )
		{
			if ( m_flResponseAddedTime[i] )
			{
				// Should it have expired by now?
				if ( (m_flResponseAddedTime[i] + CITIZEN_RESPONSE_GIVEUP_TIME) < gpGlobals->curtime )
				{
					m_flResponseAddedTime[i] = 0;
				}
				else if ( m_flNextResponseTime < gpGlobals->curtime )
				{
					// Try and find the nearest citizen to the player
					float flNearestDist = (CITIZEN_RESPONSE_DISTANCE * CITIZEN_RESPONSE_DISTANCE);
					CBaseEntity *pNearestCitizen = NULL;
					CBaseEntity *pCitizen = NULL;
					CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
					while ( (pCitizen = gEntList.FindEntityByClassname( pCitizen, "npc_citizen" ) ) != NULL)
					{
						float flDistToPlayer = (pPlayer->WorldSpaceCenter() - pCitizen->WorldSpaceCenter()).LengthSqr();
						if ( flDistToPlayer < flNearestDist )
						{
							flNearestDist = flDistToPlayer;
							pNearestCitizen = pCitizen;
						}
					}

					// Found one?
					if ( pNearestCitizen && ((CNPC_Citizen*)pNearestCitizen)->RespondedTo( CitizenResponseConcepts[i], false, false ) )
					{
						m_flResponseAddedTime[i] = 0;
						m_flNextResponseTime = gpGlobals->curtime + CITIZEN_RESPONSE_REFIRE_TIME;

						// Don't issue multiple responses
						break;
					}
				}
				else
				{
					bStayActive = true;
				}
			}
		}
	}

	// Do we need to keep thinking?
	if ( bStayActive )
	{
		SetNextThink( gpGlobals->curtime + 0.1 );
	}
}

void CNPC_Citizen::AddInsignia()
{
	CBaseEntity *pMark = CreateEntityByName( "squadinsignia" );
	pMark->SetOwnerEntity( this );
	pMark->Spawn();
}

void CNPC_Citizen::RemoveInsignia()
{
	CBaseEntity *pEntity = gEntList.FirstEnt();

	while( pEntity )
	{
		if( pEntity->GetOwnerEntity() == this )
		{
			// Is this my insignia?
			CSquadInsignia *pInsignia = dynamic_cast<CSquadInsignia *>(pEntity);

			if( pInsignia )
			{
				UTIL_Remove( pInsignia );
				return;
			}
		}

		pEntity = gEntList.NextEnt( pEntity );
	}
}

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( squadinsignia, CSquadInsignia );

void CSquadInsignia::Spawn()
{
	CAI_BaseNPC *pOwner = ( GetOwnerEntity() ) ? GetOwnerEntity()->MyNPCPointer() : NULL;

	if ( pOwner )
	{
		int attachment = pOwner->LookupAttachment( "eyes" );
		if ( attachment )
		{
			SetAbsAngles( GetOwnerEntity()->GetAbsAngles() );
			SetParent( GetOwnerEntity(), attachment );

			Vector vecPosition;
			vecPosition.Init( -2.5, 0, 3.9 );
			SetLocalOrigin( vecPosition );
		}
	}

	SetModel( INSIGNIA_MODEL );
	SetSolid( SOLID_NONE );	
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CNPC_Citizen::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf(tempstr,sizeof(tempstr),"Expression type: %s", szExpressionTypes[m_ExpressionType]);
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}
