//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef SecobMod__ENABLE_IN_TESTING_ITEMS

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
	#include "c_te_effect_dispatch.h"
	#include "input.h"
#else
	#include "hl2mp_player.h"
	#include "te_effect_dispatch.h"
	#include "IEffects.h"
	#include "Sprite.h"
	#include "SpriteTrail.h"
	#include "beam_shared.h"
	#include "../../server/secobmod/npc_secobmodportal1.h"
	#include "../../server/secobmod/npc_secobmodportal2.h"
	#include "point_camera.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "effect_dispatch_data.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BOLT_MODEL	""

#define BOLT_AIR_VELOCITY	3500
#define BOLT_WATER_VELOCITY	1500
#define	BOLT_SKIN_NORMAL	0
#define BOLT_SKIN_GLOW		1


#ifndef CLIENT_DLL

extern ConVar sk_plr_dmg_crossbow;
extern ConVar sk_npc_dmg_crossbow;

void TE_StickyBolt( IRecipientFilter& filter, float delay,	Vector vecDirection, const Vector *origin );

//-----------------------------------------------------------------------------
// Crossbow Bolt
//-----------------------------------------------------------------------------
class CPortalBolt : public CBaseCombatCharacter
{
	DECLARE_CLASS( CPortalBolt, CBaseCombatCharacter );

public:
	CPortalBolt() { };
	~CPortalBolt();

	Class_T Classify( void ) { return CLASS_NONE; }

public:
	void Spawn( void );
	void Spawn2( void );
	void Precache( void );
	void BubbleThink( void );
	void BoltTouch( CBaseEntity *pOther );
	void BoltTouch2( CBaseEntity *pOther );
	bool CreateVPhysics( void );
	unsigned int PhysicsSolidMaskForEntity() const;
	static CPortalBolt *BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, int iDamage, CBasePlayer *pentOwner = NULL );
	static CPortalBolt *BoltCreate2( const Vector &vecOrigin, const QAngle &angAngles, int iDamage, CBasePlayer *pentOwner = NULL );

protected:

	bool	CreateSprites( void );

	CHandle<CSprite>		m_pGlowSprite;
	//CHandle<CSpriteTrail>	m_pGlowTrail;
	
	int		m_iDamage;

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
};
LINK_ENTITY_TO_CLASS( portal_bolt, CPortalBolt );

BEGIN_DATADESC( CPortalBolt )
	// Function Pointers
	DEFINE_FUNCTION( BubbleThink ),
	DEFINE_FUNCTION( BoltTouch ),

	// These are recreated on reload, they don't need storage
	DEFINE_FIELD( m_pGlowSprite, FIELD_EHANDLE ),
	//DEFINE_FIELD( m_pGlowTrail, FIELD_EHANDLE ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPortalBolt, DT_PortalBolt )
END_SEND_TABLE()

CPortalBolt *CPortalBolt::BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, int iDamage, CBasePlayer *pentOwner )
{
	// Create a new entity with CPortalBolt private data
	CPortalBolt *pBolt = (CPortalBolt *)CreateEntityByName( "portal_bolt" );
	UTIL_SetOrigin( pBolt, vecOrigin );
	pBolt->SetAbsAngles( angAngles );
	pBolt->Spawn();
	pBolt->SetOwnerEntity( pentOwner );

	pBolt->m_iDamage = iDamage;

	return pBolt;
}

