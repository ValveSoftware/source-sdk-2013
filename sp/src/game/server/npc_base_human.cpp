#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "entitylist.h"
#include "activitylist.h"
#include "ai_basenpc.h"

#include "ai_basehumanoid.h"
#include "ai_sentence.h"
#include "ai_baseactor.h"

#include "engine/IEngineSound.h"


#include "tier0/memdbgon.h"

//=========================================================
// Private activities
//=========================================================
//int	ACT_MYCUSTOMACTIVITY = -1;
//Activity ACT_RUN_RIFLE;
Activity ACT_WALK_UNARMED;

//=========================================================
// Custom schedules
//=========================================================
enum
{
	//SCHED_MYCUSTOMSCHEDULE = LAST_SHARED_SCHEDULE,
};

//=========================================================
// Custom tasks
//=========================================================
enum 
{
	//TASK_MYCUSTOMTASK = LAST_SHARED_TASK,
};


//=========================================================
// Custom Conditions
//=========================================================
enum 
{
	//COND_MYCUSTOMCONDITION = LAST_SHARED_CONDITION,
};


//=========================================================
//=========================================================
class CNPCHuman : public CAI_BaseHumanoid
{
	DECLARE_CLASS( CNPCHuman, CAI_BaseHumanoid );

public:
	void	Precache( void );
	void	Spawn( void );
	Class_T	Classify( void );
	int		SelectSchedule( void );
	void	StartTask( const Task_t *pTask );
};

LINK_ENTITY_TO_CLASS( npc_hn_human, CNPCHuman );

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPCHuman::Precache( void )
{
	PrecacheModel( "models/humans/marine.mdl" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPCHuman::Spawn( void )
{
	Precache();

	SetModel( "models/humans/marine.mdl" );
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	SetFlexWeight("cheek_depth",RandomFloat(0,1));
	SetFlexWeight("cheek_fat_max",RandomFloat(0,1));
	SetFlexWeight("cheek_fat_min",RandomFloat(0,0.5));
	SetFlexWeight("chin_butt",RandomFloat(0,1));
	SetFlexWeight("chin_width",RandomFloat(0,1));
	SetFlexWeight("face_d_min",RandomFloat(0,1));
	SetFlexWeight("face_d_max",RandomFloat(0,1));
	SetFlexWeight("head_height",RandomFloat(0,1));
	SetFlexWeight("head_w_min",RandomFloat(0,1));
	SetFlexWeight("head_w_max",RandomFloat(0,1));
	SetFlexWeight("neck_size",RandomFloat(0,1));
	SetFlexWeight("ears_angle",RandomFloat(0,1));
	SetFlexWeight("ears_height",RandomFloat(0,1));
	SetFlexWeight("eyes_ang_min",RandomFloat(0,1));
	SetFlexWeight("eyes_ang_max",RandomFloat(0,1));
	SetFlexWeight("eyes_height",RandomFloat(0,1));
	SetFlexWeight("jaw_depth",RandomFloat(0,1));
	SetFlexWeight("lowlip_size",RandomFloat(0,1));
	SetFlexWeight("uplip_size",RandomFloat(0,1));
	SetFlexWeight("mouth_w_min",RandomFloat(0,1));
	SetFlexWeight("mouth_w_max",RandomFloat(0,1));
	SetFlexWeight("mouth_h_min",RandomFloat(0,1));
	SetFlexWeight("mouth_h_max",RandomFloat(0,1));
	SetFlexWeight("mouth_depth",RandomFloat(0,1));
	SetFlexWeight("nose_w_min",RandomFloat(0,1));
	SetFlexWeight("nose_w_max",RandomFloat(0,1));
	SetFlexWeight("nose_h_min",RandomFloat(0,1));
	SetFlexWeight("nose_h_max",RandomFloat(0,1));
	SetFlexWeight("nose_angle",RandomFloat(0,1));
	SetFlexWeight("nose_d_max",RandomFloat(0,1));
	SetFlexWeight("nost_height",RandomFloat(0,1));
	SetFlexWeight("nost_width",RandomFloat(0,1));
	SetFlexWeight("nose_tip",RandomFloat(0,1));
	SetFlexWeight("hairline_puff",RandomFloat(0,1));

	if (RandomInt(0,5)==4) {
		SetBodygroup( 2, 1 );
		SetFlexWeight("cigar_mouth",1);
	}
	else {
		SetBodygroup( 2, 0 );
		SetFlexWeight("cigar_mouth",0);
	}
	m_nSkin=RandomInt(0,14);
	SetBodygroup( 3, RandomInt(0,1) );
	//SetBodygroup( 4, RandomInt(0,1) );
	m_iHealth			= 40;
	m_flFieldOfView		= 0.5;
	m_NPCState			= NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND );
	CapabilitiesAdd( bits_CAP_MOVE_JUMP );
	CapabilitiesAdd( bits_CAP_MOVE_GROUND );
	CapabilitiesAdd( bits_CAP_MOVE_CLIMB );
	CapabilitiesAdd( bits_CAP_MOVE_CRAWL );
	CapabilitiesAdd( bits_CAP_DUCK );
	
	CapabilitiesAdd( bits_CAP_USE_WEAPONS );
	CapabilitiesAdd( bits_CAP_RANGE_ATTACK_GROUP );
	CapabilitiesAdd( bits_CAP_MELEE_ATTACK_GROUP );
	CapabilitiesAdd( bits_CAP_INNATE_MELEE_ATTACK1 );
	CapabilitiesAdd( bits_CAP_MOVE_SHOOT );

	CapabilitiesAdd( bits_CAP_NO_HIT_SQUADMATES );
	CapabilitiesAdd( bits_CAP_SQUAD );

	CapabilitiesAdd( bits_CAP_USE );
	CapabilitiesAdd( bits_CAP_DOORS_GROUP );

	CapabilitiesAdd( bits_CAP_TURN_HEAD );
	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );

	CapabilitiesAdd( bits_CAP_AIM_GUN );
	CapabilitiesAdd( bits_CAP_MOVE_GROUND );

	CBaseCombatWeapon *pWeapon = Weapon_Create( "weapon_shotgun" );
	pWeapon->SetName( AllocPooledString(UTIL_VarArgs("%s_weapon", STRING(GetEntityName()))) );
	pWeapon->AddEffects( EF_NOSHADOW );
	Weapon_Equip( pWeapon );

	NPCInit();
}

