//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Dissolve entity to be attached to target entity. Serves two purposes:
//
//			1) An entity that can be placed by a level designer and triggered
//			   to ignite a target entity.
//
//			2) An entity that can be created at runtime to ignite a target entity.
//
//=============================================================================//

#include "cbase.h"
#include "RagdollBoogie.h"
#include "physics_prop_ragdoll.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "IEffects.h"
#ifdef MAPBASE
#include "saverestore_utlvector.h"
#include "interval.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Make electriciy every so often
//-----------------------------------------------------------------------------
static const char *s_pZapContext = "ZapContext";


//-----------------------------------------------------------------------------
// Save/load 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CRagdollBoogie )

	DEFINE_FIELD( m_flStartTime, FIELD_TIME ),
	DEFINE_FIELD( m_flBoogieLength, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMagnitude, FIELD_FLOAT ),

	// Think this should be handled by StartTouch/etc.
//	DEFINE_FIELD( m_nSuppressionCount, FIELD_INTEGER ),

#ifdef MAPBASE
	DEFINE_FIELD( m_vecColor, FIELD_VECTOR ),
#endif

	DEFINE_FUNCTION( BoogieThink ),
	DEFINE_FUNCTION( ZapThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( env_ragdoll_boogie, CRagdollBoogie );


//-----------------------------------------------------------------------------
// Purpose: Creates a flame and attaches it to a target entity.
// Input  : pTarget - 
//-----------------------------------------------------------------------------
CRagdollBoogie *CRagdollBoogie::Create( CBaseEntity *pTarget, float flMagnitude, 
#ifdef MAPBASE
	float flStartTime, float flLengthTime, int nSpawnFlags, const Vector *vecColor )
#else
	float flStartTime, float flLengthTime, int nSpawnFlags )
#endif
{
	CRagdollProp *pRagdoll = dynamic_cast< CRagdollProp* >( pTarget );
	if ( !pRagdoll )
		return NULL;

	CRagdollBoogie *pBoogie = (CRagdollBoogie *)CreateEntityByName( "env_ragdoll_boogie" );
	if ( pBoogie == NULL )
		return NULL;

	pBoogie->AddSpawnFlags( nSpawnFlags );
	pBoogie->AttachToEntity( pTarget );
	pBoogie->SetBoogieTime( flStartTime, flLengthTime );
	pBoogie->SetMagnitude( flMagnitude );
#ifdef MAPBASE
	if (vecColor != NULL)
		pBoogie->SetColor( *vecColor );
#endif
	pBoogie->Spawn();
	return pBoogie;
}


//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CRagdollBoogie::Spawn()
{
	BaseClass::Spawn();

	SetThink( &CRagdollBoogie::BoogieThink );
	SetNextThink( gpGlobals->curtime + 0.01f );

	if ( HasSpawnFlags( SF_RAGDOLL_BOOGIE_ELECTRICAL ) )
	{
		SetContextThink( &CRagdollBoogie::ZapThink, gpGlobals->curtime + random->RandomFloat( 0.1f, 0.3f ), s_pZapContext ); 
	}
}


//-----------------------------------------------------------------------------
// Zap!
//-----------------------------------------------------------------------------
void CRagdollBoogie::ZapThink()
{
	if ( !GetMoveParent() )
		return;

	CBaseAnimating *pRagdoll = GetMoveParent()->GetBaseAnimating();
	if ( !pRagdoll )
		return;

	// Make electricity on the client
	CStudioHdr *pStudioHdr = pRagdoll->GetModelPtr( );
	if (!pStudioHdr)
		return;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pRagdoll->GetHitboxSet() );

	if ( set->numhitboxes == 0 )
		return;

	if ( m_nSuppressionCount == 0 )
	{
		CEffectData	data;
		
		data.m_nEntIndex = GetMoveParent()->entindex();
		data.m_flMagnitude = 4;
		data.m_flScale = HasSpawnFlags(SF_RAGDOLL_BOOGIE_ELECTRICAL_NARROW_BEAM) ? 1.0f : 2.0f;
#ifdef MAPBASE
		if (!m_vecColor.IsZero())
		{
			data.m_bCustomColors = true;
			data.m_CustomColors.m_vecColor1 = m_vecColor;
		}
#endif

		DispatchEffect( "TeslaHitboxes", data );	
	}

#ifdef HL2_EPISODIC
	EmitSound( "RagdollBoogie.Zap" );
#endif

	SetContextThink( &CRagdollBoogie::ZapThink, gpGlobals->curtime + random->RandomFloat( 0.1f, 0.3f ), s_pZapContext ); 
}


//-----------------------------------------------------------------------------
// Suppression count
//-----------------------------------------------------------------------------
void CRagdollBoogie::IncrementSuppressionCount( CBaseEntity *pTarget )
{
	// Look for other boogies on the ragdoll + kill them
	for ( CBaseEntity *pChild = pTarget->FirstMoveChild(); pChild; pChild = pChild->NextMovePeer() )
	{
		CRagdollBoogie *pBoogie = dynamic_cast<CRagdollBoogie*>(pChild);
		if ( !pBoogie )
			continue;

		++pBoogie->m_nSuppressionCount;
	}
}

