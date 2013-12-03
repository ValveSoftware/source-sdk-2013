//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "AI_Default.h"
#include "AI_Senses.h"
#include "ai_node.h"	  // for hint defintions
#include "ai_network.h"
#include "AI_Hint.h"
#include "ai_squad.h"
#include "beam_shared.h"
#include "globalstate.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "entitylist.h"
#include "npc_citizen17.h"
#include "scriptedtarget.h"
#include "ai_interactions.h"
#include "spotlightend.h"
#include "beam_flags.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	SPOTLIGHT_SWING_FORWARD		1
#define	SPOTLIGHT_SWING_BACK		-1

//------------------------------------
// Spawnflags
//------------------------------------
#define SF_SPOTLIGHT_START_TRACK_ON		(1 << 16)
#define SF_SPOTLIGHT_START_LIGHT_ON		(1 << 17)
#define SF_SPOTLIGHT_NO_DYNAMIC_LIGHT	(1 << 18)
#define SF_SPOTLIGHT_NEVER_MOVE			(1 << 19)


//-----------------------------------------------------------------------------
// Parameters for how the spotlight behaves
//-----------------------------------------------------------------------------
#define SPOTLIGHT_ENTITY_INSPECT_LENGTH	15		// How long does the inspection last
#define SPOTLIGHT_HINT_INSPECT_LENGTH	15		// How long does the inspection last
#define SPOTLIGHT_SOUND_INSPECT_LENGTH	 1		// How long does the inspection last

#define SPOTLIGHT_HINT_INSPECT_DELAY	20		// Check for hint nodes this often
#define SPOTLIGHT_ENTITY_INSPECT_DELAY		1		// Check for citizens this often

#define SPOTLIGHT_HINT_SEARCH_DIST		1000	// How far from self do I look for hints?
#define SPOTLIGHT_ENTITY_SEARCH_DIST	100		// How far from spotlight do I look for entities?
#define SPOTLIGHT_ACTIVE_SEARCH_DIST	200		// How far from spotlight do I look when already have entity

#define	SPOTLIGHT_BURN_TARGET_THRESH	60		// How close need to get to burn target
#define	SPOTLIGHT_MAX_SPEED_SCALE		2	

//#define SPOTLIGHT_DEBUG


// -----------------------------------
//  Spotlight flags
// -----------------------------------
enum SpotlightFlags_t
{
	BITS_SPOTLIGHT_LIGHT_ON			= 0x00000001,	// Light is on
	BITS_SPOTLIGHT_TRACK_ON			= 0x00000002,	// Tracking targets / scanning
	BITS_SPOTLIGHT_SMOOTH_RETURN	= 0x00001000,	// If out of range, don't pop back to position
};


class CBeam;


class CNPC_Spotlight : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Spotlight, CAI_BaseNPC );

	public:
		CNPC_Spotlight();
		Class_T			Classify(void);
		int 			UpdateTransmitState(void);
		void			Event_Killed( const CTakeDamageInfo &info );
		int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
		int				GetSoundInterests( void );

		bool			FValidateHintType(CAI_Hint *pHint);

		Disposition_t	IRelationType(CBaseEntity *pTarget);
		float			HearingSensitivity( void ) { return 4.0; };

		void			NPCThink(void);
		bool			HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);

		void			UpdateTargets(void);
		void			Precache(void);
		void			Spawn(void);

	public:

		int					m_fSpotlightFlags;

		// ------------------------------
		//	Scripted Spotlight Motion
		// ------------------------------
		CScriptedTarget*	m_pScriptedTarget;		// My current scripted target
		void				SetScriptedTarget( CScriptedTarget *pScriptedTarget );

		// ------------------------------
		//	Inspecting
		// ------------------------------
		Vector				m_vInspectPos;
		float				m_flInspectEndTime;
		float				m_flNextEntitySearchTime;	
		float				m_flNextHintSearchTime;		// Time to look for hints to inspect

		void				SetInspectTargetToEntity(CBaseEntity *pEntity, float fInspectDuration);
		void				SetInspectTargetToEnemy(CBaseEntity *pEntity);
		void				SetInspectTargetToPos(const Vector &vInspectPos, float fInspectDuration);
		void				SetInspectTargetToHint(CAI_Hint *pHint, float fInspectDuration);
		void				ClearInspectTarget(void);
		bool				HaveInspectTarget(void);
		Vector				InspectTargetPosition(void);
		CBaseEntity*		BestInspectTarget(void);
		void				RequestInspectSupport(void);

		// -------------------------------
		//  Outputs
		// -------------------------------
		bool				m_bHadEnemy;			// Had an enemy
		COutputEvent		m_pOutputAlert;			// Alerted by sound
		COutputEHANDLE		m_pOutputDetect;		// Found enemy, output it's name
		COutputEHANDLE		m_pOutputLost;			// Lost enemy
		COutputEHANDLE		m_pOutputSquadDetect;	// Squad Found enemy
		COutputEHANDLE		m_pOutputSquadLost;		// Squad Lost enemy
		COutputVector		m_pOutputPosition;		// End position of spotlight beam

		// ------------------------------
		//	Spotlight
		// ------------------------------
		float				m_flYaw;
		float				m_flYawCenter;
		float				m_flYawRange;		// +/- around center
		float				m_flYawSpeed;
		float				m_flYawDir;

		float				m_flPitch;
		float				m_flPitchCenter;	
		float				m_flPitchMin;
		float				m_flPitchMax;
		float				m_flPitchSpeed;
		float				m_flPitchDir;

		float				m_flIdleSpeed;			// Speed when no enemy
		float				m_flAlertSpeed;			// Speed when found enemy

		Vector				m_vSpotlightTargetPos;
		Vector				m_vSpotlightCurrentPos;
		CBeam*				m_pSpotlight;
		CSpotlightEnd*		m_pSpotlightTarget;
		Vector				m_vSpotlightDir;
		int					m_nHaloSprite;
		
		float				m_flSpotlightMaxLength;
		float				m_flSpotlightCurLength;
		float				m_flSpotlightGoalWidth;

		void				SpotlightUpdate(void);
		Vector				SpotlightCurrentPos(void);
		void				SpotlightSetTargetYawAndPitch(void);
		float				SpotlightSpeed(void);
		void				SpotlightCreate(void);
		void				SpotlightDestroy(void);
		bool				SpotlightIsPositionLegal(const Vector &vTestPos);

		// ------------------------------
		//  Inputs
		// ------------------------------
		void InputLightOn( inputdata_t &inputdata );
		void InputLightOff( inputdata_t &inputdata );
		void InputTrackOn( inputdata_t &inputdata );
		void InputTrackOff( inputdata_t &inputdata );

	protected:

		DECLARE_DATADESC();
};


