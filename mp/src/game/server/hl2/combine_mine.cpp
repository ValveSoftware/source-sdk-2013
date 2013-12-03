//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "soundenvelope.h"
#include "Sprite.h"
#include "entitylist.h"
#include "ai_basenpc.h"
#include "soundent.h"
#include "explode.h"
#include "physics.h"
#include "physics_saverestore.h"
#include "combine_mine.h"
#include "movevars_shared.h"
#include "vphysics/constraints.h"
#include "ai_hint.h"

enum
{
	MINE_STATE_DORMANT = 0,
	MINE_STATE_DEPLOY,		// Try to lock down and arm
	MINE_STATE_CAPTIVE,		// Held in the physgun
	MINE_STATE_ARMED,		// Locked down and looking for targets
	MINE_STATE_TRIGGERED,	// No turning back. I'm going to explode when I touch something.
	MINE_STATE_LAUNCHED,	// Similar. Thrown from physgun.
};

// for the Modification keyfield
enum
{
	MINE_MODIFICATION_NORMAL  = 0,
	MINE_MODIFICATION_CAVERN,
};

// the citizen modified skins for the mine (inclusive):
#define MINE_CITIZEN_SKIN_MIN 1
#define MINE_CITIZEN_SKIN_MAX 2

char *pszMineStateNames[] =
{
	"Dormant",
	"Deploy",
	"Captive",
	"Armed",
	"Triggered",
	"Launched",
};

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// After this many flips, seriously cut the frequency with which you try.
#define BOUNCEBOMB_MAX_FLIPS	5

// Approximate radius of the bomb's model
#define BOUNCEBOMB_RADIUS		24

