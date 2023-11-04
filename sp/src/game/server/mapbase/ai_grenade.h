//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef AI_GRENADE_H
#define AI_GRENADE_H
#ifdef _WIN32
#pragma once
#endif

// I wish I didn't have to #include all this, but I don't really have a choice.
// I guess something similar to CAI_BehaviorHost's backbridges could be tried.
#include "cbase.h"
#include "ai_basenpc.h"
#include "npcevent.h"
#include "grenade_frag.h"
#include "basegrenade_shared.h"
#include "ai_squad.h"
#include "GlobalStrings.h"
#include "gameweaponmanager.h"
#include "hl2_gamerules.h"
#include "weapon_physcannon.h"
#include "globalstate.h"
#include "ai_hint.h"

#define COMBINE_AE_GREN_TOSS		( 7 )

#define COMBINE_GRENADE_THROW_SPEED 650
#define COMBINE_GRENADE_TIMER		3.5
#define COMBINE_GRENADE_FLUSH_TIME	3.0		// Don't try to flush an enemy who has been out of sight for longer than this.
#define COMBINE_GRENADE_FLUSH_DIST	256.0	// Don't try to flush an enemy who has moved farther than this distance from the last place I saw him.

#define	COMBINE_MIN_GRENADE_CLEAR_DIST	250

#define	DEFINE_AIGRENADE_DATADESC() \
	DEFINE_KEYFIELD( m_iNumGrenades, FIELD_INTEGER, "NumGrenades" ),	\
	DEFINE_FIELD( m_flNextGrenadeCheck, FIELD_TIME ),	\
	DEFINE_FIELD( m_hForcedGrenadeTarget, FIELD_EHANDLE ),	\
	DEFINE_FIELD( m_flNextAltFireTime, FIELD_TIME ),	\
	DEFINE_FIELD( m_vecAltFireTarget, FIELD_VECTOR ),	\
	DEFINE_FIELD( m_vecTossVelocity, FIELD_VECTOR ),	\
	DEFINE_FIELD( m_iLastAnimEventHandled, FIELD_INTEGER ),	\
	DEFINE_INPUTFUNC( FIELD_STRING,	"ThrowGrenadeAtTarget",	InputThrowGrenadeAtTarget ),	\
	DEFINE_INPUTFUNC( FIELD_STRING,	"ThrowGrenadeGestureAtTarget",	InputThrowGrenadeGestureAtTarget ),	\
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SetGrenades",	InputSetGrenades ),	\
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"AddGrenades",	InputAddGrenades ),	\
	DEFINE_OUTPUT(m_OnThrowGrenade, "OnThrowGrenade"),	\
	DEFINE_OUTPUT(m_OnOutOfGrenades, "OnOutOfGrenades"),  \

// Use extern float GetCurrentGravity( void );
#define SMGGrenadeArc(shootpos, targetpos) \
	Vector vecShootPos = shootpos; \
	Vector vecThrow = (targetpos - vecShootPos); \
	float time = vecThrow.Length() / 600.0; \
	vecThrow = vecThrow * (1.0 / time); \
	vecThrow.z += (GetCurrentGravity() * 0.5) * time * 0.5; \
	Vector vecFace = vecShootPos + (vecThrow * 0.5); \
	AddFacingTarget(vecFace, 1.0, 0.5); \

// Mask used for Combine ball hull traces.
// This used MASK_SHOT before, but this has been changed to MASK_SHOT_HULL.
// This fixes the existing problem of soldiers trying to fire energy balls through grates,
// but it's also important to prevent soldiers from blowing themselves up with their newfound SMG grenades.
#define MASK_COMBINE_BALL_LOS MASK_SHOT_HULL

extern int COMBINE_AE_BEGIN_ALTFIRE;
extern int COMBINE_AE_ALTFIRE;

extern ConVar ai_grenade_always_drop;

enum eGrenadeCapabilities
{
	GRENCAP_GRENADE = (1 << 0),
	GRENCAP_ALTFIRE = (1 << 1),
};