void CNPCHuman::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask) {
		case TASK_RUN_PATH:
			GetNavigator()->SetMovementActivity(ACT_RUN_RIFLE);
			break;
		case TASK_WALK_PATH:
			GetNavigator()->SetMovementActivity(ACT_WALK_UNARMED);
			break;
		default:
			BaseClass::StartTask(pTask);
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPCHuman::Classify( void )
{
	return	CLASS_MILITARY;
}

int CNPCHuman::SelectSchedule( void )
{
	if ( HasCondition( COND_HEAR_PHYSICS_DANGER ) )
	{
		return SCHED_FLINCH_PHYSICS;
	}

	// grunts place HIGH priority on running away from danger sounds.
	if ( HasCondition(COND_HEAR_DANGER) )
	{
		CSound *pSound;
		pSound = GetBestSound();

		Assert( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & SOUND_DANGER)
			{
				// I hear something dangerous, probably need to take cover.
				// dangerous sound nearby!, call it out
				/*const char *pSentenceName = "COMBINE_DANGER";

				CBaseEntity *pSoundOwner = pSound->m_hOwner;
				if ( pSoundOwner )
				{
					CBaseGrenade *pGrenade = dynamic_cast<CBaseGrenade *>(pSoundOwner);
					if ( pGrenade && pGrenade->GetThrower() )
					{
						if ( IRelationType( pGrenade->GetThrower() ) != D_LI )
						{
							// special case call out for enemy grenades
							pSentenceName = "COMBINE_GREN";
						}
					}
				}*/

				//m_Sentences.Speak( pSentenceName, SENTENCE_PRIORITY_NORMAL, SENTENCE_CRITERIA_NORMAL );

				// If the sound is approaching danger, I have no enemy, and I don't see it, turn to face.
				if( !GetEnemy() && pSound->IsSoundType(SOUND_CONTEXT_DANGER_APPROACH) && pSound->m_hOwner && !FInViewCone(pSound->GetSoundReactOrigin()) )
				{
					GetMotor()->SetIdealYawToTarget( pSound->GetSoundReactOrigin() );
					return SCHED_ALERT_FACE_BESTSOUND;
				}

				return SCHED_TAKE_COVER_FROM_BEST_SOUND;
			}
		}
	}

	switch	( m_NPCState )
	{
		case NPC_STATE_IDLE:
			return SCHED_IDLE_STAND;
			break;
		case NPC_STATE_COMBAT:
			if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
				return SCHED_MELEE_ATTACK1;

			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
				return SCHED_RANGE_ATTACK1;

			return SCHED_CHASE_ENEMY;
			break;
	}
	return SCHED_FAIL;
}