BEGIN_DATADESC( CBounceBomb )
	DEFINE_THINKFUNC( ExplodeThink ),
	DEFINE_ENTITYFUNC( ExplodeTouch ),
	DEFINE_THINKFUNC( SearchThink ),
	DEFINE_THINKFUNC( BounceThink ),
	DEFINE_THINKFUNC( SettleThink ),
	DEFINE_THINKFUNC( CaptiveThink ),
	DEFINE_THINKFUNC( CavernBounceThink ),

	DEFINE_SOUNDPATCH( m_pWarnSound ),

	DEFINE_KEYFIELD( m_flExplosionDelay,	FIELD_FLOAT, "ExplosionDelay" ),
	DEFINE_KEYFIELD( m_bBounce,			FIELD_BOOLEAN, "Bounce" ),

	DEFINE_FIELD( m_bAwake, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hNearestNPC, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hSprite, FIELD_EHANDLE ),
	DEFINE_FIELD( m_LastSpriteColor, FIELD_COLOR32 ),

	DEFINE_FIELD( m_flHookPositions, FIELD_FLOAT ),
	DEFINE_FIELD( m_iHookN, FIELD_INTEGER ),
	DEFINE_FIELD( m_iHookE, FIELD_INTEGER ),
	DEFINE_FIELD( m_iHookS, FIELD_INTEGER ),
	DEFINE_FIELD( m_iAllHooks, FIELD_INTEGER ),

	DEFINE_KEYFIELD( m_bLockSilently, FIELD_BOOLEAN, "LockSilently" ),
	DEFINE_FIELD( m_bFoeNearest, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flIgnoreWorldTime, FIELD_TIME ),
	DEFINE_KEYFIELD( m_bDisarmed, FIELD_BOOLEAN, "StartDisarmed" ),
	DEFINE_KEYFIELD( m_iModification, FIELD_INTEGER, "Modification" ),

	DEFINE_FIELD( m_bPlacedByPlayer, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bHeldByPhysgun, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_iFlipAttempts, FIELD_INTEGER ),

	DEFINE_FIELD( m_flTimeGrabbed, FIELD_TIME ),
	DEFINE_FIELD( m_iMineState, FIELD_INTEGER ),

	// Physics Influence
	DEFINE_FIELD( m_hPhysicsAttacker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flLastPhysicsInfluenceTime, FIELD_TIME ),

	DEFINE_PHYSPTR( m_pConstraint ),

	DEFINE_OUTPUT( m_OnPulledUp, "OnPulledUp" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disarm", InputDisarm ),

END_DATADESC()

string_t CBounceBomb::gm_iszFloorTurretClassname;
string_t CBounceBomb::gm_iszGroundTurretClassname;

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::Precache()
{
	PrecacheModel("models/props_combine/combine_mine01.mdl");

	PrecacheScriptSound( "NPC_CombineMine.Hop" );
	PrecacheScriptSound( "NPC_CombineMine.FlipOver" );
	PrecacheScriptSound( "NPC_CombineMine.TurnOn" );
	PrecacheScriptSound( "NPC_CombineMine.TurnOff" );
	PrecacheScriptSound( "NPC_CombineMine.OpenHooks" );
	PrecacheScriptSound( "NPC_CombineMine.CloseHooks" );

	PrecacheScriptSound( "NPC_CombineMine.ActiveLoop" );

	PrecacheModel( "sprites/glow01.vmt" );

	gm_iszFloorTurretClassname = AllocPooledString( "npc_turret_floor" );
	gm_iszGroundTurretClassname = AllocPooledString( "npc_turret_ground" );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::Spawn()
{
	Precache();

	Wake( false );

	SetModel("models/props_combine/combine_mine01.mdl");

	SetSolid( SOLID_VPHYSICS );

	m_hSprite.Set( NULL );
	m_takedamage = DAMAGE_EVENTS_ONLY;

	// Find my feet!
	m_iHookN = LookupPoseParameter( "blendnorth" );
	m_iHookE = LookupPoseParameter( "blendeast" );
	m_iHookS = LookupPoseParameter( "blendsouth" );
	m_iAllHooks = LookupPoseParameter( "blendstates" );
	m_flHookPositions = 0;

	SetHealth( 100 );

	m_bBounce = true;

	SetSequence( SelectWeightedSequence( ACT_IDLE ) );

	OpenHooks( true );

	m_bHeldByPhysgun = false;	

	m_iFlipAttempts = 0;

	if( !GetParent() )
	{
		// Create vphysics now if I'm not being carried.
		CreateVPhysics();
	}

	m_flTimeGrabbed = FLT_MAX;

	if( m_bDisarmed )
	{
		SetMineState( MINE_STATE_DORMANT );
	}
	else
	{
		SetMineState( MINE_STATE_DEPLOY );
	}

	// default to a different skin for cavern turrets (unless explicitly overridden)
	if ( m_iModification == MINE_MODIFICATION_CAVERN )
	{
		// look for this value in the first datamap
		// loop through the data description list, restoring each data desc block
		datamap_t *dmap = GetDataDescMap();

		bool bFoundSkin = false;
		// search through all the readable fields in the data description, looking for a match
		for ( int i = 0; i < dmap->dataNumFields; ++i )
		{
			if ( dmap->dataDesc[i].flags & (FTYPEDESC_OUTPUT | FTYPEDESC_KEY) )
			{
				if ( !Q_stricmp(dmap->dataDesc[i].externalName, "Skin") )
				{
					bFoundSkin = true; 
					break;
				}
			}
		}

		if (!bFoundSkin)
		{
			// select a random skin for the mine. Actually, we'll cycle through the available skins 
			// using a static variable to provide better distribution. The static isn't saved but
			// really it's only cosmetic.
			static unsigned int nextSkin = MINE_CITIZEN_SKIN_MIN;
			m_nSkin = nextSkin;
			// increment the skin for next time
			nextSkin = (nextSkin >= MINE_CITIZEN_SKIN_MAX) ? MINE_CITIZEN_SKIN_MIN : nextSkin + 1;
		}	

		// pretend like the player set me down.
		m_bPlacedByPlayer = true;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::OnRestore()
{
	BaseClass::OnRestore();
	if ( gpGlobals->eLoadType == MapLoad_Transition && !m_hSprite && m_LastSpriteColor.GetRawColor() != 0 )
	{
		UpdateLight( true, m_LastSpriteColor.r(), m_LastSpriteColor.g(), m_LastSpriteColor.b(), m_LastSpriteColor.a() );
	}

	if( VPhysicsGetObject() )
	{
		VPhysicsGetObject()->Wake();
	}
}
	
//---------------------------------------------------------
//---------------------------------------------------------
int CBounceBomb::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();
	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr), "%s", pszMineStateNames[m_iMineState] );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::SetMineState( int iState )
{
	m_iMineState = iState;

	switch( iState )
	{
	case MINE_STATE_DORMANT:
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			controller.SoundChangeVolume( m_pWarnSound, 0.0, 0.1 );
			UpdateLight( false, 0, 0, 0, 0 );
			SetThink( NULL );
		}
		break;

	case MINE_STATE_CAPTIVE:
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			controller.SoundChangeVolume( m_pWarnSound, 0.0, 0.2 );

			// Unhook
			unsigned int flags = VPhysicsGetObject()->GetCallbackFlags();
			VPhysicsGetObject()->SetCallbackFlags( flags | CALLBACK_GLOBAL_TOUCH_STATIC );
			OpenHooks();
			physenv->DestroyConstraint( m_pConstraint );
			m_pConstraint = NULL;

			UpdateLight( true, 0, 0, 255, 190 );
			SetThink( &CBounceBomb::CaptiveThink );
			SetNextThink( gpGlobals->curtime + 0.1f );
			SetTouch( NULL );
		}
		break;

	case MINE_STATE_DEPLOY:
		OpenHooks( true );
		UpdateLight( true, 0, 0, 255, 190 );
		SetThink( &CBounceBomb::SettleThink );
		SetTouch( NULL );
		SetNextThink( gpGlobals->curtime + 0.1f );
		break;

	case MINE_STATE_ARMED:
		UpdateLight( false, 0, 0, 0, 0 );
		SetThink( &CBounceBomb::SearchThink );
		SetNextThink( gpGlobals->curtime + 0.1f );
		break;

	case MINE_STATE_TRIGGERED:
		{
			OpenHooks();

			if( m_pConstraint )
			{
				physenv->DestroyConstraint( m_pConstraint );
				m_pConstraint = NULL;
			}

			// Scare NPC's
			CSoundEnt::InsertSound( SOUND_DANGER, GetAbsOrigin(), 300, 1.0f, this );

			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			controller.SoundChangeVolume( m_pWarnSound, 0.0, 0.2 );

			SetTouch( &CBounceBomb::ExplodeTouch );
			unsigned int flags = VPhysicsGetObject()->GetCallbackFlags();
			VPhysicsGetObject()->SetCallbackFlags( flags | CALLBACK_GLOBAL_TOUCH_STATIC );

			Vector vecNudge;

			vecNudge.x = random->RandomFloat( -1, 1 );
			vecNudge.y = random->RandomFloat( -1, 1 );
			vecNudge.z = 1.5;
			vecNudge *= 350;

			VPhysicsGetObject()->Wake();
			VPhysicsGetObject()->ApplyForceCenter( vecNudge );

			float x, y;
			x = 10 + random->RandomFloat( 0, 20 );
			y = 10 + random->RandomFloat( 0, 20 );

			VPhysicsGetObject()->ApplyTorqueCenter( AngularImpulse( x, y, 0 ) );

			// Since we just nudged the mine, ignore collisions with the world until
			// the mine is in the air. We only want to explode if the player tries to 
			// run over the mine before it jumps up.
			m_flIgnoreWorldTime = gpGlobals->curtime + 1.0;
			UpdateLight( true, 255, 0, 0, 190 );

			// use the correct bounce behavior
			if (m_iModification == MINE_MODIFICATION_CAVERN)
			{
				SetThink ( &CBounceBomb::CavernBounceThink );
				SetNextThink( gpGlobals->curtime + 0.15 );
			}
			else
			{
				SetThink( &CBounceBomb::BounceThink );
				SetNextThink( gpGlobals->curtime + 0.5 );
			}
		}
		break;

	case MINE_STATE_LAUNCHED:
		{
			UpdateLight( true, 255, 0, 0, 190 );
			SetThink( NULL );
			SetNextThink( gpGlobals->curtime + 0.5 );

			SetTouch( &CBounceBomb::ExplodeTouch );
			unsigned int flags = VPhysicsGetObject()->GetCallbackFlags();
			VPhysicsGetObject()->SetCallbackFlags( flags | CALLBACK_GLOBAL_TOUCH_STATIC );
		}
		break;

	default:
		DevMsg("**Unknown Mine State: %d\n", iState );
		break;
	}
}

