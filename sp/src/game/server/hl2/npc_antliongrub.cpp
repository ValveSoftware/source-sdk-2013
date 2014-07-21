//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Antlion Grub - cannon fodder
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "gib.h"
#include "Sprite.h"
#include "te_effect_dispatch.h"
#include "npc_antliongrub.h"
#include "ai_utils.h"
#include "particle_parse.h"
#include "items.h"
#include "item_dynamic_resupply.h"
#include "npc_vortigaunt_episodic.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	sk_grubnugget_health_small( "sk_grubnugget_health_small", "1" );
ConVar	sk_grubnugget_health_medium( "sk_grubnugget_health_medium", "4" );
ConVar	sk_grubnugget_health_large( "sk_grubnugget_health_large", "6" );
ConVar	sk_grubnugget_enabled( "sk_grubnugget_enabled", "1" );

#define	ANTLIONGRUB_MODEL				"models/antlion_grub.mdl"
#define	ANTLIONGRUB_SQUASHED_MODEL		"models/antlion_grub_squashed.mdl"

#define	SF_ANTLIONGRUB_NO_AUTO_PLACEMENT	(1<<0)


enum GrubState_e
{
	GRUB_STATE_IDLE,
	GRUB_STATE_AGITATED,
};

enum
{
	NUGGET_NONE,
	NUGGET_SMALL = 1,
	NUGGET_MEDIUM,
	NUGGET_LARGE
};

//
//  Grub nugget
//