CPortalBolt *CPortalBolt::BoltCreate2( const Vector &vecOrigin, const QAngle &angAngles, int iDamage, CBasePlayer *pentOwner )
{
	// Create a new entity with CPortalBolt private data
	CPortalBolt *pBolt2 = (CPortalBolt *)CreateEntityByName( "portal_bolt" );
	UTIL_SetOrigin( pBolt2, vecOrigin );
	pBolt2->SetAbsAngles( angAngles );
	pBolt2->Spawn2();
	pBolt2->SetOwnerEntity( pentOwner );

	pBolt2->m_iDamage = iDamage;

	return pBolt2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPortalBolt::~CPortalBolt( void )
{
	if ( m_pGlowSprite )
	{
		UTIL_Remove( m_pGlowSprite );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPortalBolt::CreateVPhysics( void )
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE, false );

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int CPortalBolt::PhysicsSolidMaskForEntity() const
{
	return ( BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX ) & ~CONTENTS_GRATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPortalBolt::CreateSprites( void )
{
	// Start up the eye glow
	m_pGlowSprite = CSprite::SpriteCreate( "sprites/light_glow02_noz.vmt", GetLocalOrigin(), false );

	if ( m_pGlowSprite != NULL )
	{
		m_pGlowSprite->FollowEntity( this );
		m_pGlowSprite->SetTransparency( kRenderGlow, 255, 255, 255, 128, kRenderFxNoDissipation );
		m_pGlowSprite->SetScale( 0.2f );
		m_pGlowSprite->TurnOff();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPortalBolt::Spawn( void )
{
	Precache( );

	SetModel( "" );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	UTIL_SetSize( this, -Vector(1,1,1), Vector(1,1,1) );
	SetSolid( SOLID_BBOX );
	SetGravity( 0.05f );
	
	// Make sure we're updated if we're underwater
	UpdateWaterState();

	SetTouch( &CPortalBolt::BoltTouch );

	SetThink( &CPortalBolt::BubbleThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
	
	CreateSprites();

	// Make us glow until we've hit the wall
	m_nSkin = BOLT_SKIN_GLOW;
}

//-----------------------------------------------------------------------------
// Purpose: Portal 2 Spawner.
//-----------------------------------------------------------------------------
void CPortalBolt::Spawn2( void )
{
	Precache( );

	SetModel( "" );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	UTIL_SetSize( this, -Vector(1,1,1), Vector(1,1,1) );
	SetSolid( SOLID_BBOX );
	SetGravity( 0.05f );
	
	// Make sure we're updated if we're underwater
	UpdateWaterState();

	SetTouch( &CPortalBolt::BoltTouch2 );

	SetThink( &CPortalBolt::BubbleThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
	
	CreateSprites();

	// Make us glow until we've hit the wall
	m_nSkin = BOLT_SKIN_GLOW;
}


void CPortalBolt::Precache( void )
{
	PrecacheModel( BOLT_MODEL );

	// This is used by C_TEStickyBolt, despte being different from above!!!
	PrecacheModel( "" );

	PrecacheModel( "sprites/light_glow02_noz.vmt" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CPortalBolt::BoltTouch( CBaseEntity *pOther )
{
	const char *PlayerSteamID = engine->GetPlayerNetworkIDString(GetOwnerEntity()->edict()); //Finds the current players Steam ID.		
	if( PlayerSteamID == NULL)
	return;
	//Create two new strings for either portal, using the players SteamID to make them uniquely named.
    char Portal1Name[ 512 ];
    Q_strncpy( Portal1Name, "Portal1_" ,sizeof(Portal1Name));
    Q_strncat( Portal1Name, PlayerSteamID,sizeof(Portal1Name), COPY_ALL_CHARACTERS );
			
			
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;

	if ( pOther->m_takedamage != DAMAGE_NO )
	{
		trace_t	tr, tr2;
		tr = BaseClass::GetTouchTrace();
		Vector	vecNormalizedVel = GetAbsVelocity();

		ClearMultiDamage();
		VectorNormalize( vecNormalizedVel );

		if( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() && pOther->IsNPC() )
		{
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), m_iDamage, DMG_NEVERGIB );
			dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );
		}
		else
		{
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), m_iDamage, DMG_BULLET | DMG_NEVERGIB );
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );
		}

		ApplyMultiDamage();

		//Adrian: keep going through the glass.
		if ( pOther->GetCollisionGroup() == COLLISION_GROUP_BREAKABLE_GLASS )
			 return;

		SetAbsVelocity( Vector( 0, 0, 0 ) );

		// play body "thwack" sound
EmitSound( "weapon_portalgun.BoltHitBody" );

		Vector vForward;

		AngleVectors( GetAbsAngles(), &vForward );
		VectorNormalize ( vForward );

		UTIL_TraceLine( GetAbsOrigin(),	GetAbsOrigin() + vForward * 128, MASK_OPAQUE, pOther, COLLISION_GROUP_NONE, &tr2 );

		if ( tr2.fraction != 1.0f )
		{
//			NDebugOverlay::Box( tr2.endpos, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 255, 0, 0, 10 );
//			NDebugOverlay::Box( GetAbsOrigin(), Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 0, 255, 0, 10 );

			if ( tr2.m_pEnt == NULL || ( tr2.m_pEnt && tr2.m_pEnt->GetMoveType() == MOVETYPE_NONE ) )
			{
				CEffectData	data;

				data.m_vOrigin = tr2.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = tr2.fraction != 1.0f;
			
				//DispatchEffect( "BoltImpact", data );
			}
		}
		
		SetTouch( NULL );
		SetThink( NULL );

		UTIL_Remove( this );
	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();

		// See if we struck the world
		if ( pOther->GetMoveType() == MOVETYPE_NONE && !( tr.surface.flags & SURF_SKY ) )
		{
EmitSound( "weapon_portalgun.BoltHitWorld" );
			

			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = GetAbsVelocity();
			float speed = VectorNormalize( vecDir );

			// See if we should reflect off this surface
			float hitDot = DotProduct( tr.plane.normal, -vecDir );
			
			if ( ( hitDot < 0.5f ) && ( speed > 100 ) )
			{
				Vector vReflection = 2.0f * tr.plane.normal * hitDot + vecDir;
				
				QAngle reflectAngles;

				VectorAngles( vReflection, reflectAngles );

				SetLocalAngles( reflectAngles );

				SetAbsVelocity( vReflection * speed * 0.75f );

				// Start to sink faster
				SetGravity( 1.0f );
			}
			else
			{
				SetThink( &CPortalBolt::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );
				
				//FIXME: We actually want to stick (with hierarchy) to what we've hit
				SetMoveType( MOVETYPE_NONE );
			
				Vector vForward;

				AngleVectors( GetAbsAngles(), &vForward );
				VectorNormalize ( vForward );

				CEffectData	data;

				data.m_vOrigin = tr.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = 0;
			
				//DispatchEffect( "BoltImpact", data );
				
				//UTIL_ImpactTrace( &tr, DMG_BULLET );

				AddEffects( EF_NODRAW );
				SetTouch( NULL );
				SetThink( &CPortalBolt::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );

				if ( m_pGlowSprite != NULL )
				{
					m_pGlowSprite->TurnOn();
					m_pGlowSprite->FadeAndDie( 3.0f );
				}
			}
			
			// Shoot some sparks
			if ( UTIL_PointContents( GetAbsOrigin() ) != CONTENTS_WATER)
			{
				g_pEffects->Sparks( GetAbsOrigin() );
			}
		}
		else
		{
			// Put a mark unless we've hit the sky
			if ( ( tr.surface.flags & SURF_SKY ) == false )
			{
				//UTIL_ImpactTrace( &tr, DMG_BULLET );
			}

			UTIL_Remove( this );
		}
	}

	if ( g_pGameRules->IsMultiplayer() )
	{
//		SetThink( &CPortalBolt::ExplodeThink );
//		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//SecobMod__Information  If a Portal1 already exists, delete it.
CBaseEntity *ent1 = gEntList.FindEntityByName( NULL, Portal1Name );
				while ( ent1 )
				{
					CBaseEntity *next1 = gEntList.FindEntityByClassname( ent1, "info_playerstart" );
					EmitSound( "Portal.close_blue" );
					UTIL_Remove( ent1 );
					ent1 = next1;
				}
CNPC_SecobModportal1 *pBullseye = static_cast<CNPC_SecobModportal1*>( CreateEntityByName( "npc_SecobModportal1" ) );
				if( pBullseye )
				{
				EmitSound( "Portal.open_blue" );
				pBullseye->SetAbsOrigin( this->GetAbsOrigin() );
				pBullseye->SetAbsAngles( this->GetAbsAngles()  );
				pBullseye->KeyValue( "solid", "6" );
				pBullseye->KeyValue( "targetname", Portal1Name );
				pBullseye->Spawn();
				DispatchSpawn(pBullseye);
				pBullseye->Activate();
				pBullseye->SetHealth(999999999);
				}
				
//SecobMod__Information  If a Portal1 camera already exists, delete it.
CBaseEntity *cam1 = gEntList.FindEntityByName( NULL, "portalcamera1" );
				while ( cam1 )
				{
					CBaseEntity *next2 = gEntList.FindEntityByClassname( cam1, "info_playerstart" );
					UTIL_Remove( cam1 );
					cam1 = next2;
				}
				CPointCamera *cam2 = static_cast<CPointCamera*>( CreateEntityByName( "point_camera" ) );
				if (cam2)
				{
				cam2->SetAbsOrigin( this->GetAbsOrigin() );
				cam2->SetAbsAngles( this->GetAbsAngles()  );
				cam2->KeyValue( "targetname", "portalcamera1" );
				cam2->Spawn();
				DispatchSpawn(cam2);
				cam2->Activate();
				cam2->SetActive(true);
				}
				
//SecobMod__Information  If a Portal1 monitor already exists, delete it.
CBaseEntity *ent2 = gEntList.FindEntityByClassname( NULL, "my_model_entity1" );
				while ( ent2 )
				{
					CBaseEntity *next2 = gEntList.FindEntityByClassname( ent2, "info_playerstart" );
					UTIL_Remove( ent2 );
					ent2 = next2;
				}
				CBaseEntity *pMonitor = CreateEntityByName( "my_model_entity1" );
				if (pMonitor)
				{
				pMonitor->SetAbsOrigin( this->GetAbsOrigin() );
				pMonitor->SetAbsAngles( this->GetAbsAngles()  );
				pMonitor->Spawn();
				DispatchSpawn(pMonitor);
				pMonitor->Activate();
				}
				
				
				return;
				}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!	
	
//}

//-----------------------------------------------------------------------------
// Purpose: Portal2 Touch Function.
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CPortalBolt::BoltTouch2( CBaseEntity *pOther )
{
	const char *PlayerSteamID = engine->GetPlayerNetworkIDString(GetOwnerEntity()->edict()); //Finds the current players Steam ID.				
	if( PlayerSteamID == NULL)
	return;
	char Portal2Name[ 512 ];
	Q_strncpy( Portal2Name, "Portal2_" ,sizeof(Portal2Name));
	Q_strncat( Portal2Name, PlayerSteamID,sizeof(Portal2Name), COPY_ALL_CHARACTERS );
			
			
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;

	if ( pOther->m_takedamage != DAMAGE_NO )
	{
		trace_t	tr, tr2;
		tr = BaseClass::GetTouchTrace();
		Vector	vecNormalizedVel = GetAbsVelocity();

		ClearMultiDamage();
		VectorNormalize( vecNormalizedVel );

		if( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() && pOther->IsNPC() )
		{
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), m_iDamage, DMG_NEVERGIB );
			dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );
		}
		else
		{
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), m_iDamage, DMG_BULLET | DMG_NEVERGIB );
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );
		}

		ApplyMultiDamage();

		//Adrian: keep going through the glass.
		if ( pOther->GetCollisionGroup() == COLLISION_GROUP_BREAKABLE_GLASS )
			 return;

		SetAbsVelocity( Vector( 0, 0, 0 ) );

		// play body "thwack" sound

		Vector vForward;

		AngleVectors( GetAbsAngles(), &vForward );
		VectorNormalize ( vForward );

		UTIL_TraceLine( GetAbsOrigin(),	GetAbsOrigin() + vForward * 128, MASK_OPAQUE, pOther, COLLISION_GROUP_NONE, &tr2 );

		if ( tr2.fraction != 1.0f )
		{
//			NDebugOverlay::Box( tr2.endpos, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 255, 0, 0, 10 );
//			NDebugOverlay::Box( GetAbsOrigin(), Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 0, 255, 0, 10 );

			if ( tr2.m_pEnt == NULL || ( tr2.m_pEnt && tr2.m_pEnt->GetMoveType() == MOVETYPE_NONE ) )
			{
				CEffectData	data;

				data.m_vOrigin = tr2.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = tr2.fraction != 1.0f;
			
				//DispatchEffect( "BoltImpact", data );
			}
		}
		
		SetTouch( NULL );
		SetThink( NULL );

		UTIL_Remove( this );
	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();

		// See if we struck the world
		if ( pOther->GetMoveType() == MOVETYPE_NONE && !( tr.surface.flags & SURF_SKY ) )
		{

			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = GetAbsVelocity();
			float speed = VectorNormalize( vecDir );

			// See if we should reflect off this surface
			float hitDot = DotProduct( tr.plane.normal, -vecDir );
			
			if ( ( hitDot < 0.5f ) && ( speed > 100 ) )
			{
				Vector vReflection = 2.0f * tr.plane.normal * hitDot + vecDir;
				
				QAngle reflectAngles;

				VectorAngles( vReflection, reflectAngles );

				SetLocalAngles( reflectAngles );

				SetAbsVelocity( vReflection * speed * 0.75f );

				// Start to sink faster
				SetGravity( 1.0f );
			}
			else
			{
				SetThink( &CPortalBolt::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );
				
				//FIXME: We actually want to stick (with hierarchy) to what we've hit
				SetMoveType( MOVETYPE_NONE );
			
				Vector vForward;

				AngleVectors( GetAbsAngles(), &vForward );
				VectorNormalize ( vForward );

				CEffectData	data;

				data.m_vOrigin = tr.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = 0;
			
				//DispatchEffect( "BoltImpact", data );
				
				//UTIL_ImpactTrace( &tr, DMG_BULLET );

				AddEffects( EF_NODRAW );
				SetTouch( NULL );
				SetThink( &CPortalBolt::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );

				if ( m_pGlowSprite != NULL )
				{
					m_pGlowSprite->TurnOn();
					m_pGlowSprite->FadeAndDie( 3.0f );
				}
			}
			
			// Shoot some sparks
			if ( UTIL_PointContents( GetAbsOrigin() ) != CONTENTS_WATER)
			{
				g_pEffects->Sparks( GetAbsOrigin() );
			}
		}
		else
		{
			// Put a mark unless we've hit the sky
			if ( ( tr.surface.flags & SURF_SKY ) == false )
			{
				//UTIL_ImpactTrace( &tr, DMG_BULLET );
			}

			UTIL_Remove( this );
		}
	}

	if ( g_pGameRules->IsMultiplayer() )
	{
//		SetThink( &CPortalBolt::ExplodeThink );
//		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//SecobMod__Information  If a Portal1 already exists, delete it.
CBaseEntity *ent3 = gEntList.FindEntityByName( NULL, Portal2Name );
				while ( ent3 )
				{
					CBaseEntity *next3 = gEntList.FindEntityByClassname( ent3, "info_playerstart" );
					EmitSound( "Portal.close_red" );
					UTIL_Remove( ent3 );
					ent3 = next3;
				}
CNPC_SecobModportal1 *pBullseye = static_cast<CNPC_SecobModportal1*>( CreateEntityByName( "npc_SecobModportal2" ) );
				if( pBullseye )
				{
				EmitSound( "Portal.open_red" );
				pBullseye->SetAbsOrigin( this->GetAbsOrigin() );
				pBullseye->SetAbsAngles( this->GetAbsAngles()  );
				pBullseye->KeyValue( "solid", "6" );
				pBullseye->KeyValue( "targetname", Portal2Name );
				pBullseye->Spawn();
				DispatchSpawn(pBullseye);
				pBullseye->Activate();
				pBullseye->SetHealth(999999999);
				}
				
//SecobMod__Information  If a Portal2 camera already exists, delete it.
CBaseEntity *cam3 = gEntList.FindEntityByName( NULL, "portalcamera2" );
				while ( cam3 )
				{
					CBaseEntity *next2 = gEntList.FindEntityByClassname( cam3, "info_playerstart" );
					UTIL_Remove( cam3 );
					cam3 = next2;
				}
				CPointCamera *cam4 = static_cast<CPointCamera*>( CreateEntityByName( "point_camera" ) );
				if (cam4)
				{
				Msg ("point cam created");
				cam4->SetAbsOrigin( this->GetAbsOrigin() );
				cam4->SetAbsAngles( this->GetAbsAngles()  );
				cam4->KeyValue( "targetname", "portalcamera2" );
				cam4->Spawn();
				DispatchSpawn(cam4);
				cam4->Activate();
				cam4->SetActive(true);
				}	
				
//SecobMod__Information  If a Portal2 monitor already exists, delete it.
CBaseEntity *ent4 = gEntList.FindEntityByClassname( NULL, "my_model_entity2" );
				while ( ent4 )
				{
					CBaseEntity *next4 = gEntList.FindEntityByClassname( ent4, "info_playerstart" );
					UTIL_Remove( ent4 );
					ent4 = next4;
				}
				CBaseEntity *pMonitor = CreateEntityByName( "my_model_entity2" );
				if (pMonitor)
				{
				pMonitor->SetAbsOrigin( this->GetAbsOrigin() );
				pMonitor->SetAbsAngles( this->GetAbsAngles()  );
				pMonitor->Spawn();
				DispatchSpawn(pMonitor);
				pMonitor->Activate();
				}
				
				return;
				}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPortalBolt::BubbleThink( void )
{
	QAngle angNewAngles;

	VectorAngles( GetAbsVelocity(), angNewAngles );
	SetAbsAngles( angNewAngles );

	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( GetWaterLevel()  == 0 )
		return;

	UTIL_BubbleTrail( GetAbsOrigin() - GetAbsVelocity() * 0.1f, GetAbsOrigin(), 5 );
}