// What grenade/item types NPCs are capable of dropping
enum eGrenadeDropCapabilities
{
	GRENDROPCAP_GRENADE = (1 << 0),
	GRENDROPCAP_ALTFIRE = (1 << 1),
	GRENDROPCAP_INTERRUPTED = (1 << 2), // Drops grenades when interrupted mid-animation
};

//-----------------------------------------------------------------------------
// Other classes can use this and access some CAI_GrenadeUser functions.
//-----------------------------------------------------------------------------
class CAI_GrenadeUserSink
{
public:
	CAI_GrenadeUserSink() { }

	virtual bool			UsingOnThrowGrenade() { return false; }
};

//-----------------------------------------------------------------------------
//
// Template class for NPCs using grenades or weapon alt-fire stuff.
// You'll still have to use DEFINE_AIGRENADE_DATADESC() in your derived class's datadesc.
// 
// I wanted to have these functions defined in a CPP file, but template class definitions must be in the header.
// Please excuse the bloat below the class definition.
//
//-----------------------------------------------------------------------------
template <class BASE_NPC>
class CAI_GrenadeUser : public BASE_NPC, public CAI_GrenadeUserSink
{
	DECLARE_CLASS_NOFRIEND( CAI_GrenadeUser, BASE_NPC );

public:
	CAI_GrenadeUser() : CAI_GrenadeUserSink() { }

	void AddGrenades( int inc, CBaseEntity *pLastGrenade = NULL )
	{
		m_iNumGrenades += inc;
		if (m_iNumGrenades <= 0)
			m_OnOutOfGrenades.Set( pLastGrenade, pLastGrenade, this );
	}

	// Use secondary ammo as a way of checking if this is a weapon which can be alt-fired (e.g. AR2 or SMG)
	virtual bool	IsAltFireCapable() { return (this->GetActiveWeapon() && this->GetActiveWeapon()->UsesSecondaryAmmo()); }
	virtual bool	IsGrenadeCapable() { return true; }
	inline bool		HasGrenades() { return m_iNumGrenades > 0; }

	void InputSetGrenades( inputdata_t &inputdata ) { AddGrenades( inputdata.value.Int() - m_iNumGrenades ); }
	void InputAddGrenades( inputdata_t &inputdata ) { AddGrenades( inputdata.value.Int() ); }
	void InputThrowGrenadeAtTarget( inputdata_t &inputdata );
	void InputThrowGrenadeGestureAtTarget( inputdata_t &inputdata );

	virtual void DelayGrenadeCheck( float delay ) { m_flNextGrenadeCheck = gpGlobals->curtime + delay; }

	void 			HandleAnimEvent( animevent_t *pEvent );
	void			SetActivity( Activity NewActivity );

	// Soldiers use "lefthand", cops use "LHand", and citizens use "anim_attachment_LH"
	virtual const char*		GetGrenadeAttachment() { return "anim_attachment_LH"; }

	void			ClearAttackConditions( void );

	bool			FValidateHintType( CAI_Hint *pHint );

	Vector			GetAltFireTarget() { return m_vecAltFireTarget; }
	virtual bool	CanAltFireEnemy( bool bUseFreeKnowledge );
	void			DelayAltFireAttack( float flDelay );
	void			DelaySquadAltFireAttack( float flDelay );

	virtual bool	CanGrenadeEnemy( bool bUseFreeKnowledge = true );
	bool			CanThrowGrenade( const Vector &vecTarget );
	bool			CheckCanThrowGrenade( const Vector &vecTarget );

	// For OnThrowGrenade + point_entity_replace, see grenade_frag.cpp
	bool			UsingOnThrowGrenade() { return m_OnThrowGrenade.NumberOfElements() > 0; }

	// For dropping grenades and beyond
	void			DropGrenadeItemsOnDeath( const CTakeDamageInfo &info, CBasePlayer *pPlayer );
	virtual bool	ShouldDropGrenades() { return HasGrenades(); }
	virtual bool	ShouldDropInterruptedGrenades() { return true; }
	virtual bool	ShouldDropAltFire() { return HasGrenades(); }

protected:

	void			StartTask_FaceAltFireTarget( const Task_t *pTask );
	void			StartTask_GetPathToForced( const Task_t *pTask );
	void			StartTask_DeferSquad( const Task_t *pTask );

	void			RunTask_FaceAltFireTarget( const Task_t *pTask );
	void			RunTask_GetPathToForced( const Task_t *pTask );
	void			RunTask_FaceTossDir( const Task_t *pTask );

protected: // We can't have any private saved variables because only derived classes use the datadescs

	int m_iNumGrenades;
	float m_flNextGrenadeCheck;
	EHANDLE m_hForcedGrenadeTarget;

	float			m_flNextAltFireTime;
	Vector			m_vecAltFireTarget;
	Vector			m_vecTossVelocity;

	// CNPC_Combine port for determining if we tossed a grenade
	int				m_iLastAnimEventHandled;

	COutputEHANDLE	m_OnThrowGrenade;
	COutputEHANDLE	m_OnOutOfGrenades;
};

//------------------------------------------------------------------------------
// Purpose: Handle animation events
//------------------------------------------------------------------------------
template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == COMBINE_AE_BEGIN_ALTFIRE )
	{
		if (this->GetActiveWeapon())
			this->GetActiveWeapon()->WeaponSound( SPECIAL1 );

		m_iLastAnimEventHandled = pEvent->event;

		//SpeakIfAllowed( TLK_CMB_THROWGRENADE, "altfire:1" );
		return;
	}
	if ( pEvent->event == COMBINE_AE_ALTFIRE )
	{
		animevent_t fakeEvent;

		fakeEvent.pSource = this;
		fakeEvent.event = EVENT_WEAPON_AR2_ALTFIRE;

		// Weapon could've been dropped while playing animation
		if (this->GetActiveWeapon())
			this->GetActiveWeapon()->Operator_HandleAnimEvent( &fakeEvent, this );

		// Stop other squad members from combine balling for a while.
		DelaySquadAltFireAttack( 10.0f );

		AddGrenades(-1);

		m_iLastAnimEventHandled = pEvent->event;

		return;
	}

	if ( pEvent->event == COMBINE_AE_GREN_TOSS )
	{
		Vector vecSpin;
		vecSpin.x = random->RandomFloat( -1000.0, 1000.0 );
		vecSpin.y = random->RandomFloat( -1000.0, 1000.0 );
		vecSpin.z = random->RandomFloat( -1000.0, 1000.0 );

		Vector vecStart;
		this->GetAttachment( GetGrenadeAttachment(), vecStart );

		if( this->GetState() == NPC_STATE_SCRIPT )
		{
			// Use a fixed velocity for grenades thrown in scripted state.
			// Grenades thrown from a script do not count against grenades remaining for the AI to use.
			Vector forward, up, vecThrow;

			this->GetVectors( &forward, NULL, &up );
			vecThrow = forward * 750 + up * 175;

			// This code is used by player allies now, so it's only "combine spawned" if the thrower isn't allied with the player.
			CBaseEntity *pGrenade = Fraggrenade_Create( vecStart, vec3_angle, vecThrow, vecSpin, this, COMBINE_GRENADE_TIMER, !this->IsPlayerAlly() );
			m_OnThrowGrenade.Set(pGrenade, pGrenade, this);
		}
		else
		{
			// Use the Velocity that AI gave us.
			CBaseEntity *pGrenade = Fraggrenade_Create( vecStart, vec3_angle, m_vecTossVelocity, vecSpin, this, COMBINE_GRENADE_TIMER, !this->IsPlayerAlly() );
			m_OnThrowGrenade.Set(pGrenade, pGrenade, this);
			AddGrenades(-1, pGrenade);
		}

		// wait six seconds before even looking again to see if a grenade can be thrown.
		m_flNextGrenadeCheck = gpGlobals->curtime + 6;

		m_iLastAnimEventHandled = pEvent->event;

		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::SetActivity( Activity NewActivity )
{
	BaseClass::SetActivity( NewActivity );

	m_iLastAnimEventHandled = -1;
}

//-----------------------------------------------------------------------------
// Purpose: Force the combine soldier to throw a grenade at the target
//			If I'm a combine elite, fire my combine ball at the target instead.
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::InputThrowGrenadeAtTarget( inputdata_t &inputdata )
{
	// Ignore if we're inside a scripted sequence
	if ( this->GetState() == NPC_STATE_SCRIPT && this->m_hCine )
		return;

	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.String(), this, inputdata.pActivator, inputdata.pCaller );
	if ( !pEntity )
	{
		DevMsg("%s (%s) received ThrowGrenadeAtTarget input, but couldn't find target entity '%s'\n", this->GetClassname(), this->GetDebugName(), inputdata.value.String() );
		return;
	}

	m_hForcedGrenadeTarget = pEntity;
	m_flNextGrenadeCheck = 0;

	this->ClearSchedule( "Told to throw grenade via input" );
}