class CGrubNugget : public CItem
{
public:
	DECLARE_CLASS( CGrubNugget, CItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void Event_Killed( const CTakeDamageInfo &info ); 
	virtual bool VPhysicsIsFlesh( void );
	
	bool	MyTouch( CBasePlayer *pPlayer );
	void	SetDenomination( int nSize ) { Assert( nSize <= NUGGET_LARGE && nSize >= NUGGET_SMALL ); m_nDenomination = nSize; }

	DECLARE_DATADESC();

private:
	int		m_nDenomination;	// Denotes size and health amount given
};

BEGIN_DATADESC( CGrubNugget )
	DEFINE_FIELD( m_nDenomination, FIELD_INTEGER ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( item_grubnugget, CGrubNugget );

//
//  Simple grub
//

class CAntlionGrub : public CBaseAnimating
{
public:
	DECLARE_CLASS( CAntlionGrub, CBaseAnimating );

	virtual void	Activate( void );
	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual void	UpdateOnRemove( void );
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	void	InputSquash( inputdata_t &data );

	void	IdleThink( void );
	void	FlinchThink( void );
	void	GrubTouch( CBaseEntity *pOther );

	DECLARE_DATADESC();

protected:

	inline bool InPVS( void );
	void		SetNextThinkByDistance( void );

	int		GetNuggetDenomination( void );
	void	CreateNugget( void );
	void	MakeIdleSounds( void );
	void	MakeSquashDecals( const Vector &vecOrigin );
	void	AttachToSurface( void );
	void	CreateGlow( void );
	void	FadeGlow( void );
	void	Squash( CBaseEntity *pOther, bool bDealDamage, bool bSpawnBlood );
	void	SpawnSquashedGrub( void );
	void	InputAgitate( inputdata_t &inputdata );

	inline bool ProbeSurface( const Vector &vecTestPos, const Vector &vecDir, Vector *vecResult, Vector *vecNormal );

	CHandle<CSprite>	m_hGlowSprite;
	int					m_nGlowSpriteHandle;
	float				m_flFlinchTime;
	float				m_flNextIdleSoundTime;
	float				m_flNextSquealSoundTime;
	bool				m_bOutsidePVS;
	GrubState_e			m_State;

	COutputEvent		m_OnAgitated;
	COutputEvent		m_OnDeath;
	COutputEvent		m_OnDeathByPlayer;
};

BEGIN_DATADESC( CAntlionGrub )

	DEFINE_FIELD( m_hGlowSprite, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flFlinchTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flNextIdleSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextSquealSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_State, FIELD_INTEGER ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "Agitate", InputAgitate ),

	DEFINE_OUTPUT( m_OnAgitated, "OnAgitated" ),
	DEFINE_OUTPUT( m_OnDeath, "OnDeath" ),
	DEFINE_OUTPUT( m_OnDeathByPlayer, "OnDeathByPlayer" ),

	// Functions
	DEFINE_ENTITYFUNC( GrubTouch ),
	DEFINE_ENTITYFUNC( IdleThink ),
	DEFINE_ENTITYFUNC( FlinchThink ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Squash", InputSquash ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_antlion_grub, CAntlionGrub );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::CreateGlow( void )
{
	// Create the glow sprite
	m_hGlowSprite = CSprite::SpriteCreate( "sprites/grubflare1.vmt", GetLocalOrigin(), false );
	Assert( m_hGlowSprite );
	if ( m_hGlowSprite == NULL )
		return;

	m_hGlowSprite->TurnOn();
	m_hGlowSprite->SetTransparency( kRenderWorldGlow, 156, 169, 121, 164, kRenderFxNoDissipation );
	m_hGlowSprite->SetScale( 0.5f );
	m_hGlowSprite->SetGlowProxySize( 16.0f );
	int nAttachment = LookupAttachment( "glow" );
	m_hGlowSprite->SetParent( this, nAttachment );
	m_hGlowSprite->SetLocalOrigin( vec3_origin );
	
	// Don't uselessly animate, we're a static sprite!
	m_hGlowSprite->SetThink( NULL );
	m_hGlowSprite->SetNextThink( TICK_NEVER_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::FadeGlow( void )
{
	if ( m_hGlowSprite )
	{
		m_hGlowSprite->FadeAndDie( 0.25f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::UpdateOnRemove( void )
{
	FadeGlow();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: Find what size of nugget to spawn
//-----------------------------------------------------------------------------
int CAntlionGrub::GetNuggetDenomination( void )
{
	// Find the desired health perc we want to be at
	float flDesiredHealthPerc = DynamicResupply_GetDesiredHealthPercentage();
	
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer == NULL )
		return -1;

	// Get the player's current health percentage
	float flPlayerHealthPerc = (float) pPlayer->GetHealth() / (float) pPlayer->GetMaxHealth();

	// If we're already maxed out, return the small nugget
	if ( flPlayerHealthPerc >= flDesiredHealthPerc )
	{
		return NUGGET_SMALL;
	}

	// Find where we fall in the desired health's range
	float flPercDelta = flPlayerHealthPerc / flDesiredHealthPerc;

	// The larger to discrepancy, the higher the chance to move quickly to close it
	float flSeed = random->RandomFloat( 0.0f, 1.0f );
	float flRandomPerc = Bias( flSeed, (1.0f-flPercDelta) );
	
	int nDenomination;
	if ( flRandomPerc < 0.25f )
	{
		nDenomination = NUGGET_SMALL;
	}
	else if ( flRandomPerc < 0.625f )
	{
		nDenomination = NUGGET_MEDIUM;
	}
	else
	{
		nDenomination = NUGGET_LARGE;
	}

	// Msg("Player: %.02f, Desired: %.02f, Seed: %.02f, Perc: %.02f, Result: %d\n", flPlayerHealthPerc, flDesiredHealthPerc, flSeed, flRandomPerc, nDenomination );

	return nDenomination;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::CreateNugget( void )
{
	CGrubNugget *pNugget = (CGrubNugget *) CreateEntityByName( "item_grubnugget" );
	if ( pNugget == NULL )
		return;

	Vector vecOrigin;
	Vector vecForward;
	GetAttachment( LookupAttachment( "glow" ), vecOrigin, &vecForward );

	// Find out what size to make this nugget!
	int nDenomination = GetNuggetDenomination();
	pNugget->SetDenomination( nDenomination );
	
	pNugget->SetAbsOrigin( vecOrigin );
	pNugget->SetAbsAngles( RandomAngle( 0, 360 ) );
	DispatchSpawn( pNugget );

	IPhysicsObject *pPhys = pNugget->VPhysicsGetObject();
	if ( pPhys )
	{
		Vector vecForward;
		GetVectors( &vecForward, NULL, NULL );
		
		Vector vecVelocity = RandomVector( -35.0f, 35.0f ) + ( vecForward * -RandomFloat( 50.0f, 75.0f ) );
		AngularImpulse vecAngImpulse = RandomAngularImpulse( -100.0f, 100.0f );

		pPhys->AddVelocity( &vecVelocity, &vecAngImpulse );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
void CAntlionGrub::Event_Killed( const CTakeDamageInfo &info )
{
	// Fire our output only if the player is the one that killed us
	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
	{
		m_OnDeathByPlayer.FireOutput( info.GetAttacker(), info.GetAttacker() );
	}

	m_OnDeath.FireOutput( info.GetAttacker(), info.GetAttacker() );
	SendOnKilledGameEvent( info );

	// Crush and crowbar damage hurt us more than others
	bool bSquashed = ( info.GetDamageType() & (DMG_CRUSH|DMG_CLUB)) ? true : false;
	Squash( info.GetAttacker(), false, bSquashed );

	m_takedamage = DAMAGE_NO;

	if ( sk_grubnugget_enabled.GetBool() )
	{
		CreateNugget();
	}

	// Go away
	SetThink( &CBaseEntity::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 0.1f );

	// we deliberately do not call BaseClass::EventKilled
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
int CAntlionGrub::OnTakeDamage( const CTakeDamageInfo &info )
{
	// Animate a flinch of pain if we're dying
	bool bSquashed = ( ( GetEffects() & EF_NODRAW ) != 0 );
	if ( bSquashed == false )
	{
		SetSequence( SelectWeightedSequence( ACT_SMALL_FLINCH ) );
		m_flFlinchTime = gpGlobals->curtime + random->RandomFloat( 0.5f, 1.0f );

		SetThink( &CAntlionGrub::FlinchThink );
		SetNextThink( gpGlobals->curtime + 0.05f );
	}

	return BaseClass::OnTakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: Whether or not we're in the PVS
//-----------------------------------------------------------------------------
inline bool CAntlionGrub::InPVS( void )
{
	return ( UTIL_FindClientInPVS( edict() ) != NULL ) || (UTIL_ClientPVSIsExpanded() && UTIL_FindClientInVisibilityPVS( edict() ));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::SetNextThinkByDistance( void )
{
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer == NULL )
	{
		SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.5f, 3.0f ) );
		return;
	}

	float flDistToPlayerSqr = ( GetAbsOrigin() - pPlayer->GetAbsOrigin() ).LengthSqr();
	float scale = RemapValClamped( flDistToPlayerSqr, Square( 400 ), Square( 5000 ), 1.0f, 5.0f );
	float time = random->RandomFloat( 1.0f, 3.0f );
	SetNextThink( gpGlobals->curtime + ( time * scale ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	SetModel( ANTLIONGRUB_MODEL );
	
	// FIXME: This is a big perf hit with the number of grubs we're using! - jdw
	CreateGlow();

	SetSolid( SOLID_BBOX );
	SetSolidFlags( FSOLID_TRIGGER );
	SetMoveType( MOVETYPE_NONE );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	AddEffects( EF_NOSHADOW );

	CollisionProp()->UseTriggerBounds(true,1);

	SetTouch( &CAntlionGrub::GrubTouch );

	SetHealth( 1 );
	m_takedamage = DAMAGE_YES;

	// Stick to the nearest surface
	if ( HasSpawnFlags( SF_ANTLIONGRUB_NO_AUTO_PLACEMENT ) == false )
	{
		AttachToSurface();
	}

	// At this point, alter our bounds to make sure we're within them
	Vector vecMins, vecMaxs;
	RotateAABB( EntityToWorldTransform(), CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), vecMins, vecMaxs );

	UTIL_SetSize( this, vecMins, vecMaxs );

	// Start our idle activity
	SetSequence( SelectWeightedSequence( ACT_IDLE ) );
	SetCycle( random->RandomFloat( 0.0f, 1.0f ) );
	ResetSequenceInfo();

	m_State = GRUB_STATE_IDLE;

	// Reset
	m_flFlinchTime = 0.0f;
	m_flNextIdleSoundTime = gpGlobals->curtime + random->RandomFloat( 4.0f, 8.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::Activate( void )
{
	BaseClass::Activate();

	// Idly think
	SetThink( &CAntlionGrub::IdleThink );
	SetNextThinkByDistance();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecTestPos - 
//			*vecResult - 
//			*flDist - 
// Output : inline bool
//-----------------------------------------------------------------------------
inline bool CAntlionGrub::ProbeSurface( const Vector &vecTestPos, const Vector &vecDir, Vector *vecResult, Vector *vecNormal )
{
	// Trace down to find a surface
	trace_t tr;
	UTIL_TraceLine( vecTestPos, vecTestPos + (vecDir*256.0f), MASK_NPCSOLID&(~CONTENTS_MONSTER), this, COLLISION_GROUP_NONE, &tr );

	if ( vecResult )
	{
		*vecResult = tr.endpos;
	}

	if ( vecNormal )
	{
		*vecNormal = tr.plane.normal;
	}

	return ( tr.fraction < 1.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Attaches the grub to the surface underneath its abdomen
//-----------------------------------------------------------------------------
void CAntlionGrub::AttachToSurface( void )
{
	// Get our downward direction
	Vector vecForward, vecRight, vecDown;
	GetVectors( &vecForward, &vecRight, &vecDown );
	vecDown.Negate();
	
	Vector vecOffset = ( vecDown * -8.0f );

	// Middle
	Vector vecMid, vecMidNormal;
	if ( ProbeSurface( WorldSpaceCenter() + vecOffset, vecDown, &vecMid, &vecMidNormal ) == false )
	{
		// A grub was left hanging in the air, it must not be near any valid surfaces!
		Warning("Antlion grub stranded in space at (%.02f, %.02f, %.02f) : REMOVED\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		UTIL_Remove( this );
		return;
	}

	// Sit at the mid-point
	UTIL_SetOrigin( this, vecMid );

	Vector vecPivot;
	Vector vecPivotNormal;

	bool bNegate = true;

	// First test our tail (more crucial that it doesn't interpenetrate with the world)
	if ( ProbeSurface( WorldSpaceCenter() - ( vecForward * 12.0f ) + vecOffset, vecDown, &vecPivot, &vecPivotNormal ) == false )
	{
		// If that didn't find a surface, try the head
		if ( ProbeSurface( WorldSpaceCenter() + ( vecForward * 12.0f ) + vecOffset, vecDown, &vecPivot, &vecPivotNormal ) == false )
		{
			// Worst case, just site at the middle
			UTIL_SetOrigin( this, vecMid );

			QAngle vecAngles;
			VectorAngles( vecForward, vecMidNormal, vecAngles );
			SetAbsAngles( vecAngles );
			return;
		}

		bNegate = false;
	}
	
	// Find the line we'll lay on if these two points are connected by a line
	Vector vecLieDir = ( vecPivot - vecMid );
	VectorNormalize( vecLieDir );
	if ( bNegate )
	{
		// We need to try and maintain our facing
		vecLieDir.Negate();
	}

	// Use the average of the surface normals to be our "up" direction
	Vector vecPseudoUp = ( vecMidNormal + vecPivotNormal ) * 0.5f;

	QAngle vecAngles;
	VectorAngles( vecLieDir, vecPseudoUp, vecAngles );

	SetAbsAngles( vecAngles );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::MakeIdleSounds( void )
{
	if ( m_State == GRUB_STATE_AGITATED )
	{
		if ( m_flNextSquealSoundTime < gpGlobals->curtime )
		{
			EmitSound( "NPC_Antlion_Grub.Stimulated" );
			m_flNextSquealSoundTime = gpGlobals->curtime + random->RandomFloat( 1.5f, 3.0f );
			m_flNextIdleSoundTime = gpGlobals->curtime + random->RandomFloat( 4.0f, 8.0f );
		}
	}
	else
	{
		if ( m_flNextIdleSoundTime < gpGlobals->curtime )
		{
			EmitSound( "NPC_Antlion_Grub.Idle" );
			m_flNextIdleSoundTime = gpGlobals->curtime + random->RandomFloat( 8.0f, 12.0f );
		}
	}
}

#define DEBUG_GRUB_THINK_TIMES 0

#if DEBUG_GRUB_THINK_TIMES
	int nFrame = 0;
	int nNumThinks = 0;
#endif // DEBUG_GRUB_THINK_TIMES

//-----------------------------------------------------------------------------
// Purpose: Advance our thinks
//-----------------------------------------------------------------------------
void CAntlionGrub::IdleThink( void )
{
#if DEBUG_GRUB_THINK_TIMES
	// Test for a new frame
	if ( gpGlobals->framecount != nFrame )
	{
		if ( nNumThinks > 10 )
		{
			Msg("%d npc_antlion_grubs thinking per frame!\n", nNumThinks );
		}

		nFrame = gpGlobals->framecount;
		nNumThinks = 0;
	}

	nNumThinks++;
#endif // DEBUG_GRUB_THINK_TIMES

	// Check the PVS status
	if ( InPVS() == false )
	{
		// Push out into the future until they're in our PVS
		SetNextThinkByDistance();
		m_bOutsidePVS = true;
		return;
	}

	// Stagger our sounds if we've just re-entered the PVS
	if ( m_bOutsidePVS )
	{
		m_flNextIdleSoundTime = gpGlobals->curtime + random->RandomFloat( 1.0f, 4.0f );
		m_bOutsidePVS = false;
	}

	// See how close the player is
	CBasePlayer *pPlayerEnt = AI_GetSinglePlayer();
	float flDistToPlayerSqr = ( GetAbsOrigin() - pPlayerEnt->GetAbsOrigin() ).LengthSqr();

	bool bFlinching = ( m_flFlinchTime > gpGlobals->curtime );

	// If they're far enough away, just wait to think again
	if ( flDistToPlayerSqr > Square( 40*12 ) && bFlinching == false )
	{
		SetNextThinkByDistance();
		return;
	}
	
	// At this range, the player agitates us with his presence
	bool bPlayerWithinAgitationRange = ( flDistToPlayerSqr <= Square( (6*12) ) );
	bool bAgitated = (bPlayerWithinAgitationRange || bFlinching );

	// If we're idle and the player has come close enough, get agry
	if ( ( m_State == GRUB_STATE_IDLE ) && bAgitated )
	{
		SetSequence( SelectWeightedSequence( ACT_SMALL_FLINCH ) );
		m_State = GRUB_STATE_AGITATED;
	}
	else if ( IsSequenceFinished() )
	{
		// See if it's time to choose a new sequence
		ResetSequenceInfo();
		SetCycle( 0.0f );

		// If we're near enough, we want to play an "alert" animation
		if ( bAgitated )
		{
			SetSequence( SelectWeightedSequence( ACT_SMALL_FLINCH ) );
			m_State = GRUB_STATE_AGITATED;
		}
		else
		{
			// Just idle
			SetSequence( SelectWeightedSequence( ACT_IDLE ) );
			m_State = GRUB_STATE_IDLE;
		}

		// Add some variation because we're often in large bunches
		SetPlaybackRate( random->RandomFloat( 0.8f, 1.2f ) );
	}

	// Idle normally
	StudioFrameAdvance();
	MakeIdleSounds();
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::FlinchThink( void )
{
	StudioFrameAdvance();
	SetNextThink( gpGlobals->curtime + 0.1f );

	// See if we're done
	if ( m_flFlinchTime < gpGlobals->curtime )
	{
		SetSequence( SelectWeightedSequence( ACT_IDLE ) );
		SetThink( &CAntlionGrub::IdleThink );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::GrubTouch( CBaseEntity *pOther )
{
	// We can be squished by the player, Vort, or flying heavy things.
	IPhysicsObject *pPhysOther = pOther->VPhysicsGetObject(); // bool bThrown = ( pTarget->VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_WAS_THROWN ) != 0;
	if ( pOther->IsPlayer() || FClassnameIs(pOther,"npc_vortigaunt") || ( pPhysOther && (pPhysOther->GetGameFlags() & FVPHYSICS_WAS_THROWN )) )
	{
		m_OnAgitated.FireOutput( pOther, pOther );
		Squash( pOther, true, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::Precache( void )
{
	PrecacheModel( ANTLIONGRUB_MODEL );
	PrecacheModel( ANTLIONGRUB_SQUASHED_MODEL );

	m_nGlowSpriteHandle = PrecacheModel("sprites/grubflare1.vmt");

	PrecacheScriptSound( "NPC_Antlion_Grub.Idle" );
	PrecacheScriptSound( "NPC_Antlion_Grub.Alert" );
	PrecacheScriptSound( "NPC_Antlion_Grub.Stimulated" );
	PrecacheScriptSound( "NPC_Antlion_Grub.Die" );
	PrecacheScriptSound( "NPC_Antlion_Grub.Squish" );

	PrecacheParticleSystem( "GrubSquashBlood" );
	PrecacheParticleSystem( "GrubBlood" );

	UTIL_PrecacheOther( "item_grubnugget" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Squish the grub!
//-----------------------------------------------------------------------------
void CAntlionGrub::InputSquash( inputdata_t &data )
{
	Squash( data.pActivator, true, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::SpawnSquashedGrub( void )
{
	// If we're already invisible, we're done
	if ( GetEffects() & EF_NODRAW )
		return;

	Vector vecUp;
	GetVectors( NULL, NULL, &vecUp );
	CBaseEntity *pGib = CreateRagGib( ANTLIONGRUB_SQUASHED_MODEL, GetAbsOrigin(), GetAbsAngles(), vecUp * 16.0f );
	if ( pGib )
	{
		pGib->AddEffects( EF_NOSHADOW );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::MakeSquashDecals( const Vector &vecOrigin )
{
	trace_t tr;
	Vector	vecStart;
	Vector	vecTraceDir;

	GetVectors( NULL, NULL, &vecTraceDir );
	vecTraceDir.Negate();

	for ( int i = 0 ; i < 8; i++ )
	{
		vecStart.x = vecOrigin.x + random->RandomFloat( -16.0f, 16.0f );
		vecStart.y = vecOrigin.y + random->RandomFloat( -16.0f, 16.0f );
		vecStart.z = vecOrigin.z + 4;

		UTIL_TraceLine( vecStart, vecStart + ( vecTraceDir * (5*12) ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_YELLOW );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionGrub::Squash( CBaseEntity *pOther, bool bDealDamage, bool bSpawnBlood )
{
	// If we're already squashed, then don't bother doing it again!
	if ( GetEffects() & EF_NODRAW )
		return;

	SpawnSquashedGrub();

	AddEffects( EF_NODRAW );
	AddSolidFlags( FSOLID_NOT_SOLID );
	
	// Stop being attached to us
	if ( m_hGlowSprite )
	{
		FadeGlow();
		m_hGlowSprite->SetParent( NULL );
	}

	EmitSound( "NPC_Antlion_Grub.Die" );
	EmitSound( "NPC_Antlion_Grub.Squish" );

	// if vort stepped on me, maybe he wants to say something
	if ( pOther && FClassnameIs( pOther, "npc_vortigaunt" ) )
	{
		Assert(dynamic_cast<CNPC_Vortigaunt *>(pOther));
		static_cast<CNPC_Vortigaunt *>(pOther)->OnSquishedGrub(this);
	}

	SetTouch( NULL );

	//if ( bSpawnBlood )
	{
		// Temp squash effect
		Vector vecForward, vecUp;
		AngleVectors( GetAbsAngles(), &vecForward, NULL, &vecUp );

		// Start effects at either end of the grub
		Vector vecSplortPos = GetAbsOrigin() + vecForward * 14.0f;
		DispatchParticleEffect( "GrubSquashBlood", vecSplortPos, GetAbsAngles() );

		vecSplortPos = GetAbsOrigin() - vecForward * 16.0f;
		Vector vecDir = -vecForward;
		QAngle vecAngles;
		VectorAngles( vecDir, vecAngles );
		DispatchParticleEffect( "GrubSquashBlood", vecSplortPos, vecAngles );
		
		MakeSquashDecals( GetAbsOrigin() + vecForward * 32.0f );
		MakeSquashDecals( GetAbsOrigin() - vecForward * 32.0f );
	}

	// Deal deadly damage to ourself
	if ( bDealDamage )
	{
		CTakeDamageInfo info( pOther, pOther, Vector( 0, 0, -1 ), GetAbsOrigin(), GetHealth()+1, DMG_CRUSH );
		TakeDamage( info );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			&vecDir - 
//			*ptr - 
//-----------------------------------------------------------------------------
void CAntlionGrub::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	QAngle vecAngles;
	VectorAngles( -vecDir, vecAngles );
	DispatchParticleEffect( "GrubBlood", ptr->endpos, vecAngles );

	BaseClass::TraceAttack( info, vecDir, ptr );
}

//-----------------------------------------------------------------------------
// Purpose: Make the grub angry!
//-----------------------------------------------------------------------------
void CAntlionGrub::InputAgitate( inputdata_t &inputdata )
{
	SetSequence( SelectWeightedSequence( ACT_SMALL_FLINCH ) );
	m_State = GRUB_STATE_AGITATED;
	m_flNextSquealSoundTime = gpGlobals->curtime;

	m_flFlinchTime = gpGlobals->curtime + inputdata.value.Float();

	SetNextThink( gpGlobals->curtime );
}

// =====================================================================
//
//  Tasty grub nugget!
//
// =====================================================================

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrubNugget::Spawn( void )
{
	Precache();
	
	if ( m_nDenomination == NUGGET_LARGE )
	{
		SetModel( "models/grub_nugget_large.mdl" );
	}
	else if ( m_nDenomination == NUGGET_MEDIUM )
	{
		SetModel( "models/grub_nugget_medium.mdl" );	
	}
	else
	{
		SetModel( "models/grub_nugget_small.mdl" );
	}

	// We're self-illuminating, so we don't take or give shadows
	AddEffects( EF_NOSHADOW|EF_NORECEIVESHADOW );

	m_iHealth = 1;

	BaseClass::Spawn();

	m_takedamage = DAMAGE_YES;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrubNugget::Precache( void )
{
	PrecacheModel("models/grub_nugget_small.mdl");
	PrecacheModel("models/grub_nugget_medium.mdl");
	PrecacheModel("models/grub_nugget_large.mdl");

	PrecacheScriptSound( "GrubNugget.Touch" );
	PrecacheScriptSound( "NPC_Antlion_Grub.Explode" );

	PrecacheParticleSystem( "antlion_spit_player" );
}

//-----------------------------------------------------------------------------
// Purpose: Let us be picked up by the gravity gun, regardless of our material
//-----------------------------------------------------------------------------
bool CGrubNugget::VPhysicsIsFlesh( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGrubNugget::MyTouch( CBasePlayer *pPlayer )
{
	//int nHealthToGive = sk_grubnugget_health.GetFloat() * m_nDenomination;
	int nHealthToGive;
	switch (m_nDenomination)
	{
	case NUGGET_SMALL:
		nHealthToGive = sk_grubnugget_health_small.GetInt();
		break;
	case NUGGET_LARGE:
		nHealthToGive = sk_grubnugget_health_large.GetInt();
		break;
	default:
		nHealthToGive = sk_grubnugget_health_medium.GetInt();
	}

	// Attempt to give the player health
	if ( pPlayer->TakeHealth( nHealthToGive, DMG_GENERIC ) == 0 )
		return false;

	CSingleUserRecipientFilter user( pPlayer );
	user.MakeReliable();

	UserMessageBegin( user, "ItemPickup" );
	WRITE_STRING( GetClassname() );
	MessageEnd();

	CPASAttenuationFilter filter( pPlayer, "GrubNugget.Touch" );
	EmitSound( filter, pPlayer->entindex(), "GrubNugget.Touch" );

	UTIL_Remove( this );	

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//			*pEvent - 
//-----------------------------------------------------------------------------
void CGrubNugget::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	int damageType;
	float damage = CalculateDefaultPhysicsDamage( index, pEvent, 1.0f, true, damageType );
	if ( damage > 5.0f )
	{
		CBaseEntity *pHitEntity = pEvent->pEntities[!index];
		if ( pHitEntity == NULL )
		{
			// hit world
			pHitEntity = GetContainingEntity( INDEXENT(0) );
		}
		
		Vector damagePos;
		pEvent->pInternalData->GetContactPoint( damagePos );
		Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass();
		if ( damageForce == vec3_origin )
		{
			// This can happen if this entity is motion disabled, and can't move.
			// Use the velocity of the entity that hit us instead.
			damageForce = pEvent->postVelocity[!index] * pEvent->pObjects[!index]->GetMass();
		}

		// FIXME: this doesn't pass in who is responsible if some other entity "caused" this collision
		PhysCallbackDamage( this, CTakeDamageInfo( pHitEntity, pHitEntity, damageForce, damagePos, damage, damageType ), *pEvent, index );
	}

	BaseClass::VPhysicsCollision( index, pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
void CGrubNugget::Event_Killed( const CTakeDamageInfo &info )
{
	AddEffects( EF_NODRAW );
	DispatchParticleEffect( "antlion_spit_player", GetAbsOrigin(), QAngle( -90, 0, 0 ) );
	EmitSound( "NPC_Antlion_Grub.Explode" );

	BaseClass::Event_Killed( info );
}