void CRagdollBoogie::DecrementSuppressionCount( CBaseEntity *pTarget )
{
	// Look for other boogies on the ragdoll + kill them
	CBaseEntity *pNext;
	for ( CBaseEntity *pChild = pTarget->FirstMoveChild(); pChild; pChild = pNext )
	{
		pNext = pChild->NextMovePeer();
		CRagdollBoogie *pBoogie = dynamic_cast<CRagdollBoogie*>(pChild);
		if ( !pBoogie )
			continue;

		if ( --pBoogie->m_nSuppressionCount <= 0 )
		{
			pBoogie->m_nSuppressionCount = 0;

			float dt = gpGlobals->curtime - pBoogie->m_flStartTime;
			if ( dt >= pBoogie->m_flBoogieLength )
			{
				PhysCallbackRemove( pBoogie->NetworkProp() );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Attach to an entity
//-----------------------------------------------------------------------------
void CRagdollBoogie::AttachToEntity( CBaseEntity *pTarget )
{
	m_nSuppressionCount = 0;

	// Look for other boogies on the ragdoll + kill them
	CBaseEntity *pNext;
	for ( CBaseEntity *pChild = pTarget->FirstMoveChild(); pChild; pChild = pNext )
	{
		pNext = pChild->NextMovePeer();
		CRagdollBoogie *pBoogie = dynamic_cast<CRagdollBoogie*>(pChild);
		if ( !pBoogie )
			continue;

		m_nSuppressionCount = pBoogie->m_nSuppressionCount;
		UTIL_Remove( pChild );
	}

	FollowEntity( pTarget );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : lifetime - 
//-----------------------------------------------------------------------------
void CRagdollBoogie::SetBoogieTime( float flStartTime, float flLengthTime )
{
	m_flStartTime = flStartTime;
	m_flBoogieLength = flLengthTime;
}


//-----------------------------------------------------------------------------
// Purpose: Burn targets around us
//-----------------------------------------------------------------------------
void CRagdollBoogie::SetMagnitude( float flMagnitude )
{
	m_flMagnitude = flMagnitude;
}


//-----------------------------------------------------------------------------
// Purpose: Burn targets around us
//-----------------------------------------------------------------------------
void CRagdollBoogie::BoogieThink( void )
{
	CRagdollProp *pRagdoll = dynamic_cast< CRagdollProp* >( GetMoveParent() );
	if ( !pRagdoll )
	{
		UTIL_Remove( this );
		return;
	}

	float flMagnitude = m_flMagnitude;
	if ( m_flBoogieLength != 0 )
	{
		float dt = gpGlobals->curtime - m_flStartTime;
		if ( dt >= m_flBoogieLength )
		{
			// Don't remove while suppressed... this helps if we try to start another boogie
			if ( m_nSuppressionCount == 0 )
			{
				UTIL_Remove( this );
			}
			SetThink( NULL );
			return;
		}

		if ( dt < 0 )
		{
			SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.1, 0.2f ) );
			return;
		}

		flMagnitude = SimpleSplineRemapVal( dt, 0.0f, m_flBoogieLength, m_flMagnitude, 0.0f ); 
	}

#ifndef _XBOX
	if ( m_nSuppressionCount == 0 )
	{
		ragdoll_t *pRagdollPhys = pRagdoll->GetRagdoll( );
		for ( int j = 0; j < pRagdollPhys->listCount; ++j )
		{
			float flMass = pRagdollPhys->list[j].pObject->GetMass();
			float flForce = m_flMagnitude * flMass;

			Vector vecForce;
			vecForce = RandomVector( -flForce, flForce );
			pRagdollPhys->list[j].pObject->ApplyForceCenter( vecForce ); 
		}
	}
#endif // !_XBOX

	SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.1, 0.2f ) );
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Allows mappers to control ragdoll dancing
//-----------------------------------------------------------------------------
class CPointRagdollBoogie : public CBaseEntity 
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CPointRagdollBoogie, CBaseEntity );

public:
	bool ApplyBoogie(CBaseEntity *pTarget, CBaseEntity *pActivator);

	void InputActivate( inputdata_t &inputdata );
	void InputDeactivate( inputdata_t &inputdata );
	void InputBoogieTarget( inputdata_t &inputdata );
	void InputSetZapColor( inputdata_t &inputdata );

	bool KeyValue( const char *szKeyName, const char *szValue );

private:
	float m_flStartTime;
	interval_t m_BoogieLength;
	float m_flMagnitude;

	Vector m_vecZapColor;

	// This allows us to change or remove active boogies later.
	CUtlVector<CHandle<CRagdollBoogie>> m_Boogies;
};