//---------------------------------------------------------
// Bouncbomb flips to try to right itself, try to get off
// of and object that it's not allowed to clamp to, or 
// to get away from a hint node that inhibits placement
// of mines.
//---------------------------------------------------------
void CBounceBomb::Flip( const Vector &vecForce, const AngularImpulse &torque )
{
	if( m_iFlipAttempts > BOUNCEBOMB_MAX_FLIPS )
	{
		// Not allowed to try anymore.
		SetThink(NULL);
		return;
	}

	EmitSound( "NPC_CombineMine.FlipOver" );
	VPhysicsGetObject()->ApplyForceCenter( vecForce );
	VPhysicsGetObject()->ApplyTorqueCenter( torque );
	m_iFlipAttempts++;
}

//---------------------------------------------------------
//---------------------------------------------------------
#define MINE_MIN_PROXIMITY_SQR	676 // 27 inches
bool CBounceBomb::IsValidLocation() 
{
	CBaseEntity *pAvoidObject = NULL;
	float flAvoidForce = 0.0f;
	CAI_Hint *pHint;
	CHintCriteria criteria;
	criteria.SetHintType( HINT_WORLD_INHIBIT_COMBINE_MINES );
	criteria.SetFlag( bits_HINT_NODE_NEAREST );
	criteria.AddIncludePosition( GetAbsOrigin(), 12.0f * 15.0f );
	pHint = CAI_HintManager::FindHint( GetAbsOrigin(), criteria );

	if( pHint )
	{
		pAvoidObject = pHint;
		flAvoidForce = 120.0f;
	}
	else
	{
		// Look for other mines that are too close to me.
		CBaseEntity *pEntity = gEntList.FirstEnt();
		Vector vecMyPosition = GetAbsOrigin();
		while( pEntity )
		{
			if( pEntity->m_iClassname == m_iClassname && pEntity != this )
			{
				// Don't lock down if I'm near a mine that's already locked down.
				if( vecMyPosition.DistToSqr(pEntity->GetAbsOrigin()) < MINE_MIN_PROXIMITY_SQR )
				{
					pAvoidObject = pEntity;
					flAvoidForce = 60.0f;
					break;
				}
			}

			pEntity = gEntList.NextEnt( pEntity );
		}
	}

	if( pAvoidObject )
	{
		// Build a force vector to push us away from the inhibitor.
		// Start by pushing upwards.
		Vector vecForce = Vector( 0, 0, VPhysicsGetObject()->GetMass() * 200.0f );

		// Now add some force in the direction that takes us away from the inhibitor.
		Vector vecDir = GetAbsOrigin() - pAvoidObject->GetAbsOrigin();
		vecDir.z = 0.0f;
		VectorNormalize( vecDir );
		vecForce += vecDir * VPhysicsGetObject()->GetMass() * flAvoidForce;

		Flip( vecForce, AngularImpulse( 100, 0, 0 ) );

		// Tell the code that asked that this position isn't valid.
		return false;
	}

	return true;
}