#endif

//-----------------------------------------------------------------------------
// CWeaponPortalGun
//-----------------------------------------------------------------------------

#ifdef CLIENT_DLL
#define CWeaponPortalGun C_WeaponPortalGun
#endif

class CWeaponPortalGun : public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS( CWeaponPortalGun, CBaseHL2MPCombatWeapon );
public:

	CWeaponPortalGun( void );
	
	virtual void	Precache( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool	Reload( void );
	virtual void	ItemPostFrame( void );
	virtual void	ItemBusyFrame( void );
	virtual bool	SendWeaponAnim( int iActivity );

#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

private:
	
	void	SetSkin( int skinNum );
	void	FireBolt( void );
	void	FireBolt2( void );
	
	// Various states for the crossbow's charger
	enum ChargerState_t
	{
		CHARGER_STATE_START_LOAD,
		CHARGER_STATE_START_CHARGE,
		CHARGER_STATE_READY,
		CHARGER_STATE_DISCHARGE,
		CHARGER_STATE_OFF,
	};

	void	CreateChargerEffects( void );
	void	SetChargerState( ChargerState_t state );
	void	DoLoadEffect( void );

	DECLARE_ACTTABLE();

private:
	
	// Charger effects
	ChargerState_t		m_nChargeState;

#ifndef CLIENT_DLL
	CHandle<CSprite>	m_hChargerSprite;
#endif

	CNetworkVar( bool,	m_bMustReload );

	CWeaponPortalGun( const CWeaponPortalGun & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPortalGun, DT_WeaponPortalGun )

BEGIN_NETWORK_TABLE( CWeaponPortalGun, DT_WeaponPortalGun )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bMustReload ) ),
#else
	SendPropBool( SENDINFO( m_bMustReload ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponPortalGun )
	DEFINE_PRED_FIELD( m_bMustReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_portalgun, CWeaponPortalGun );

PRECACHE_WEAPON_REGISTER( weapon_portalgun );

acttable_t	CWeaponPortalGun::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_CROSSBOW,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_CROSSBOW,				false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_CROSSBOW,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_CROSSBOW,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW,			false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_CROSSBOW,					false },
};