BEGIN_DATADESC( CNPC_Spotlight )
	DEFINE_FIELD( m_vInspectPos, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flYaw, FIELD_FLOAT ),
	DEFINE_FIELD( m_flYawCenter, FIELD_FLOAT ),
	DEFINE_FIELD( m_flYawSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_flYawDir, FIELD_FLOAT ),
	DEFINE_FIELD( m_flPitch, FIELD_FLOAT ),
	DEFINE_FIELD( m_flPitchCenter, FIELD_FLOAT ),
	DEFINE_FIELD( m_flPitchSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_flPitchDir, FIELD_FLOAT ),
	DEFINE_FIELD( m_flSpotlightCurLength, FIELD_FLOAT ),

	DEFINE_FIELD( m_fSpotlightFlags,		FIELD_INTEGER ),
	DEFINE_FIELD( m_flInspectEndTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flNextEntitySearchTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flNextHintSearchTime,	FIELD_TIME ),
	DEFINE_FIELD( m_bHadEnemy,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vSpotlightTargetPos,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vSpotlightCurrentPos,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_pSpotlight,				FIELD_CLASSPTR ),
	DEFINE_FIELD( m_pSpotlightTarget,		FIELD_CLASSPTR ),
	DEFINE_FIELD( m_vSpotlightDir,			FIELD_VECTOR ),
	DEFINE_FIELD( m_nHaloSprite,			FIELD_INTEGER ),
	DEFINE_FIELD( m_pScriptedTarget,		FIELD_CLASSPTR ),

	DEFINE_KEYFIELD( m_flYawRange,			FIELD_FLOAT, "YawRange"),
	DEFINE_KEYFIELD( m_flPitchMin,			FIELD_FLOAT, "PitchMin"),
	DEFINE_KEYFIELD( m_flPitchMax,			FIELD_FLOAT, "PitchMax"),
	DEFINE_KEYFIELD( m_flIdleSpeed,			FIELD_FLOAT, "IdleSpeed"),
	DEFINE_KEYFIELD( m_flAlertSpeed,		FIELD_FLOAT, "AlertSpeed"),
	DEFINE_KEYFIELD( m_flSpotlightMaxLength,FIELD_FLOAT, "SpotlightLength"),
	DEFINE_KEYFIELD( m_flSpotlightGoalWidth,FIELD_FLOAT, "SpotlightWidth"),

	// DEBUG						m_pScriptedTarget

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID,		"LightOn",		InputLightOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"LightOff",		InputLightOff ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"TrackOn",		InputTrackOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"TrackOff",		InputTrackOff ),

	// Outputs
	DEFINE_OUTPUT(m_pOutputAlert,			"OnAlert"				),
	DEFINE_OUTPUT(m_pOutputDetect,			"DetectedEnemy"			),
	DEFINE_OUTPUT(m_pOutputLost,			"LostEnemy"				),
	DEFINE_OUTPUT(m_pOutputSquadDetect,		"SquadDetectedEnemy"	),
	DEFINE_OUTPUT(m_pOutputSquadLost,		"SquadLostEnemy"		),
	DEFINE_OUTPUT(m_pOutputPosition,		"LightPosition"			),

END_DATADESC()


LINK_ENTITY_TO_CLASS(npc_spotlight, CNPC_Spotlight);