//---------------------------------------------------------
// Release the spikes
//---------------------------------------------------------
void CBounceBomb::BounceThink()
{
	SetNextThink( gpGlobals->curtime + 0.1 );
	StudioFrameAdvance();

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	
	if ( pPhysicsObject != NULL )
	{
		const float MINE_MAX_JUMP_HEIGHT = 200;

		// Figure out how much headroom the mine has, and hop to within a few inches of that.
		trace_t tr;
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, MINE_MAX_JUMP_HEIGHT ), MASK_SHOT, this, COLLISION_GROUP_INTERACTIVE, &tr );

		float height;

		if( tr.m_pEnt && tr.m_pEnt->VPhysicsGetObject() )
		{
			// Physics object resting on me. Jump as hard as allowed to try to knock it away.
			height = MINE_MAX_JUMP_HEIGHT;
		}
		else
		{
			height = tr.endpos.z - GetAbsOrigin().z;
			height -= BOUNCEBOMB_RADIUS;
			if ( height < 0.1 )
				height = 0.1;
		}

		float time = sqrt( height / (0.5 * GetCurrentGravity()) );
		float velocity = GetCurrentGravity() * time;

		// or you can just AddVelocity to the object instead of ApplyForce
		float force = velocity * pPhysicsObject->GetMass();

		Vector up;

		GetVectors( NULL, NULL, &up );
		pPhysicsObject->Wake();
		pPhysicsObject->ApplyForceCenter( up * force );

		pPhysicsObject->ApplyTorqueCenter( AngularImpulse( random->RandomFloat( 5, 25 ), random->RandomFloat( 5, 25 ), 0 ) );
		

		if( m_hNearestNPC )
		{
			Vector vecPredict = m_hNearestNPC->GetSmoothedVelocity();

			pPhysicsObject->ApplyForceCenter( vecPredict * 10 );
		}

		EmitSound( "NPC_CombineMine.Hop" );
		SetThink( NULL );
	}
}


