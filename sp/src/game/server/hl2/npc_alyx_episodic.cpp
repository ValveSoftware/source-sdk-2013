//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Alyx, the female sidekick and love interest that's taking the world by storm!
//
//			Try the new Alyx Brite toothpaste!
//			Alyx lederhosen!
//
//			FIXME: need a better comment block
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "ai_basehumanoid.h"
#include "ai_behavior_follow.h"
#include "npc_alyx_episodic.h"
#include "npc_headcrab.h"
#include "npc_BaseZombie.h"
#include "ai_senses.h"
#include "ai_memory.h"
#include "soundent.h"
#include "props.h"
#include "IEffects.h"
#include "globalstate.h"
#include "weapon_physcannon.h"
#include "info_darknessmode_lightsource.h"
#include "sceneentity.h"
#include "hl2_gamerules.h"
#include "scripted.h"
#include "hl2_player.h"
#include "env_alyxemp_shared.h"
#include "basehlcombatweapon.h"
#include "basegrenade_shared.h"
#include "ai_interactions.h"
#include "weapon_flaregun.h"
#include "env_debughistory.h"

extern Vector PointOnLineNearestPoint(const Vector& vStartPos, const Vector& vEndPos, const Vector& vPoint);

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool g_HackOutland10DamageHack;

int ACT_ALYX_DRAW_TOOL;
int ACT_ALYX_IDLE_TOOL;
int ACT_ALYX_ZAP_TOOL;
int ACT_ALYX_HOLSTER_TOOL;
int ACT_ALYX_PICKUP_RACK;

string_t CLASSNAME_ALYXGUN;
string_t CLASSNAME_SMG1;
string_t CLASSNAME_SHOTGUN;
string_t CLASSNAME_AR2;

bool IsInCommentaryMode( void );

#define ALYX_BREATHING_VOLUME_MAX		1.0

#define ALYX_DARKNESS_LOST_PLAYER_DIST	( 120 * 120 ) // 12 feet

#define ALYX_MIN_MOB_DIST_SQR Square(120)		// Any enemy closer than this adds to the 'mob' 
#define ALYX_MIN_CONSIDER_DIST	Square(1200)	// Only enemies within this range are counted and considered to generate AI speech

#define CONCEPT_ALYX_REQUEST_ITEM		"TLK_ALYX_REQUEST_ITEM"
#define CONCEPT_ALYX_INTERACTION_DONE	"TLK_ALYX_INTERACTION_DONE"
#define CONCEPT_ALYX_CANCEL_INTERACTION	"TLK_ALYX_CANCEL_INTERACTION"

#define ALYX_MIN_ENEMY_DIST_TO_CROUCH			360			// Minimum distance that our enemy must be for me to crouch
#define ALYX_MIN_ENEMY_HEALTH_TO_CROUCH			15
#define ALYX_CROUCH_DELAY						5			// Time after crouching before Alyx will crouch again

//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
extern int g_interactionZombieMeleeWarning;

LINK_ENTITY_TO_CLASS( npc_alyx, CNPC_Alyx );

BEGIN_DATADESC( CNPC_Alyx )

	DEFINE_FIELD( m_hEmpTool, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hHackTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hStealthLookTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bInteractionAllowed, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fTimeNextSearchForInteractTargets, FIELD_TIME ),
	DEFINE_FIELD( m_bDarknessSpeechAllowed, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsEMPHolstered, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsFlashlightBlind, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fStayBlindUntil, FIELD_TIME ),
	DEFINE_FIELD( m_flDontBlindUntil, FIELD_TIME ),
	DEFINE_FIELD( m_bSpokeLostPlayerInDarkness, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPlayerFlashlightState, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bHadCondSeeEnemy, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iszCurrentBlindScene, FIELD_STRING ),
	DEFINE_FIELD( m_fTimeUntilNextDarknessFoundPlayer, FIELD_TIME ),
	DEFINE_FIELD( m_fCombatStartTime, FIELD_TIME ),
	DEFINE_FIELD( m_fCombatEndTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextCrouchTime, FIELD_TIME ),
	DEFINE_FIELD( m_WeaponType, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_bShouldHaveEMP, FIELD_BOOLEAN, "ShouldHaveEMP" ),
	
	DEFINE_SOUNDPATCH( m_sndDarknessBreathing ),

	DEFINE_EMBEDDED( m_SpeechWatch_LostPlayer ),
	DEFINE_EMBEDDED( m_SpeechTimer_HeardSound ),
	DEFINE_EMBEDDED( m_SpeechWatch_SoundDelay ),
	DEFINE_EMBEDDED( m_SpeechWatch_BreathingRamp ),
	DEFINE_EMBEDDED( m_SpeechWatch_FoundPlayer ),

	DEFINE_EMBEDDED( m_MoveMonitor ),

	DEFINE_INPUTFUNC( FIELD_VOID,		"DisallowInteraction",	InputDisallowInteraction ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"AllowInteraction",		InputAllowInteraction ),
	DEFINE_INPUTFUNC( FIELD_STRING,		"GiveWeapon",			InputGiveWeapon ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN,	"AllowDarknessSpeech",	InputAllowDarknessSpeech ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN,	"GiveEMP",				InputGiveEMP ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"VehiclePunted",		InputVehiclePunted ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"OutsideTransition",	InputOutsideTransition ),

	DEFINE_OUTPUT( m_OnFinishInteractWithObject, "OnFinishInteractWithObject" ),
	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse" ),

	DEFINE_USEFUNC( Use ),

END_DATADESC()

#define ALYX_FEAR_ZOMBIE_DIST_SQR	Square(60)
#define ALYX_FEAR_ANTLION_DIST_SQR	Square(360)

//-----------------------------------------------------------------------------
// Anim events
//-----------------------------------------------------------------------------
static int AE_ALYX_EMPTOOL_ATTACHMENT;
static int AE_ALYX_EMPTOOL_SEQUENCE;
static int AE_ALYX_EMPTOOL_USE;
static int COMBINE_AE_BEGIN_ALTFIRE;
static int COMBINE_AE_ALTFIRE;

ConVar npc_alyx_readiness( "npc_alyx_readiness", "1" );
ConVar npc_alyx_force_stop_moving( "npc_alyx_force_stop_moving", "1" );
ConVar npc_alyx_readiness_transitions( "npc_alyx_readiness_transitions", "1" );
ConVar npc_alyx_crouch( "npc_alyx_crouch", "1" );

// global pointer to Alyx for fast lookups
CEntityClassList<CNPC_Alyx> g_AlyxList;
template <> CNPC_Alyx *CEntityClassList<CNPC_Alyx>::m_pClassList = NULL;

//=========================================================
// initialize Alyx before keyvalues are processed
//=========================================================
CNPC_Alyx::CNPC_Alyx()
{
	g_AlyxList.Insert(this);
	// defaults to having an EMP
	m_bShouldHaveEMP = true;
}

CNPC_Alyx::~CNPC_Alyx( )
{
	g_AlyxList.Remove(this);
}