//-----------------------------------------------------------------------------
// Purpose: Force the combine soldier to throw a grenade at the target using the gesture animation.
//			If I'm a combine elite, fire my combine ball at the target instead.
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::InputThrowGrenadeGestureAtTarget( inputdata_t &inputdata )
{
	// Ignore if we're inside a scripted sequence
	//if ( this->GetState() == NPC_STATE_SCRIPT && this->m_hCine )
	//	return;

	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.String(), this, inputdata.pActivator, inputdata.pCaller );
	if ( !pEntity )
	{
		DevMsg("%s (%s) received ThrowGrenadeGestureAtTarget input, but couldn't find target entity '%s'\n", this->GetClassname(), this->GetDebugName(), inputdata.value.String() );
		return;
	}

	m_hForcedGrenadeTarget = pEntity;
	m_flNextGrenadeCheck = 0;

	Vector vecTarget = m_hForcedGrenadeTarget->WorldSpaceCenter();

#if SHARED_COMBINE_ACTIVITIES
	if (IsAltFireCapable())
	{
		if (this->FVisible( m_hForcedGrenadeTarget ))
		{
			m_vecAltFireTarget = vecTarget;
			m_hForcedGrenadeTarget = NULL;

			int iLayer = this->AddGesture( ACT_GESTURE_COMBINE_AR2_ALTFIRE );
			if (iLayer != -1)
			{
				this->GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + this->GetLayerDuration( iLayer ) );
			}
		}
	}
	else
	{
		// If we can, throw a grenade at the target. 
		// Ignore grenade count / distance / etc
		if (CheckCanThrowGrenade( vecTarget ))
		{
			int iLayer = this->AddGesture( ACT_GESTURE_COMBINE_THROW_GRENADE );
			if (iLayer != -1)
			{
				this->GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + this->GetLayerDuration( iLayer ) );
			}
		}
	}