//---------------------------------------------------------
// A different bounce behavior for the citizen-modified mine. Detonates at the top of its apex, 
// and does not attempt to track enemies.
//---------------------------------------------------------
void CBounceBomb::CavernBounceThink()
{
	SetNextThink( gpGlobals->curtime + 0.1 );
	StudioFrameAdvance();

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if ( pPhysicsObject != NULL )
	{
		const float MINE_MAX_JUMP_HEIGHT = 78;

		// Figure out how much headroom the mine has, and hop to within a few inches of that.
		trace_t tr;
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, MINE_MAX_JUMP_HEIGHT ), MASK_SHOT, this, COLLISION_GROUP_INTERACTIVE, &tr );

		float height;

		if( tr.m_pEnt && tr.m_pEnt->VPhysicsGetObject() )
		{
			// Physics object resting on me. Jump as hard as allowed to try to knock it away.
			height = MINE_MAX_JUMP_HEIGHT;
		}
		else
		{
			height = tr.endpos.z - GetAbsOrigin().z;
			height -= BOUNCEBOMB_RADIUS;
			if ( height < 0.1 )
				height = 0.1;
		}

		float time = sqrt( height / (0.5 * GetCurrentGravity()) );
		float velocity = GetCurrentGravity() * time;

		// or you can just AddVelocity to the object instead of ApplyForce
		float force = velocity * pPhysicsObject->GetMass();

		Vector up;

		GetVectors( NULL, NULL, &up );
		
		pPhysicsObject->Wake();
		pPhysicsObject->ApplyForceCenter( up * force );
		if( m_hNearestNPC )
		{
			Vector vecPredict = m_hNearestNPC->GetSmoothedVelocity();

			pPhysicsObject->ApplyForceCenter( vecPredict * (pPhysicsObject->GetMass() * 0.65f) );
		}

		pPhysicsObject->ApplyTorqueCenter( AngularImpulse( random->RandomFloat( 15, 40 ), random->RandomFloat( 15, 40 ), random->RandomFloat( 30, 60 ) ) );
		
		EmitSound( "NPC_CombineMine.Hop" );

		SetThink( &CBounceBomb::ExplodeThink );
		SetNextThink( gpGlobals->curtime + 0.33f );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::CaptiveThink()
{
	SetNextThink( gpGlobals->curtime + 0.05 );
	StudioFrameAdvance();

	float phase = fabs( sin( gpGlobals->curtime * 4.0f ) );
	phase *= BOUNCEBOMB_HOOK_RANGE;
	SetPoseParameter( m_iAllHooks, phase );
	return;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::SettleThink()
{
	SetNextThink( gpGlobals->curtime + 0.05 );
	StudioFrameAdvance();

	if( GetParent() )
	{
		// A scanner or something is carrying me. Just keep checking back.
		return;
	}

	// Not being carried.
	if( !VPhysicsGetObject() )
	{
		// Probably was just dropped. Get physics going.
		CreateVPhysics();

		if( !VPhysicsGetObject() )
		{
			Msg("**** Can't create vphysics for combine_mine!\n" );
			UTIL_Remove( this );
			return;
		}

		VPhysicsGetObject()->Wake();
		return;
	}

	if( !m_bDisarmed )
	{
		if( VPhysicsGetObject()->IsAsleep() && !(VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD) )
		{
			// If i'm not resting on the world, jump randomly.
			trace_t tr;
			UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector( 0, 0, 1024 ), MASK_SHOT|CONTENTS_GRATE, this, COLLISION_GROUP_NONE, &tr );

			bool bHop = false;
			if( tr.m_pEnt )
			{
				IPhysicsObject *pPhysics = tr.m_pEnt->VPhysicsGetObject();

				if( pPhysics && pPhysics->GetMass() <= 1000 )
				{
					// Light physics objects can be moved out from under the mine.
					bHop = true;
				}
				else if( tr.m_pEnt->m_takedamage != DAMAGE_NO )
				{
					// Things that can be harmed can likely be broken.
					bHop = true;
				}

				if( bHop )
				{
					Vector vecForce;
					vecForce.x = random->RandomFloat( -1000, 1000 );
					vecForce.y = random->RandomFloat( -1000, 1000 );
					vecForce.z = 2500;

					AngularImpulse torque( 160, 0, 160 );

					Flip( vecForce, torque );
					return;
				}

				// Check for upside-down
				Vector vecUp;
				GetVectors( NULL, NULL, &vecUp );
				if( vecUp.z <= 0.8 )
				{
					// Landed upside down. Right self
					Vector vecForce( 0, 0, 2500 );
					Flip( vecForce, AngularImpulse( 60, 0, 0 ) );
					return;
				}
			}

			// Check to make sure I'm not in a forbidden location
			if( !IsValidLocation() )
			{
				return;
			}

			// Lock to what I'm resting on
			constraint_ballsocketparams_t ballsocket;
			ballsocket.Defaults();
			ballsocket.constraint.Defaults();
			ballsocket.constraint.forceLimit = lbs2kg(1000);
			ballsocket.constraint.torqueLimit = lbs2kg(1000);
			ballsocket.InitWithCurrentObjectState( g_PhysWorldObject, VPhysicsGetObject(), GetAbsOrigin() );
			m_pConstraint = physenv->CreateBallsocketConstraint( g_PhysWorldObject, VPhysicsGetObject(), NULL, ballsocket );
			CloseHooks();

			SetMineState( MINE_STATE_ARMED );
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
int CBounceBomb::OnTakeDamage( const CTakeDamageInfo &info )
{
	if( m_pConstraint || !VPhysicsGetObject())
	{
		return false;
	}

	VPhysicsTakeDamage( info );
	return true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::UpdateLight( bool bTurnOn, unsigned int r, unsigned int g, unsigned int b, unsigned int a )
{
	if( bTurnOn )
	{
		Assert( a > 0 );

		// Throw the old sprite away
		if( m_hSprite )
		{
			UTIL_Remove( m_hSprite );
			m_hSprite.Set( NULL );
		}

		if( !m_hSprite.Get() )
		{
			Vector up;
			GetVectors( NULL, NULL, &up );

			// Light isn't on.
			m_hSprite = CSprite::SpriteCreate( "sprites/glow01.vmt", GetAbsOrigin() + up * 10.0f, false );
			CSprite *pSprite = (CSprite *)m_hSprite.Get();

			if( m_hSprite )
			{
				pSprite->SetParent( this );		
				pSprite->SetTransparency( kRenderTransAdd, r, g, b, a, kRenderFxNone );
				pSprite->SetScale( 0.35, 0.0 );
			}
		}
		else
		{
			// Update color
			CSprite *pSprite = (CSprite *)m_hSprite.Get();
			pSprite->SetTransparency( kRenderTransAdd, r, g, b, a, kRenderFxNone );
		}
	}

	if( !bTurnOn )
	{
		if( m_hSprite )
		{
			UTIL_Remove( m_hSprite );
			m_hSprite.Set( NULL );
		}
	}
	
	if ( !m_hSprite )
	{
		m_LastSpriteColor.SetRawColor( 0 );
	}
	else
	{
		m_LastSpriteColor.SetColor( r, g, b, a );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::Wake( bool bAwake )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	CReliableBroadcastRecipientFilter filter;
	
	if( !m_pWarnSound )
	{
		m_pWarnSound = controller.SoundCreate( filter, entindex(), "NPC_CombineMine.ActiveLoop" );
		controller.Play( m_pWarnSound, 1.0, PITCH_NORM  );
	}

	if( bAwake )
	{
		// Turning on
		if( m_bFoeNearest )
		{
			EmitSound( "NPC_CombineMine.TurnOn" );
			controller.SoundChangeVolume( m_pWarnSound, 1.0, 0.1 );
		}

		unsigned char r, g, b;
		r = g = b = 0;

		if( m_bFoeNearest )
		{
			r = 255;
		}
		else
		{
			g = 255;
		}

		UpdateLight( true, r, g, b, 190 );
	}
	else
	{
		// Turning off
		if( m_bFoeNearest )
		{
			EmitSound( "NPC_CombineMine.TurnOff" );
		}

		SetNearestNPC( NULL );
		controller.SoundChangeVolume( m_pWarnSound, 0.0, 0.1 );
		UpdateLight( false, 0, 0, 0, 0 );
	}

	m_bAwake = bAwake;
}

//---------------------------------------------------------
// Returns distance to the nearest BaseCombatCharacter.
//---------------------------------------------------------
float CBounceBomb::FindNearestNPC()
{
	float flNearest = (BOUNCEBOMB_WARN_RADIUS * BOUNCEBOMB_WARN_RADIUS) + 1.0;

	// Assume this search won't find anyone.
	SetNearestNPC( NULL );

	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAIs = g_AI_Manager.NumAIs();

	for ( int i = 0; i < nAIs; i++ )
	{
		CAI_BaseNPC *pNPC = ppAIs[ i ];

		if( pNPC->IsAlive() )
		{
			// ignore hidden objects
			if ( pNPC->IsEffectActive( EF_NODRAW ) )
				continue;

			// Don't bother with NPC's that are below me.
			if( pNPC->EyePosition().z < GetAbsOrigin().z )
				continue;

			// Disregard things that want to be disregarded
			if( pNPC->Classify() == CLASS_NONE )
				continue; 

			// Disregard bullseyes
			if( pNPC->Classify() == CLASS_BULLSEYE )
				continue;

			// Disregard turrets
			if( pNPC->m_iClassname == gm_iszFloorTurretClassname || pNPC->m_iClassname == gm_iszGroundTurretClassname )
				continue;


			float flDist = (GetAbsOrigin() - pNPC->GetAbsOrigin()).LengthSqr();

			if( flDist < flNearest )
			{
				// Now do a visibility test.
				if( FVisible( pNPC, MASK_SOLID_BRUSHONLY ) )
				{
					flNearest = flDist;
					SetNearestNPC( pNPC );
				}
			}
		}
	}

	// finally, check the player.
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	if( pPlayer && !(pPlayer->GetFlags() & FL_NOTARGET) )
	{
		float flDist = (pPlayer->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();

		if( flDist < flNearest && FVisible( pPlayer, MASK_SOLID_BRUSHONLY ) )
		{
			flNearest = flDist;
			SetNearestNPC( pPlayer );
		}
	}

	if( m_hNearestNPC.Get() )
	{
		// If sprite is active, update its color to reflect who is nearest.
		if( IsFriend( m_hNearestNPC ) )
		{
			if( m_bFoeNearest )
			{
				// Changing state to where a friend is nearest.

				if( IsFriend( m_hNearestNPC ) )
				{
					// Friend
					UpdateLight( true, 0, 255, 0, 190 );
					m_bFoeNearest = false;
				}
			}
		}
		else // it's a foe
		{
			if( !m_bFoeNearest )
			{
				// Changing state to where a foe is nearest.
				UpdateLight( true, 255, 0, 0, 190 );
				m_bFoeNearest = true;
			}
		}
	}

	return sqrt( flNearest );
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CBounceBomb::IsFriend( CBaseEntity *pEntity )
{
	int classify = pEntity->Classify();
	bool bIsCombine = false;

	// Unconditional enemies to combine and Player.
	if( classify == CLASS_ZOMBIE || classify == CLASS_HEADCRAB || classify == CLASS_ANTLION )
	{
		return false;
	}

  	if( classify == CLASS_METROPOLICE || 
  		classify == CLASS_COMBINE ||
  		classify == CLASS_MILITARY ||
  		classify == CLASS_COMBINE_HUNTER ||
  		classify == CLASS_SCANNER )
	{
		bIsCombine = true;
	}

	if( m_bPlacedByPlayer )
	{
		return !bIsCombine;
	}
	else
	{
		return bIsCombine;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::SearchThink()
{
	if( !UTIL_FindClientInPVS(edict()) )
	{
		// Sleep!
		SetNextThink( gpGlobals->curtime + 0.5 );
		return;
	}

	if(	(CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI) )
	{
		if( IsAwake() )
		{
			Wake(false);
		}

		SetNextThink( gpGlobals->curtime + 0.5 );
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.1 );
	StudioFrameAdvance();

	if( m_pConstraint && gpGlobals->curtime - m_flTimeGrabbed >= 1.0f )
	{
		m_OnPulledUp.FireOutput( this, this );
		SetMineState( MINE_STATE_CAPTIVE );
		return;
	}

	float flNearestNPCDist = FindNearestNPC();

	if( flNearestNPCDist <= BOUNCEBOMB_WARN_RADIUS )
	{
		if( !IsAwake() )
		{
			Wake( true );
		}
	}
	else
	{
 		if( IsAwake() )
		{
			Wake( false );
		}

		return;
	}

	if( flNearestNPCDist <= BOUNCEBOMB_DETONATE_RADIUS && !IsFriend( m_hNearestNPC ) )
	{
		if( m_bBounce )
		{
			SetMineState( MINE_STATE_TRIGGERED );
		}
		else
		{
			// Don't pop up in the air, just explode if the NPC gets closer than explode radius.
			SetThink( &CBounceBomb::ExplodeThink );
			SetNextThink( gpGlobals->curtime + m_flExplosionDelay );
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::ExplodeTouch( CBaseEntity *pOther )
{
	// Don't touch anything if held by physgun.
	if( m_bHeldByPhysgun )
		return;

	// Don't touch triggers.
	if( pOther->IsSolidFlagSet(FSOLID_TRIGGER) )
		return;

	// Don't touch gibs and other debris
	if( pOther->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
	{
		if( hl2_episodic.GetBool() )
		{
			Vector vecVelocity;

			VPhysicsGetObject()->GetVelocity( &vecVelocity, NULL );

			if( vecVelocity == vec3_origin )
			{
				ExplodeThink();
			}
		}

		return;
	}

	// Don't detonate against the world if not allowed. Actually, don't
	// detonate against anything that's probably not an NPC (such as physics props)
	if( m_flIgnoreWorldTime > gpGlobals->curtime && !pOther->MyCombatCharacterPointer() )
	{
		return;
	}

	ExplodeThink();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::ExplodeThink()
{
	SetSolid( SOLID_NONE );

	// Don't catch self in own explosion!
	m_takedamage = DAMAGE_NO;

	if( m_hSprite )
	{
		UpdateLight( false, 0, 0, 0, 0 );
	}

	if( m_pWarnSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundDestroy( m_pWarnSound );
	}


	CBaseEntity *pThrower = HasPhysicsAttacker( 0.5 );

	if (m_iModification == MINE_MODIFICATION_CAVERN)
	{
		ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), (pThrower) ? pThrower : this, BOUNCEBOMB_EXPLODE_DAMAGE, BOUNCEBOMB_EXPLODE_RADIUS, true,
			NULL, CLASS_PLAYER_ALLY );
	}
	else
	{
		ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), (pThrower) ? pThrower : this, BOUNCEBOMB_EXPLODE_DAMAGE, BOUNCEBOMB_EXPLODE_RADIUS, true);
	}
	UTIL_Remove( this );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::OpenHooks( bool bSilent )
{
	if( !bSilent )
	{
		EmitSound( "NPC_CombineMine.OpenHooks" );
	}

	if( VPhysicsGetObject() )
	{
		// It's possible to not have a valid physics object here, since this function doubles as an initialization function.
		PhysClearGameFlags( VPhysicsGetObject(), FVPHYSICS_CONSTRAINT_STATIC );

		VPhysicsGetObject()->EnableMotion( true );
	}

	SetPoseParameter( m_iAllHooks, BOUNCEBOMB_HOOK_RANGE );

#ifdef _XBOX 
	RemoveEffects( EF_NOSHADOW );
#endif

}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::CloseHooks()
{
	if( !m_bLockSilently )
	{
		EmitSound( "NPC_CombineMine.CloseHooks" );
	}

	if( VPhysicsGetObject() )
	{
		// It's possible to not have a valid physics object here, since this function doubles as an initialization function.
		PhysSetGameFlags( VPhysicsGetObject(), FVPHYSICS_CONSTRAINT_STATIC );
	}

	// Only lock silently the first time we call this.
	m_bLockSilently = false;

	SetPoseParameter( m_iAllHooks, 0 );

	VPhysicsGetObject()->EnableMotion( false );

	// Once I lock down, forget how many tries it took.
	m_iFlipAttempts = 0;

#ifdef _XBOX 
	AddEffects( EF_NOSHADOW );
#endif
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::InputDisarm( inputdata_t &inputdata )
{
	// Only affect a mine that's armed and not placed by player.
	if( !m_bPlacedByPlayer && m_iMineState == MINE_STATE_ARMED )
	{
		if( m_pConstraint )
		{
			physenv->DestroyConstraint( m_pConstraint );
			m_pConstraint = NULL;
		}

		m_bDisarmed = true;
		OpenHooks(false);

		SetMineState(MINE_STATE_DORMANT);
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

	m_flTimeGrabbed = FLT_MAX;

	m_bHeldByPhysgun = false;

	if( m_iMineState == MINE_STATE_ARMED )
	{
		// Put the mine back to searching.
		Wake( false );
		return;
	}

	if( Reason == DROPPED_BY_CANNON )
	{
		// Set to lock down to ground again.
		m_bPlacedByPlayer = true;
		OpenHooks( true );
		SetMineState( MINE_STATE_DEPLOY );
	}
	else if ( Reason == LAUNCHED_BY_CANNON )
	{
		SetMineState( MINE_STATE_LAUNCHED );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
CBasePlayer *CBounceBomb::HasPhysicsAttacker( float dt )
{
	if (gpGlobals->curtime - dt <= m_flLastPhysicsInfluenceTime)
	{
		return m_hPhysicsAttacker;
	}
	return NULL;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBounceBomb::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

	m_iFlipAttempts = 0;

	if( reason != PUNTED_BY_CANNON )
	{
		if( m_iMineState == MINE_STATE_ARMED )
		{
			// Yanking on a mine that is locked down, trying to rip it loose.
			UpdateLight( true, 255, 255, 0, 190 );
			m_flTimeGrabbed = gpGlobals->curtime;
			m_bHeldByPhysgun = true;

			VPhysicsGetObject()->EnableMotion( true );

			// Try to scatter NPCs without panicking them. Make a move away sound up around their 
			// ear level.
			CSoundEnt::InsertSound( SOUND_MOVE_AWAY, GetAbsOrigin() + Vector( 0, 0, 60), 32.0f, 0.2f );
			return;
		}
		else
		{
			// Picked up a mine that was not locked down.
			m_bHeldByPhysgun = true;

			if( m_iMineState == MINE_STATE_TRIGGERED )
			{
				// This mine's already set to blow. Player can't place it.
				return;
			}
			else
			{
				m_bDisarmed = false;
				SetMineState( MINE_STATE_DEPLOY );
			}
		}
	}
	else
	{
		m_bHeldByPhysgun = false;
	}

	if( reason == PUNTED_BY_CANNON )
	{
		if( m_iMineState == MINE_STATE_TRIGGERED || m_iMineState == MINE_STATE_ARMED )
		{
			// Already set to blow
			return;
		}

		m_bDisarmed = false;
		m_bPlacedByPlayer = true;
		SetTouch( NULL );
		SetThink( &CBounceBomb::SettleThink );
		SetNextThink( gpGlobals->curtime + 0.1);

		// Since being punted causes the mine to flip, sometimes it 'catches an edge'
		// and ends up touching the ground from whence it came, exploding instantly. 
		// This little stunt prevents that by ignoring world collisions for a very short time.
		m_flIgnoreWorldTime = gpGlobals->curtime + 0.1;
	}
}


LINK_ENTITY_TO_CLASS( bounce_bomb, CBounceBomb );
LINK_ENTITY_TO_CLASS( combine_bouncemine, CBounceBomb );
LINK_ENTITY_TO_CLASS( combine_mine, CBounceBomb );

/*
*/
