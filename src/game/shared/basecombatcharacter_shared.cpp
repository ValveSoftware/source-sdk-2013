//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Purpose: Switches to the best weapon that is also better than the given weapon.
// Input  : pCurrent - The current weapon used by the player.
// Output : Returns true if the weapon was switched, false if there was no better
//			weapon to switch to.
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::SwitchToNextBestWeapon(CBaseCombatWeapon *pCurrent)
{
	CBaseCombatWeapon *pNewWeapon = g_pGameRules->GetNextBestWeapon(this, pCurrent);
	
	if ( ( pNewWeapon != NULL ) && ( pNewWeapon != pCurrent ) )
	{
		return Weapon_Switch( pNewWeapon );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Switches to the given weapon (providing it has ammo)
// Input  :
// Output : true is switch succeeded
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex /*=0*/ ) 
{
	if ( pWeapon == NULL )
		return false;

	// Already have it out?
	if ( m_hActiveWeapon.Get() == pWeapon )
	{
		if ( !m_hActiveWeapon->IsWeaponVisible() || m_hActiveWeapon->IsHolstered() )
			return m_hActiveWeapon->Deploy( );
		return false;
	}

	if (!Weapon_CanSwitchTo(pWeapon))
	{
		return false;
	}

	if ( m_hActiveWeapon )
	{
		if ( !m_hActiveWeapon->Holster( pWeapon ) )
			return false;
	}

	m_hActiveWeapon = pWeapon;

	return pWeapon->Deploy( );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not we can switch to the given weapon.
// Input  : pWeapon - 
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon )
{
	if (IsPlayer())
	{
		CBasePlayer *pPlayer = (CBasePlayer *)this;
#if !defined( CLIENT_DLL )
		IServerVehicle *pVehicle = pPlayer->GetVehicle();
#else
		IClientVehicle *pVehicle = pPlayer->GetVehicle();
#endif
		if (pVehicle && !pPlayer->UsingStandardWeaponsInVehicle())
			return false;
	}

	if ( !pWeapon->HasAnyAmmo() && !GetAmmoCount( pWeapon->m_iPrimaryAmmoType ) )
		return false;

	if ( !pWeapon->CanDeploy() )
		return false;
	
	if ( m_hActiveWeapon )
	{
		if ( !m_hActiveWeapon->CanHolster() )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseCombatWeapon
//-----------------------------------------------------------------------------
CBaseCombatWeapon *CBaseCombatCharacter::GetActiveWeapon() const
{
	return m_hActiveWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iCount - 
//			iAmmoIndex - 
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::RemoveAmmo( int iCount, int iAmmoIndex )
{
	if (iCount <= 0)
		return;

	// Infinite ammo?
	if ( GetAmmoDef()->MaxCarry( iAmmoIndex ) == INFINITE_AMMO )
		return;

	// Ammo pickup sound
	m_iAmmo.Set( iAmmoIndex, MAX( m_iAmmo[iAmmoIndex] - iCount, 0 ) );
}

void CBaseCombatCharacter::RemoveAmmo( int iCount, const char *szName )
{
	RemoveAmmo( iCount, GetAmmoDef()->Index(szName) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::RemoveAllAmmo( )
{
	for ( int i = 0; i < MAX_AMMO_SLOTS; i++ )
	{
		m_iAmmo.Set( i, 0 );
	}
}

//-----------------------------------------------------------------------------
// FIXME: This is a sort of hack back-door only used by physgun!
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetAmmoCount( int iCount, int iAmmoIndex )
{
	// NOTE: No sound, no max check! Seems pretty bogus to me!
	m_iAmmo.Set( iAmmoIndex, iCount );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of a particular type owned
//			owned by the character
// Input  :	Ammo Index
// Output :	The amount of ammo
//-----------------------------------------------------------------------------
int CBaseCombatCharacter::GetAmmoCount( int iAmmoIndex ) const
{
	if ( iAmmoIndex == -1 )
		return 0;

	// Infinite ammo?
	if ( GetAmmoDef()->MaxCarry( iAmmoIndex ) == INFINITE_AMMO )
		return 999;

	return m_iAmmo[ iAmmoIndex ];
}

//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of the specified type the character's carrying
//-----------------------------------------------------------------------------
int	CBaseCombatCharacter::GetAmmoCount( char *szName ) const
{
	return GetAmmoCount( GetAmmoDef()->Index(szName) );
}

//-----------------------------------------------------------------------------
// Purpose: Returns weapon if already owns a weapon of this class
//-----------------------------------------------------------------------------
CBaseCombatWeapon* CBaseCombatCharacter::Weapon_OwnsThisType( const char *pszWeapon, int iSubType ) const
{
	// Check for duplicates
	for (int i=0;i<MAX_WEAPONS;i++) 
	{
		if ( m_hMyWeapons[i].Get() && FClassnameIs( m_hMyWeapons[i], pszWeapon ) )
		{
			// Make sure it matches the subtype
			if ( m_hMyWeapons[i]->GetSubType() == iSubType )
				return m_hMyWeapons[i];
		}
	}
	return NULL;
}


int CBaseCombatCharacter::BloodColor()
{
	return m_bloodColor;
}


//-----------------------------------------------------------------------------
// Blood color (see BLOOD_COLOR_* macros in baseentity.h)
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetBloodColor( int nBloodColor )
{
	m_bloodColor = nBloodColor;
}

//-----------------------------------------------------------------------------
/**
	The main visibility check.  Checks all the entity specific reasons that could 
	make IsVisible fail.  Then checks points in space to get environmental reasons.
	This is LOS, plus invisibility and fog and smoke and such.
*/

enum VisCacheResult_t
{
	VISCACHE_UNKNOWN = 0,
	VISCACHE_IS_VISIBLE,
	VISCACHE_IS_NOT_VISIBLE,
};

enum
{
	VIS_CACHE_INVALID = 0x80000000
};

#define VIS_CACHE_ENTRY_LIFE .090f

class CCombatCharVisCache : public CAutoGameSystemPerFrame
{
public:
	virtual void FrameUpdatePreEntityThink();
	virtual void LevelShutdownPreEntity();

	int LookupVisibility( const CBaseCombatCharacter *pChar1, CBaseCombatCharacter *pChar2 );
	VisCacheResult_t HasVisibility( int iCache ) const;
	void RegisterVisibility( int iCache, bool bChar1SeesChar2, bool bChar2SeesChar1 );

private:
	struct VisCacheEntry_t
	{
		CHandle< CBaseCombatCharacter >	m_hEntity1;
		CHandle< CBaseCombatCharacter >	m_hEntity2;
		float							m_flTime;
		bool							m_bEntity1CanSeeEntity2;
		bool							m_bEntity2CanSeeEntity1;
	};

	class CVisCacheEntryLess
	{
	public:
		CVisCacheEntryLess( int ) {}
		bool operator!() const { return false; }
		bool operator()( const VisCacheEntry_t &lhs, const VisCacheEntry_t &rhs ) const
		{
			return ( memcmp( &lhs, &rhs, offsetof( VisCacheEntry_t, m_flTime ) ) < 0 );
		}
	};

	CUtlRBTree< VisCacheEntry_t, unsigned short, CVisCacheEntryLess > m_VisCache;

	mutable int m_nTestCount;
	mutable int m_nHitCount;
};

void CCombatCharVisCache::FrameUpdatePreEntityThink()
{
	//	Msg( "test: %d/%d\n", m_nHitCount, m_nTestCount );

	// Lazy retirement of vis cache
	// NOTE: 256 was chosen heuristically based on a playthrough where 200
	// was the max # in the viscache where nothing could be retired.
	if ( m_VisCache.Count() < 256 )
		return;

	int nMaxIndex = m_VisCache.MaxElement() - 1;
	for ( int i = 0; i < 8; ++i )
	{
		int n = RandomInt( 0, nMaxIndex );
		if ( !m_VisCache.IsValidIndex( n ) )
			continue;

		const VisCacheEntry_t &entry = m_VisCache[n];
		if ( !entry.m_hEntity1.IsValid() || !entry.m_hEntity2.IsValid() || ( gpGlobals->curtime - entry.m_flTime > 10.0f ) )
		{
			m_VisCache.RemoveAt( n );
		}
	}
}

void CCombatCharVisCache::LevelShutdownPreEntity()
{
	m_VisCache.Purge();
}

int CCombatCharVisCache::LookupVisibility( const CBaseCombatCharacter *pChar1, CBaseCombatCharacter *pChar2 )
{
	VisCacheEntry_t cacheEntry;
	if ( pChar1 < pChar2 )
	{
		cacheEntry.m_hEntity1 = pChar1;
		cacheEntry.m_hEntity2 = pChar2;
	}
	else
	{
		cacheEntry.m_hEntity1 = pChar2;
		cacheEntry.m_hEntity2 = pChar1;
	}

	int iCache = m_VisCache.Find( cacheEntry );
	if ( iCache == m_VisCache.InvalidIndex() )
	{
		if ( m_VisCache.Count() == m_VisCache.InvalidIndex() )
			return VIS_CACHE_INVALID;

		iCache = m_VisCache.Insert( cacheEntry );
		m_VisCache[iCache].m_flTime = gpGlobals->curtime - 2.0f * VIS_CACHE_ENTRY_LIFE;
	}

	return ( pChar1 < pChar2 ) ? iCache : - iCache - 1;
}

VisCacheResult_t CCombatCharVisCache::HasVisibility( int iCache ) const
{
	if ( iCache == VIS_CACHE_INVALID )
		return VISCACHE_UNKNOWN;
	
	m_nTestCount++;

	bool bReverse = ( iCache < 0 );
	if ( bReverse )
	{
		iCache = - iCache - 1;
	}

	const VisCacheEntry_t &entry = m_VisCache[iCache];
	if ( gpGlobals->curtime - entry.m_flTime > VIS_CACHE_ENTRY_LIFE )
		return VISCACHE_UNKNOWN;

	m_nHitCount++;

	bool bIsVisible = !bReverse ? entry.m_bEntity1CanSeeEntity2 : entry.m_bEntity2CanSeeEntity1;
	return bIsVisible ? VISCACHE_IS_VISIBLE : VISCACHE_IS_NOT_VISIBLE;
}

void CCombatCharVisCache::RegisterVisibility( int iCache, bool bEntity1CanSeeEntity2, bool bEntity2CanSeeEntity1 )
{
	if ( iCache == VIS_CACHE_INVALID )
		return;

	bool bReverse = ( iCache < 0 );
	if ( bReverse )
	{
		iCache = - iCache - 1;
	}

	VisCacheEntry_t &entry = m_VisCache[iCache];
	entry.m_flTime = gpGlobals->curtime;
	if ( !bReverse )
	{
		entry.m_bEntity1CanSeeEntity2 = bEntity1CanSeeEntity2;
		entry.m_bEntity2CanSeeEntity1 = bEntity2CanSeeEntity1;
	}
	else
	{
		entry.m_bEntity1CanSeeEntity2 = bEntity2CanSeeEntity1;
		entry.m_bEntity2CanSeeEntity1 = bEntity1CanSeeEntity2;
	}
}

static CCombatCharVisCache s_CombatCharVisCache;

bool CBaseCombatCharacter::IsAbleToSee( const CBaseEntity *pEntity, FieldOfViewCheckType checkFOV )
{
	CBaseCombatCharacter *pBCC = const_cast<CBaseEntity *>( pEntity )->MyCombatCharacterPointer();
	if ( pBCC )
		return IsAbleToSee( pBCC, checkFOV );

	// Test this every time; it's cheap.
	Vector vecEyePosition = EyePosition();
	Vector vecTargetPosition = pEntity->WorldSpaceCenter();

#ifdef GAME_DLL
	Vector vecEyeToTarget;
	VectorSubtract( vecTargetPosition, vecEyePosition, vecEyeToTarget );
	float flDistToOther = VectorNormalize( vecEyeToTarget ); 

	// We can't see because they are too far in the fog
	if ( IsHiddenByFog( flDistToOther ) )
		return false;
#endif

	if ( !ComputeLOS( vecEyePosition, vecTargetPosition ) )
		return false;

#if defined(GAME_DLL) && defined(TERROR)
	if ( flDistToOther > NavObscureRange.GetFloat() )
	{
		const float flMaxDistance = 100.0f;
		TerrorNavArea *pTargetArea = static_cast< TerrorNavArea* >( TheNavMesh->GetNearestNavArea( vecTargetPosition, false, flMaxDistance ) );
		if ( !pTargetArea || pTargetArea->HasSpawnAttributes( TerrorNavArea::SPAWN_OBSCURED ) )
			return false;

		if ( ComputeTargetIsInDarkness( vecEyePosition, pTargetArea, vecTargetPosition ) )
			return false;
	}
#endif

	return ( checkFOV != USE_FOV || IsInFieldOfView( vecTargetPosition ) );
}

static void ComputeSeeTestPosition( Vector *pEyePosition, CBaseCombatCharacter *pBCC )
{
#if defined(GAME_DLL) && defined(TERROR)
	if ( pBCC->IsPlayer() )
	{
		CTerrorPlayer *pPlayer = ToTerrorPlayer( pBCC );
		*pEyePosition = !pPlayer->IsDead() ? pPlayer->EyePosition() : pPlayer->GetDeathPosition();
	}
	else
#endif
	{
		*pEyePosition = pBCC->EyePosition();
	}	
}

bool CBaseCombatCharacter::IsAbleToSee( CBaseCombatCharacter *pBCC, FieldOfViewCheckType checkFOV )
{
	Vector vecEyePosition, vecOtherEyePosition;
	ComputeSeeTestPosition( &vecEyePosition, this );
	ComputeSeeTestPosition( &vecOtherEyePosition, pBCC );

#ifdef GAME_DLL
	Vector vecEyeToTarget;
	VectorSubtract( vecOtherEyePosition, vecEyePosition, vecEyeToTarget );
	float flDistToOther = VectorNormalize( vecEyeToTarget ); 

	// Test this every time; it's cheap.
	// We can't see because they are too far in the fog
	if ( IsHiddenByFog( flDistToOther ) )
		return false;

#ifdef TERROR
	// Check this every time also, it's cheap; check to see if the enemy is in an obscured area.
	bool bIsInNavObscureRange = ( flDistToOther > NavObscureRange.GetFloat() );
	if ( bIsInNavObscureRange )
	{
		TerrorNavArea *pOtherNavArea = static_cast< TerrorNavArea* >( pBCC->GetLastKnownArea() );
		if ( !pOtherNavArea || pOtherNavArea->HasSpawnAttributes( TerrorNavArea::SPAWN_OBSCURED ) )
			return false;
	}
#endif // TERROR
#endif

	// Check if we have a cached-off visibility
	int iCache = s_CombatCharVisCache.LookupVisibility( this, pBCC );
	VisCacheResult_t nResult = s_CombatCharVisCache.HasVisibility( iCache );

	// Compute symmetric visibility
	if ( nResult == VISCACHE_UNKNOWN )
	{
		bool bThisCanSeeOther = false, bOtherCanSeeThis = false;
		if ( ComputeLOS( vecEyePosition, vecOtherEyePosition ) )
		{
#if defined(GAME_DLL) && defined(TERROR)
			if ( !bIsInNavObscureRange )
			{
				bThisCanSeeOther = true, bOtherCanSeeThis = true;
			}
			else
			{
				bThisCanSeeOther = !ComputeTargetIsInDarkness( vecEyePosition, pBCC->GetLastKnownArea(), vecOtherEyePosition );
				bOtherCanSeeThis = !ComputeTargetIsInDarkness( vecOtherEyePosition, GetLastKnownArea(), vecEyePosition );
			}
#else
			bThisCanSeeOther = true, bOtherCanSeeThis = true;
#endif
		}

		s_CombatCharVisCache.RegisterVisibility( iCache, bThisCanSeeOther, bOtherCanSeeThis );
		nResult = bThisCanSeeOther ? VISCACHE_IS_VISIBLE : VISCACHE_IS_NOT_VISIBLE;
	}

	if ( nResult == VISCACHE_IS_VISIBLE )
		return ( checkFOV != USE_FOV || IsInFieldOfView( pBCC ) );

	return false;
}

class CTraceFilterNoCombatCharacters : public CTraceFilterSimple
{
public:
	CTraceFilterNoCombatCharacters( const IHandleEntity *passentity = NULL, int collisionGroup = COLLISION_GROUP_NONE )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask ) )
		{
			CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
			if ( !pEntity )
				return NULL;

			if ( pEntity->MyCombatCharacterPointer() || pEntity->MyCombatWeaponPointer() )
				return false;

			// Honor BlockLOS - this lets us see through partially-broken doors, etc
			if ( !pEntity->BlocksLOS() )
				return false;

			return true;
		}

		return false;
	}
};

bool CBaseCombatCharacter::ComputeLOS( const Vector &vecEyePosition, const Vector &vecTarget ) const
{
	// We simply can't see because the world is in the way.
	trace_t result;
	CTraceFilterNoCombatCharacters traceFilter( NULL, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEyePosition, vecTarget, MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE | CONTENTS_MONSTER, &traceFilter, &result );
	return ( result.fraction == 1.0f );
}

#if defined(GAME_DLL) && defined(TERROR)
bool CBaseCombatCharacter::ComputeTargetIsInDarkness( const Vector &vecEyePosition, CNavArea *pTargetNavArea, const Vector &vecTargetPos ) const
{
	if ( GetTeamNumber() != TEAM_SURVIVOR )
		return false;

	// Check light info
	const float flMinLightIntensity = 0.1f;

	if ( !pTargetNavArea || ( pTargetNavArea->GetLightIntensity() >= flMinLightIntensity ) )
		return false;

	CTraceFilterNoNPCsOrPlayer lightingFilter( this, COLLISION_GROUP_NONE );

	Vector vecSightDirection;
	VectorSubtract( vecTargetPos, vecEyePosition, vecSightDirection );
	VectorNormalize( vecSightDirection );

	trace_t result;
	UTIL_TraceLine( vecTargetPos, vecTargetPos + vecSightDirection * 32768.0f, MASK_L4D_VISION, &lightingFilter, &result );
	if ( ( result.fraction < 1.0f ) && ( ( result.surface.flags & SURF_SKY ) == 0 ) )	
	{
		const float flMaxDistance = 100.0f;
		TerrorNavArea *pFarArea = (TerrorNavArea *)TheNavMesh->GetNearestNavArea( result.endpos, false, flMaxDistance );

		// Target is in darkness, the wall behind him is too, and we are too far away
		if ( pFarArea && pFarArea->GetLightIntensity() < flMinLightIntensity )
			return true;
	}

	return false;
}
#endif


//-----------------------------------------------------------------------------
/**
	Return true if our view direction is pointing at the given target, 
	within the cosine of the angular tolerance. LINE OF SIGHT IS NOT CHECKED.
*/
bool CBaseCombatCharacter::IsLookingTowards( const CBaseEntity *target, float cosTolerance ) const
{
	return IsLookingTowards( target->WorldSpaceCenter(), cosTolerance ) || IsLookingTowards( target->EyePosition(), cosTolerance ) || IsLookingTowards( target->GetAbsOrigin(), cosTolerance );
}


//-----------------------------------------------------------------------------
/**
	Return true if our view direction is pointing at the given target, 
	within the cosine of the angular tolerance. LINE OF SIGHT IS NOT CHECKED.
*/
bool CBaseCombatCharacter::IsLookingTowards( const Vector &target, float cosTolerance ) const
{
	Vector toTarget = target - EyePosition();
	toTarget.NormalizeInPlace();

	Vector forward;
	AngleVectors( EyeAngles(), &forward );

	return ( DotProduct( forward, toTarget ) >= cosTolerance );
}


//-----------------------------------------------------------------------------
/**
	Returns true if we are looking towards something within a tolerence determined 
	by our field of view
*/
bool CBaseCombatCharacter::IsInFieldOfView( CBaseEntity *entity ) const
{
	CBasePlayer *pPlayer = ToBasePlayer( const_cast< CBaseCombatCharacter* >( this ) );
	float flTolerance = pPlayer ? cos( (float)pPlayer->GetFOV() * 0.5f ) : BCC_DEFAULT_LOOK_TOWARDS_TOLERANCE;

	Vector vecForward;
	Vector vecEyePosition = EyePosition();
	AngleVectors( EyeAngles(), &vecForward );

	// FIXME: Use a faster check than this!

	// Check 3 spots, or else when standing right next to someone looking at their eyes, 
	// the angle will be too great to see their center.
	Vector vecToTarget = entity->GetAbsOrigin() - vecEyePosition;
	vecToTarget.NormalizeInPlace();
	if ( DotProduct( vecForward, vecToTarget ) >= flTolerance )
		return true;

	vecToTarget = entity->WorldSpaceCenter() - vecEyePosition;
	vecToTarget.NormalizeInPlace();
	if ( DotProduct( vecForward, vecToTarget ) >= flTolerance )
		return true;

	vecToTarget = entity->EyePosition() - vecEyePosition;
	vecToTarget.NormalizeInPlace();
	return ( DotProduct( vecForward, vecToTarget ) >= flTolerance );
}

//-----------------------------------------------------------------------------
/**
	Returns true if we are looking towards something within a tolerence determined 
	by our field of view
*/
bool CBaseCombatCharacter::IsInFieldOfView( const Vector &pos ) const
{
	CBasePlayer *pPlayer = ToBasePlayer( const_cast< CBaseCombatCharacter* >( this ) );

	if ( pPlayer )
		return IsLookingTowards( pos, cos( (float)pPlayer->GetFOV() * 0.5f ) );

	return IsLookingTowards( pos );
}

//-----------------------------------------------------------------------------
/**
	Strictly checks Line of Sight only.
*/

bool CBaseCombatCharacter::IsLineOfSightClear( CBaseEntity *entity, LineOfSightCheckType checkType ) const
{
#ifdef CLIENT_DLL
	if ( entity->MyCombatCharacterPointer() )
		return IsLineOfSightClear( entity->EyePosition(), checkType, entity );
	return IsLineOfSightClear( entity->WorldSpaceCenter(), checkType, entity );
#else
	// FIXME: Should we do the same check here as the client does?
	return IsLineOfSightClear( entity->WorldSpaceCenter(), checkType, entity ) || IsLineOfSightClear( entity->EyePosition(), checkType, entity ) || IsLineOfSightClear( entity->GetAbsOrigin(), checkType, entity );
#endif
}

//-----------------------------------------------------------------------------
/**
	Strictly checks Line of Sight only.
*/
static bool TraceFilterNoCombatCharacters( IHandleEntity *pServerEntity, int contentsMask )
{
	// Honor BlockLOS also to allow seeing through partially-broken doors
	CBaseEntity *entity = EntityFromEntityHandle( pServerEntity );
	return ( entity->MyCombatCharacterPointer() == NULL && !entity->MyCombatWeaponPointer() && entity->BlocksLOS() );
}

bool CBaseCombatCharacter::IsLineOfSightClear( const Vector &pos, LineOfSightCheckType checkType, CBaseEntity *entityToIgnore ) const
{
#if defined(GAME_DLL) && defined(COUNT_BCC_LOS)
	static int count, frame;
	if ( frame != gpGlobals->framecount )
	{
		Msg( ">> %d\n", count );
		frame = gpGlobals->framecount;
		count = 0;
	}
	count++;
#endif
	if( checkType == IGNORE_ACTORS )
	{

		// use the query cache unless it causes problems
#if defined(GAME_DLL) && defined(TERROR)
		return IsLineOfSightBetweenTwoEntitiesClear( const_cast<CBaseCombatCharacter *>(this), EOFFSET_MODE_EYEPOSITION,
			entityToIgnore, EOFFSET_MODE_WORLDSPACE_CENTER,
			entityToIgnore, COLLISION_GROUP_NONE,
			MASK_L4D_VISION, TraceFilterNoCombatCharacters, 1.0 );
#else
		trace_t trace;
		CTraceFilterNoCombatCharacters traceFilter( entityToIgnore, COLLISION_GROUP_NONE );
		UTIL_TraceLine( EyePosition(), pos, MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE | CONTENTS_MONSTER, &traceFilter, &trace );

		return trace.fraction == 1.0f;
#endif
	}
	else
	{
		trace_t trace;
		CTraceFilterSkipTwoEntities traceFilter( this, entityToIgnore, COLLISION_GROUP_NONE );
		UTIL_TraceLine( EyePosition(), pos, MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE, &traceFilter, &trace );

		return trace.fraction == 1.0f;
	}
}


/*
//---------------------------------------------------------------------------------------------------------------------------
surfacedata_t * CBaseCombatCharacter::GetGroundSurface( void ) const
{
	Vector start( vec3_origin );
	Vector end( 0, 0, -64 );

	Vector vecMins, vecMaxs;
	CollisionProp()->WorldSpaceAABB( &vecMins, &vecMaxs );

	Ray_t ray;
	ray.Init( start, end, vecMins, vecMaxs );

	trace_t	trace;
	UTIL_TraceRay( ray, MASK_SOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );

	if ( trace.fraction == 1.0f )
		return NULL;	// no ground

	return physprops->GetSurfaceData( trace.surface.surfaceProps );
}
*/