#else
	Warning("Gesture grenades/alt-fire not supported\n");
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
bool CAI_GrenadeUser<BASE_NPC>::CanAltFireEnemy( bool bUseFreeKnowledge )
{
	if (!HasGrenades())
		return false;

	if (!IsAltFireCapable())
		return false;

	if (!this->GetActiveWeapon())
		return false;

	if (this->IsCrouching())
		return false;

	if ( gpGlobals->curtime < m_flNextAltFireTime || gpGlobals->curtime < m_flNextGrenadeCheck )
		return false;

	if( !this->GetEnemy() )
		return false;

	if (!EntIsClass(this->GetActiveWeapon(), gm_isz_class_AR2) && !EntIsClass(this->GetActiveWeapon(), gm_isz_class_SMG1))
		return false;

	CBaseEntity *pEnemy = this->GetEnemy();

	Vector vecTarget;

	// Determine what point we're shooting at
	if( bUseFreeKnowledge )
	{
		vecTarget = this->GetEnemies()->LastKnownPosition( pEnemy ) + (pEnemy->GetViewOffset()*0.75);// approximates the chest
	}
	else
	{
		vecTarget = this->GetEnemies()->LastSeenPosition( pEnemy ) + (pEnemy->GetViewOffset()*0.75);// approximates the chest
	}

	// Trace a hull about the size of the combine ball (don't shoot through grates!)
	trace_t tr;

	Vector mins( -12, -12, -12 );
	Vector maxs( 12, 12, 12 );

	Vector vShootPosition = this->EyePosition();

	if ( this->GetActiveWeapon() )
	{
		this->GetActiveWeapon()->GetAttachment( "muzzle", vShootPosition );
	}

	// Trace a hull about the size of the combine ball.
	UTIL_TraceHull( vShootPosition, vecTarget, mins, maxs, MASK_COMBINE_BALL_LOS, this, COLLISION_GROUP_NONE, &tr );

	float flLength = (vShootPosition - vecTarget).Length();

	flLength *= tr.fraction;

	// If the ball can travel at least 65% of the distance to the player then let the NPC shoot it.
	// (unless it hit the world)
	if( tr.fraction >= 0.65 && (!tr.m_pEnt || !tr.m_pEnt->IsWorld()) && flLength > 128.0f )
	{
		// Target is valid
		m_vecAltFireTarget = vecTarget;
		return true;
	}


	// Check again later
	m_vecAltFireTarget = vec3_origin;
	m_flNextGrenadeCheck = gpGlobals->curtime + 1.0f;
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::DelayAltFireAttack( float flDelay )
{
	float flNextAltFire = gpGlobals->curtime + flDelay;

	if( flNextAltFire > m_flNextAltFireTime )
	{
		// Don't let this delay order preempt a previous request to wait longer.
		m_flNextAltFireTime = flNextAltFire;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::DelaySquadAltFireAttack( float flDelay )
{
	// Make sure to delay my own alt-fire attack.
	DelayAltFireAttack( flDelay );

	AISquadIter_t iter;
	CAI_Squad *pSquad = this->GetSquad();
	CAI_BaseNPC *pSquadmate = pSquad ? pSquad->GetFirstMember( &iter ) : NULL;
	while ( pSquadmate )
	{
		CAI_GrenadeUser *pUser = dynamic_cast<CAI_GrenadeUser*>(pSquadmate);
		if( pUser && pUser->IsAltFireCapable() )
		{
			pUser->DelayAltFireAttack( flDelay );
		}

		pSquadmate = pSquad->GetNextMember( &iter );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
bool CAI_GrenadeUser<BASE_NPC>::CanGrenadeEnemy( bool bUseFreeKnowledge )
{
	CBaseEntity *pEnemy = this->GetEnemy();

	Assert( pEnemy != NULL );

	if( pEnemy )
	{
		// I'm not allowed to throw grenades during dustoff
		if ( this->IsCurSchedule(SCHED_DROPSHIP_DUSTOFF) )
			return false;

		if( bUseFreeKnowledge )
		{
			// throw to where we think they are.
			return CanThrowGrenade( this->GetEnemies()->LastKnownPosition( pEnemy ) );
		}
		else
		{
			// hafta throw to where we last saw them.
			return CanThrowGrenade( this->GetEnemies()->LastSeenPosition( pEnemy ) );
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the combine has grenades, hasn't checked lately, and
//			can throw a grenade at the target point.
// Input  : &vecTarget - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
template <class BASE_NPC>
bool CAI_GrenadeUser<BASE_NPC>::CanThrowGrenade( const Vector &vecTarget )
{
	if( m_iNumGrenades < 1 )
	{
		// Out of grenades!
		return false;
	}

	if (!IsGrenadeCapable())
	{
		// Must be capable of throwing grenades
		return false;
	}

	if ( gpGlobals->curtime < m_flNextGrenadeCheck )
	{
		// Not allowed to throw another grenade right now.
		return false;
	}

	float flDist;
	flDist = ( vecTarget - this->GetAbsOrigin() ).Length();

	if( flDist > 1024 || flDist < 128 )
	{
		// Too close or too far!
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return false;
	}

	// -----------------------
	// If moving, don't check.
	// -----------------------
	if ( this->m_flGroundSpeed != 0 )
		return false;

	// ---------------------------------------------------------------------
	// Are any of my squad members near the intended grenade impact area?
	// ---------------------------------------------------------------------
	CAI_Squad *pSquad = this->GetSquad();
	if ( pSquad )
	{
		if (pSquad->SquadMemberInRange( vecTarget, COMBINE_MIN_GRENADE_CLEAR_DIST ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.

			// Tell my squad members to clear out so I can get a grenade in
			// Mapbase uses a new context here that gets all nondescript allies away since this code is shared between Combine and non-Combine now.
			CSoundEnt::InsertSound( SOUND_MOVE_AWAY | SOUND_CONTEXT_OWNER_ALLIES, vecTarget, COMBINE_MIN_GRENADE_CLEAR_DIST, 0.1, this );
			return false;
		}
	}

	CHintCriteria hintCriteria;
	hintCriteria.SetHintType( HINT_TACTICAL_GRENADE_THROW );
	hintCriteria.SetFlag( bits_HINT_NPC_IN_NODE_FOV );
	hintCriteria.SetGroup( this->GetHintGroup() );
	hintCriteria.AddIncludePosition( this->GetAbsOrigin(), 1024 );

	if (this->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT)
		hintCriteria.SetFlag( bits_HINT_NODE_REPORT_FAILURES );
	
	// If there's a grenade throw hint nearby, try using it
	CAI_Hint *pHint = CAI_HintManager::FindHint( this, vecTarget, hintCriteria );
	if ( pHint )
	{
		if ( CheckCanThrowGrenade( pHint->GetAbsOrigin() ) )
		{
			return true;
		}
		else
		{
			DevMsg( this, "Unable to throw grenade at hint %s\n", pHint->GetDebugName() );
		}
	}

	return CheckCanThrowGrenade( vecTarget );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the combine can throw a grenade at the specified target point
// Input  : &vecTarget - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
template <class BASE_NPC>
bool CAI_GrenadeUser<BASE_NPC>::CheckCanThrowGrenade( const Vector &vecTarget )
{
	//NDebugOverlay::Line( this->EyePosition(), vecTarget, 0, 255, 0, false, 5 );

	// ---------------------------------------------------------------------
	// Check that throw is legal and clear
	// ---------------------------------------------------------------------
	// FIXME: this is only valid for hand grenades, not RPG's
	Vector vecToss;
	Vector vecMins = -Vector(4,4,4);
	Vector vecMaxs = Vector(4,4,4);
	if( this->FInViewCone( vecTarget ) && CBaseEntity::FVisible( vecTarget ) )
	{
		vecToss = VecCheckThrow( this, this->EyePosition(), vecTarget, COMBINE_GRENADE_THROW_SPEED, 1.0, &vecMins, &vecMaxs );
	}
	else
	{
		// Have to try a high toss. Do I have enough room?
		trace_t tr;
		AI_TraceLine( this->EyePosition(), this->EyePosition() + Vector( 0, 0, 64 ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		if( tr.fraction != 1.0 )
		{
			return false;
		}

		vecToss = VecCheckToss( this, this->EyePosition(), vecTarget, -1, 1.0, true, &vecMins, &vecMaxs );
	}

	if ( vecToss != vec3_origin )
	{
		m_vecTossVelocity = vecToss;

		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // 1/3 second.
		return true;
	}
	else
	{
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: This was copied from soldier code for general AI grenades.
//			
//			"Soldiers use CAN_RANGE_ATTACK2 to indicate whether they can throw
//			a grenade. Because they check only every half-second or so, this
//			condition must persist until it is updated again by the code
//			that determines whether a grenade can be thrown, so prevent the 
//			base class from clearing it out. (sjb)"
//-----------------------------------------------------------------------------
template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::ClearAttackConditions()
{
	bool fCanRangeAttack2 = IsGrenadeCapable() && this->HasCondition( COND_CAN_RANGE_ATTACK2 );

	// Call the base class.
	BaseClass::ClearAttackConditions();

	if( fCanRangeAttack2 )
	{
		// We don't allow the base class to clear this condition because we
		// don't sense for it every frame.
		this->SetCondition( COND_CAN_RANGE_ATTACK2 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template <class BASE_NPC>
bool CAI_GrenadeUser<BASE_NPC>::FValidateHintType( CAI_Hint *pHint )
{
	if ( pHint->HintType() == HINT_TACTICAL_GRENADE_THROW )
		return true;

	return BaseClass::FValidateHintType( pHint );
}

//-----------------------------------------------------------------------------
// Purpose: Drops grenades and alt-fire items on death. Based on code from npc_combines.cpp and npc_combine.cpp
//-----------------------------------------------------------------------------
template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::DropGrenadeItemsOnDeath( const CTakeDamageInfo &info, CBasePlayer *pPlayer )
{
	// Elites drop alt-fire ammo, so long as they weren't killed by dissolving.
	if( IsAltFireCapable() && ShouldDropAltFire() )
	{
		CBaseEntity *pItem;
		if (this->GetActiveWeapon() && FClassnameIs( this->GetActiveWeapon(), "weapon_smg1" ))
			pItem = this->DropItem( "item_ammo_smg1_grenade", this->WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );
		else
			pItem = this->DropItem( "item_ammo_ar2_altfire", this->WorldSpaceCenter() + RandomVector( -4, 4 ), RandomAngle( 0, 360 ) );

		if ( pItem )
		{
			IPhysicsObject *pObj = pItem->VPhysicsGetObject();

			if ( pObj )
			{
				Vector			vel		= RandomVector( -64.0f, 64.0f );
				AngularImpulse	angImp	= RandomAngularImpulse( -300.0f, 300.0f );

				vel[2] = 0.0f;
				pObj->AddVelocity( &vel, &angImp );
			}

			if( info.GetDamageType() & DMG_DISSOLVE )
			{
				CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating*>(pItem);

				if( pAnimating )
				{
					pAnimating->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
				}
			}
			else
			{
				WeaponManager_AddManaged( pItem );
			}
		}
	}
	
	if ( IsGrenadeCapable() )
	{
		if ( ShouldDropGrenades() )
		{
			CHalfLife2 *pHL2GameRules = static_cast<CHalfLife2 *>(g_pGameRules);

			// Attempt to drop a grenade
			if ( pHL2GameRules->NPC_ShouldDropGrenade( pPlayer ) )
			{
				this->DropItem( "weapon_frag", this->WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );
				pHL2GameRules->NPC_DroppedGrenade();
			}
		}

		// if I was killed before I could finish throwing my grenade, drop
		// a grenade item that the player can retrieve.
		if (this->GetActivity() == ACT_RANGE_ATTACK2 && ShouldDropInterruptedGrenades())
		{
			if( m_iLastAnimEventHandled != COMBINE_AE_GREN_TOSS )
			{
				// Drop the grenade as an item.
				Vector vecStart;
				this->GetAttachment( GetGrenadeAttachment(), vecStart );

				CBaseEntity *pItem = this->DropItem( "weapon_frag", vecStart, RandomAngle(0,360) );

				if ( pItem )
				{
					IPhysicsObject *pObj = pItem->VPhysicsGetObject();

					if ( pObj )
					{
						Vector			vel;
						vel.x = random->RandomFloat( -100.0f, 100.0f );
						vel.y = random->RandomFloat( -100.0f, 100.0f );
						vel.z = random->RandomFloat( 800.0f, 1200.0f );
						AngularImpulse	angImp	= RandomAngularImpulse( -300.0f, 300.0f );

						vel[2] = 0.0f;
						pObj->AddVelocity( &vel, &angImp );
					}

					// In the Citadel we need to dissolve this
					if ( PlayerHasMegaPhysCannon() && GlobalEntity_GetCounter("super_phys_gun") != 1 )
					{
						CBaseCombatWeapon *pWeapon = static_cast<CBaseCombatWeapon *>(pItem);

						pWeapon->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Task helpers
//-----------------------------------------------------------------------------
template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::StartTask_FaceAltFireTarget( const Task_t *pTask )
{
	this->SetIdealActivity( (Activity)(int)pTask->flTaskData );
	this->GetMotor()->SetIdealYawToTargetAndUpdate( m_vecAltFireTarget, AI_KEEP_YAW_SPEED );
}

template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::StartTask_GetPathToForced( const Task_t *pTask )
{
	if ( !m_hForcedGrenadeTarget )
	{
		this->TaskFail(FAIL_NO_ENEMY);
		return;
	}

	float flMaxRange = 2000;
	float flMinRange = 0;

	Vector vecEnemy = m_hForcedGrenadeTarget->GetAbsOrigin();
	Vector vecEnemyEye = vecEnemy + m_hForcedGrenadeTarget->GetViewOffset();

	Vector posLos;
	bool found = false;

	if ( this->GetTacticalServices()->FindLateralLos( vecEnemyEye, &posLos ) )
	{
		float dist = ( posLos - vecEnemyEye ).Length();
		if ( dist < flMaxRange && dist > flMinRange )
			found = true;
	}

	if ( !found && this->GetTacticalServices()->FindLos( vecEnemy, vecEnemyEye, flMinRange, flMaxRange, 1.0, &posLos ) )
	{
		found = true;
	}

	if ( !found )
	{
		this->TaskFail( FAIL_NO_SHOOT );
	}
	else
	{
		// else drop into run task to offer an interrupt
		this->m_vInterruptSavePosition = posLos;
	}
}

template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::StartTask_DeferSquad( const Task_t *pTask )
{
	CAI_Squad *pSquad = this->GetSquad();
	if ( pSquad )
	{
		// iterate my squad and stop everyone from throwing grenades for a little while.
		AISquadIter_t iter;

		CAI_BaseNPC *pSquadmate = pSquad ? pSquad->GetFirstMember( &iter ) : NULL;
		while ( pSquadmate )
		{
			pSquadmate->DelayGrenadeCheck(5);

			pSquadmate = pSquad->GetNextMember( &iter );
		}
	}

	this->TaskComplete();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::RunTask_FaceAltFireTarget( const Task_t *pTask )
{
	this->GetMotor()->SetIdealYawToTargetAndUpdate( m_vecAltFireTarget, AI_KEEP_YAW_SPEED );

	// New Mapbase thing that fixes forced alt-fires not changing weapon yaw/pitch
	this->SetAim( m_vecAltFireTarget - this->Weapon_ShootPosition() );

	if (this->IsActivityFinished())
	{
		this->TaskComplete();
	}
}

template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::RunTask_GetPathToForced( const Task_t *pTask )
{
	if ( !m_hForcedGrenadeTarget )
	{
		this->TaskFail(FAIL_NO_ENEMY);
		return;
	}

	if ( this->GetTaskInterrupt() > 0 )
	{
		this->ClearTaskInterrupt();

		Vector vecEnemy = m_hForcedGrenadeTarget->GetAbsOrigin();
		AI_NavGoal_t goal( this->m_vInterruptSavePosition, ACT_RUN, AIN_HULL_TOLERANCE );

		this->GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET );
		this->GetNavigator()->SetArrivalDirection( vecEnemy - goal.dest );
	}
	else
	{
		this->TaskInterrupt();
	}
}

template <class BASE_NPC>
void CAI_GrenadeUser<BASE_NPC>::RunTask_FaceTossDir( const Task_t *pTask )
{
	// project a point along the toss vector and turn to face that point.
	this->GetMotor()->SetIdealYawToTargetAndUpdate( this->GetLocalOrigin() + m_vecTossVelocity * 64, AI_KEEP_YAW_SPEED );

	if ( this->FacingIdeal() )
	{
		this->TaskComplete( true );
	}
}

#endif