IMPLEMENT_ACTTABLE(CWeaponPortalGun);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponPortalGun::CWeaponPortalGun( void )
{
	m_bReloadsSingly	= true;
	m_bFiresUnderwater	= true;
	m_bMustReload		= false;
}

#define	CROSSBOW_GLOW_SPRITE	"sprites/light_glow02_noz.vmt"
#define	CROSSBOW_GLOW_SPRITE2	"sprites/blueflare1.vmt"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPortalGun::Precache( void )
{
#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "portal_bolt" );
#endif

	PrecacheScriptSound( "Weapon_Crossbow.BoltHitBody" );
	PrecacheScriptSound( "Weapon_Crossbow.BoltHitWorld" );
	PrecacheScriptSound( "Weapon_Crossbow.BoltSkewer" );
	PrecacheScriptSound( "Portal.close_blue" );
	PrecacheScriptSound( "Portal.open_red" );

	PrecacheModel( CROSSBOW_GLOW_SPRITE );
	PrecacheModel( CROSSBOW_GLOW_SPRITE2 );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponPortalGun::PrimaryAttack( void )
{
		FireBolt();

	// Signal a reload
	m_bMustReload = true;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration( ACT_VM_PRIMARYATTACK ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponPortalGun::SecondaryAttack( void )
{
		FireBolt2();

	// Signal a reload
	m_bMustReload = true;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration( ACT_VM_PRIMARYATTACK ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponPortalGun::Reload( void )
{
	if ( BaseClass::Reload() )
	{
		m_bMustReload = false;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPortalGun::ItemBusyFrame( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPortalGun::ItemPostFrame( void )
{
	if ( m_bMustReload && HasWeaponIdleTimeElapsed() )
	{
		Reload();
	}
	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPortalGun::FireBolt( void )
{
EmitSound( "Weapon_Portalgun.powerup" );
	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			//WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}
EmitSound( "Weapon_Portalgun.fire_blue" );
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

#ifndef CLIENT_DLL
	Vector vecAiming	= pOwner->GetAutoaimVector( 0 );	
	Vector vecSrc		= pOwner->Weapon_ShootPosition();

	QAngle angAiming;
	VectorAngles( vecAiming, angAiming );

	CPortalBolt *pBolt = CPortalBolt::BoltCreate( vecSrc, angAiming, GetHL2MPWpnData().m_iPlayerDamage, pOwner );

	if ( pOwner->GetWaterLevel() == 3 )
	{
		pBolt->SetAbsVelocity( vecAiming * BOLT_WATER_VELOCITY );
	}
	else
	{
		pBolt->SetAbsVelocity( vecAiming * BOLT_AIR_VELOCITY );
	}

#endif

	//m_iClip1--;

	pOwner->ViewPunch( QAngle( -2, 0, 0 ) );

	//WeaponSound( SINGLE );
	//WeaponSound( SPECIAL2 );

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	if ( !m_iClip1 && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pOwner->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack	= gpGlobals->curtime + 0.75;

	DoLoadEffect();
	SetChargerState( CHARGER_STATE_DISCHARGE );
}


//-----------------------------------------------------------------------------
// Purpose: To function as the Portal2 spawner.
//-----------------------------------------------------------------------------
void CWeaponPortalGun::FireBolt2( void )
{
EmitSound( "Weapon_Portalgun.powerup" );
	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			//WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}
EmitSound( "Weapon_Portalgun.fire_red" );
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

#ifndef CLIENT_DLL
	Vector vecAiming	= pOwner->GetAutoaimVector( 0 );	
	Vector vecSrc		= pOwner->Weapon_ShootPosition();

	QAngle angAiming;
	VectorAngles( vecAiming, angAiming );

	CPortalBolt *pBolt2 = CPortalBolt::BoltCreate2( vecSrc, angAiming, GetHL2MPWpnData().m_iPlayerDamage, pOwner );

	if ( pOwner->GetWaterLevel() == 3 )
	{
		pBolt2->SetAbsVelocity( vecAiming * BOLT_WATER_VELOCITY );
	}
	else
	{
		pBolt2->SetAbsVelocity( vecAiming * BOLT_AIR_VELOCITY );
	}

#endif

	//m_iClip1--;

	pOwner->ViewPunch( QAngle( -2, 0, 0 ) );

	//WeaponSound( SINGLE );
	//WeaponSound( SPECIAL2 );

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	if ( !m_iClip1 && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pOwner->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack	= gpGlobals->curtime + 0.75;

	DoLoadEffect();
	SetChargerState( CHARGER_STATE_DISCHARGE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponPortalGun::Deploy( void )
{
	if ( m_iClip1 <= 0 )
	{
		return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_CROSSBOW_DRAW_UNLOADED, (char*)GetAnimPrefix() );
	}

	SetSkin( BOLT_SKIN_GLOW );

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSwitchingTo - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponPortalGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	SetChargerState( CHARGER_STATE_OFF );

	return BaseClass::Holster( pSwitchingTo );
}

#define	BOLT_TIP_ATTACHMENT	2

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPortalGun::CreateChargerEffects( void )
{
#ifndef CLIENT_DLL
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( m_hChargerSprite != NULL )
		return;

	m_hChargerSprite = CSprite::SpriteCreate( CROSSBOW_GLOW_SPRITE, GetAbsOrigin(), false );

	if ( m_hChargerSprite )
	{
		m_hChargerSprite->SetAttachment( pOwner->GetViewModel(), BOLT_TIP_ATTACHMENT );
		m_hChargerSprite->SetTransparency( kRenderTransAdd, 255, 128, 0, 255, kRenderFxNoDissipation );
		m_hChargerSprite->SetBrightness( 0 );
		m_hChargerSprite->SetScale( 0.1f );
		m_hChargerSprite->TurnOff();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : skinNum - 
//-----------------------------------------------------------------------------
void CWeaponPortalGun::SetSkin( int skinNum )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if ( pViewModel == NULL )
		return;

	pViewModel->m_nSkin = skinNum;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPortalGun::DoLoadEffect( void )
{
	SetSkin( BOLT_SKIN_GLOW );

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	//Tony; change this up a bit; on the server, dispatch an effect but don't send it to the client who fires
	//on the client, create an effect either in the view model, or on the world model if first person.
	CEffectData	data;

	data.m_nAttachmentIndex = 1;

#ifdef GAME_DLL
	data.m_nEntIndex = entindex();

	CPASFilter filter( data.m_vOrigin );
	filter.RemoveRecipient( pOwner );
	//te->DispatchEffect( filter, 0.0, data.m_vOrigin, "CrossbowLoad", data );
#else
	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if ( pViewModel != NULL )
	{

		if ( ::input->CAM_IsThirdPerson() )
			data.m_hEntity = pViewModel->GetRefEHandle();
		else
			data.m_hEntity = GetRefEHandle();
		//DispatchEffect( "CrossbowLoad", data );
	}
#endif

	//Tony; switched this up, always attach it to the weapon, not the view model!!
#ifndef CLIENT_DLL
	CSprite *pBlast = CSprite::SpriteCreate( CROSSBOW_GLOW_SPRITE2, GetAbsOrigin(), false );

	if ( pBlast )
	{
		pBlast->SetAttachment( this, 1 );
		pBlast->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone );
		pBlast->SetBrightness( 128 );
		pBlast->SetScale( 0.2f );
		pBlast->FadeOutFromSpawn();
	}
#endif
	
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CWeaponPortalGun::SetChargerState( ChargerState_t state )
{
	// Make sure we're setup
	CreateChargerEffects();

	// Don't do this twice
	if ( state == m_nChargeState )
		return;

	m_nChargeState = state;

	switch( m_nChargeState )
	{
	case CHARGER_STATE_START_LOAD:
	
		WeaponSound( SPECIAL1 );
		
		// Shoot some sparks and draw a beam between the two outer points
		DoLoadEffect();
		
		break;
#ifndef CLIENT_DLL
	case CHARGER_STATE_START_CHARGE:
		{
			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 32, 0.5f );
			m_hChargerSprite->SetScale( 0.025f, 0.5f );
			m_hChargerSprite->TurnOn();
		}

		break;

	case CHARGER_STATE_READY:
		{
			// Get fully charged
			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 80, 1.0f );
			m_hChargerSprite->SetScale( 0.1f, 0.5f );
			m_hChargerSprite->TurnOn();
		}

		break;

	case CHARGER_STATE_DISCHARGE:
		{
			SetSkin( BOLT_SKIN_NORMAL );
			
			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 0 );
			m_hChargerSprite->TurnOff();
		}

		break;
#endif
	case CHARGER_STATE_OFF:
		{
			SetSkin( BOLT_SKIN_NORMAL );

#ifndef CLIENT_DLL
			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 0 );
			m_hChargerSprite->TurnOff();
#endif
		}
		break;

	default:
		break;
	}
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponPortalGun::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_THROW:
		SetChargerState( CHARGER_STATE_START_LOAD );
		break;

	case EVENT_WEAPON_THROW2:
		SetChargerState( CHARGER_STATE_START_CHARGE );
		break;
	
	case EVENT_WEAPON_THROW3:
		SetChargerState( CHARGER_STATE_READY );
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Set the desired activity for the weapon and its viewmodel counterpart
// Input  : iActivity - activity to play
//-----------------------------------------------------------------------------
bool CWeaponPortalGun::SendWeaponAnim( int iActivity )
{
	int newActivity = iActivity;

	// The last shot needs a non-loaded activity
	if ( ( newActivity == ACT_VM_IDLE ) && ( m_iClip1 <= 0 ) )
	{
		newActivity = ACT_VM_FIDGET;
	}

	//For now, just set the ideal activity and be done with it
	return BaseClass::SendWeaponAnim( newActivity );
}
#endif //SecobMod__ENABLE_IN_TESTING_ITEMS