//=========================================================
// Classify - indicates this NPC's place in the 
// relationship table.
//=========================================================
Class_T	CNPC_Alyx::Classify ( void )
{
	return	CLASS_PLAYER_ALLY_VITAL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Alyx::FValidateHintType( CAI_Hint *pHint )
{
	switch( pHint->HintType() )
	{
	case HINT_WORLD_VISUALLY_INTERESTING:
		return true;
		break;
	case HINT_WORLD_VISUALLY_INTERESTING_STEALTH:
		return true;
		break;
	}

	return BaseClass::FValidateHintType( pHint );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Alyx::ObjectCaps() 
{
	int caps = BaseClass::ObjectCaps();

	if( m_FuncTankBehavior.IsMounted() )
	{
		caps &= ~FCAP_IMPULSE_USE;
	}

	return caps;
}

//=========================================================
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_Alyx::HandleAnimEvent( animevent_t *pEvent )
{
	if (pEvent->event == AE_ALYX_EMPTOOL_ATTACHMENT)
	{
		if (!m_hEmpTool)
		{
			// Old savegame?
			CreateEmpTool();
			if (!m_hEmpTool)
				return;
		}

		int iAttachment = LookupAttachment( pEvent->options );
		m_hEmpTool->SetParent(this, iAttachment);
		m_hEmpTool->SetLocalOrigin( Vector( 0, 0, 0 ) );
		m_hEmpTool->SetLocalAngles( QAngle( 0, 0, 0 ) );

		if( !stricmp( pEvent->options, "Emp_Holster" ) )
		{
			SetEMPHolstered(true);
		}
		else
		{
			SetEMPHolstered(false);
		}

		return;
	}
	else if (pEvent->event == AE_ALYX_EMPTOOL_SEQUENCE)
	{
		if (!m_hEmpTool)
			return;

		CDynamicProp *pEmpTool = dynamic_cast<CDynamicProp *>(m_hEmpTool.Get());

		if (!pEmpTool)
			return;

		int iSequence = pEmpTool->LookupSequence( pEvent->options );
		if (iSequence != ACT_INVALID)
		{
			pEmpTool->PropSetSequence( iSequence );
		}

		return;
	}
	else if (pEvent->event == AE_ALYX_EMPTOOL_USE)
	{
		if( m_OperatorBehavior.IsGoalReady() )
		{
			if( m_OperatorBehavior.m_hContextTarget.Get() != NULL )
			{
				EmpZapTarget( m_OperatorBehavior.m_hContextTarget );
			}
		}
		return;
	}
	else if ( pEvent->event == COMBINE_AE_BEGIN_ALTFIRE )
	{
		EmitSound( "Weapon_CombineGuard.Special1" );
		return;
	}
	else if ( pEvent->event == COMBINE_AE_ALTFIRE )
	{
		animevent_t fakeEvent;

		fakeEvent.pSource = this;
		fakeEvent.event = EVENT_WEAPON_AR2_ALTFIRE;
		GetActiveWeapon()->Operator_HandleAnimEvent( &fakeEvent, this );
		//m_iNumGrenades--;

		return;
	}

	switch( pEvent->event )
	{
	case 1:
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Returns a pointer to Alyx's entity
//=========================================================
CNPC_Alyx *CNPC_Alyx::GetAlyx( void )
{
	return g_AlyxList.m_pClassList;
}

//=========================================================
// 
//=========================================================
bool CNPC_Alyx::CreateBehaviors()
{
	AddBehavior( &m_FuncTankBehavior );
	bool result = BaseClass::CreateBehaviors();

	return result;
}


//=========================================================
// Spawn
//=========================================================
void CNPC_Alyx::Spawn()
{
	BaseClass::Spawn();

	// If Alyx has a parent, she's currently inside a pod. Prevent her from moving.
	if ( GetMoveParent() )
	{
		SetMoveType( MOVETYPE_NONE );
		CapabilitiesClear();

		CapabilitiesAdd( bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD );
		CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );
	}
	else
	{
		SetupAlyxWithoutParent();
		CreateEmpTool( );
	}

	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );

	m_iHealth			= 80;
	m_bloodColor		= DONT_BLEED;

	NPCInit();

	SetUse( &CNPC_Alyx::Use );

	m_bInteractionAllowed = true;

	m_fTimeNextSearchForInteractTargets = gpGlobals->curtime;

	SetEMPHolstered(true);

	m_bDontPickupWeapons = true;

	m_bDarknessSpeechAllowed = true;
		
	m_fCombatStartTime = 0.0f;
	m_fCombatEndTime   = 0.0f;

	m_AnnounceAttackTimer.Set( 3, 5 );
}

//=========================================================
// Precache - precaches all resources this NPC needs
//=========================================================
void CNPC_Alyx::Precache()
{
	BaseClass::Precache();
	PrecacheScriptSound( "npc_alyx.die" );
	PrecacheModel( STRING( GetModelName() ) );
	PrecacheModel( "models/alyx_emptool_prop.mdl" );

	// For hacking
	PrecacheScriptSound( "DoSpark" );
	PrecacheScriptSound( "npc_alyx.starthacking" );
	PrecacheScriptSound( "npc_alyx.donehacking" );
	PrecacheScriptSound( "npc_alyx.readytohack" );
	PrecacheScriptSound( "npc_alyx.interruptedhacking" );
	PrecacheScriptSound( "ep_01.al_dark_breathing01" );
	PrecacheScriptSound( "Weapon_CombineGuard.Special1" );

	UTIL_PrecacheOther( "env_alyxemp" );

	CLASSNAME_ALYXGUN = AllocPooledString( "weapon_alyxgun" );
	CLASSNAME_SMG1 = AllocPooledString( "weapon_smg1" );
	CLASSNAME_SHOTGUN = AllocPooledString( "weapon_shotgun" );
	CLASSNAME_AR2 = AllocPooledString( "weapon_ar2" );
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::Activate( void )
{
	// Alyx always kicks her health back up to full after loading a savegame.
	// Avoids problems with players saving the game in places where she dies immediately afterwards.
	m_iHealth = 80;

	BaseClass::Activate();

	// Alyx always assumes she has said hello to Gordon!
	SetSpokeConcept( TLK_HELLO, NULL, false );	

	// Add my personal concepts
	CAI_AllySpeechManager *pSpeechManager = GetAllySpeechManager();

	if( pSpeechManager )
	{
		ConceptInfo_t conceptRequestItem =
		{
			CONCEPT_ALYX_REQUEST_ITEM,	SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		 -1,	-1,		AICF_TARGET_PLAYER
		};

		pSpeechManager->AddCustomConcept( conceptRequestItem );
	}

	// cleanup savegames that may not have this set
	if (m_hEmpTool)
	{
		m_hEmpTool->AddEffects( EF_PARENT_ANIMATES );
	}

	m_WeaponType = ComputeWeaponType();

	// !!!HACKHACK for Overwatch, If we're in ep2_outland_10, do half damage to Combine
	// Be advised, this will also happen in 10a, but this is not a problem.
	g_HackOutland10DamageHack = false;
	if( !Q_strnicmp( STRING(gpGlobals->mapname), "ep2_outland_10", 14) )
	{
		g_HackOutland10DamageHack = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::StopLoopingSounds( void )
{
	CSoundEnvelopeController::GetController().SoundDestroy( m_sndDarknessBreathing );
	m_sndDarknessBreathing = NULL;

	BaseClass::StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::SelectModel()
{
	// Alyx is allowed to use multiple models, because she appears in the pod.
	// She defaults to her normal model.
	const char *szModel = STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		SetModelName( AllocPooledString("models/alyx.mdl") );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::SetupAlyxWithoutParent( void )
{
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );

	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_DOORS_GROUP | bits_CAP_TURN_HEAD | bits_CAP_DUCK | bits_CAP_SQUAD );
	CapabilitiesAdd( bits_CAP_USE_WEAPONS );
	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );
	CapabilitiesAdd( bits_CAP_AIM_GUN );
	CapabilitiesAdd( bits_CAP_MOVE_SHOOT );
	CapabilitiesAdd( bits_CAP_USE_SHOT_REGULATOR );
}


//-----------------------------------------------------------------------------
// Purpose: Create and initialized Alyx's EMP tool
//-----------------------------------------------------------------------------

void CNPC_Alyx::CreateEmpTool( void )
{
	if (!m_bShouldHaveEMP || m_hEmpTool)
		return;

	m_hEmpTool = (CBaseAnimating*)CreateEntityByName( "prop_dynamic" );
	if ( m_hEmpTool )
	{
		m_hEmpTool->SetModel( "models/alyx_emptool_prop.mdl" );
		m_hEmpTool->SetName( AllocPooledString("Alyx_Emptool") );
		int iAttachment = LookupAttachment( "Emp_Holster" );
		m_hEmpTool->SetParent(this, iAttachment);
		m_hEmpTool->SetOwnerEntity(this);
		m_hEmpTool->SetSolid( SOLID_NONE );
		m_hEmpTool->SetLocalOrigin( Vector( 0, 0, 0 ) );
		m_hEmpTool->SetLocalAngles( QAngle( 0, 0, 0 ) );
		m_hEmpTool->AddSpawnFlags(SF_DYNAMICPROP_NO_VPHYSICS);
		m_hEmpTool->AddEffects( EF_PARENT_ANIMATES );
		m_hEmpTool->Spawn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Map input to create or destroy alyx's EMP tool
//-----------------------------------------------------------------------------

void CNPC_Alyx::InputGiveEMP( inputdata_t &inputdata )
{
	m_bShouldHaveEMP = inputdata.value.Bool();
	if (m_bShouldHaveEMP)
	{
		if (!m_hEmpTool)
		{
			CreateEmpTool( );
		}
	}
	else
	{
		if (m_hEmpTool)
		{
			UTIL_Remove( m_hEmpTool );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

struct ReadinessTransition_t
{
	int				iPreviousLevel;
	int				iCurrentLevel;
	Activity		requiredActivity;
	Activity		transitionActivity;
};


void CNPC_Alyx::ReadinessLevelChanged( int iPriorLevel )
{
	BaseClass::ReadinessLevelChanged( iPriorLevel );

	// When we drop from agitated to stimulated, stand up if we were crouching.
	if ( iPriorLevel == AIRL_AGITATED && GetReadinessLevel() == AIRL_STIMULATED )
	{
		//Warning("CROUCH: Standing, dropping back to stimulated.\n" );
		Stand();
	}

	if ( GetActiveWeapon() == NULL )
		return;

	//If Alyx is going from Relaxed to Agitated or Stimulated, let her raise her weapon before she's able to fire.
	if ( iPriorLevel == AIRL_RELAXED && GetReadinessLevel() > iPriorLevel )
	{
		GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + 0.5 );
	}

	// FIXME: Are there certain animations that we DO want to interrupt?
	if ( HasActiveLayer() )
		return;

	if ( npc_alyx_readiness_transitions.GetBool() )
	{
		// We don't have crouching readiness transitions yet
		if ( IsCrouching() )
			return;

		static ReadinessTransition_t readinessTransitions[] =
		{
			//Previous Readiness level - Current Readiness Level - Activity NPC needs to be playing - Gesture to play
			{ AIRL_RELAXED,	AIRL_STIMULATED, ACT_IDLE, ACT_READINESS_RELAXED_TO_STIMULATED, },
			{ AIRL_RELAXED,	AIRL_STIMULATED, ACT_WALK, ACT_READINESS_RELAXED_TO_STIMULATED_WALK, },
			{ AIRL_AGITATED, AIRL_STIMULATED, ACT_IDLE, ACT_READINESS_AGITATED_TO_STIMULATED, },
			{ AIRL_STIMULATED, AIRL_RELAXED, ACT_IDLE, ACT_READINESS_STIMULATED_TO_RELAXED, }
		};

		for ( int i = 0; i < ARRAYSIZE( readinessTransitions ); i++ )
		{
			if ( GetIdealActivity() != readinessTransitions[i].requiredActivity )
				continue;

			Activity translatedTransitionActivity = Weapon_TranslateActivity( readinessTransitions[i].transitionActivity );

			if ( translatedTransitionActivity == ACT_INVALID || translatedTransitionActivity == readinessTransitions[i].transitionActivity )
				continue;

			Activity finalActivity = TranslateActivityReadiness( translatedTransitionActivity );

			if ( iPriorLevel == readinessTransitions[i].iPreviousLevel && GetReadinessLevel() == readinessTransitions[i].iCurrentLevel )
			{
				RestartGesture( finalActivity );
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	// Figure out if Alyx has just been removed from her parent
	if ( GetMoveType() == MOVETYPE_NONE && !GetMoveParent() )
	{
		// Don't confuse the passenger behavior with just removing Alyx's parent!
		if ( m_PassengerBehavior.IsEnabled() == false )
		{
			SetupAlyxWithoutParent();
			SetupVPhysicsHull();
		}
	}

	// If Alyx is in combat, and she doesn't have her gun out, fetch it
	if ( GetState() == NPC_STATE_COMBAT && IsWeaponHolstered() && !m_FuncTankBehavior.IsRunning() )
	{
		SetDesiredWeaponState( DESIREDWEAPONSTATE_UNHOLSTERED );
	}

	// If we're in stealth mode, and we can still see the stealth node, keep using it
	if ( GetReadinessLevel() == AIRL_STEALTH )
	{
		if ( m_hStealthLookTarget && !m_hStealthLookTarget->IsDisabled() )
		{
			if ( m_hStealthLookTarget->IsInNodeFOV(this) && FVisible( m_hStealthLookTarget ) )
				return;
		}

		// Break out of stealth mode
		SetReadinessLevel( AIRL_STIMULATED, true, true );
		ClearLookTarget( m_hStealthLookTarget );
		m_hStealthLookTarget = NULL;
	}

	// If we're being blinded by the flashlight, see if we should stop
	if ( m_bIsFlashlightBlind )
	{
		// we used to have a bug where if we tried to remove alyx from the blind scene before it got loaded asynchronously,
		// she would get stuck in the animation with m_bIsFlashlightBlind set to false.  that should be fixed, but just to
		// be sure, we wait a bit to prevent this from happening.
		if ( m_fStayBlindUntil < gpGlobals->curtime )
		{
 			CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
 			if ( pPlayer && (!CanBeBlindedByFlashlight( true ) || !pPlayer->IsIlluminatedByFlashlight(this, NULL ) || !PlayerFlashlightOnMyEyes( pPlayer )) &&
				!BlindedByFlare() )
			{
				// Remove the actor from the flashlight scene
				ADD_DEBUG_HISTORY( HISTORY_ALYX_BLIND, UTIL_VarArgs( "(%0.2f) Alyx: end blind scene '%s'\n", gpGlobals->curtime, STRING(m_iszCurrentBlindScene) ) );
				RemoveActorFromScriptedScenes( this, true, false, STRING(m_iszCurrentBlindScene) );

				// Allow firing again, but prevent myself from firing until I'm done
				GetShotRegulator()->EnableShooting();
				GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + 1.0 );
				
				m_bIsFlashlightBlind = false;
				m_flDontBlindUntil = gpGlobals->curtime + RandomFloat( 1, 3 );
			}
		}
	}
	else
	{
		CheckBlindedByFlare();
	}
}

//-----------------------------------------------------------------------------
// Periodically look for opportunities to interact with objects in the world.
// Right now Alyx only interacts with things the player picks up with
// physcannon.
//-----------------------------------------------------------------------------
#define ALYX_INTERACT_SEARCH_FREQUENCY 1.0f // seconds
void CNPC_Alyx::SearchForInteractTargets()
{
	if( m_fTimeNextSearchForInteractTargets > gpGlobals->curtime )
	{
		return;
	}

	m_fTimeNextSearchForInteractTargets = gpGlobals->curtime + ALYX_INTERACT_SEARCH_FREQUENCY;

	// Ensure player can be seen.
	if( !HasCondition( COND_SEE_PLAYER) )
	{
		//Msg("ALYX Can't interact: can't see player\n");
		return;
	}

	CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);

	if( !pPlayer )
	{
		return;
	}

	CBaseEntity *pProspect = PhysCannonGetHeldEntity(pPlayer->GetActiveWeapon());

	if( !pProspect )
	{
		//Msg("ALYX Can't interact: player not holding anything\n");
		return;
	}

	if( !IsValidInteractTarget(pProspect) )
	{
		//Msg("ALYX Can't interact: player holding an invalid object\n");
		return;
	}

	SetInteractTarget(pProspect);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Alyx::GatherConditions()
{
	BaseClass::GatherConditions();

	if( HasCondition( COND_HEAR_DANGER ) )
	{
		// Don't let Alyx worry about combat sounds if she's panicking 
		// from danger sounds. This prevents her from running ALERT_FACE_BEST_SOUND
		// as soon as a grenade explodes (which makes a loud combat sound). If Alyx
		// is NOT panicking over a Danger sound, she'll hear the combat sounds as normal.
		ClearCondition( COND_HEAR_COMBAT );
	}

	// Update flashlight state
	ClearCondition( COND_ALYX_PLAYER_FLASHLIGHT_EXPIRED );
	ClearCondition( COND_ALYX_PLAYER_TURNED_ON_FLASHLIGHT );
	ClearCondition( COND_ALYX_PLAYER_TURNED_OFF_FLASHLIGHT );
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
	if ( pPlayer )
	{
		bool bFlashlightState = pPlayer->FlashlightIsOn() != 0;
		if ( bFlashlightState != m_bPlayerFlashlightState )
		{
			if ( bFlashlightState )
			{
				SetCondition( COND_ALYX_PLAYER_TURNED_ON_FLASHLIGHT );
			}
			else
			{
				// If the power level is low, consider it expired, due
				// to it running out or the player turning it off in anticipation.
				CHL2_Player *pHLPlayer = assert_cast<CHL2_Player*>( pPlayer );
				if ( pHLPlayer->SuitPower_GetCurrentPercentage() < 15 )
				{
					SetCondition( COND_ALYX_PLAYER_FLASHLIGHT_EXPIRED );
				}
				else
				{
					SetCondition( COND_ALYX_PLAYER_TURNED_OFF_FLASHLIGHT );
				}
			}

			m_bPlayerFlashlightState = bFlashlightState;
		}
	}


	if ( m_NPCState == NPC_STATE_COMBAT )
	{
		DoCustomCombatAI();
	}

	if( HasInteractTarget() )
	{
		// Check that any current interact target is still valid.
		if( !IsValidInteractTarget(GetInteractTarget()) )
		{
			SetInteractTarget(NULL);
		}
	}

	// This is not an else...if because the code above could have started
	// with an interact target and ended without one.
	if( !HasInteractTarget() )
	{
		SearchForInteractTargets();
	}

	// Set up our interact conditions.
	if( HasInteractTarget() )
	{
		if( CanInteractWithTarget(GetInteractTarget()) )
		{
			SetCondition(COND_ALYX_CAN_INTERACT_WITH_TARGET);
			ClearCondition(COND_ALYX_CAN_NOT_INTERACT_WITH_TARGET);
		}
		else
		{
			SetCondition(COND_ALYX_CAN_NOT_INTERACT_WITH_TARGET);
			ClearCondition(COND_ALYX_CAN_INTERACT_WITH_TARGET);
		}

		SetCondition( COND_ALYX_HAS_INTERACT_TARGET );
		ClearCondition( COND_ALYX_NO_INTERACT_TARGET );
	}
	else
	{
		SetCondition( COND_ALYX_NO_INTERACT_TARGET );
		ClearCondition( COND_ALYX_HAS_INTERACT_TARGET );
	}

	// Check for explosions!
	if( HasCondition(COND_HEAR_COMBAT) )
	{
		CSound *pSound = GetBestSound(); 

		if ( IsInAVehicle() == false )  // For now, don't do these animations while in the vehicle
		{
			if( (pSound->SoundTypeNoContext() & SOUND_COMBAT) && (pSound->SoundContext() & SOUND_CONTEXT_EXPLOSION) )
			{
				if ( HasShotgun() )
				{
					if ( !IsPlayingGesture(ACT_GESTURE_FLINCH_BLAST_SHOTGUN) && !IsPlayingGesture(ACT_GESTURE_FLINCH_BLAST_DAMAGED_SHOTGUN) )
					{
						RestartGesture( ACT_GESTURE_FLINCH_BLAST_SHOTGUN );
						GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + SequenceDuration( ACT_GESTURE_FLINCH_BLAST_SHOTGUN ) + 0.5f ); // Allow another second for Alyx to bring her weapon to bear after the flinch.
					}
				}
				else
				{
					if ( !IsPlayingGesture(ACT_GESTURE_FLINCH_BLAST) && !IsPlayingGesture(ACT_GESTURE_FLINCH_BLAST_DAMAGED) )
					{
						RestartGesture( ACT_GESTURE_FLINCH_BLAST );
						GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + SequenceDuration( ACT_GESTURE_FLINCH_BLAST ) + 0.5f ); // Allow another second for Alyx to bring her weapon to bear after the flinch.
					}
				}
			}
		}
	}

	// ROBIN: This was here to solve a problem in a playtest. We've since found what we think was the cause.
	// It's a useful piece of debug to have lying there, so I've left it in.
	if ( (GetFlags() & FL_FLY) && m_NPCState != NPC_STATE_SCRIPT && !m_ActBusyBehavior.IsActive() && !m_PassengerBehavior.IsEnabled() )
	{
		Warning( "Removed FL_FLY from Alyx, who wasn't running a script or actbusy. Time %.2f, map %s.\n", gpGlobals->curtime, STRING(gpGlobals->mapname) );
		RemoveFlag( FL_FLY );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Alyx::ShouldPlayerAvoid( void )
{
	if( IsCurSchedule(SCHED_ALYX_NEW_WEAPON, false) )
		return true;

#if 1
	if( IsCurSchedule( SCHED_PC_GET_OFF_COMPANION, false) )
	{
		CBaseEntity *pGroundEnt = GetGroundEntity();
		if( pGroundEnt != NULL && pGroundEnt->IsPlayer() )
		{
			if( GetAbsOrigin().z < pGroundEnt->EyePosition().z )
				return true;
		}
	}
#endif 
	return BaseClass::ShouldPlayerAvoid();
}

//-----------------------------------------------------------------------------
// Just heard a gunfire sound. Try to figure out how much we should know 
// about it.
//-----------------------------------------------------------------------------
void CNPC_Alyx::AnalyzeGunfireSound( CSound *pSound )
{
	Assert( pSound != NULL );

	if( GetState() != NPC_STATE_ALERT && GetState() != NPC_STATE_IDLE )
	{
		// Only have code for IDLE and ALERT now. 
		return;
	}

	// Have to verify a bunch of stuff about the sound. It must have a valid BaseCombatCharacter as the owner,
	// must have a valid target, and we need a valid pointer to the player.
	if( pSound->m_hOwner.Get() == NULL )
		return;

	if( pSound->m_hTarget.Get() == NULL )
		return;

	CBaseCombatCharacter *pSoundOriginBCC = pSound->m_hOwner->MyCombatCharacterPointer();
	if( pSoundOriginBCC == NULL )
		return;

	CBaseEntity *pSoundTarget = pSound->m_hTarget.Get();

	CBasePlayer *pPlayer = AI_GetSinglePlayer();

	Assert( pPlayer != NULL );

	if( pSoundTarget == this )
	{
		// The shooter is firing at me. Assume if Alyx can hear the gunfire, she can deduce its origin.
		UpdateEnemyMemory( pSoundOriginBCC, pSoundOriginBCC->GetAbsOrigin(), this );
	}
	else if( pSoundTarget == pPlayer )
	{
		// The shooter is firing at the player. Assume Alyx can deduce the origin if the player COULD see the origin, and Alyx COULD see the player.
		if( pPlayer->FVisible(pSoundOriginBCC) && FVisible(pPlayer) )
		{
			UpdateEnemyMemory( pSoundOriginBCC, pSoundOriginBCC->GetAbsOrigin(), this );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Alyx::IsValidEnemy( CBaseEntity *pEnemy )
{
	if ( HL2GameRules()->IsAlyxInDarknessMode() )
	{
		if ( !CanSeeEntityInDarkness( pEnemy ) )
			return false;
	}

	// Alyx can only take a stalker as her enemy which is angry at the player or her.
	if ( pEnemy->Classify() == CLASS_STALKER )
	{
		if( !pEnemy->GetEnemy() )
		{
			return false;
		}

		if( pEnemy->GetEnemy() != this && !pEnemy->GetEnemy()->IsPlayer() )
		{
			return false;
		}
	}

	if ( m_AssaultBehavior.IsRunning() && IsTurret( pEnemy ) )
	{
		CBaseCombatCharacter *pBCC = dynamic_cast<CBaseCombatCharacter*>(pEnemy);

		if ( pBCC != NULL && !pBCC->FInViewCone(this) )
		{
			// Don't let turrets that can't shoot me distract me from my assault behavior.
			// This fixes a very specific problem that appeared in Episode 2 map ep2_outland_09
			// Where Alyx wouldn't terminate an assault while standing on an assault point because
			// she was afraid of a turret that was visible from the assault point, but facing the 
			// other direction and thus not a threat. 
			return false;
		}
	}

	return BaseClass::IsValidEnemy(pEnemy);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::Event_Killed( const CTakeDamageInfo &info )
{
	// Destroy our EMP tool since it won't follow us onto the ragdoll anyway
	if ( m_hEmpTool != NULL )
	{
		UTIL_Remove( m_hEmpTool	);
	}

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Alyx::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	// comment on killing npc's
	if ( pVictim->IsNPC() )
	{
		SpeakIfAllowed( TLK_ALYX_ENEMY_DEAD );
	}

	// Alyx builds a proxy for the dead enemy so she has something to shoot at for a short time after
	// the enemy ragdolls.
	if( !(pVictim->GetFlags() & FL_ONGROUND) || pVictim->GetMoveType() != MOVETYPE_STEP )
	{
		// Don't fire up in the air, since the dead enemy will have fallen.
		return;
	}

	if( pVictim->GetAbsOrigin().DistTo(GetAbsOrigin()) < 96.0f )
	{
		// Don't shoot at an enemy corpse that dies very near to me. This will prevent Alyx attacking
		// Other nearby enemies.
		return;
	}

	if( !HasShotgun() )
	{
		CAI_BaseNPC *pTarget = CreateCustomTarget( pVictim->GetAbsOrigin(), 2.0f );

		AddEntityRelationship( pTarget, IRelationType(pVictim), IRelationPriority(pVictim) );

		// Update or Create a memory entry for this target and make Alyx think she's seen this target recently.
		// This prevents the baseclass from not recognizing this target and forcing Alyx into 
		// SCHED_WAKE_ANGRY, which wastes time and causes her to change animation sequences rapidly.
		GetEnemies()->UpdateMemory( GetNavigator()->GetNetwork(), pTarget, pTarget->GetAbsOrigin(), 0.0f, true );
		AI_EnemyInfo_t *pMemory = GetEnemies()->Find( pTarget );

		if( pMemory )
		{
			// Pretend we've known about this target longer than we really have.
			pMemory->timeFirstSeen = gpGlobals->curtime - 10.0f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called by enemy NPC's when they are ignited
// Input  : pVictim - entity that was ignited
//-----------------------------------------------------------------------------
void CNPC_Alyx::EnemyIgnited( CAI_BaseNPC *pVictim )
{
	if ( FVisible( pVictim ) )
	{
		SpeakIfAllowed( TLK_ENEMY_BURNING );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called by combine balls when they're socketed
// Input  : pVictim - entity killed by player
//-----------------------------------------------------------------------------
void CNPC_Alyx::CombineBallSocketed( int iNumBounces )
{
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	
	if ( !pPlayer || !FVisible(pPlayer) )
	{
		return;
	}

	// set up the speech modifiers
	CFmtStrN<128> modifiers( "num_bounces:%d", iNumBounces );

	// fire off a ball socketed concept
	SpeakIfAllowed( TLK_BALLSOCKETED, modifiers );
}

//-----------------------------------------------------------------------------
// Purpose: If we're a passenger in a vehicle
//-----------------------------------------------------------------------------
bool CNPC_Alyx::RunningPassengerBehavior( void )
{
	// Must be active and not outside the vehicle
	if ( m_PassengerBehavior.IsRunning() && m_PassengerBehavior.GetPassengerState() != PASSENGER_STATE_OUTSIDE )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Handle "mobbed" combat condition when Alyx is overwhelmed by force
//-----------------------------------------------------------------------------
void CNPC_Alyx::DoMobbedCombatAI( void )
{
	AIEnemiesIter_t iter;

	float visibleEnemiesScore = 0.0f;
	float closeEnemiesScore = 0.0f;

	for ( AI_EnemyInfo_t *pEMemory = GetEnemies()->GetFirst(&iter); pEMemory != NULL; pEMemory = GetEnemies()->GetNext(&iter) )
	{
		if ( IRelationType( pEMemory->hEnemy ) != D_NU && IRelationType( pEMemory->hEnemy ) != D_LI && pEMemory->hEnemy->GetAbsOrigin().DistToSqr(GetAbsOrigin()) <= ALYX_MIN_CONSIDER_DIST )
		{
			if( pEMemory->hEnemy && pEMemory->hEnemy->IsAlive() && gpGlobals->curtime - pEMemory->timeLastSeen <= 0.5f && pEMemory->hEnemy->Classify() != CLASS_BULLSEYE )
			{
				if( pEMemory->hEnemy->GetAbsOrigin().DistToSqr(GetAbsOrigin()) <= ALYX_MIN_MOB_DIST_SQR )
				{
					closeEnemiesScore += 1.0f;
				}
				else
				{
					visibleEnemiesScore += 1.0f;
				}
			}
		}
	}

	if( closeEnemiesScore > 2 )
	{
		SetCondition( COND_MOBBED_BY_ENEMIES );

		// mark anyone in the mob as having mobbed me
		for ( AI_EnemyInfo_t *pEMemory = GetEnemies()->GetFirst(&iter); pEMemory != NULL; pEMemory = GetEnemies()->GetNext(&iter) )
		{
			if ( pEMemory->bMobbedMe )
				continue;

			if ( IRelationType( pEMemory->hEnemy ) != D_NU && IRelationType( pEMemory->hEnemy ) != D_LI && pEMemory->hEnemy->GetAbsOrigin().DistToSqr(GetAbsOrigin()) <= ALYX_MIN_CONSIDER_DIST )
			{
				if( pEMemory->hEnemy && pEMemory->hEnemy->IsAlive() && gpGlobals->curtime - pEMemory->timeLastSeen <= 0.5f && pEMemory->hEnemy->Classify() != CLASS_BULLSEYE )
				{
					if( pEMemory->hEnemy->GetAbsOrigin().DistToSqr(GetAbsOrigin()) <= ALYX_MIN_MOB_DIST_SQR )
					{
						pEMemory->bMobbedMe = true;
					}
				}
			}
		}
	}
	else
	{
		ClearCondition( COND_MOBBED_BY_ENEMIES );
	}

	// Alyx's gun can never run out of ammo. Allow Alyx to ignore LOW AMMO warnings
	// if she's in a close quarters fight with several enemies. She'll attempt to reload
	// as soon as her combat situation is less pressing.
	if( HasCondition( COND_MOBBED_BY_ENEMIES ) )
	{
		ClearCondition( COND_LOW_PRIMARY_AMMO );
	}

	// Say a combat thing
	if( HasCondition( COND_MOBBED_BY_ENEMIES ) )
	{
		SpeakIfAllowed( TLK_MOBBED );		
	}
	else if( visibleEnemiesScore > 4 )
	{
		SpeakIfAllowed( TLK_MANY_ENEMIES );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Custom AI for Alyx while in combat
//-----------------------------------------------------------------------------
void CNPC_Alyx::DoCustomCombatAI( void )
{
	// Only run the following code if we're not in a vehicle
	if ( RunningPassengerBehavior() == false )
	{
		// Do our mobbed by enemies logic
		DoMobbedCombatAI();
	}

	CBaseEntity *pEnemy = GetEnemy();

	if( HasCondition( COND_LOW_PRIMARY_AMMO ) )
	{
		if( pEnemy )
		{
			if( GetAbsOrigin().DistToSqr( pEnemy->GetAbsOrigin() ) < Square( 60.0f ) )
			{
				// Don't reload if an enemy is right in my face.
				ClearCondition( COND_LOW_PRIMARY_AMMO );
			}
		}
	}

	if ( HasCondition( COND_LIGHT_DAMAGE ) )
	{
		if ( pEnemy && !IsCrouching() )
		{
			// If my enemy is shooting at me from a distance, crouch for protection
			if ( EnemyDistance( pEnemy ) > ALYX_MIN_ENEMY_DIST_TO_CROUCH )
			{
				DesireCrouch();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::DoCustomSpeechAI( void )
{
	BaseClass::DoCustomSpeechAI();

	CBasePlayer *pPlayer = AI_GetSinglePlayer();

	if ( HasCondition(COND_NEW_ENEMY) && GetEnemy() )
	{
		if ( GetEnemy()->Classify() == CLASS_HEADCRAB )
		{
			CBaseHeadcrab *pHC = assert_cast<CBaseHeadcrab*>(GetEnemy());
			// If we see a headcrab for the first time as he's jumping at me, freak out!
			if ( ( GetEnemy()->GetEnemy() == this ) && pHC->IsJumping() && gpGlobals->curtime - GetEnemies()->FirstTimeSeen(GetEnemy()) < 0.5 )
			{
				SpeakIfAllowed( "TLK_SPOTTED_INCOMING_HEADCRAB" );
			}
			// If we see a headcrab leaving a zombie that just died, mention it
			else if ( pHC->GetOwnerEntity() && ( pHC->GetOwnerEntity()->Classify() == CLASS_ZOMBIE ) && !pHC->GetOwnerEntity()->IsAlive() )
			{
				SpeakIfAllowed( "TLK_SPOTTED_HEADCRAB_LEAVING_ZOMBIE" );
			}
		}
		else if ( GetEnemy()->Classify() == CLASS_ZOMBIE ) 
		{
			CNPC_BaseZombie *pZombie = assert_cast<CNPC_BaseZombie*>(GetEnemy());
			// If we see a zombie getting up, mention it
			if ( pZombie->IsGettingUp() )
			{
				SpeakIfAllowed( "TLK_SPOTTED_ZOMBIE_WAKEUP" );
			}
		}
	}

	// Darkness mode speech
	ClearCondition( COND_ALYX_IN_DARK );
 	if ( HL2GameRules()->IsAlyxInDarknessMode() )
	{
		// Even though the darkness light system will take flares into account when Alyx
		// says she's lost the player in the darkness, players still think she's silly
		// when they're too far from the flare to be seen. 
		// So, check for lit flares or other dynamic lights, and don't do
		// a bunch of the darkness speech if there's a lit flare nearby.
  		bool bNearbyFlare = DarknessLightSourceWithinRadius( this, 500 );
		if ( !bNearbyFlare )
		{
			SetCondition( COND_ALYX_IN_DARK );
			if ( HasCondition( COND_ALYX_PLAYER_TURNED_OFF_FLASHLIGHT ) || HasCondition( COND_ALYX_PLAYER_FLASHLIGHT_EXPIRED ) )
			{
				// Player just turned off the flashlight. Start ramping up Alyx's breathing.
				if ( !m_sndDarknessBreathing )
				{
					CPASAttenuationFilter filter( this );
					m_sndDarknessBreathing = CSoundEnvelopeController::GetController().SoundCreate( filter, entindex(), CHAN_STATIC, 
						"ep_01.al_dark_breathing01", SNDLVL_TALKING );
					CSoundEnvelopeController::GetController().Play( m_sndDarknessBreathing, 0.0f, PITCH_NORM );
				}
				
				if ( m_sndDarknessBreathing )
				{
 					CSoundEnvelopeController::GetController().SoundChangeVolume( m_sndDarknessBreathing, ALYX_BREATHING_VOLUME_MAX, RandomFloat(10,20) );
					m_SpeechWatch_BreathingRamp.Stop();
				}
			}
		}

		// If we lose an enemy due to the flashlight, comment about it
		if ( !HasCondition( COND_SEE_ENEMY ) && m_bHadCondSeeEnemy && !HasCondition( COND_TALKER_PLAYER_DEAD ) )
		{
			if ( m_bDarknessSpeechAllowed && HasCondition( COND_ALYX_PLAYER_TURNED_OFF_FLASHLIGHT ) && 
				GetEnemy() && ( GetEnemy()->Classify() != CLASS_BULLSEYE ) )
			{
				SpeakIfAllowed( "TLK_DARKNESS_LOSTENEMY_BY_FLASHLIGHT" );
			}
			else if ( m_bDarknessSpeechAllowed && HasCondition( COND_ALYX_PLAYER_FLASHLIGHT_EXPIRED ) &&
				GetEnemy() && ( GetEnemy()->Classify() != CLASS_BULLSEYE ) )
			{
				SpeakIfAllowed( "TLK_DARKNESS_LOSTENEMY_BY_FLASHLIGHT_EXPIRED" );
			}
			else if ( m_bDarknessSpeechAllowed && GetEnemy() && ( GetEnemy()->Classify() != CLASS_BULLSEYE ) && 
				pPlayer && pPlayer->FlashlightIsOn() && !pPlayer->IsIlluminatedByFlashlight(GetEnemy(), NULL ) && 
				FVisible( GetEnemy() ) )
			{
				SpeakIfAllowed( TLK_DARKNESS_ENEMY_IN_DARKNESS );
			}
			m_bHadCondSeeEnemy = false;
		}
		else if ( HasCondition( COND_SEE_ENEMY ) )
		{
			m_bHadCondSeeEnemy = true;
		}
		else if ( ( !GetEnemy() || ( GetEnemy()->Classify() == CLASS_BULLSEYE ) ) && m_bDarknessSpeechAllowed )
		{
			if ( HasCondition( COND_ALYX_PLAYER_FLASHLIGHT_EXPIRED ) )
			{
				SpeakIfAllowed( TLK_DARKNESS_FLASHLIGHT_EXPIRED );
			}
			else if ( HasCondition( COND_ALYX_PLAYER_TURNED_OFF_FLASHLIGHT ) )
			{
				SpeakIfAllowed( TLK_FLASHLIGHT_OFF );
			}
			else if ( HasCondition( COND_ALYX_PLAYER_TURNED_ON_FLASHLIGHT ) )
			{
				SpeakIfAllowed( TLK_FLASHLIGHT_ON );
			}
		}

		// If we've just seen a new enemy, and it's illuminated by the flashlight, 
		// tell the player to keep the flashlight on 'em.
		if ( HasCondition(COND_NEW_ENEMY) && !HasCondition( COND_TALKER_PLAYER_DEAD ) )
		{
			// First time we've seen this guy?
			if ( gpGlobals->curtime - GetEnemies()->FirstTimeSeen(GetEnemy()) < 0.5 )
			{
				if ( pPlayer && pPlayer->IsIlluminatedByFlashlight(GetEnemy(), NULL ) && m_bDarknessSpeechAllowed && 
					!LookerCouldSeeTargetInDarkness( this, GetEnemy() ) )
				{
					SpeakIfAllowed( "TLK_DARKNESS_FOUNDENEMY_BY_FLASHLIGHT" );
				}
			}
		}

		// When we lose the player, start lost-player talker after some time
 		if ( !bNearbyFlare && m_bDarknessSpeechAllowed )
		{
			if ( !HasCondition(COND_SEE_PLAYER) && !m_SpeechWatch_LostPlayer.IsRunning() )
			{
				m_SpeechWatch_LostPlayer.Set( 5,8 );
				m_SpeechWatch_LostPlayer.Start();
				m_MoveMonitor.SetMark( AI_GetSinglePlayer(), 48 );
			}
			else if ( m_SpeechWatch_LostPlayer.Expired() )
			{
				// Can't see the player?
				if ( !HasCondition(COND_SEE_PLAYER) && !HasCondition( COND_TALKER_PLAYER_DEAD ) && !HasCondition( COND_SEE_ENEMY ) &&
					( !pPlayer || pPlayer->GetAbsOrigin().DistToSqr(GetAbsOrigin()) > ALYX_DARKNESS_LOST_PLAYER_DIST ) )
				{
					// only speak if player hasn't moved.
					if ( m_MoveMonitor.TargetMoved( AI_GetSinglePlayer() ) )
					{
						SpeakIfAllowed( "TLK_DARKNESS_LOSTPLAYER" );
						m_SpeechWatch_LostPlayer.Set(10);
						m_SpeechWatch_LostPlayer.Start();
						m_bSpokeLostPlayerInDarkness = true;
					}
				}
			}

			// Speech concepts that only occur when the player's flashlight is off
			if ( pPlayer && !HasCondition( COND_TALKER_PLAYER_DEAD ) && !pPlayer->FlashlightIsOn() )
 			{
				// When the player first turns off the light, don't talk about sounds for a bit
				if ( HasCondition( COND_ALYX_PLAYER_TURNED_OFF_FLASHLIGHT ) || HasCondition( COND_ALYX_PLAYER_FLASHLIGHT_EXPIRED ) )
				{
					m_SpeechTimer_HeardSound.Set(4);
				}
				else if ( m_SpeechWatch_SoundDelay.Expired() )
				{
					// We've waited for a bit after the sound, now talk about it
					SpeakIfAllowed( "TLK_DARKNESS_HEARDSOUND" );
					m_SpeechWatch_SoundDelay.Stop();
				}
				else if ( HasCondition( COND_HEAR_SPOOKY ) )
				{
					// If we hear anything while the player's flashlight is off, randomly mention it
					if ( m_SpeechTimer_HeardSound.Expired() )
					{
						m_SpeechTimer_HeardSound.Set(10);

						// Wait for the sound to play for a bit before speaking about it
						m_SpeechWatch_SoundDelay.Set( 1.0,3.0 );
						m_SpeechWatch_SoundDelay.Start();
					}
				}
			}
		}

		// Stop the heard sound response if the player turns the flashlight on
		if ( bNearbyFlare || HasCondition( COND_ALYX_PLAYER_TURNED_ON_FLASHLIGHT ) )
		{
			m_SpeechWatch_SoundDelay.Stop();

			if ( m_sndDarknessBreathing )
			{
				CSoundEnvelopeController::GetController().SoundChangeVolume( m_sndDarknessBreathing, 0.0f, 0.5 );
				m_SpeechWatch_BreathingRamp.Stop();
			}
		}
	}
	else
	{
		if ( m_sndDarknessBreathing )
		{
			CSoundEnvelopeController::GetController().SoundChangeVolume( m_sndDarknessBreathing, 0.0f, 0.5 );
			m_SpeechWatch_BreathingRamp.Stop();
		}

		if ( !HasCondition(COND_SEE_PLAYER) && !m_SpeechWatch_FoundPlayer.IsRunning() )
		{
			// wait a minute before saying something when alyx sees him again
			m_SpeechWatch_FoundPlayer.Set( 60, 75 );
			m_SpeechWatch_FoundPlayer.Start();
		}
		else if ( HasCondition(COND_SEE_PLAYER) )
		{
			if ( m_SpeechWatch_FoundPlayer.Expired() && m_bDarknessSpeechAllowed )
			{
				SpeakIfAllowed( "TLK_FOUNDPLAYER" );
			}
			m_SpeechWatch_FoundPlayer.Stop();
		}
	}

	// If we spoke lost-player, and now we see him/her, say so
 	if ( m_bSpokeLostPlayerInDarkness )
	{
		// If we've left darkness mode, or if the player has blinded me with 
		// the flashlight, don't bother speaking the found player line.
		if ( !m_bIsFlashlightBlind && HL2GameRules()->IsAlyxInDarknessMode() && m_bDarknessSpeechAllowed )
		{
			if ( HasCondition(COND_SEE_PLAYER) && !HasCondition( COND_TALKER_PLAYER_DEAD ) )
			{
				if ( ( m_fTimeUntilNextDarknessFoundPlayer == AI_INVALID_TIME ) || ( gpGlobals->curtime < m_fTimeUntilNextDarknessFoundPlayer ) )
				{
					SpeakIfAllowed( "TLK_DARKNESS_FOUNDPLAYER" );
				}
				m_bSpokeLostPlayerInDarkness = false;
			}
		}
		else
		{
			m_bSpokeLostPlayerInDarkness = false;
		}
	}


	if ( ( !m_bDarknessSpeechAllowed || HasCondition(COND_SEE_PLAYER) ) && m_SpeechWatch_LostPlayer.IsRunning() )
	{
		m_SpeechWatch_LostPlayer.Stop();
		m_MoveMonitor.ClearMark();
	}

	// Ramp the breathing back up after speaking
 	if ( m_SpeechWatch_BreathingRamp.IsRunning() )
	{
		if ( m_SpeechWatch_BreathingRamp.Expired() )
		{
			CSoundEnvelopeController::GetController().SoundChangeVolume( m_sndDarknessBreathing, ALYX_BREATHING_VOLUME_MAX, RandomFloat(5,10) );
			m_SpeechWatch_BreathingRamp.Stop();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Alyx::SpeakIfAllowed( AIConcept_t concept, const char *modifiers /*= NULL*/, bool bRespondingToPlayer /*= false*/, char *pszOutResponseChosen /*= NULL*/, size_t bufsize /* = 0 */ )
{
	if ( BaseClass::SpeakIfAllowed( concept, modifiers, bRespondingToPlayer, pszOutResponseChosen, bufsize ) )
	{
		// If we're breathing in the darkness, drop the volume quickly
		if ( m_sndDarknessBreathing && CSoundEnvelopeController::GetController().SoundGetVolume( m_sndDarknessBreathing ) > 0.0 )
		{
			CSoundEnvelopeController::GetController().SoundChangeVolume( m_sndDarknessBreathing, 0.0f, 0.1 );

			// Ramp up the sound again after the response is over
			float flDelay = (GetTimeSpeechComplete() - gpGlobals->curtime);
			m_SpeechWatch_BreathingRamp.Set( flDelay );
			m_SpeechWatch_BreathingRamp.Start();
		}

		return true;
	}

	return false;
}

extern int ACT_ANTLION_FLIP;
extern int ACT_ANTLION_ZAP_FLIP;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Disposition_t CNPC_Alyx::IRelationType( CBaseEntity *pTarget )
{
	Disposition_t disposition = BaseClass::IRelationType( pTarget );

	if ( pTarget == NULL )
		return disposition;

	if( pTarget->Classify() == CLASS_ANTLION )
	{
		if( disposition == D_HT )
		{
			// If Alyx hates this antlion (default relationship), make her fear it, if it is very close.
			if( GetAbsOrigin().DistToSqr(pTarget->GetAbsOrigin()) < ALYX_FEAR_ANTLION_DIST_SQR )
			{
				disposition = D_FR;
			}

			// Fall through...
		}
	}
	else if( pTarget->Classify() == CLASS_ZOMBIE && disposition == D_HT && GetActiveWeapon() )
	{
		if( GetAbsOrigin().DistToSqr(pTarget->GetAbsOrigin()) < ALYX_FEAR_ZOMBIE_DIST_SQR )
		{
			// Be afraid of a zombie that's near if I'm not allowed to dodge. This will make Alyx back away.
			return D_FR;
		}
	}
	else if ( pTarget->Classify() == CLASS_MISSILE )
	{
		// Fire at missiles while in the vehicle
		if ( IsInAVehicle() )
			return D_HT;
	}

	return disposition;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Alyx::IRelationPriority( CBaseEntity *pTarget )
{
	int priority = BaseClass::IRelationPriority( pTarget );

	if( pTarget->Classify() == CLASS_ANTLION )
	{
		// Make Alyx prefer Antlions that are flipped onto their backs.
		// UNLESS she has a different enemy that could melee attack her while her back is turned.
		CAI_BaseNPC *pNPC = pTarget->MyNPCPointer();
		if ( pNPC && ( pNPC->GetActivity() == ACT_ANTLION_FLIP || pNPC->GetActivity() == ACT_ANTLION_ZAP_FLIP  ) )
		{
			if( GetEnemy() && GetEnemy() != pTarget )
			{
				// I have an enemy that is not this thing. If that enemy is near, I shouldn't
				// become distracted.
				if( GetAbsOrigin().DistToSqr(GetEnemy()->GetAbsOrigin()) < Square(180) )
				{
					return priority;
				}
			}

			priority += 1;
		}
	}

	return priority;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define ALYX_360_VIEW_DIST_SQR	129600 // 30 feet
bool CNPC_Alyx::FInViewCone( CBaseEntity *pEntity )
{
	// Alyx can see 360 degrees but only at limited distance. This allows her to be aware of a 
	// large mob of enemies (usually antlions or zombies) closing in. This situation is so obvious to the 
	// player that it doesn't make sense for Alyx to be unaware of the entire group simply because she 
	// hasn't seen all of the enemies with her own eyes.
	if( ( pEntity->IsNPC() || pEntity->IsPlayer() ) && pEntity->GetAbsOrigin().DistToSqr(GetAbsOrigin()) <= ALYX_360_VIEW_DIST_SQR )
	{
		// Only see players and NPC's with 360 cone
		// For instance, DON'T tell the eyeball/head tracking code that you can see an object that is behind you!
		return true;
	}

	// Else, fall through...
 	if ( HL2GameRules()->IsAlyxInDarknessMode() )
	{
		if ( CanSeeEntityInDarkness( pEntity ) )
			return true;
	}

	return BaseClass::FInViewCone( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
bool CNPC_Alyx::CanSeeEntityInDarkness( CBaseEntity *pEntity )
{
	/*
	// Alyx can see enemies that are right next to her
	// Robin: Disabled, made her too effective, you could safely leave her alone.
  	if ( pEntity->IsNPC() )
	{
		if ( (pEntity->WorldSpaceCenter() - EyePosition()).LengthSqr() < (80*80) )
			return true;
	}
	*/

	CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
	if ( pPlayer && pEntity != pPlayer )
	{
		if ( pPlayer->IsIlluminatedByFlashlight(pEntity, NULL ) )
			return true;
	}

	return LookerCouldSeeTargetInDarkness( this, pEntity );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Alyx::QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC)
{
	if ( HL2GameRules()->IsAlyxInDarknessMode() )
	{
		if ( !CanSeeEntityInDarkness( pEntity ) )
			return false;
	}

	return BaseClass::QuerySeeEntity(pEntity, bOnlyHateOrFearIfNPC);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Alyx::IsCoverPosition( const Vector &vecThreat, const Vector &vecPosition )
{
	return BaseClass::IsCoverPosition( vecThreat, vecPosition );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Activity CNPC_Alyx::NPC_TranslateActivity( Activity activity )
{
	activity = BaseClass::NPC_TranslateActivity( activity );

	if ( activity == ACT_RUN && GetEnemy() && GetEnemy()->Classify() == CLASS_COMBINE_GUNSHIP )
	{
		// Always cower from gunship!
		if ( HaveSequenceForActivity( ACT_RUN_PROTECTED ) )
			activity = ACT_RUN_PROTECTED;
	}

	switch ( activity )
	{
		// !!!HACK - Alyx doesn't have the required animations for shotguns, 
		// so trick her into using the rifle counterparts for now (sjb)
		case ACT_RUN_AIM_SHOTGUN:			return ACT_RUN_AIM_RIFLE;
		case ACT_WALK_AIM_SHOTGUN:			return ACT_WALK_AIM_RIFLE;
		case ACT_IDLE_ANGRY_SHOTGUN:		return ACT_IDLE_ANGRY_SMG1;
		case ACT_RANGE_ATTACK_SHOTGUN_LOW:	return ACT_RANGE_ATTACK_SMG1_LOW;

		case ACT_PICKUP_RACK:				return (Activity)ACT_ALYX_PICKUP_RACK;
		case ACT_DROP_WEAPON:				if ( HasShotgun() ) return (Activity)ACT_DROP_WEAPON_SHOTGUN;
	}

	return activity;
}

bool CNPC_Alyx::ShouldDeferToFollowBehavior()
{
	return BaseClass::ShouldDeferToFollowBehavior();
}

void CNPC_Alyx::BuildScheduleTestBits()
{
	bool bIsInteracting = false;

	bIsInteracting = ( IsCurSchedule(SCHED_ALYX_PREPARE_TO_INTERACT_WITH_TARGET, false)	||
	IsCurSchedule(SCHED_ALYX_WAIT_TO_INTERACT_WITH_TARGET, false)					||
	IsCurSchedule(SCHED_ALYX_INTERACT_WITH_TARGET, false)							||
	IsCurSchedule(SCHED_ALYX_INTERACTION_INTERRUPTED, false)						||
	IsCurSchedule(SCHED_ALYX_FINISH_INTERACTING_WITH_TARGET, false) );

	if( !bIsInteracting && IsAllowedToInteract() )
	{
		switch( m_NPCState )
		{
		case NPC_STATE_COMBAT:
			SetCustomInterruptCondition( COND_ALYX_HAS_INTERACT_TARGET );
			SetCustomInterruptCondition( COND_ALYX_CAN_INTERACT_WITH_TARGET );
			break;

		case NPC_STATE_ALERT:
		case NPC_STATE_IDLE:
			SetCustomInterruptCondition( COND_ALYX_HAS_INTERACT_TARGET );
			SetCustomInterruptCondition( COND_ALYX_CAN_INTERACT_WITH_TARGET );
			break;
		}
	}

	// This nugget fixes a bug where Alyx will continue to attack an enemy she no longer hates in the
	// case where her relationship with the enemy changes while she's running a SCHED_SCENE_GENERIC. 
	// Since we don't run ChooseEnemy() when we're running a schedule that doesn't interrupt on COND_NEW_ENEMY,
	// we also do not re-evaluate and flush enemies we don't hate anymore. (sjb 6/9/2005)
	if( IsCurSchedule(SCHED_SCENE_GENERIC) && GetEnemy() && GetEnemy()->VPhysicsGetObject() )
	{
		if( GetEnemy()->VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
		{
			SetCustomInterruptCondition( COND_NEW_ENEMY );
		}
	}

	if( GetCurSchedule()->HasInterrupt( COND_IDLE_INTERRUPT ) )
	{
		SetCustomInterruptCondition( COND_BETTER_WEAPON_AVAILABLE );
	}

	// If we're not in a script, keep an eye out for falling
	if ( m_NPCState != NPC_STATE_SCRIPT && !IsInAVehicle() && !IsCurSchedule(SCHED_ALYX_FALL_TO_GROUND,false) )
	{
		SetCustomInterruptCondition( COND_FLOATING_OFF_GROUND );
	}

	BaseClass::BuildScheduleTestBits();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Alyx::ShouldBehaviorSelectSchedule( CAI_BehaviorBase *pBehavior )
{
	if( pBehavior == &m_AssaultBehavior )
	{
		if( HasCondition( COND_MOBBED_BY_ENEMIES ))
			return false;
	}

	return BaseClass::ShouldBehaviorSelectSchedule( pBehavior );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Alyx::SelectSchedule( void )
{
    // If we're in darkness mode, and the player has the flashlight off, and we hear a zombie footstep,
	// and the player isn't nearby, deliberately turn away from the zombie to let the zombie grab me.
	if ( HL2GameRules()->IsAlyxInDarknessMode() && m_NPCState == NPC_STATE_ALERT )
	{
		if ( HasCondition ( COND_HEAR_COMBAT ) && !HasCondition(COND_SEE_PLAYER) )
		{
			CSound *pBestSound = GetBestSound();
			if ( pBestSound && pBestSound->m_hOwner )
			{
				if ( pBestSound->m_hOwner->Classify() == CLASS_ZOMBIE && pBestSound->SoundChannel() == SOUNDENT_CHANNEL_NPC_FOOTSTEP )
					return SCHED_ALYX_ALERT_FACE_AWAYFROM_BESTSOUND;
			}
		}
	}

	if( HasCondition(COND_ALYX_CAN_INTERACT_WITH_TARGET) )
		return SCHED_ALYX_INTERACT_WITH_TARGET;

	if( HasCondition(COND_ALYX_HAS_INTERACT_TARGET) && HasCondition(COND_SEE_PLAYER) && IsAllowedToInteract() )
	{
		ExpireCurrentRandomLookTarget();
		if( IsEMPHolstered() )
		{
			return SCHED_ALYX_PREPARE_TO_INTERACT_WITH_TARGET;
		}

		return SCHED_ALYX_WAIT_TO_INTERACT_WITH_TARGET;
	}

	if( !IsEMPHolstered() && !HasInteractTarget() && !m_ActBusyBehavior.IsActive() )
		return SCHED_ALYX_HOLSTER_EMP;

	if ( HasCondition(COND_BETTER_WEAPON_AVAILABLE) )
	{
		if( m_iszPendingWeapon != NULL_STRING )
		{
			return SCHED_SWITCH_TO_PENDING_WEAPON;
		}
		else
		{
			CBaseHLCombatWeapon *pWeapon = dynamic_cast<CBaseHLCombatWeapon *>(Weapon_FindUsable( WEAPON_SEARCH_DELTA ));
			if ( pWeapon )
			{
				m_flNextWeaponSearchTime = gpGlobals->curtime + 10.0;
				// Now lock the weapon for several seconds while we go to pick it up.
				pWeapon->Lock( 10.0, this );
				SetTarget( pWeapon );
				return SCHED_ALYX_NEW_WEAPON;
			}
		}
	}

	if ( HasCondition(COND_ENEMY_OCCLUDED) )
	{
		//Warning("CROUCH: Standing, enemy is occluded.\n" );
		Stand();
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Alyx::SelectScheduleDanger( void )
{
	if( HasCondition( COND_HEAR_DANGER ) )
	{
		CSound *pSound;
		pSound = GetBestSound( SOUND_DANGER );

		ASSERT( pSound != NULL );

		if ( pSound && (pSound->m_iType & SOUND_DANGER) && ( pSound->SoundChannel() == SOUNDENT_CHANNEL_ZOMBINE_GRENADE ) )
		{
			SpeakIfAllowed( TLK_DANGER_ZOMBINE_GRENADE );
		}
	}
	
	return BaseClass::SelectScheduleDanger();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Alyx::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_ALERT_FACE_BESTSOUND:
		return SCHED_ALYX_ALERT_FACE_BESTSOUND;
		break;

	case SCHED_COMBAT_FACE:
		if ( !HasCondition(COND_TASK_FAILED) && !IsCrouching() )
			return SCHED_ALYX_COMBAT_FACE;
		break;

	case SCHED_WAKE_ANGRY:
		return SCHED_ALYX_WAKE_ANGRY;
		break;

	case SCHED_FALL_TO_GROUND:
		return SCHED_ALYX_FALL_TO_GROUND;
		break;

	case SCHED_ALERT_REACT_TO_COMBAT_SOUND:
		return SCHED_ALYX_ALERT_REACT_TO_COMBAT_SOUND;
		break;

	case SCHED_COWER:
	case SCHED_PC_COWER:
		// Alyx doesn't have cower animations.
		return SCHED_FAIL;

	case SCHED_RANGE_ATTACK1:
		{
			if ( GetEnemy() )
			{
				CBaseEntity *pEnemy = GetEnemy();
				if ( !IsCrouching() )
				{
					// Does my enemy have enough health to warrant crouching?
					if ( pEnemy->GetHealth() > ALYX_MIN_ENEMY_HEALTH_TO_CROUCH )
					{
						// And are they far enough away? Expand the min dist so we don't crouch & stand immediately.
						if ( EnemyDistance( pEnemy ) > (ALYX_MIN_ENEMY_DIST_TO_CROUCH * 1.5) && (pEnemy->GetFlags() & FL_ONGROUND) )
						{
							//Warning("CROUCH: Desiring due to enemy far away.\n" );
							DesireCrouch();
						}
					}
				}

				// Are we supposed to be crouching?
				if ( IsCrouching() || ( CrouchIsDesired() && !HasCondition( COND_HEAVY_DAMAGE ) ) ) 
				{
					// See if they're a valid crouch target
					if ( EnemyIsValidCrouchTarget( pEnemy ) )
					{
						Crouch();
					}
					else
					{
						//Warning("CROUCH: Standing, enemy not valid crouch target.\n" );
						Stand();
					}
				}
				else
				{
					//Warning("CROUCH: Standing, no enemy.\n" );
					Stand();
				}
			}

			return SCHED_ALYX_RANGE_ATTACK1;
		}
		break;

	case SCHED_HIDE_AND_RELOAD:
		{
			if ( HL2GameRules()->IsAlyxInDarknessMode() )
				return SCHED_RELOAD;

			// If I don't have a ranged attacker as an enemy, don't try to hide
			AIEnemiesIter_t iter;
			for ( AI_EnemyInfo_t *pEMemory = GetEnemies()->GetFirst(&iter); pEMemory != NULL; pEMemory = GetEnemies()->GetNext(&iter) )
			{
				CAI_BaseNPC *pEnemy = pEMemory->hEnemy.Get()->MyNPCPointer();
				if ( !pEnemy )
					continue;

				// Ignore enemies that don't hate me
				if ( pEnemy->IRelationType( this ) != D_HT )
					continue;

				// Look for enemies with ranged capabilities
				if ( pEnemy->CapabilitiesGet() & ( bits_CAP_WEAPON_RANGE_ATTACK1 | bits_CAP_WEAPON_RANGE_ATTACK2 | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_RANGE_ATTACK2 ) )
					return SCHED_HIDE_AND_RELOAD;
			}

			return SCHED_RELOAD;
		}
		break;

	case SCHED_RUN_FROM_ENEMY:
		if ( HasCondition( COND_MOBBED_BY_ENEMIES ) )
		{
			return SCHED_RUN_FROM_ENEMY_MOB;
		}
		break;

	case SCHED_IDLE_STAND:
		return SCHED_ALYX_IDLE_STAND;

#ifdef HL2_EPISODIC
	case SCHED_RUN_RANDOM:
		if( GetEnemy() && HasCondition(COND_SEE_ENEMY) && GetActiveWeapon() )
		{
			// SCHED_RUN_RANDOM is a last ditch effort, it's the bottom of a chain of 
			// sequential schedule failures. Since this can cause Alyx to freeze up, 
			// just let her fight if she can. (sjb).
			return SCHED_RANGE_ATTACK1;
		}
		break;
#endif// HL2_EPISODIC
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SOUND_WAKE:
		LocateEnemySound();
		// Don't do the half second wait here that the PlayerCompanion class does. (sbj) 1/4/2006
		TaskComplete();
		break;

	case TASK_ANNOUNCE_ATTACK:
		{
			SpeakAttacking();
			BaseClass::StartTask( pTask );
			break;
		}

	case TASK_ALYX_BUILD_COMBAT_FACE_PATH:
		{
			if ( GetEnemy() && !FInAimCone( GetEnemyLKP() ) && FVisible( GetEnemyLKP() ) )
			{
				Vector vecToEnemy = GetEnemyLKP() - GetAbsOrigin();
				VectorNormalize( vecToEnemy );

				Vector vecMoveGoal = GetAbsOrigin() - (vecToEnemy * 24.0f);

				if ( !GetNavigator()->SetGoal( vecMoveGoal ) )
				{
					TaskFail(FAIL_NO_ROUTE);
				}
				else
				{
					GetMotor()->SetIdealYawToTarget( GetEnemy()->WorldSpaceCenter() );
					GetNavigator()->SetArrivalDirection( GetEnemy() );
					TaskComplete();
				}
			}
			else
			{
				TaskFail("Defaulting To BaseClass::CombatFace");
			}
		}
		break;

	case TASK_ALYX_HOLSTER_AND_DESTROY_PISTOL:
		{
			// If we don't have the alyx gun, throw away our current,
			// since the alyx gun is the only one we can tuck away.
			if ( HasAlyxgun() )
			{
				SetDesiredWeaponState( DESIREDWEAPONSTATE_HOLSTERED_DESTROYED );
			}
			else
			{
				Weapon_Drop( GetActiveWeapon() );
			}

			SetWait( 1 ); // Wait while she does it.
		}
		break;

	case TASK_STOP_MOVING:
		if ( npc_alyx_force_stop_moving.GetBool() )
		{
			if ( ( GetNavigator()->IsGoalSet() && GetNavigator()->IsGoalActive() ) || GetNavType() == NAV_JUMP )
			{
				DbgNavMsg( this, "Start TASK_STOP_MOVING\n" );
				DbgNavMsg( this, "Initiating stopping path\n" );
				GetNavigator()->StopMoving( false );

				// E3 Hack
				if ( HasPoseMoveYaw() ) 
				{
					SetPoseParameter( m_poseMove_Yaw, 0 );
				}
			}
			else
			{
				if ( GetNavigator()->SetGoalFromStoppingPath() )
				{
					DbgNavMsg( this, "Start TASK_STOP_MOVING\n" );
					DbgNavMsg( this, "Initiating stopping path\n" );
				}
				else
				{
					GetNavigator()->ClearGoal();

					if ( IsMoving() )
					{
						SetIdealActivity( GetStoppedActivity() );
					}
					TaskComplete();
				}
			}
		}
		else
		{
			BaseClass::StartTask( pTask );
		}
		break;

	case TASK_REACT_TO_COMBAT_SOUND:
		{
			CSound *pSound = GetBestSound();

			if( pSound && pSound->IsSoundType(SOUND_COMBAT) && pSound->IsSoundType(SOUND_CONTEXT_GUNFIRE) )
			{
				AnalyzeGunfireSound(pSound);
			}

			TaskComplete();
		}
		break;

	case TASK_ALYX_HOLSTER_PISTOL:
		HolsterPistol();
		TaskComplete();
		break;

	case TASK_ALYX_DRAW_PISTOL:
		DrawPistol();
		TaskComplete();
		break;

	case TASK_ALYX_WAIT_HACKING:
		SetWait( pTask->flTaskData );
		break;

	case TASK_ALYX_GET_PATH_TO_INTERACT_TARGET:
		{
			if( !HasInteractTarget() )
			{
				TaskFail("No interact target");
				return;
			}

			AI_NavGoal_t goal;

			goal.type = GOALTYPE_LOCATION;
			goal.dest = GetInteractTarget()->WorldSpaceCenter();
			goal.pTarget = GetInteractTarget();

			GetNavigator()->SetGoal( goal );
		}
		break;

	case TASK_ALYX_ANNOUNCE_HACK:
		SpeakIfAllowed( CONCEPT_ALYX_REQUEST_ITEM );
		TaskComplete();
		break;

	case TASK_ALYX_BEGIN_INTERACTION:
		{
			INPCInteractive *pInteractive = dynamic_cast<INPCInteractive *>(GetInteractTarget());
			if ( pInteractive )
			{
				EmpZapTarget( GetInteractTarget() );

				pInteractive->AlyxStartedInteraction();
				pInteractive->NotifyInteraction(this);
				pInteractive->AlyxFinishedInteraction();
				m_OnFinishInteractWithObject.FireOutput( GetInteractTarget(), this );
			}

			TaskComplete();
		}
		break;

	case TASK_ALYX_COMPLETE_INTERACTION:
		{
			INPCInteractive *pInteractive = dynamic_cast<INPCInteractive *>(GetInteractTarget());

			if( pInteractive )
			{
				for( int i = 0 ; i < 3 ; i++ )
				{
					g_pEffects->Sparks( GetInteractTarget()->WorldSpaceCenter() );
				}

				GetInteractTarget()->EmitSound("DoSpark");
				Speak( CONCEPT_ALYX_INTERACTION_DONE );

				SetInteractTarget(NULL);
			}

			TaskComplete();
		}
		break;

	case TASK_ALYX_SET_IDLE_ACTIVITY:
		{
			Activity goalActivity = (Activity)((int)pTask->flTaskData);
			if ( IsActivityFinished() )
			{
				SetIdealActivity( goalActivity );
			}
		}
		break;

	case TASK_ALYX_FALL_TO_GROUND:
		// If we wait this long without landing, we'll fall to our death
		SetWait(2);
		break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Alyx::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
		case TASK_ALYX_HOLSTER_AND_DESTROY_PISTOL:
			if( IsWaitFinished() )
				TaskComplete();
			break;

		case TASK_STOP_MOVING:
		{
			if ( npc_alyx_force_stop_moving.GetBool() )
			{
				ChainRunTask( TASK_WAIT_FOR_MOVEMENT );
				if ( !TaskIsRunning() )
				{
					DbgNavMsg( this, "TASK_STOP_MOVING Complete\n" );
				}
			}
			else
			{
				BaseClass::RunTask( pTask );
			}
			break;
		}

	case TASK_ALYX_WAIT_HACKING:
		if( GetInteractTarget() && random->RandomInt(0, 3) == 0 )
		{
			g_pEffects->Sparks( GetInteractTarget()->WorldSpaceCenter() );
			GetInteractTarget()->EmitSound("DoSpark");
		}

		if ( IsWaitFinished() )
		{
			TaskComplete();
		}
		break;

	case TASK_ALYX_SET_IDLE_ACTIVITY:
		{
			if ( IsActivityStarted() )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_ALYX_FALL_TO_GROUND:
		if ( GetFlags() & FL_ONGROUND )
		{
			TaskComplete();
		}
		else if( IsWaitFinished() )
		{
			// Call back to the base class & see if it can find a ground for us
			// If it can't, we'll fall to our death
			ChainRunTask( TASK_FALL_TO_GROUND );
			if ( TaskIsRunning() )
			{
				CTakeDamageInfo info;
				info.SetDamage( m_iHealth );
				info.SetAttacker( this );
				info.SetInflictor( this );
				info.SetDamageType( DMG_GENERIC );
				TakeDamage( info );
			}
		}
		break;

	default:
		BaseClass::RunTask(pTask);
		break;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Alyx::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	switch( NewState )
	{
	case NPC_STATE_COMBAT:
		{
			m_fCombatStartTime = gpGlobals->curtime;
		}
		break;

	default:
		if( OldState == NPC_STATE_COMBAT )
		{
			// coming out of combat state.
			m_fCombatEndTime = gpGlobals->curtime + 2.0f;
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Alyx::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );

	// FIXME: hack until some way of removing decals after healing
	m_fNoDamageDecal = true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Alyx::CanBeHitByMeleeAttack( CBaseEntity *pAttacker )
{
	if( IsCurSchedule(SCHED_DUCK_DODGE) )
	{
		return false;
	}

	return BaseClass::CanBeHitByMeleeAttack( pAttacker );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Alyx::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	//!!!HACKHACK - EP1 - Stop alyx taking all physics damage to prevent her dying
	// in freak accidents resembling spontaneous stress damage death (which are now impossible)
	// Also stop her taking damage from flames: Fixes her being burnt to death from entity flames
	// attached to random debris chunks while inside scripted sequences.
	if( info.GetDamageType() & (DMG_CRUSH | DMG_BURN) )
		return 0;

	// If we're in commentary mode, prevent her taking damage from other NPCs
	if ( IsInCommentaryMode() && info.GetAttacker() && info.GetAttacker()->IsNPC() )
		return 0;

	int taken = BaseClass::OnTakeDamage_Alive(info);

	if ( taken && HL2GameRules()->IsAlyxInDarknessMode() && !HasCondition( COND_TALKER_PLAYER_DEAD ) )
	{
		if ( !HasCondition(COND_SEE_ENEMY) && (info.GetDamageType() & (DMG_SLASH | DMG_CLUB) ) )
		{
			// I've taken melee damage. If I haven't seen the enemy for a few seconds, make some noise.
  			float flLastTimeSeen = GetEnemies()->LastTimeSeen( info.GetAttacker(), false );
			if ( flLastTimeSeen == AI_INVALID_TIME || gpGlobals->curtime - flLastTimeSeen > 3.0 )
			{
				SpeakIfAllowed( "TLK_DARKNESS_UNKNOWN_WOUND" );
				m_fTimeUntilNextDarknessFoundPlayer = gpGlobals->curtime + RandomFloat( 3, 5 );
			}
		}
	}

	if( taken && (info.GetDamageType() & DMG_BLAST) )
	{
		if ( HasShotgun() )
		{
			if ( !IsPlayingGesture(ACT_GESTURE_FLINCH_BLAST) && !IsPlayingGesture(ACT_GESTURE_FLINCH_BLAST_DAMAGED_SHOTGUN) )
			{
				RestartGesture( ACT_GESTURE_FLINCH_BLAST_DAMAGED_SHOTGUN );
				GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + SequenceDuration( ACT_GESTURE_FLINCH_BLAST_DAMAGED_SHOTGUN ) + 0.5f );
			}
		}
		else
		{
			if ( !IsPlayingGesture(ACT_GESTURE_FLINCH_BLAST) && !IsPlayingGesture(ACT_GESTURE_FLINCH_BLAST_DAMAGED) )
			{
				RestartGesture( ACT_GESTURE_FLINCH_BLAST_DAMAGED );
				GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + SequenceDuration( ACT_GESTURE_FLINCH_BLAST_DAMAGED ) + 0.5f );
			}
		}
	}

	return taken;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool CNPC_Alyx::FCanCheckAttacks()
{
	if( GetEnemy() && IsGunship( GetEnemy() ) )
	{
		// Don't attack gunships
		return false;
	}

	return BaseClass::FCanCheckAttacks();
}

//-----------------------------------------------------------------------------
// Purpose: Half damage against Combine Soldiers in outland_10
//-----------------------------------------------------------------------------
float CNPC_Alyx::GetAttackDamageScale( CBaseEntity *pVictim )
{
	if( g_HackOutland10DamageHack && pVictim->Classify() == CLASS_COMBINE )
	{
		return 0.75f;
	}

	return BaseClass::GetAttackDamageScale( pVictim );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Alyx::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if( interactionType == g_interactionZombieMeleeWarning && IsAllowedToDodge() )
	{
		// If a zombie is attacking, ditch my current schedule and duck if I'm running a schedule that will
		// be interrupted if I'm hit.
		if( ConditionInterruptsCurSchedule(COND_LIGHT_DAMAGE) || ConditionInterruptsCurSchedule( COND_HEAVY_DAMAGE) )
		{
			//Only dodge an NPC you can see attacking.
			if( sourceEnt && FInViewCone(sourceEnt) )
			{
				SetSchedule(SCHED_DUCK_DODGE);
			}
		}

		return true;
	}

	if( interactionType == g_interactionPlayerPuntedHeavyObject )
	{
		// Try to get Alyx out of the way when player is punting cars around.
		CBaseEntity *pProp = (CBaseEntity*)(data);

		if( pProp )
		{
			float distToProp = pProp->WorldSpaceCenter().DistTo( GetAbsOrigin() );
			float distToPlayer = sourceEnt->WorldSpaceCenter().DistTo( GetAbsOrigin() );

			// Do this if the prop is within 60 feet, and is closer to me than the player is.
			if( distToProp < (60.0f * 12.0f) && (distToProp < distToPlayer) )
			{
				if( fabs(pProp->WorldSpaceCenter().z - WorldSpaceCenter().z) <= 120.0f )
				{
					if( sourceEnt->FInViewCone(this) )
					{
						CSoundEnt::InsertSound( SOUND_MOVE_AWAY, EarPosition(), 16, 1.0f, pProp );
					}
				}
			}
		}
		return true;
	}

	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Alyx::HolsterPistol()
{
	if( GetActiveWeapon() )
	{
		GetActiveWeapon()->AddEffects(EF_NODRAW);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Alyx::DrawPistol()
{
	if( GetActiveWeapon() )
	{
		GetActiveWeapon()->RemoveEffects(EF_NODRAW);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Alyx::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity )
{
	BaseClass::Weapon_Drop( pWeapon, pvecTarget, pVelocity );

	if( pWeapon && pWeapon->ClassMatches( CLASSNAME_ALYXGUN ) )
	{
		pWeapon->SUB_Remove();
	}

	m_WeaponType = WT_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Alyx::IsAllowedToAim()
{
	// Alyx can aim only if fully agitated
	if( GetReadinessLevel() != AIRL_AGITATED )
		return false;

	return BaseClass::IsAllowedToAim();
}


//-----------------------------------------------------------------------------
void CNPC_Alyx::PainSound( const CTakeDamageInfo &info )
{
	// Alex has specific sounds for when attacked in the dark
	if ( !HasCondition( COND_ALYX_IN_DARK ) )
	{
		// set up the speech modifiers
		CFmtStrN<128> modifiers( "damageammo:%s", info.GetAmmoName() );

		SpeakIfAllowed( TLK_WOUND, modifiers );
	}
}

//-----------------------------------------------------------------------------

void CNPC_Alyx::DeathSound( const CTakeDamageInfo &info )
{
	// Sentences don't play on dead NPCs
	SentenceStop();

	if ( !SpokeConcept( TLK_SELF_IN_BARNACLE ) )
	{
		EmitSound( "npc_alyx.die" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::OnSeeEntity( CBaseEntity *pEntity )
{
	BaseClass::OnSeeEntity(pEntity);

	if( pEntity->IsPlayer() &&  pEntity->IsEFlagSet(EFL_IS_BEING_LIFTED_BY_BARNACLE) )
	{
		SpeakIfAllowed( TLK_ALLY_IN_BARNACLE );
	}
}


//---------------------------------------------------------
// A sort of trivial rejection, this function tells us whether
// this object is something Alyx can interact with at all.
// (Alyx's state and the object's state are not considered
// at this stage)
//---------------------------------------------------------
bool CNPC_Alyx::IsValidInteractTarget( CBaseEntity *pTarget )
{
	INPCInteractive *pInteractive = dynamic_cast<INPCInteractive *>(pTarget);

	if( !pInteractive )
	{
		// Not an INPCInteractive entity.
		return false;
	}

	if( !pInteractive->CanInteractWith(this) )
	{
		return false;
	}

	if( pInteractive->HasBeenInteractedWith() )
	{
		// Already been interacted with.
		return false;
	}

	IPhysicsObject *pPhysics;

	pPhysics = pTarget->VPhysicsGetObject();
	if( pPhysics )
	{
		if( !(pPhysics->GetGameFlags() & FVPHYSICS_PLAYER_HELD) )
		{
			// Player isn't holding this physics object
			return false;
		}
	}

	if( GetAbsOrigin().DistToSqr(pTarget->WorldSpaceCenter()) > (360.0f * 360.0f) )
	{
		// Too far away!
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::SetInteractTarget( CBaseEntity *pTarget )
{
	if( !pTarget )
	{
		ClearCondition( COND_ALYX_HAS_INTERACT_TARGET );
		ClearCondition( COND_ALYX_CAN_INTERACT_WITH_TARGET );

		SetCondition( COND_ALYX_NO_INTERACT_TARGET );
		SetCondition( COND_ALYX_CAN_NOT_INTERACT_WITH_TARGET );
	}

	m_hHackTarget.Set(pTarget);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Alyx::EmpZapTarget( CBaseEntity *pTarget )
{
	g_pEffects->Sparks( pTarget->WorldSpaceCenter() );

	CAlyxEmpEffect *pEmpEffect = (CAlyxEmpEffect*)CreateEntityByName( "env_alyxemp" );

	if( pEmpEffect )
	{
		pEmpEffect->Spawn();
		pEmpEffect->ActivateAutomatic( this, pTarget );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Alyx::CanInteractWithTarget( CBaseEntity *pTarget )
{
	if( !IsValidInteractTarget(pTarget) )
		return false;

	float flDist;

	flDist = (WorldSpaceCenter() - pTarget->WorldSpaceCenter()).Length();

	if( flDist > 80.0f )
	{
		return false;
	}

	if( !IsAllowedToInteract() )
	{
		SpeakIfAllowed( TLK_CANT_INTERACT_NOW );
		return false;
	}

	if( IsEMPHolstered() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Player has illuminated this NPC with the flashlight
//-----------------------------------------------------------------------------
void CNPC_Alyx::PlayerHasIlluminatedNPC( CBasePlayer *pPlayer, float flDot )
{
 	if ( m_bIsFlashlightBlind )
		return;

	if ( !CanBeBlindedByFlashlight( true ) )
		return;

	// Ignore the flashlight if it's not shining at my eyes
	if ( PlayerFlashlightOnMyEyes( pPlayer ) )
	{
		char szResponse[AI_Response::MAX_RESPONSE_NAME];

		// Only say the blinding speech if it's time to
		if ( SpeakIfAllowed( "TLK_FLASHLIGHT_ILLUM", NULL, false, szResponse, AI_Response::MAX_RESPONSE_NAME  ) )
		{
			m_iszCurrentBlindScene = AllocPooledString( szResponse );
			ADD_DEBUG_HISTORY( HISTORY_ALYX_BLIND, UTIL_VarArgs( "(%0.2f) Alyx: start flashlight blind scene '%s'\n", gpGlobals->curtime, STRING(m_iszCurrentBlindScene) ) );
			GetShotRegulator()->DisableShooting();
			m_bIsFlashlightBlind = true;
			m_fStayBlindUntil = gpGlobals->curtime + 0.1f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check if player has illuminated this NPC with a flare
//-----------------------------------------------------------------------------
void CNPC_Alyx::CheckBlindedByFlare( void )
{
	if ( m_bIsFlashlightBlind )
		return;

	if ( !CanBeBlindedByFlashlight( false ) )
		return;

	// Ignore the flare if it's not too close
	if ( BlindedByFlare() )
	{
		char szResponse[AI_Response::MAX_RESPONSE_NAME];

		// Only say the blinding speech if it's time to
		if ( SpeakIfAllowed( "TLK_FLASHLIGHT_ILLUM", NULL, false, szResponse, AI_Response::MAX_RESPONSE_NAME ) )
		{
			m_iszCurrentBlindScene = AllocPooledString( szResponse );
			ADD_DEBUG_HISTORY( HISTORY_ALYX_BLIND, UTIL_VarArgs( "(%0.2f) Alyx: start flare blind scene '%s'\n", gpGlobals->curtime, 
				STRING(m_iszCurrentBlindScene) ) );
			GetShotRegulator()->DisableShooting();
			m_bIsFlashlightBlind = true;
			m_fStayBlindUntil = gpGlobals->curtime + 0.1f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input:   bCheckLightSources - if true, checks if any light darkness lightsources are near
//-----------------------------------------------------------------------------
bool CNPC_Alyx::CanBeBlindedByFlashlight( bool bCheckLightSources )
{
	// Can't be blinded if we're not in alyx darkness mode
 	/*
	if ( !HL2GameRules()->IsAlyxInDarknessMode() )
		return false;
	*/

	// Can't be blinded if I'm in a script, or in combat
	if ( IsInAScript() || GetState() == NPC_STATE_COMBAT || GetState() == NPC_STATE_SCRIPT )
		return false;
	if ( IsSpeaking() )
		return false;

	// can't be blinded if Alyx is near a light source
	if ( bCheckLightSources && DarknessLightSourceWithinRadius( this, 500 ) )
		return false;

	// Not during an actbusy
	if ( m_ActBusyBehavior.IsActive() )
		return false;
	if ( m_OperatorBehavior.IsRunning() )
		return false;

	// Can't be blinded if I've been in combat recently, to fix anim snaps
	if ( GetLastEnemyTime() != 0.0 )
	{
		if ( (gpGlobals->curtime - GetLastEnemyTime()) < 2 )
			return false;
	}

	// Can't be blinded if I'm reloading
	if ( IsCurSchedule(SCHED_RELOAD, false) )
		return false;

	// Can't be blinded right after being blind, to prevent oscillation
	if ( gpGlobals->curtime < m_flDontBlindUntil )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Alyx::PlayerFlashlightOnMyEyes( CBasePlayer *pPlayer )
{
	Vector vecEyes, vecPlayerForward;
 	vecEyes = EyePosition();
 	pPlayer->EyeVectors( &vecPlayerForward );

	Vector vecToEyes = (vecEyes - pPlayer->EyePosition());
	float flDist = VectorNormalize( vecToEyes ); 

	// We can be blinded in daylight, but only at close range
	if ( HL2GameRules()->IsAlyxInDarknessMode() == false )
	{
		if ( flDist > (8*12.0f) )
			return false;
	}

	float flDot = DotProduct( vecPlayerForward, vecToEyes );
	if ( flDot < 0.98 )
		return false;

	// Check facing to ensure we're in front of her
 	Vector los = ( pPlayer->EyePosition() - vecEyes );
	los.z = 0;
	VectorNormalize( los );
	Vector facingDir = EyeDirection2D();
 	flDot = DotProduct( los, facingDir );
	return ( flDot > 0.3 );
}

//-----------------------------------------------------------------------------
// Purpose: Checks if Alyx is blinded by a flare
// Input  : 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Alyx::BlindedByFlare( void )
{
	Vector vecEyes = EyePosition();

	Vector los;
	Vector vecToEyes;
	Vector facingDir = EyeDirection2D();

	// use a wider radius when she's already blind to help with edge cases
	// where she flickers back and forth due to animation
	float fBlindDist = ( m_bIsFlashlightBlind ) ? 35.0f : 30.0f;

	CFlare *pFlare = CFlare::GetActiveFlares();
	while( pFlare != NULL )
	{
		vecToEyes = (vecEyes - pFlare->GetAbsOrigin());
		float fDist = VectorNormalize( vecToEyes ); 
		if ( fDist < fBlindDist )
		{
			// Check facing to ensure we're in front of her
			los = ( pFlare->GetAbsOrigin() - vecEyes );
			los.z = 0;
			VectorNormalize( los );
			float flDot = DotProduct( los, facingDir );
			if ( ( flDot > 0.3 ) && FVisible( pFlare ) )
			{
				return true;
			}
		}

		pFlare = pFlare->GetNextFlare();
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Alyx::CanReload( void )
{
	if ( m_bIsFlashlightBlind )
		return false;

	return BaseClass::CanReload();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Alyx::PickTacticalLookTarget( AILookTargetArgs_t *pArgs )
{
	if( HasInteractTarget() )
	{
		pArgs->hTarget = GetInteractTarget();
		pArgs->flInfluence = 0.8f;
		pArgs->flDuration = 3.0f;
		return true;
	}

	if( m_ActBusyBehavior.IsActive() && m_ActBusyBehavior.IsCombatActBusy() )
	{
		return false;
	}

	return BaseClass::PickTacticalLookTarget( pArgs );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::OnSelectedLookTarget( AILookTargetArgs_t *pArgs )
{ 
	if ( pArgs && pArgs->hTarget )
	{
		// If it's a stealth target, we want to go into stealth mode
		CAI_Hint *pHint = dynamic_cast<CAI_Hint *>(pArgs->hTarget.Get());
		if ( pHint && pHint->HintType() == HINT_WORLD_VISUALLY_INTERESTING_STEALTH )
		{
			SetReadinessLevel( AIRL_STEALTH, true, true );
			pArgs->flDuration = 9999999;
			m_hStealthLookTarget = pHint;
			return;
		}
	}

	// If we're in stealth mode, break out now
	if ( GetReadinessLevel() == AIRL_STEALTH )
	{
		SetReadinessLevel( AIRL_STIMULATED, true, true );
		if ( m_hStealthLookTarget )
		{
			ClearLookTarget( m_hStealthLookTarget );
			m_hStealthLookTarget = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Output : Behavior to use
//-----------------------------------------------------------------------------
CAI_FollowBehavior &CNPC_Alyx::GetFollowBehavior( void )
{
	// Use the base class
	return m_FollowBehavior;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::AimGun( void )
{
	if (m_FuncTankBehavior.IsMounted())
	{
		m_FuncTankBehavior.AimGun();
		return;
	}

	// Always allow the passenger behavior to handle this
	if ( m_PassengerBehavior.IsEnabled() )
	{
		m_PassengerBehavior.AimGun();
		return;
	}

	if( !GetEnemy() )
	{
		if ( GetReadinessLevel() == AIRL_STEALTH && m_hStealthLookTarget != NULL )
		{
			// Only aim if we're not far from the node
			Vector vecAimDir = m_hStealthLookTarget->GetAbsOrigin() - Weapon_ShootPosition();
			if ( VectorNormalize( vecAimDir ) > 80 )
			{
				// Ignore nodes that are behind her
				Vector vecForward;
				GetVectors( &vecForward, NULL, NULL );
				float flDot = DotProduct( vecAimDir, vecForward );
				if ( flDot > 0 )
				{
					SetAim( vecAimDir);
					return;
				}
			}
		}
	}

	BaseClass::AimGun();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CNPC_Alyx::GetActualShootPosition( const Vector &shootOrigin )
{
	if( HasShotgun() && GetEnemy() && GetEnemy()->Classify() == CLASS_ZOMBIE && random->RandomInt( 0, 1 ) == 1 )
	{
		// 50-50 zombie headshots with shotgun!
		return GetEnemy()->HeadTarget( shootOrigin );
	}

	return BaseClass::GetActualShootPosition( shootOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Alyx::EnemyIsValidCrouchTarget( CBaseEntity *pEnemy )
{
	// Don't crouch to shoot flying enemies (or jumping antlions)
	if ( !(pEnemy->GetFlags() & FL_ONGROUND) )
		return false;

	// Don't crouch to shoot if we couldn't see them while crouching
	if ( !CouldShootIfCrouching( pEnemy ) )
	{
		//Warning("CROUCH: Not valid due to crouch-no-LOS.\n" );
		return false;
	}

	// Don't crouch to shoot enemies that are close to me
	if ( EnemyDistance( pEnemy ) <= ALYX_MIN_ENEMY_DIST_TO_CROUCH )
	{
		//Warning("CROUCH: Not valid due to enemy-too-close.\n" );
		return false;
	}
	
	// Don't crouch to shoot enemies that are too far off my vertical plane
	if ( fabs( pEnemy->GetAbsOrigin().z - GetAbsOrigin().z ) > 64 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: degrees to turn in 0.1 seconds
//-----------------------------------------------------------------------------
float CNPC_Alyx::MaxYawSpeed( void )
{
	if ( IsCrouching() )
		return 10;

	return BaseClass::MaxYawSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Alyx::Stand( void )
{
	bool bWasCrouching = IsCrouching();
	if ( !BaseClass::Stand() )
		return false;

	if ( bWasCrouching )
	{
		m_flNextCrouchTime = gpGlobals->curtime + ALYX_CROUCH_DELAY;
		OnUpdateShotRegulator();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Alyx::Crouch( void )
{
	if ( !npc_alyx_crouch.GetBool() )
		return false;

	// Alyx will ignore crouch requests while she has the shotgun
	if ( HasShotgun() )
		return false;

	bool bWasStanding = !IsCrouching();
	if ( !BaseClass::Crouch() )
		return false;

	if ( bWasStanding )
	{
		OnUpdateShotRegulator();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::DesireCrouch( void )
{
	// Ignore crouch desire if we've been crouching recently to reduce oscillation
	if ( m_flNextCrouchTime > gpGlobals->curtime )
		return;

	BaseClass::DesireCrouch();
}


//-----------------------------------------------------------------------------
// Purpose: Tack on extra criteria for responses
//-----------------------------------------------------------------------------
void CNPC_Alyx::ModifyOrAppendCriteria( AI_CriteriaSet &set )
{
	AIEnemiesIter_t iter;
	float fLengthOfLastCombat;
	int	iNumEnemies;

	if ( GetState() == NPC_STATE_COMBAT )
	{
		fLengthOfLastCombat = gpGlobals->curtime - m_fCombatStartTime;
	}
	else
	{
		fLengthOfLastCombat = m_fCombatEndTime - m_fCombatStartTime;
	}
	
	set.AppendCriteria( "combat_length", UTIL_VarArgs( "%.3f", fLengthOfLastCombat ) );

	iNumEnemies = 0;
	for ( AI_EnemyInfo_t *pEMemory = GetEnemies()->GetFirst(&iter); pEMemory != NULL; pEMemory = GetEnemies()->GetNext(&iter) )
	{
		if ( pEMemory->hEnemy->IsAlive() && ( pEMemory->hEnemy->Classify() != CLASS_BULLSEYE ) )
		{
			iNumEnemies++;
		}
	}
	set.AppendCriteria( "num_enemies", UTIL_VarArgs( "%d", iNumEnemies ) );
	set.AppendCriteria( "darkness_mode", UTIL_VarArgs( "%d", HasCondition( COND_ALYX_IN_DARK ) ) );
	set.AppendCriteria( "water_level", UTIL_VarArgs( "%d", GetWaterLevel() ) );

	CHL2_Player *pPlayer = assert_cast<CHL2_Player*>( UTIL_PlayerByIndex( 1 ) );
	set.AppendCriteria( "num_companions", UTIL_VarArgs( "%d", pPlayer ? pPlayer->GetNumSquadCommandables() : 0 ) );
	set.AppendCriteria( "flashlight_on", UTIL_VarArgs( "%d", pPlayer ? pPlayer->FlashlightIsOn() : 0 ) );

	BaseClass::ModifyOrAppendCriteria( set );
}

//-----------------------------------------------------------------------------
// Purpose: Turn off Alyx's readiness when she's around a vehicle
//-----------------------------------------------------------------------------
bool CNPC_Alyx::IsReadinessCapable( void )
{
	// Let the convar decide
	return npc_alyx_readiness.GetBool();;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Alyx::IsAllowedToInteract()
{
	if ( RunningPassengerBehavior() )
		return false;

	if( IsInAScript() )
		return false;

	if( IsCurSchedule(SCHED_SCENE_GENERIC) )
		return false;

	if( GetEnemy() )
	{
		if( GetEnemy()->GetAbsOrigin().DistTo( GetAbsOrigin() ) <= 240.0f )
		{
			// Enemy is nearby!
			return false;
		}
	}
	
	return m_bInteractionAllowed;
}

//-----------------------------------------------------------------------------
// Purpose: Allows the NPC to react to being given a weapon
// Input  : *pNewWeapon - Weapon given
//-----------------------------------------------------------------------------
void CNPC_Alyx::OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	m_WeaponType = ComputeWeaponType();
	BaseClass::OnChangeActiveWeapon( pOldWeapon, pNewWeapon );
}


//-----------------------------------------------------------------------------
// Purpose: Allows the NPC to react to being given a weapon
// Input  : *pNewWeapon - Weapon given
//-----------------------------------------------------------------------------
void CNPC_Alyx::OnGivenWeapon( CBaseCombatWeapon *pNewWeapon )
{
	// HACK: This causes Alyx to pull her gun from a holstered position
	if ( pNewWeapon->ClassMatches( CLASSNAME_ALYXGUN ) )
	{
		// Put it away so we can pull it out properly
		GetActiveWeapon()->Holster();
		SetActiveWeapon( NULL );

		// Draw the weapon when we're next able to
		SetDesiredWeaponState( DESIREDWEAPONSTATE_UNHOLSTERED );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Alyx::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	m_WeaponType = ComputeWeaponType( pWeapon );
	BaseClass::Weapon_Equip( pWeapon );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Alyx::Weapon_CanUse( CBaseCombatWeapon *pWeapon )
{
	if( !pWeapon->ClassMatches( CLASSNAME_SHOTGUN ) )
		return false;

	return BaseClass::Weapon_CanUse( pWeapon );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CNPC_Alyx::OnUpdateShotRegulator( )
{
	BaseClass::OnUpdateShotRegulator();

	if ( !HasShotgun() && IsCrouching() )
	{
		// While crouching, Alyx fires longer bursts
		int iMinBurst, iMaxBurst;
		GetShotRegulator()->GetBurstShotCountRange( &iMinBurst, &iMaxBurst );
		GetShotRegulator()->SetBurstShotCountRange( iMinBurst * 2, iMaxBurst * 2 );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Alyx::BarnacleDeathSound( void )
{
	Speak( TLK_SELF_IN_BARNACLE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : PassengerState_e
//-----------------------------------------------------------------------------
PassengerState_e CNPC_Alyx::GetPassengerState( void )
{
	return m_PassengerBehavior.GetPassengerState();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Alyx::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// if I'm in the vehicle, the player is probably trying to use the vehicle
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE && pActivator->IsPlayer() && GetParent() )
	{
		GetParent()->Use( pActivator, pCaller, useType, value );
		return;
	}
	m_bDontUseSemaphore = true;
	SpeakIfAllowed( TLK_USE );
	m_bDontUseSemaphore = false;

	m_OnPlayerUse.FireOutput( pActivator, pCaller );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Alyx::PlayerInSpread( const Vector &sourcePos, const Vector &targetPos, float flSpread, float maxDistOffCenter, bool ignoreHatedPlayers )
{
	// loop through all players
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer && ( !ignoreHatedPlayers || IRelationType( pPlayer ) != D_HT ) )
		{
			//If the player is being lifted by a barnacle then go ahead and ignore the player and shoot.
#ifdef HL2_EPISODIC
			if ( pPlayer->IsEFlagSet( EFL_IS_BEING_LIFTED_BY_BARNACLE ) )
				return false;
#endif

			if ( PointInSpread( pPlayer, sourcePos, targetPos, pPlayer->WorldSpaceCenter(), flSpread, maxDistOffCenter ) )
				return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Alyx::IsCrouchedActivity( Activity activity )
{
	Activity realActivity = TranslateActivity(activity);

	switch ( realActivity )
	{
	case ACT_RELOAD_LOW:
	case ACT_COVER_LOW:
	case ACT_COVER_PISTOL_LOW:
	case ACT_COVER_SMG1_LOW:
	case ACT_RELOAD_SMG1_LOW:

		// Aren't these supposed to be a little higher than the above?
	case ACT_RANGE_ATTACK1_LOW:
	case ACT_RANGE_ATTACK2_LOW:
	case ACT_RANGE_ATTACK_AR2_LOW:
	case ACT_RANGE_ATTACK_SMG1_LOW:
	case ACT_RANGE_ATTACK_SHOTGUN_LOW:
	case ACT_RANGE_ATTACK_PISTOL_LOW:
	case ACT_RANGE_AIM_LOW:
	case ACT_RANGE_AIM_SMG1_LOW:
	case ACT_RANGE_AIM_PISTOL_LOW:
	case ACT_RANGE_AIM_AR2_LOW:
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Alyx::OnBeginMoveAndShoot()
{
	if ( BaseClass::OnBeginMoveAndShoot() )
	{
		SpeakAttacking();
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Alyx::SpeakAttacking( void )
{
	if ( GetActiveWeapon() && m_AnnounceAttackTimer.Expired() )
	{
		SpeakIfAllowed( TLK_ATTACKING, UTIL_VarArgs("attacking_with_weapon:%s", GetActiveWeapon()->GetClassname()) );
		m_AnnounceAttackTimer.Set( 3, 5 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *lpszInteractionName - 
//			*pOther - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Alyx::ForceVehicleInteraction( const char *lpszInteractionName, CBaseCombatCharacter *pOther )
{
	return m_PassengerBehavior.ForceVehicleInteraction( lpszInteractionName, pOther );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CNPC_Alyx::WeaponType_t CNPC_Alyx::ComputeWeaponType( CBaseEntity *pWeapon )
{
	if ( !pWeapon )
	{
		pWeapon = GetActiveWeapon();
	}

	if ( !pWeapon )
	{
		return WT_NONE;
	}

	if ( pWeapon->ClassMatches( CLASSNAME_ALYXGUN ) )
	{
		return WT_ALYXGUN;
	}

	if ( pWeapon->ClassMatches( CLASSNAME_SMG1 ) )
	{
		return WT_SMG1;
	}

	if ( pWeapon->ClassMatches( CLASSNAME_SHOTGUN ) )
	{
		return WT_SHOTGUN;
	}

	if ( pWeapon->ClassMatches( CLASSNAME_AR2 ) )
	{
		return WT_AR2;
	}

	return WT_OTHER;
}

//-----------------------------------------------------------------------------
// Purpose: Complain about being punted
//-----------------------------------------------------------------------------
void CNPC_Alyx::InputVehiclePunted( inputdata_t &inputdata )
{
	// If we're in a vehicle, complain about being punted
	if ( IsInAVehicle() && GetVehicleEntity() == inputdata.pCaller )
	{
		// FIXME: Pass this up into the behavior?
		SpeakIfAllowed( TLK_PASSENGER_PUNTED );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Alyx::InputOutsideTransition( inputdata_t &inputdata )
{
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer && pPlayer->IsInAVehicle() )
	{
		if ( ShouldAlwaysTransition() == false )
			return;

		// Enter immediately
		EnterVehicle( pPlayer->GetVehicleEntity(), true );
		return;
	}

	// If the player is in the vehicle and we're not, then we need to enter the vehicle immediately
	BaseClass::InputOutsideTransition( inputdata );
}

//=========================================================
// AI Schedules Specific to this NPC
//=========================================================

AI_BEGIN_CUSTOM_NPC( npc_alyx, CNPC_Alyx )

	DECLARE_TASK( TASK_ALYX_BEGIN_INTERACTION )
	DECLARE_TASK( TASK_ALYX_COMPLETE_INTERACTION )
	DECLARE_TASK( TASK_ALYX_ANNOUNCE_HACK )
	DECLARE_TASK( TASK_ALYX_GET_PATH_TO_INTERACT_TARGET )
	DECLARE_TASK( TASK_ALYX_WAIT_HACKING )
	DECLARE_TASK( TASK_ALYX_DRAW_PISTOL )
	DECLARE_TASK( TASK_ALYX_HOLSTER_PISTOL )
	DECLARE_TASK( TASK_ALYX_HOLSTER_AND_DESTROY_PISTOL )
	DECLARE_TASK( TASK_ALYX_BUILD_COMBAT_FACE_PATH )
	DECLARE_TASK( TASK_ALYX_SET_IDLE_ACTIVITY )
	DECLARE_TASK( TASK_ALYX_FALL_TO_GROUND )

	DECLARE_ANIMEVENT( AE_ALYX_EMPTOOL_ATTACHMENT )
	DECLARE_ANIMEVENT( AE_ALYX_EMPTOOL_SEQUENCE )
	DECLARE_ANIMEVENT( AE_ALYX_EMPTOOL_USE )
	DECLARE_ANIMEVENT( COMBINE_AE_BEGIN_ALTFIRE )
	DECLARE_ANIMEVENT( COMBINE_AE_ALTFIRE )

	DECLARE_CONDITION( COND_ALYX_HAS_INTERACT_TARGET )
	DECLARE_CONDITION( COND_ALYX_NO_INTERACT_TARGET )
	DECLARE_CONDITION( COND_ALYX_CAN_INTERACT_WITH_TARGET )
	DECLARE_CONDITION( COND_ALYX_CAN_NOT_INTERACT_WITH_TARGET )
	DECLARE_CONDITION( COND_ALYX_PLAYER_TURNED_ON_FLASHLIGHT )
	DECLARE_CONDITION( COND_ALYX_PLAYER_TURNED_OFF_FLASHLIGHT )
	DECLARE_CONDITION( COND_ALYX_PLAYER_FLASHLIGHT_EXPIRED )
	DECLARE_CONDITION( COND_ALYX_IN_DARK )

	DECLARE_ACTIVITY( ACT_ALYX_DRAW_TOOL )
	DECLARE_ACTIVITY( ACT_ALYX_IDLE_TOOL )
	DECLARE_ACTIVITY( ACT_ALYX_ZAP_TOOL )
	DECLARE_ACTIVITY( ACT_ALYX_HOLSTER_TOOL )
	DECLARE_ACTIVITY( ACT_ALYX_PICKUP_RACK )

	DEFINE_SCHEDULE
		(
			SCHED_ALYX_PREPARE_TO_INTERACT_WITH_TARGET,

			"	Tasks"
			"		TASK_STOP_MOVING						0"
			"		TASK_PLAY_SEQUENCE						ACTIVITY:ACT_ALYX_DRAW_TOOL"
			"		TASK_SET_ACTIVITY						ACTIVITY:ACT_ALYX_IDLE_TOOL"
			"		TASK_FACE_PLAYER						0"
			""
			"	Interrupts"
			""
		)

		DEFINE_SCHEDULE
		(
			SCHED_ALYX_WAIT_TO_INTERACT_WITH_TARGET,
			"	Tasks"
			"		TASK_STOP_MOVING						0"
			"		TASK_ALYX_ANNOUNCE_HACK					0"
			"		TASK_FACE_PLAYER						0"
			"		TASK_SET_ACTIVITY						ACTIVITY:ACT_ALYX_IDLE_TOOL"
			"		TASK_WAIT								2"
			""
			"	Interrupts"
			"		COND_ALYX_CAN_INTERACT_WITH_TARGET"
			"		COND_ALYX_NO_INTERACT_TARGET"
			"		COND_LIGHT_DAMAGE"
			"		COND_HEAVY_DAMAGE"
		)

		DEFINE_SCHEDULE
		(
			SCHED_ALYX_INTERACT_WITH_TARGET,

			"	Tasks"
			"		TASK_STOP_MOVING						0"
			"		TASK_FACE_PLAYER						0"
			"		TASK_ALYX_BEGIN_INTERACTION				0"
			"		TASK_PLAY_SEQUENCE						ACTIVITY:ACT_ALYX_ZAP_TOOL"
			"		TASK_SET_SCHEDULE						SCHEDULE:SCHED_ALYX_FINISH_INTERACTING_WITH_TARGET"
			""
			"	Interrupts"
			"		COND_ALYX_NO_INTERACT_TARGET"
			"		COND_ALYX_CAN_NOT_INTERACT_WITH_TARGET"
		)

		DEFINE_SCHEDULE
		(
			SCHED_ALYX_FINISH_INTERACTING_WITH_TARGET,

			"	Tasks"
			"		TASK_ALYX_COMPLETE_INTERACTION			0"
			"		TASK_PLAY_SEQUENCE						ACTIVITY:ACT_ALYX_HOLSTER_TOOL"
			""
			"	Interrupts"
			""
		)

		DEFINE_SCHEDULE
		(
			SCHED_ALYX_HOLSTER_EMP,

			"	Tasks"
			"		TASK_STOP_MOVING						0"
			"		TASK_PLAY_SEQUENCE						ACTIVITY:ACT_ALYX_HOLSTER_TOOL"
			"		TASK_ALYX_DRAW_PISTOL					0"
			""
			"	Interrupts"
			""
		)

		DEFINE_SCHEDULE
		(
			SCHED_ALYX_INTERACTION_INTERRUPTED,
			"	Tasks"
			"		TASK_STOP_MOVING						0"
			"		TASK_SET_ACTIVITY						ACTIVITY:ACT_IDLE"
			"		TASK_FACE_PLAYER						0"
			"		TASK_WAIT								2"
			""
			"	Interrupts"
		)

		DEFINE_SCHEDULE
		(
			SCHED_ALYX_ALERT_FACE_AWAYFROM_BESTSOUND,
			"	Tasks"
			"		TASK_STORE_BESTSOUND_REACTORIGIN_IN_SAVEPOSITION		0"
			"		TASK_STOP_MOVING					0"
			"		TASK_FACE_AWAY_FROM_SAVEPOSITION	0"
			"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
			"		TASK_WAIT							10.0"
			"		TASK_FACE_REASONABLE				0"
			""
			"	Interrupts"
			"		COND_NEW_ENEMY"
			"		COND_SEE_FEAR"
			"		COND_LIGHT_DAMAGE"
			"		COND_HEAVY_DAMAGE"
			"		COND_PROVOKED"
		)

		//===============================================
		//	> RangeAttack1
		//===============================================
		DEFINE_SCHEDULE
		(
			SCHED_ALYX_RANGE_ATTACK1,

			"	Tasks"
			"		TASK_STOP_MOVING		0"
			"		TASK_FACE_ENEMY			0"
			"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
			"		TASK_RANGE_ATTACK1		0"
			""
			"	Interrupts"
			"		COND_ENEMY_WENT_NULL"
			"		COND_HEAVY_DAMAGE"
			"		COND_ENEMY_OCCLUDED"
			"		COND_NO_PRIMARY_AMMO"
			"		COND_HEAR_DANGER"
			"		COND_WEAPON_BLOCKED_BY_FRIEND"
			"		COND_WEAPON_SIGHT_OCCLUDED"
		)

		//===============================================
		// > SCHED_ALYX_ALERT_REACT_TO_COMBAT_SOUND
		//===============================================
		DEFINE_SCHEDULE
		(
			SCHED_ALYX_ALERT_REACT_TO_COMBAT_SOUND,

			"	Tasks"
			"		TASK_REACT_TO_COMBAT_SOUND		0"
			"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ALERT_FACE_BESTSOUND"
			""
			"	Interrupts"
			"		COND_NEW_ENEMY"
		)

		//=========================================================
		// > SCHED_ALYX_COMBAT_FACE
		//=========================================================
		DEFINE_SCHEDULE
		(
			SCHED_ALYX_COMBAT_FACE,

			"	Tasks"
			"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
			"		TASK_STOP_MOVING					0"
			"		TASK_ALYX_BUILD_COMBAT_FACE_PATH	0"
			"		TASK_RUN_PATH						0"
			"		TASK_FACE_IDEAL						0"
			"		TASK_WAIT_FOR_MOVEMENT				0"
			""
			"	Interrupts"
			"		COND_CAN_RANGE_ATTACK1"
			"		COND_CAN_RANGE_ATTACK2"
			"		COND_CAN_MELEE_ATTACK1"
			"		COND_CAN_MELEE_ATTACK2"
			"		COND_NEW_ENEMY"
			"		COND_ENEMY_DEAD"
		)

		//=========================================================
		//	> SCHED_ALYX_WAKE_ANGRY
		//=========================================================
		DEFINE_SCHEDULE
		(
			SCHED_ALYX_WAKE_ANGRY,

			"	Tasks"
			"		TASK_STOP_MOVING		0"
			"		TASK_SOUND_WAKE			0"
			""
			"	Interrupts"
		)

		//===============================================
		//	> NewWeapon
		//===============================================
		DEFINE_SCHEDULE
		(
			SCHED_ALYX_NEW_WEAPON,

			"	Tasks"
			"		TASK_STOP_MOVING						0"
			"		TASK_SET_TOLERANCE_DISTANCE				5"
			"		TASK_GET_PATH_TO_TARGET_WEAPON			0"
			"		TASK_WEAPON_RUN_PATH					0"
			"		TASK_STOP_MOVING						0"
			"		TASK_ALYX_HOLSTER_AND_DESTROY_PISTOL	0"
			"		TASK_FACE_TARGET						0"
			"		TASK_WEAPON_PICKUP						0"
			"		TASK_WAIT								1"// Don't move before done standing up
			""	
			"	Interrupts"
		)

		//===============================================
		//	> Alyx_Idle_Stand
		//===============================================
		DEFINE_SCHEDULE
		(
			SCHED_ALYX_IDLE_STAND,

			"	Tasks"
			"		TASK_STOP_MOVING		0"
			"		TASK_ALYX_SET_IDLE_ACTIVITY ACTIVITY:ACT_IDLE"
			"		TASK_WAIT				5"
			"		TASK_WAIT_PVS			0"
			""
			"	Interrupts"
			"		COND_NEW_ENEMY"
			"		COND_SEE_FEAR"
			"		COND_LIGHT_DAMAGE"
			"		COND_HEAVY_DAMAGE"
			"		COND_SMELL"
			"		COND_PROVOKED"
			"		COND_GIVE_WAY"
			"		COND_HEAR_PLAYER"
			"		COND_HEAR_DANGER"
			"		COND_HEAR_COMBAT"
			"		COND_HEAR_BULLET_IMPACT"
			"		COND_IDLE_INTERRUPT"
		)

		//===============================================
		//	Makes Alyx die if she falls too long
		//===============================================
		DEFINE_SCHEDULE
		(
			SCHED_ALYX_FALL_TO_GROUND,

			"	Tasks"
			"		TASK_ALYX_FALL_TO_GROUND		0"
			""
			"	Interrupts"
		)

		DEFINE_SCHEDULE
		(
			SCHED_ALYX_ALERT_FACE_BESTSOUND,

			"	Tasks"
			"		TASK_STORE_BESTSOUND_REACTORIGIN_IN_SAVEPOSITION		0"
			"		TASK_STOP_MOVING			0"
			"		TASK_FACE_SAVEPOSITION		0"
			""
			"	Interrupts"
			"		COND_NEW_ENEMY"
			"		COND_SEE_FEAR"
			"		COND_LIGHT_DAMAGE"
			"		COND_HEAVY_DAMAGE"
			"		COND_PROVOKED"
		);

AI_END_CUSTOM_NPC()