//-----------------------------------------------------------------------------
// Save/load 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CPointRagdollBoogie )

	DEFINE_KEYFIELD( m_flStartTime, FIELD_FLOAT, "StartTime" ),
	DEFINE_KEYFIELD( m_BoogieLength, FIELD_INTERVAL, "BoogieLength" ),
	DEFINE_KEYFIELD( m_flMagnitude, FIELD_FLOAT, "Magnitude" ),

	DEFINE_KEYFIELD( m_vecZapColor, FIELD_VECTOR, "ZapColor" ),

	// Think this should be handled by StartTouch/etc.
//	DEFINE_FIELD( m_nSuppressionCount, FIELD_INTEGER ),

	DEFINE_UTLVECTOR( m_Boogies, FIELD_EHANDLE ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Deactivate", InputDeactivate ),
	DEFINE_INPUTFUNC( FIELD_STRING, "BoogieTarget", InputBoogieTarget ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetZapColor", InputSetZapColor ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( point_ragdollboogie, CPointRagdollBoogie );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
bool CPointRagdollBoogie::ApplyBoogie( CBaseEntity *pTarget, CBaseEntity *pActivator )
{
	if (dynamic_cast<CRagdollProp*>(pTarget))
	{
		m_Boogies.AddToTail(CRagdollBoogie::Create(pTarget, m_flMagnitude, gpGlobals->curtime + m_flStartTime, RandomInterval(m_BoogieLength), GetSpawnFlags(), &m_vecZapColor));
	}
	else if (pTarget->MyCombatCharacterPointer())
	{
		// Basically CBaseCombatCharacter::BecomeRagdollBoogie(), but adjusted to our needs
		CTakeDamageInfo info(this, pActivator, 1.0f, DMG_GENERIC);

		CBaseEntity *pRagdoll = CreateServerRagdoll(pTarget->MyCombatCharacterPointer(), 0, info, COLLISION_GROUP_INTERACTIVE_DEBRIS, true);

		pRagdoll->SetCollisionBounds(CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs());

		m_Boogies.AddToTail(CRagdollBoogie::Create(pRagdoll, m_flMagnitude, gpGlobals->curtime + m_flStartTime, RandomInterval(m_BoogieLength), GetSpawnFlags(), &m_vecZapColor));

		CTakeDamageInfo ragdollInfo(this, pActivator, 10000.0, DMG_GENERIC | DMG_REMOVENORAGDOLL);
		ragdollInfo.SetDamagePosition(WorldSpaceCenter());
		ragdollInfo.SetDamageForce(Vector(0, 0, 1));
		ragdollInfo.SetForceFriendlyFire(true);
		pTarget->TakeDamage(ragdollInfo);
	}
	else
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPointRagdollBoogie::InputActivate( inputdata_t &inputdata )
{
	CBaseEntity *pEnt = gEntList.FindEntityByName(NULL, STRING(m_target), this, inputdata.pActivator, inputdata.pCaller);
	while (pEnt)
	{
		ApplyBoogie(pEnt, inputdata.pActivator);

		pEnt = gEntList.FindEntityByName(pEnt, STRING(m_target), this, inputdata.pActivator, inputdata.pCaller);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPointRagdollBoogie::InputDeactivate( inputdata_t &inputdata )
{
	if (m_Boogies.Count() == 0)
		return;

	for (int i = 0; i < m_Boogies.Count(); i++)
	{
		UTIL_Remove(m_Boogies[i]);
	}

	m_Boogies.Purge();

	//m_Boogies.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPointRagdollBoogie::InputBoogieTarget( inputdata_t &inputdata )
{
	CBaseEntity *pEnt = gEntList.FindEntityByName(NULL, inputdata.value.String(), this, inputdata.pActivator, inputdata.pCaller);
	while (pEnt)
	{
		if (!ApplyBoogie(pEnt, inputdata.pActivator))
		{
			Warning("%s was unable to apply ragdoll boogie to %s, classname %s.\n", GetDebugName(), pEnt->GetDebugName(), pEnt->GetClassname());
		}

		pEnt = gEntList.FindEntityByName(pEnt, inputdata.value.String(), this, inputdata.pActivator, inputdata.pCaller);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPointRagdollBoogie::InputSetZapColor( inputdata_t &inputdata )
{
	inputdata.value.Vector3D( m_vecZapColor );
	if (!m_vecZapColor.IsZero())
	{
		// Turn into ratios of 255
		m_vecZapColor /= 255.0f;
	}

	// Apply to existing boogies
	for (int i = 0; i < m_Boogies.Count(); i++)
	{
		if (m_Boogies[i])
		{
			m_Boogies[i]->SetColor( m_vecZapColor );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles key values from the BSP before spawn is called.
//-----------------------------------------------------------------------------
bool CPointRagdollBoogie::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "ZapColor" ) )
	{
		UTIL_StringToVector(m_vecZapColor.Base(), szValue);
		if (!m_vecZapColor.IsZero())
		{
			// Turn into ratios of 255
			m_vecZapColor /= 255.0f;
		}
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}
#endif