CNPC_Spotlight::CNPC_Spotlight()
{
#ifdef _DEBUG
	m_vInspectPos.Init();
	m_vSpotlightTargetPos.Init();
	m_vSpotlightCurrentPos.Init();
	m_vSpotlightDir.Init();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this NPC's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Spotlight::Classify(void)
{
	return(CLASS_MILITARY);
}

//-------------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model so spotlight gets proper position
// Input   :
// Output  :
//-------------------------------------------------------------------------------------
int CNPC_Spotlight::UpdateTransmitState(void)
{
	return SetTransmitState( FL_EDICT_PVSCHECK );
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_Spotlight::GetSoundInterests( void )
{
	return (SOUND_COMBAT | SOUND_DANGER);
}

//------------------------------------------------------------------------------
// Purpose : Override to split in two when attacked
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_Spotlight::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// Deflect spotlight 
	Vector vCrossProduct;
	CrossProduct(m_vSpotlightDir,g_vecAttackDir, vCrossProduct);
	if (vCrossProduct.y > 0)
	{
		m_flYaw		+= random->RandomInt(10,20);
	}
	else
	{
		m_flYaw		-= random->RandomInt(10,20);
	}

	return (BaseClass::OnTakeDamage_Alive( info ));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CNPC_Spotlight::Event_Killed( const CTakeDamageInfo &info )
{
	// Interrupt whatever schedule I'm on
	SetCondition(COND_SCHEDULE_DONE);

	// Remove spotlight
	SpotlightDestroy();

	// Otherwise, turn into a physics object and fall to the ground
	CBaseCombatCharacter::Event_Killed( info );
}


//-----------------------------------------------------------------------------
// Purpose: Tells use whether or not the NPC cares about a given type of hint node.
// Input  : sHint - 
// Output : TRUE if the NPC is interested in this hint type, FALSE if not.
//-----------------------------------------------------------------------------
bool CNPC_Spotlight::FValidateHintType(CAI_Hint *pHint)
{
	if (pHint->HintType() == HINT_WORLD_WINDOW)
	{
		Vector vHintPos;
		pHint->GetPosition(this,&vHintPos);
		if (SpotlightIsPositionLegal(vHintPos))
		{
			return true;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Plays the engine sound.
//-----------------------------------------------------------------------------
void CNPC_Spotlight::NPCThink(void)
{
	SetNextThink( gpGlobals->curtime + 0.1f );// keep npc thinking.

	if (CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI)
	{
		if (m_pSpotlightTarget)
		{
			m_pSpotlightTarget->SetAbsVelocity( vec3_origin );
		}
	}
	else if (IsAlive())
	{
		GetSenses()->Listen();
		UpdateTargets();
		SpotlightUpdate();
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Spotlight::Precache(void)
{
	//
	// Model.
	//
	PrecacheModel("models/combot.mdl");
	PrecacheModel("models/gibs/combot_gibs.mdl");

	//
	// Sprites.
	//
	PrecacheModel("sprites/spotlight.vmt");
	m_nHaloSprite		= PrecacheModel("sprites/blueflare1.vmt");
	
	BaseClass::Precache();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseEntity* CNPC_Spotlight::BestInspectTarget(void)
{
	// Only look for inspect targets when spotlight it on
	if (m_pSpotlightTarget == NULL)
	{
		return NULL;
	}

	float			fBestDistance		= MAX_COORD_RANGE;
	int				nBestPriority		= -1000;
	int				nBestRelationship	= D_NU;

	// Get my best enemy first
	CBaseEntity*	pBestEntity			= BestEnemy();
	if (pBestEntity)
	{
		// If the enemy isn't visibile
		if (!FVisible(pBestEntity))
		{
			// If he hasn't been seen in a while and hasn't already eluded
			// the squad, make the enemy as eluded and fire a lost squad output
			float flTimeLastSeen = GetEnemies()->LastTimeSeen(pBestEntity);
			if (!GetEnemies()->HasEludedMe(pBestEntity) &&
				flTimeLastSeen + 0.5 < gpGlobals->curtime)
			{
				GetEnemies()->MarkAsEluded(pBestEntity);
				m_pOutputSquadLost.Set(*((EHANDLE *)pBestEntity),this,this);
			}
			pBestEntity = NULL;
		}

		// If he has eluded me or isn't in the legal range of my spotligth reject
		else if (GetEnemies()->HasEludedMe(pBestEntity)									||
				 !SpotlightIsPositionLegal(GetEnemies()->LastKnownPosition(pBestEntity))	)
		{
			pBestEntity = NULL;
		}
	}

	CBaseEntity *pEntity = NULL;

	// Search from the spotlight position
	Vector	vSearchOrigin	= m_pSpotlightTarget->GetAbsOrigin();
	float	flSearchDist;
	if (HaveInspectTarget())
	{
		flSearchDist = SPOTLIGHT_ACTIVE_SEARCH_DIST;
	}
	else
	{
		flSearchDist = SPOTLIGHT_ENTITY_SEARCH_DIST;
	}
	for ( CEntitySphereQuery sphere( vSearchOrigin, SPOTLIGHT_ENTITY_SEARCH_DIST ); pEntity = sphere.GetCurrentEntity(); sphere.NextEntity() )
	{
		if (pEntity->GetFlags() & (FL_CLIENT|FL_NPC))
		{

			if (pEntity->GetFlags() & FL_NOTARGET)
			{
				continue;
			}

			
			if (!pEntity->IsAlive())
			{
				continue;
			}

			if ((pEntity->Classify() == CLASS_MILITARY)||
				(pEntity->Classify() == CLASS_BULLSEYE))
			{
				continue;
			}

			if (m_pSquad && m_pSquad->SquadIsMember(pEntity))
			{
				continue;
			}

			// Disregard if the entity is out of the view cone, occluded,
			if( !FVisible( pEntity ) )
			{
				continue;
			}

			// If it's a new enemy or one that had eluded me
			if (!GetEnemies()->HasMemory(pEntity)	|| 
				GetEnemies()->HasEludedMe(pEntity)	)
			{
				m_pOutputSquadDetect.Set(*((EHANDLE *)pEntity),this,this);
			}
			UpdateEnemyMemory(pEntity,pEntity->GetAbsOrigin());

			CBaseCombatCharacter* pBCC	= (CBaseCombatCharacter*)pEntity;
			float	fTestDistance		= (GetAbsOrigin() - pBCC->EyePosition()).Length();
			int		nTestRelationship	= IRelationType(pBCC);
			int		nTestPriority		= IRelationPriority ( pBCC );

			// Only follow hated entities if I'm not in idle state
			if (nTestRelationship != D_HT && m_NPCState != NPC_STATE_IDLE)
			{
				continue;
			}

			// -------------------------------------------
			//  Choose hated entites over everything else
			// -------------------------------------------
			if (nTestRelationship == D_HT && nBestRelationship != D_HT)
			{
				pBestEntity			= pBCC;
				fBestDistance		= fTestDistance;
				nBestPriority		= nTestPriority;
				nBestRelationship	= nTestRelationship;
			}
			// -------------------------------------------
			//  If both are hated, or both are not
			// -------------------------------------------
			else if(	(nTestRelationship != D_HT && nBestRelationship != D_HT) ||
						(nTestRelationship == D_HT && nBestRelationship == D_HT) )
			{
				// --------------------------------------
				//  Pick one with the higher priority
				// --------------------------------------
				if (nTestPriority > nBestPriority)
				{
					pBestEntity			= pBCC;
					fBestDistance		= fTestDistance;
					nBestPriority		= nTestPriority;
					nBestRelationship	= nTestRelationship;
				}
				// -----------------------------------------
				//  If priority the same pick best distance
				// -----------------------------------------
				else if (nTestPriority == nBestPriority)
				{
					if (fTestDistance < fBestDistance)
					{
						pBestEntity			= pBCC;
						fBestDistance		= fTestDistance;
						nBestPriority		= nTestPriority;
						nBestRelationship	= nTestRelationship;
					}
				}
			}
		}
	}
	return pBestEntity;
}

//------------------------------------------------------------------------------
// Purpose : Clears any previous inspect target and set inspect target to
//			 the given entity and set the durection of the inspection
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Spotlight::SetInspectTargetToEntity(CBaseEntity *pEntity, float fInspectDuration)
{
	ClearInspectTarget();
	SetTarget(pEntity);
	m_flInspectEndTime	= gpGlobals->curtime + fInspectDuration;
}

//------------------------------------------------------------------------------
// Purpose : Clears any previous inspect target and set inspect target to
//			 the given entity and set the durection of the inspection
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Spotlight::SetInspectTargetToEnemy(CBaseEntity *pEntity)
{
	ClearInspectTarget();
	SetEnemy( pEntity );
	m_bHadEnemy			= true;
	m_flInspectEndTime	= 0;
	SetState(NPC_STATE_COMBAT);

	EHANDLE hEnemy;
	hEnemy.Set( GetEnemy() );
	m_pOutputDetect.Set(hEnemy, NULL, this);
}

//------------------------------------------------------------------------------
// Purpose : Clears any previous inspect target and set inspect target to
//			 the given hint node and set the durection of the inspection
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Spotlight::SetInspectTargetToHint(CAI_Hint *pHintNode, float fInspectDuration)
{
	ClearInspectTarget();

	// --------------------------------------------
	// Figure out the location that the hint hits
	// --------------------------------------------
	float  fHintYaw		= DEG2RAD(pHintNode->Yaw());
	Vector vHintDir		= Vector(cos(fHintYaw),sin(fHintYaw),0);
	Vector vHintOrigin;
	pHintNode->GetPosition(this,&vHintOrigin);
	Vector vHintEnd		= vHintOrigin + (vHintDir * 500);
	trace_t tr;
	AI_TraceLine ( vHintOrigin, vHintEnd, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr);
	if (tr.fraction == 1.0)
	{
		DevMsg("ERROR: Scanner hint node not facing a surface!\n");
	}
	else
	{
		SetHintNode( pHintNode );
		m_vInspectPos	= tr.endpos;
		pHintNode->Lock(this);

		m_flInspectEndTime = gpGlobals->curtime + fInspectDuration;
	}
}

//------------------------------------------------------------------------------
// Purpose : Clears any previous inspect target and set inspect target to
//			 the given position and set the durection of the inspection
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Spotlight::SetInspectTargetToPos(const Vector &vInspectPos, float fInspectDuration)
{
	ClearInspectTarget();
	m_vInspectPos		= vInspectPos;

	m_flInspectEndTime	= gpGlobals->curtime + fInspectDuration;
}

//------------------------------------------------------------------------------
// Purpose : Clears out any previous inspection targets
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Spotlight::ClearInspectTarget(void)
{
	// If I'm losing an enemy, fire a message
	if (m_bHadEnemy)
	{
		m_bHadEnemy = false;

		EHANDLE hEnemy;
		hEnemy.Set( GetEnemy() );

		m_pOutputLost.Set(hEnemy,this,this);
	}

	// If I'm in combat state, go to alert
	if (m_NPCState == NPC_STATE_COMBAT)
	{
		SetState(NPC_STATE_ALERT);
	}
	
	SetTarget( NULL );
	SetEnemy( NULL );
	ClearHintNode( SPOTLIGHT_HINT_INSPECT_LENGTH );
	m_vInspectPos			= vec3_origin;
	m_flYawDir				= random->RandomInt(0,1) ? 1 : -1;
	m_flPitchDir			= random->RandomInt(0,1) ? 1 : -1;
}

//------------------------------------------------------------------------------
// Purpose : Returns true if there is a position to be inspected and sets
//			 vTargetPos to the inspection position
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_Spotlight::HaveInspectTarget(void)
{
	if (GetEnemy() != NULL)
	{
		return true;
	}
	else if (GetTarget() != NULL)
	{
		return true;
	}
	if (m_vInspectPos != vec3_origin)
	{
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
// Purpose : Returns true if there is a position to be inspected and sets
//			 vTargetPos to the inspection position
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CNPC_Spotlight::InspectTargetPosition(void)
{
	if (GetEnemy() != NULL)
	{
		// If in spotlight mode, aim for ground below target unless is client
		if (!(GetEnemy()->GetFlags() & FL_CLIENT))
		{
			Vector vInspectPos;
			vInspectPos.x	= GetEnemy()->GetAbsOrigin().x;
			vInspectPos.y	= GetEnemy()->GetAbsOrigin().y;
			vInspectPos.z	= GetFloorZ(GetEnemy()->GetAbsOrigin()+Vector(0,0,1));
			return vInspectPos;
		}
		// Otherwise aim for eyes
		else
		{
			return GetEnemy()->EyePosition();
		}
	}
	else if (GetTarget() != NULL)
	{
		// If in spotlight mode, aim for ground below target unless is client
		if (!(GetTarget()->GetFlags() & FL_CLIENT))
		{
			Vector vInspectPos;
			vInspectPos.x	= GetTarget()->GetAbsOrigin().x;
			vInspectPos.y	= GetTarget()->GetAbsOrigin().y;
			vInspectPos.z	= GetFloorZ(GetTarget()->GetAbsOrigin());
			return vInspectPos;
		}
		// Otherwise aim for eyes
		else
		{
			return GetTarget()->EyePosition();
		}
	}
	else if (m_vInspectPos != vec3_origin)
	{
		return m_vInspectPos;
	}
	else
	{
		DevMsg("InspectTargetPosition called with no target!\n");
		return m_vInspectPos;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Spotlight::UpdateTargets(void)
{
	if (m_fSpotlightFlags & BITS_SPOTLIGHT_TRACK_ON)
	{
		// --------------------------------------------------------------------------
		//  Look for a nearby entity to inspect 
		// --------------------------------------------------------------------------
		CBaseEntity *pBestEntity = BestInspectTarget();

		// If I found one
		if (pBestEntity)
		{
			// If it's an enemy
			if (IRelationType(pBestEntity) == D_HT)
			{
				// If I'm not already inspecting an enemy take it
				if (GetEnemy() == NULL)
				{
					SetInspectTargetToEnemy(pBestEntity);
					if (m_pSquad)
					{
						AISquadIter_t iter;
						for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
						{
							// reset members who aren't activly engaged in fighting
							if (pSquadMember->GetEnemy() != pBestEntity && !pSquadMember->HasCondition( COND_SEE_ENEMY))
							{
								// give them a new enemy
								pSquadMember->SetLastAttackTime( 0 );
								pSquadMember->SetCondition ( COND_NEW_ENEMY );
							}
						}
					}
				}
				// If I am inspecting an enemy, take it if priority is higher
				else
				{
					if (IRelationPriority(pBestEntity) > IRelationPriority(GetEnemy()))
					{
						SetInspectTargetToEnemy(pBestEntity);
					}
				}
			}
			// If its not an enemy
			else
			{
				// If I'm not already inspeting something take it
				if (GetTarget() == NULL)
				{
					SetInspectTargetToEntity(pBestEntity,SPOTLIGHT_ENTITY_INSPECT_LENGTH);
				}
				// If I am inspecting somethin, take if priority is higher
				else
				{
					if (IRelationPriority(pBestEntity) > IRelationPriority(GetTarget()))
					{
						SetInspectTargetToEntity(pBestEntity,SPOTLIGHT_ENTITY_INSPECT_LENGTH);
					}
				}
			}
		}

		// ---------------------------------------
		// If I'm not current inspecting an enemy
		// ---------------------------------------
		if (GetEnemy() == NULL)
		{
			// -----------------------------------------------------------
			// If my inspection over clear my inspect target.
			// -----------------------------------------------------------
			if (HaveInspectTarget()						&&
				gpGlobals->curtime > m_flInspectEndTime	)
			{
				m_flNextEntitySearchTime	= gpGlobals->curtime + SPOTLIGHT_ENTITY_INSPECT_DELAY;
				m_flNextHintSearchTime		= gpGlobals->curtime + SPOTLIGHT_HINT_INSPECT_DELAY;
				ClearInspectTarget();
			}

			// --------------------------------------------------------------
			//  If I heard a sound inspect it 
			// --------------------------------------------------------------
			if (HasCondition(COND_HEAR_COMBAT) || HasCondition(COND_HEAR_DANGER) )
			{
				CSound *pSound = GetBestSound();
				if (pSound)
				{
					Vector vSoundPos = pSound->GetSoundOrigin();
					// Only alert to sound if in my swing range
					if (SpotlightIsPositionLegal(vSoundPos))
					{
						SetInspectTargetToPos(vSoundPos,SPOTLIGHT_SOUND_INSPECT_LENGTH);

						// Fire alert output
						m_pOutputAlert.FireOutput(NULL,this);

						SetState(NPC_STATE_ALERT);
					}
				}
			}

			// --------------------------------------
			//  Check for hints to inspect
			// --------------------------------------
			if (gpGlobals->curtime		>	m_flNextHintSearchTime	&&
				!HaveInspectTarget()							)
			{
				SetHintNode(CAI_HintManager::FindHint(this, HINT_NONE, 0, SPOTLIGHT_HINT_SEARCH_DIST));

				if (GetHintNode())
				{
					m_flNextHintSearchTime	= gpGlobals->curtime + SPOTLIGHT_HINT_INSPECT_LENGTH;
					SetInspectTargetToHint(GetHintNode(),SPOTLIGHT_HINT_INSPECT_LENGTH);
				}
			}
		}

		// -------------------------------------------------------
		//  Make sure inspect target is still in a legal position
		//  (Don't care about enemies)
		// -------------------------------------------------------
		if (GetTarget())
		{
			if (!SpotlightIsPositionLegal(GetEnemies()->LastKnownPosition(GetTarget())))
			{
				ClearInspectTarget();
			}
			else if (!FVisible(GetTarget()))
			{
				ClearInspectTarget();
			}
		}
		if (GetEnemy())
		{
			if (!FVisible(GetEnemy()))
			{
				ClearInspectTarget();
			}
			// If enemy is dead inspect for a couple of seconds on move on
			else if (!GetEnemy()->IsAlive())
			{
				SetInspectTargetToPos( GetEnemy()->GetAbsOrigin(), 1.0);
			}
			else
			{
				UpdateEnemyMemory(GetEnemy(),GetEnemy()->GetAbsOrigin());
			}
		}

		// -----------------------------------------
		//  See if I'm at my burn target
		// ------------------------------------------
		if (!HaveInspectTarget()						&&
			m_pScriptedTarget							&&
			m_pSpotlightTarget != NULL					)
		{
			float fTargetDist = (m_vSpotlightTargetPos - m_vSpotlightCurrentPos).Length();
			if (fTargetDist < SPOTLIGHT_BURN_TARGET_THRESH  )
			{
				// Update scripted target
				SetScriptedTarget( m_pScriptedTarget->NextScriptedTarget());	
			}
			else
			{
				Vector vTargetDir = m_vSpotlightTargetPos - m_vSpotlightCurrentPos;
				VectorNormalize(vTargetDir);
				float	flDot	= DotProduct(m_vSpotlightDir,vTargetDir);
				if (flDot > 0.99  )
				{
					// Update scripted target
					SetScriptedTarget( m_pScriptedTarget->NextScriptedTarget());
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Overridden because if the player is a criminal, we hate them.
// Input  : pTarget - Entity with which to determine relationship.
// Output : Returns relationship value.
//-----------------------------------------------------------------------------
Disposition_t CNPC_Spotlight::IRelationType(CBaseEntity *pTarget)
{
	//
	// If it's the player and they are a criminal, we hate them.
	//
	if (pTarget->Classify() == CLASS_PLAYER)
	{
		if (GlobalEntity_GetState("gordon_precriminal") == GLOBAL_ON)
		{
			return(D_NU);
		}
	}

	return(CBaseCombatCharacter::IRelationType(pTarget));
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Spotlight::SpotlightDestroy(void)
{
	if (m_pSpotlight)
	{
		UTIL_Remove(m_pSpotlight);
		m_pSpotlight = NULL;
		
		UTIL_Remove(m_pSpotlightTarget);
		m_pSpotlightTarget = NULL;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Spotlight::SpotlightCreate(void)
{

	// If I have an enemy, start spotlight on my enemy
	if (GetEnemy() != NULL)
	{
		Vector vEnemyPos	= GetEnemyLKP();
		Vector vTargetPos	= vEnemyPos;
		vTargetPos.z		= GetFloorZ(vEnemyPos);
		m_vSpotlightDir = vTargetPos - GetAbsOrigin();
		VectorNormalize(m_vSpotlightDir);
	}
	// If I have an target, start spotlight on my target
	else if (GetTarget() != NULL)
	{
		Vector vTargetPos	= GetTarget()->GetAbsOrigin();
		vTargetPos.z		= GetFloorZ(GetTarget()->GetAbsOrigin());
		m_vSpotlightDir = vTargetPos - GetAbsOrigin();
		VectorNormalize(m_vSpotlightDir);
	}
	else
	{
		AngleVectors( GetAbsAngles(), &m_vSpotlightDir );
	}

	trace_t tr;
	AI_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + m_vSpotlightDir * m_flSpotlightMaxLength, 
		MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr);

	m_pSpotlightTarget				= (CSpotlightEnd*)CreateEntityByName( "spotlight_end" );
	m_pSpotlightTarget->Spawn();
	m_pSpotlightTarget->SetLocalOrigin( tr.endpos );
	m_pSpotlightTarget->SetOwnerEntity( this );
	m_pSpotlightTarget->m_clrRender = m_clrRender;
	m_pSpotlightTarget->m_Radius = m_flSpotlightMaxLength;

	if ( FBitSet (m_spawnflags, SF_SPOTLIGHT_NO_DYNAMIC_LIGHT) )
	{
		m_pSpotlightTarget->m_flLightScale = 0.0;
	}

	m_pSpotlight = CBeam::BeamCreate( "sprites/spotlight.vmt", 2.0 );
	m_pSpotlight->SetColor( m_clrRender->r, m_clrRender->g, m_clrRender->b ); 
	m_pSpotlight->SetHaloTexture(m_nHaloSprite);
	m_pSpotlight->SetHaloScale(40);
	m_pSpotlight->SetEndWidth(m_flSpotlightGoalWidth);
	m_pSpotlight->SetBeamFlags(FBEAM_SHADEOUT);
	m_pSpotlight->SetBrightness( 80 );
	m_pSpotlight->SetNoise( 0 );
	m_pSpotlight->EntsInit( this, m_pSpotlightTarget );
}

//------------------------------------------------------------------------------
// Purpose : Returns true is spotlight can reach position
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_Spotlight::SpotlightIsPositionLegal(const Vector &vTestPos)
{
	Vector vTargetDir = vTestPos - GetAbsOrigin();
	VectorNormalize(vTargetDir);
	QAngle vTargetAngles;
	VectorAngles(vTargetDir,vTargetAngles);

	// Make sure target is in a legal position
	if		(UTIL_AngleDistance( vTargetAngles[YAW], m_flYawCenter )	>	m_flYawRange)
	{
		return false;
	}
	else if (UTIL_AngleDistance( vTargetAngles[YAW], m_flYawCenter )	<  -m_flYawRange)
	{
		return false;
	}
	if		(UTIL_AngleDistance( vTargetAngles[PITCH], m_flPitchCenter ) >  m_flPitchMax)
	{
		return false;
	}
	else if (UTIL_AngleDistance( vTargetAngles[PITCH], m_flPitchCenter ) <  m_flPitchMin)
	{
		return false;
	}
	return true;
}

//------------------------------------------------------------------------------
// Purpose : Converts spotlight target position into desired yaw and pitch
//			 directions to reach target
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Spotlight::SpotlightSetTargetYawAndPitch(void)
{
	Vector vTargetDir = m_vSpotlightTargetPos - GetAbsOrigin();
	VectorNormalize(vTargetDir);
	QAngle vTargetAngles;
	VectorAngles(vTargetDir,vTargetAngles);

	float flYawDiff = UTIL_AngleDistance(vTargetAngles[YAW], m_flYaw);
	if ( flYawDiff > 0)
	{
		m_flYawDir = SPOTLIGHT_SWING_FORWARD;
	}
	else
	{
		m_flYawDir = SPOTLIGHT_SWING_BACK;
	}

	//DevMsg("%f %f (%f)\n",vTargetAngles[YAW], m_flYaw,flYawDiff);

	float flPitchDiff = UTIL_AngleDistance(vTargetAngles[PITCH], m_flPitch);
	if (flPitchDiff > 0)
	{
		m_flPitchDir = SPOTLIGHT_SWING_FORWARD;
	}
	else
	{
		m_flPitchDir = SPOTLIGHT_SWING_BACK;
	}

	//DevMsg("%f %f (%f)\n",vTargetAngles[PITCH], m_flPitch,flPitchDiff);


	if ( fabs(flYawDiff) < 2)
	{
		m_flYawDir *= 0.5;
	}
	if ( fabs(flPitchDiff) < 2)
	{
		m_flPitchDir *= 0.5;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
float	CNPC_Spotlight::SpotlightSpeed(void)
{
	float fSpeedScale  = 1.0;
	float fInspectDist = (m_vSpotlightTargetPos - m_vSpotlightCurrentPos).Length();
	if (fInspectDist < 100)
	{
		fSpeedScale = 0.25;
	}

	if (!HaveInspectTarget() && m_pScriptedTarget)
	{
		return (fSpeedScale * m_pScriptedTarget->MoveSpeed());
	}
	else if (m_NPCState == NPC_STATE_COMBAT ||
		m_NPCState == NPC_STATE_ALERT	)
	{
		return (fSpeedScale * m_flAlertSpeed);
	}
	else
	{
		return (fSpeedScale * m_flIdleSpeed);
	}
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CNPC_Spotlight::SpotlightCurrentPos(void)
{
	if (!m_pSpotlight)
	{
		DevMsg("Spotlight pos. called w/o spotlight!\n");
		return vec3_origin;
	}
	
	if (HaveInspectTarget())
	{
		m_vSpotlightTargetPos	= InspectTargetPosition();
		SpotlightSetTargetYawAndPitch();
	}
	else if (m_pScriptedTarget)
	{
		m_vSpotlightTargetPos	= m_pScriptedTarget->GetAbsOrigin();
		SpotlightSetTargetYawAndPitch();

		// I'm allowed to move outside my normal range when
		// tracking burn targets.  Return smoothly when I'm done
		m_fSpotlightFlags |= BITS_SPOTLIGHT_SMOOTH_RETURN;
	}
	else
	{
		// Make random movement frame independent
		if (random->RandomInt(0,10) == 0)
		{
			m_flYawDir *= -1;
		}
		if (random->RandomInt(0,10) == 0)
		{
			m_flPitchDir *= -1;
		}
	}
	// Calculate new pitch and yaw velocity
	float flSpeed			= SpotlightSpeed();
	float flNewYawSpeed		= m_flYawDir	* flSpeed;
	float flNewPitchSpeed	= m_flPitchDir	* flSpeed;

	// Adjust current velocity
	float	myYawDecay		= 0.8;
	float	myPitchDecay	= 0.7;
	m_flYawSpeed			= (myYawDecay   * m_flYawSpeed	 +  (1-myYawDecay)   * flNewYawSpeed  );
	m_flPitchSpeed			= (myPitchDecay * m_flPitchSpeed +  (1-myPitchDecay) * flNewPitchSpeed);
	
	// Keep speed with in bounds
	float flMaxSpeed = SPOTLIGHT_MAX_SPEED_SCALE * SpotlightSpeed();
	if		(m_flYawSpeed	>  flMaxSpeed)	m_flYawSpeed   =  flMaxSpeed;
	else if (m_flYawSpeed	< -flMaxSpeed)	m_flYawSpeed   = -flMaxSpeed;
	if		(m_flPitchSpeed >  flMaxSpeed)	m_flPitchSpeed =  flMaxSpeed;
	else if (m_flPitchSpeed < -flMaxSpeed)	m_flPitchSpeed = -flMaxSpeed;

	// Calculate new pitch and yaw positions
	m_flYaw		+= m_flYawSpeed;
	m_flPitch	+= m_flPitchSpeed;

	// Keep yaw in 0/360 range
	if (m_flYaw < 0	 ) m_flYaw +=360;
	if (m_flYaw > 360) m_flYaw -=360;

	// ---------------------------------------------
	// Check yaw and pitch boundaries unless I have
	// a burn target, or an enemy 
	// ---------------------------------------------
	if (( HaveInspectTarget() &&  GetEnemy()	== NULL ) || 
		(!HaveInspectTarget() && !m_pScriptedTarget ) )
	{
		bool bInRange = true;
		if	(UTIL_AngleDistance( m_flYaw, m_flYawCenter )  >		m_flYawRange)
		{
			if (m_fSpotlightFlags & BITS_SPOTLIGHT_SMOOTH_RETURN)
			{
				bInRange	= false;	
			}
			else
			{
				m_flYaw		= m_flYawCenter + m_flYawRange;
			}
			m_flYawDir	= SPOTLIGHT_SWING_BACK;
		}
		else if (UTIL_AngleDistance( m_flYaw, m_flYawCenter ) < -m_flYawRange)
		{	
			if (m_fSpotlightFlags & BITS_SPOTLIGHT_SMOOTH_RETURN)
			{
				bInRange	= false;
			}
			else
			{
				m_flYaw		= m_flYawCenter - m_flYawRange;
			}
			m_flYawDir	= SPOTLIGHT_SWING_FORWARD;
		}
		if	(UTIL_AngleDistance( m_flPitch, m_flPitchCenter ) > m_flPitchMax)
		{
			if (m_fSpotlightFlags & BITS_SPOTLIGHT_SMOOTH_RETURN)
			{
				bInRange	= false;
			}
			else
			{
				m_flPitch	= m_flPitchCenter + m_flPitchMax;
			}
			m_flPitchDir = SPOTLIGHT_SWING_BACK;
		}
		else if (UTIL_AngleDistance( m_flPitch, m_flPitchCenter ) < m_flPitchMin)
		{
			if (m_fSpotlightFlags & BITS_SPOTLIGHT_SMOOTH_RETURN)
			{
				bInRange	= false;	
			}
			else
			{
				m_flPitch	= m_flPitchCenter + m_flPitchMin;
			}
			m_flPitchDir = SPOTLIGHT_SWING_FORWARD;
		}

		// If in range I'm done doing a smooth return
		if (bInRange)
		{
			m_fSpotlightFlags &= ~BITS_SPOTLIGHT_SMOOTH_RETURN;
		}
	}
	// Convert pitch and yaw to forward angle
	QAngle vAngle	= vec3_angle;
	vAngle[YAW]		= m_flYaw;
	vAngle[PITCH]	= m_flPitch;
	AngleVectors( vAngle, &m_vSpotlightDir );

	// ---------------------------------------------
	//	Get beam end point.  Only collide with
	//  solid objects, not npcs
	// ---------------------------------------------
	trace_t tr;
	AI_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + (m_vSpotlightDir * 2 * m_flSpotlightMaxLength), 
		(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_DEBRIS),
		this, COLLISION_GROUP_NONE, &tr);

	return (tr.endpos);
}


//------------------------------------------------------------------------------
// Purpose : Update the direction and position of my spotlight
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Spotlight::SpotlightUpdate(void)
{
	// ---------------------------------------------------
	//  Go back to idle state after a while
	// ---------------------------------------------------
	if (m_NPCState == NPC_STATE_ALERT					&&
		m_flLastStateChangeTime + 30 < gpGlobals->curtime	)
	{
		SetState(NPC_STATE_IDLE);
	}

	// ---------------------------------------------------
	//  If I don't have a spotlight attempt to create one
	// ---------------------------------------------------
	if (!m_pSpotlight && 
		m_fSpotlightFlags & BITS_SPOTLIGHT_LIGHT_ON	)
	{
		SpotlightCreate();
	}
	if (!m_pSpotlight)
	{
		return;
	}

	// -----------------------------------------------------
	//  If spotlight flag is off destroy spotlight and exit
	// -----------------------------------------------------
	if (!(m_fSpotlightFlags & BITS_SPOTLIGHT_LIGHT_ON))
	{
		if (m_pSpotlight)
		{
			SpotlightDestroy();
			return;
		}
	}
	
	if (m_fSpotlightFlags & BITS_SPOTLIGHT_TRACK_ON)
	{
		// -------------------------------------------
		//  Calculate the new spotlight position
		// --------------------------------------------
		m_vSpotlightCurrentPos = SpotlightCurrentPos();
	}
	// --------------------------------------------------------------
	//  Update spotlight target velocity
	// --------------------------------------------------------------
	Vector vTargetDir  = (m_vSpotlightCurrentPos - m_pSpotlightTarget->GetAbsOrigin());
	float  vTargetDist = vTargetDir.Length();

	Vector vecNewVelocity = vTargetDir;
	VectorNormalize(vecNewVelocity);
	vecNewVelocity *= (10 * vTargetDist);

	// If a large move is requested, just jump to final spot as we
	// probably hit a discontinuity
	if (vecNewVelocity.Length() > 200)
	{
		VectorNormalize(vecNewVelocity);
		vecNewVelocity *= 200;
		VectorNormalize(vTargetDir);
		m_pSpotlightTarget->SetLocalOrigin( m_vSpotlightCurrentPos );
	}
	m_pSpotlightTarget->SetAbsVelocity( vecNewVelocity );
	m_pSpotlightTarget->m_vSpotlightOrg = GetAbsOrigin();

	// Avoid sudden change in where beam fades out when cross disconinuities
	m_pSpotlightTarget->m_vSpotlightDir = m_pSpotlightTarget->GetLocalOrigin() - m_pSpotlightTarget->m_vSpotlightOrg;
	float flBeamLength	= VectorNormalize( m_pSpotlightTarget->m_vSpotlightDir );
	m_flSpotlightCurLength = (0.60*m_flSpotlightCurLength) + (0.4*flBeamLength);

	// Fade out spotlight end if past max length.  
	if (m_flSpotlightCurLength > 2*m_flSpotlightMaxLength)
	{
		m_pSpotlightTarget->SetRenderColorA( 0 );
		m_pSpotlight->SetFadeLength(m_flSpotlightMaxLength);
	}
	else if (m_flSpotlightCurLength > m_flSpotlightMaxLength)		
	{
		m_pSpotlightTarget->SetRenderColorA( (1-((m_flSpotlightCurLength-m_flSpotlightMaxLength)/m_flSpotlightMaxLength)) );
		m_pSpotlight->SetFadeLength(m_flSpotlightMaxLength);
	}
	else
	{
		m_pSpotlightTarget->SetRenderColorA( 1.0 );
		m_pSpotlight->SetFadeLength(m_flSpotlightCurLength);
	}


	// Adjust end width to keep beam width constant
	float flNewWidth = m_flSpotlightGoalWidth*(flBeamLength/m_flSpotlightMaxLength);
	m_pSpotlight->SetEndWidth(flNewWidth);

	// Adjust width of light on the end.  
	if ( FBitSet (m_spawnflags, SF_SPOTLIGHT_NO_DYNAMIC_LIGHT) )
	{
		m_pSpotlightTarget->m_flLightScale = 0.0;
	}
	else
	{
		// <<TODO>> - magic number 1.8 depends on sprite size
		m_pSpotlightTarget->m_flLightScale = 1.8*flNewWidth;
	}

	m_pOutputPosition.Set(m_pSpotlightTarget->GetLocalOrigin(),this,this);

#ifdef SPOTLIGHT_DEBUG
	NDebugOverlay::Cross3D(m_vSpotlightCurrentPos,Vector(-5,-5,-5),Vector(5,5,5),0,255,0,true,0.1);
	NDebugOverlay::Cross3D(m_vSpotlightTargetPos,Vector(-5,-5,-5),Vector(5,5,5),255,0,0,true,0.1);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Spotlight::Spawn(void)
{
	// Check for user error
	if (m_flSpotlightMaxLength <= 0)
	{
		DevMsg("CNPC_Spotlight::Spawn: Invalid spotlight length <= 0, setting to 500\n");
		m_flSpotlightMaxLength = 500;
	}
	
	if (m_flSpotlightGoalWidth <= 0)
	{
		DevMsg("CNPC_Spotlight::Spawn: Invalid spotlight width <= 0, setting to 10\n");
		m_flSpotlightGoalWidth = 10;
	}

	if (m_flSpotlightGoalWidth > MAX_BEAM_WIDTH)
	{
		DevMsg("CNPC_Spotlight::Spawn: Invalid spotlight width %.1f (max %.1f)\n", m_flSpotlightGoalWidth, MAX_BEAM_WIDTH );
		m_flSpotlightGoalWidth = MAX_BEAM_WIDTH; 
	}

	Precache();

	// This is a dummy model that is never used!
	SetModel( "models/player.mdl" );

	// No Hull for now
	UTIL_SetSize(this,vec3_origin,vec3_origin);

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	m_bloodColor		= DONT_BLEED;
	SetViewOffset( Vector(0, 0, 10) );		// Position of the eyes relative to NPC's origin.
	m_flFieldOfView		= VIEW_FIELD_FULL;
	m_NPCState			= NPC_STATE_IDLE;

	CapabilitiesAdd( bits_CAP_SQUAD);

	// ------------------------------------
	//	Init all class vars 
	// ------------------------------------
	m_vInspectPos			= vec3_origin;
	m_flInspectEndTime		= 0;
	m_flNextEntitySearchTime= gpGlobals->curtime + SPOTLIGHT_ENTITY_INSPECT_DELAY;
	m_flNextHintSearchTime	= gpGlobals->curtime + SPOTLIGHT_HINT_INSPECT_DELAY;
	m_bHadEnemy				= false;

	m_vSpotlightTargetPos	= vec3_origin;
	m_vSpotlightCurrentPos	= vec3_origin;
	m_pSpotlight			= NULL;
	m_pSpotlightTarget		= NULL;
	m_vSpotlightDir			= vec3_origin;
	//m_nHaloSprite			// Set in precache
	m_flSpotlightCurLength	= m_flSpotlightMaxLength;

	m_flYaw					= 0;
	m_flYawSpeed			= 0;
	m_flYawCenter			= GetLocalAngles().y;
	m_flYawDir				= random->RandomInt(0,1) ? 1 : -1;
	//m_flYawRange			= 90;	// Keyfield in WC

	m_flPitch				= 0;
	m_flPitchSpeed			= 0;
	m_flPitchCenter			= GetLocalAngles().x;
	m_flPitchDir			= random->RandomInt(0,1) ? 1 : -1;
	//m_flPitchMin			= 35;	// Keyfield in WC
	//m_flPitchMax			= 50;	// Keyfield in WC
	//m_flIdleSpeed			= 2;	// Keyfield in WC
	//m_flAlertSpeed		= 5;	// Keyfield in WC

	m_fSpotlightFlags = 0;
	if (FBitSet ( m_spawnflags, SF_SPOTLIGHT_START_TRACK_ON ))
	{
		m_fSpotlightFlags		|= BITS_SPOTLIGHT_TRACK_ON;
	}
	if (FBitSet ( m_spawnflags, SF_SPOTLIGHT_START_LIGHT_ON ))
	{
		m_fSpotlightFlags		|= BITS_SPOTLIGHT_LIGHT_ON;
	}

	// If I'm never moving just turn on the spotlight and don't think again
	if (FBitSet ( m_spawnflags, SF_SPOTLIGHT_NEVER_MOVE ))
	{
		SpotlightCreate();
	}
	else
	{
		NPCInit();
		SetThink(CallNPCThink);
	}

	AddEffects( EF_NODRAW );
	SetMoveType( MOVETYPE_NONE );
	SetGravity( 0.0 );
}

//------------------------------------------------------------------------------
// Purpose: Inputs
//------------------------------------------------------------------------------
void CNPC_Spotlight::InputLightOn( inputdata_t &inputdata )
{
	m_fSpotlightFlags |= BITS_SPOTLIGHT_LIGHT_ON;
}

void CNPC_Spotlight::InputLightOff( inputdata_t &inputdata )
{
	m_fSpotlightFlags &= ~BITS_SPOTLIGHT_LIGHT_ON;
}

void CNPC_Spotlight::InputTrackOn( inputdata_t &inputdata )
{
	m_fSpotlightFlags |= BITS_SPOTLIGHT_TRACK_ON;
}

void CNPC_Spotlight::InputTrackOff( inputdata_t &inputdata )
{
	m_fSpotlightFlags &= ~BITS_SPOTLIGHT_TRACK_ON;
}


//------------------------------------------------------------------------------
// Purpose : Starts cremator doing scripted burn to a location
//------------------------------------------------------------------------------
void CNPC_Spotlight::SetScriptedTarget( CScriptedTarget *pScriptedTarget )
{
	if (pScriptedTarget)
	{
		m_pScriptedTarget		= pScriptedTarget;
		m_vSpotlightTargetPos	= m_pScriptedTarget->GetAbsOrigin();
	}
	else
	{
		m_pScriptedTarget		= NULL;
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
bool CNPC_Spotlight::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if (interactionType == g_interactionScriptedTarget)
	{
		// If I already have a scripted target, reject the new one
		if (m_pScriptedTarget  && sourceEnt)
		{
			return false;
		}
		else
		{
			SetScriptedTarget((CScriptedTarget*)sourceEnt);
			return true;
		}
	}
	return false;
}
