//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "info_darknessmode_lightsource.h"
#include "ai_debug_shared.h"

void CV_Debug_Darkness( IConVar *var, const char *pOldString, float flOldValue );
ConVar g_debug_darkness( "g_debug_darkness", "0", FCVAR_NONE, "Show darkness mode lightsources.", CV_Debug_Darkness );
ConVar darkness_ignore_LOS_to_sources( "darkness_ignore_LOS_to_sources", "1", FCVAR_NONE );

class CInfoDarknessLightSource;

//-----------------------------------------------------------------------------
// Purpose: Manages entities that provide light while in darkness mode
//-----------------------------------------------------------------------------
class CDarknessLightSourcesSystem : public CAutoGameSystem
{
public:
	CDarknessLightSourcesSystem() : CAutoGameSystem( "CDarknessLightSourcesSystem" )
	{
	}

	void LevelInitPreEntity();

	void AddLightSource( CInfoDarknessLightSource *pEntity, float flRadius );
	void RemoveLightSource( CInfoDarknessLightSource *pEntity );
	bool IsEntityVisibleToTarget( CBaseEntity *pLooker, CBaseEntity *pTarget );
	bool AreThereLightSourcesWithinRadius( CBaseEntity *pLooker, float flRadius );
	void SetDebug( bool bDebug );

private:
	struct lightsource_t
	{
		float	flLightRadiusSqr;
		CHandle<CInfoDarknessLightSource>	hEntity;
	};

	CUtlVector<lightsource_t> m_LightSources;
};

CDarknessLightSourcesSystem *DarknessLightSourcesSystem();

//-----------------------------------------------------------------------------
// Darkness mode light source entity
//-----------------------------------------------------------------------------
class CInfoDarknessLightSource : public CBaseEntity
{
	DECLARE_CLASS( CInfoDarknessLightSource, CBaseEntity );
public:
	DECLARE_DATADESC();

	virtual void Activate()
	{
		if ( m_bDisabled == false )
		{
			DarknessLightSourcesSystem()->AddLightSource( this, m_flLightRadius );

			if ( g_debug_darkness.GetBool() )
			{
				SetThink( &CInfoDarknessLightSource::DebugThink );
				SetNextThink( gpGlobals->curtime );
			}
		}

		BaseClass::Activate();
	}
	virtual void UpdateOnRemove()
	{
		DarknessLightSourcesSystem()->RemoveLightSource( this );
		BaseClass::UpdateOnRemove();
	}
	void SetLightRadius( float flRadius )
	{
		m_flLightRadius = flRadius;
	}

	void InputEnable( inputdata_t &inputdata )
	{
		DarknessLightSourcesSystem()->AddLightSource( this, m_flLightRadius );
		m_bDisabled = false;
	}

	void InputDisable( inputdata_t &inputdata )
	{
		DarknessLightSourcesSystem()->RemoveLightSource( this );
		m_bDisabled = true;
	}

	void DebugThink( void )
	{
		Vector vecRadius( m_flLightRadius, m_flLightRadius, m_flLightRadius );
		NDebugOverlay::Box( GetAbsOrigin(), -vecRadius, vecRadius, 255,255,255, 8, 0.1 );
		NDebugOverlay::Box( GetAbsOrigin(), -Vector(5,5,5), Vector(5,5,5), 255,0,0, 8, 0.1 );
		SetNextThink( gpGlobals->curtime + 0.1 );

		int textoffset = 0;
		EntityText( textoffset, UTIL_VarArgs("Org: %.2f %.2f %.2f", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z ), 0.1 );
		textoffset++;
		EntityText( textoffset, UTIL_VarArgs("Radius %.2f", m_flLightRadius), 0.1 );
		textoffset++;
		if ( m_bIgnoreLOS )
		{
			EntityText( textoffset, "Ignoring LOS", 0.1 );
			textoffset++;
		}
		if ( m_bDisabled )
		{
			EntityText( textoffset, "DISABLED", 0.1 );
			textoffset++;
		}
	}

	void IgnoreLOS( void )
	{
		m_bIgnoreLOS = true;
	}

	bool ShouldIgnoreLOS( void )
	{
		return m_bIgnoreLOS;
	}

private:
	float	m_flLightRadius;
	bool	m_bDisabled;
	bool	m_bIgnoreLOS;
};

LINK_ENTITY_TO_CLASS( info_darknessmode_lightsource, CInfoDarknessLightSource );

BEGIN_DATADESC( CInfoDarknessLightSource )
	DEFINE_KEYFIELD( m_flLightRadius, FIELD_FLOAT, "LightRadius" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_KEYFIELD( m_bDisabled,	FIELD_BOOLEAN,	"StartDisabled" ),
	DEFINE_FIELD( m_bIgnoreLOS, FIELD_BOOLEAN ),

	DEFINE_THINKFUNC( DebugThink ),
END_DATADESC()

CDarknessLightSourcesSystem g_DarknessLightSourcesSystem;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDarknessLightSourcesSystem *DarknessLightSourcesSystem()
{
	return &g_DarknessLightSourcesSystem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDarknessLightSourcesSystem::LevelInitPreEntity()
{
	m_LightSources.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDarknessLightSourcesSystem::AddLightSource( CInfoDarknessLightSource *pEntity, float flRadius )
{
	lightsource_t sNewSource;
	sNewSource.hEntity = pEntity;
	sNewSource.flLightRadiusSqr = flRadius * flRadius;
	m_LightSources.AddToTail( sNewSource );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDarknessLightSourcesSystem::RemoveLightSource( CInfoDarknessLightSource *pEntity )
{
	for ( int i = m_LightSources.Count() - 1; i >= 0; i-- )
	{
		if ( m_LightSources[i].hEntity == pEntity )
		{
			m_LightSources.Remove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDarknessLightSourcesSystem::IsEntityVisibleToTarget( CBaseEntity *pLooker, CBaseEntity *pTarget )
{
	if ( pTarget->IsEffectActive( EF_BRIGHTLIGHT ) || pTarget->IsEffectActive( EF_DIMLIGHT ) )
		return true;

	bool bDebug = g_debug_darkness.GetBool();
	if ( bDebug && pLooker )
	{
		bDebug = (pLooker->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) != 0;
	}

	trace_t tr;

	// Loop through all the light sources. Do it backwards, so we can remove dead ones.
	for ( int i = m_LightSources.Count() - 1; i >= 0; i-- )
	{
		// Removed?
		if ( m_LightSources[i].hEntity == NULL || m_LightSources[i].hEntity->IsMarkedForDeletion() )
		{
			m_LightSources.FastRemove( i );
			continue;
		}

		CInfoDarknessLightSource *pLightSource = m_LightSources[i].hEntity;

		// Close enough to a light source?
		float flDistanceSqr = (pTarget->WorldSpaceCenter() - pLightSource->GetAbsOrigin()).LengthSqr();
		if ( flDistanceSqr < m_LightSources[i].flLightRadiusSqr )
		{
			if ( pLightSource->ShouldIgnoreLOS() )
			{
				if ( bDebug )
				{
					NDebugOverlay::Line( pTarget->WorldSpaceCenter(), pLightSource->GetAbsOrigin(), 0,255,0,true, 0.1);
				}
				return true;
			}

			// Check LOS from the light to the target
			CTraceFilterSkipTwoEntities filter( pTarget, pLooker, COLLISION_GROUP_NONE );
			AI_TraceLine( pTarget->WorldSpaceCenter(), pLightSource->GetAbsOrigin(), MASK_BLOCKLOS, &filter, &tr );
			if ( tr.fraction == 1.0 )
			{
				if ( bDebug )
				{
					NDebugOverlay::Line( tr.startpos, tr.endpos, 0,255,0,true, 0.1);
				}
				return true;
			}

			if ( bDebug )
			{
				NDebugOverlay::Line( tr.startpos, tr.endpos, 255,0,0,true, 0.1);
				NDebugOverlay::Line( tr.endpos, pLightSource->GetAbsOrigin(), 128,0,0,true, 0.1);
			}

			// If the target is within the radius of the light, don't do sillhouette checks
			continue;
		}

 		if ( !pLooker )
			continue;

		// Between a light source and the looker?
		Vector vecLookerToLight = (pLightSource->GetAbsOrigin() - pLooker->WorldSpaceCenter());
		Vector vecLookerToTarget = (pTarget->WorldSpaceCenter() - pLooker->WorldSpaceCenter());
		float flDistToSource = VectorNormalize( vecLookerToLight );
		float flDistToTarget = VectorNormalize( vecLookerToTarget );
 		float flDot = DotProduct( vecLookerToLight, vecLookerToTarget );
		if ( flDot > 0 )
		{
			// Make sure the target is in front of the lightsource
			if ( flDistToTarget < flDistToSource )
			{
				if ( bDebug )
				{
					NDebugOverlay::Line( pLooker->WorldSpaceCenter(), pLooker->WorldSpaceCenter() + (vecLookerToLight * 128), 255,255,255,true, 0.1);
					NDebugOverlay::Line( pLooker->WorldSpaceCenter(), pLooker->WorldSpaceCenter() + (vecLookerToTarget * 128), 255,0,0,true, 0.1);
				}

				// Now, we need to find out if the light source is obscured by anything. 
				// To do this, we want to calculate the point of intersection between the light source 
				// sphere and the line from the looker through the target. 
 				float flASqr = (flDistToSource * flDistToSource);
				float flB = -2 * flDistToSource * flDot;
				float flCSqr = m_LightSources[i].flLightRadiusSqr;
				float flDesc = (flB * flB) - (4 * (flASqr - flCSqr));
				if ( flDesc >= 0 )
				{
 					float flLength = (-flB - sqrt(flDesc)) / 2;
  					Vector vecSpherePoint = pLooker->WorldSpaceCenter() + (vecLookerToTarget * flLength);

					// We've got the point of intersection. See if we can see it.
					CTraceFilterSkipTwoEntities filter( pTarget, pLooker, COLLISION_GROUP_NONE );
					AI_TraceLine( pLooker->EyePosition(), vecSpherePoint, MASK_SOLID_BRUSHONLY, &filter, &tr );

					if ( bDebug )
					{
						if (tr.fraction != 1.0)
						{
							NDebugOverlay::Line( pLooker->WorldSpaceCenter(), vecSpherePoint, 255,0,0,true, 0.1);
						}
						else
						{
							NDebugOverlay::Line( pLooker->WorldSpaceCenter(), vecSpherePoint, 0,255,0,true, 0.1);
							NDebugOverlay::Line( pLightSource->GetAbsOrigin(), vecSpherePoint, 255,0,0,true, 0.1);
						}
					}

					if ( tr.fraction == 1.0 )
						return true;
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDarknessLightSourcesSystem::AreThereLightSourcesWithinRadius( CBaseEntity *pLooker, float flRadius )
{
	float flRadiusSqr = (flRadius * flRadius);
	for ( int i = m_LightSources.Count() - 1; i >= 0; i-- )
	{
		// Removed?
		if ( m_LightSources[i].hEntity == NULL || m_LightSources[i].hEntity->IsMarkedForDeletion() )
		{
			m_LightSources.FastRemove( i );
			continue;
		}

		CBaseEntity *pLightSource = m_LightSources[i].hEntity;

		// Close enough to a light source?
		float flDistanceSqr = (pLooker->WorldSpaceCenter() - pLightSource->GetAbsOrigin()).LengthSqr();
		if ( flDistanceSqr < flRadiusSqr )
		{
			trace_t tr;
			AI_TraceLine( pLooker->EyePosition(), pLightSource->GetAbsOrigin(), MASK_SOLID_BRUSHONLY, pLooker, COLLISION_GROUP_NONE, &tr );

			if ( g_debug_darkness.GetBool() )
			{
				if (tr.fraction != 1.0)
				{
					NDebugOverlay::Line( pLooker->WorldSpaceCenter(), tr.endpos, 255,0,0,true, 0.1);
				}
				else
				{
					NDebugOverlay::Line( pLooker->WorldSpaceCenter(), tr.endpos, 0,255,0,true, 0.1);
					NDebugOverlay::Line( pLightSource->GetAbsOrigin(), tr.endpos, 255,0,0,true, 0.1);
				}
			}

			if ( tr.fraction == 1.0 )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDarknessLightSourcesSystem::SetDebug( bool bDebug )
{
	for ( int i = m_LightSources.Count() - 1; i >= 0; i-- )
	{
		CInfoDarknessLightSource *pLightSource = dynamic_cast<CInfoDarknessLightSource*>(m_LightSources[i].hEntity.Get());
		if ( pLightSource )
		{
			if ( bDebug )
			{
				pLightSource->SetThink( &CInfoDarknessLightSource::DebugThink );
				pLightSource->SetNextThink( gpGlobals->curtime );
			}
			else
			{
				pLightSource->SetThink( NULL );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CV_Debug_Darkness( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	ConVarRef var( pConVar );
	DarknessLightSourcesSystem()->SetDebug( var.GetBool() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void AddEntityToDarknessCheck( CBaseEntity *pEntity, float flLightRadius /*=DARKNESS_LIGHTSOURCE_SIZE*/ )
{
	// Create a light source, and attach it to the entity
	CInfoDarknessLightSource *pLightSource = (CInfoDarknessLightSource *) CreateEntityByName( "info_darknessmode_lightsource" );
	if ( pLightSource )	
	{
		pLightSource->SetLightRadius( flLightRadius );
		DispatchSpawn( pLightSource );
  		pLightSource->SetAbsOrigin( pEntity->WorldSpaceCenter() );
		pLightSource->SetParent( pEntity );
		pLightSource->Activate();

		// Dynamically created darkness sources can ignore LOS
		// to match the (broken) visual representation of our dynamic lights.
		if ( darkness_ignore_LOS_to_sources.GetBool() )
		{
			pLightSource->IgnoreLOS();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void RemoveEntityFromDarknessCheck( CBaseEntity *pEntity )
{
	// Find any light sources parented to this entity, and remove them
	CBaseEntity *pChild = pEntity->FirstMoveChild();
	while ( pChild )
	{
		CBaseEntity *pPrevChild = pChild;
		pChild = pChild->NextMovePeer();

		if ( dynamic_cast<CInfoDarknessLightSource*>(pPrevChild) )
		{
			UTIL_Remove( pPrevChild );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
bool LookerCouldSeeTargetInDarkness( CBaseEntity *pLooker, CBaseEntity *pTarget )
{
	if ( DarknessLightSourcesSystem()->IsEntityVisibleToTarget( pLooker, pTarget ) )
	{
		//NDebugOverlay::Line( pTarget->WorldSpaceCenter(), pLooker->WorldSpaceCenter(), 0,255,0,true, 0.1);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if there is at least 1 darkness light source within
//			the specified radius of the looker.
//-----------------------------------------------------------------------------
bool DarknessLightSourceWithinRadius( CBaseEntity *pLooker, float flRadius )
{
	return DarknessLightSourcesSystem()->AreThereLightSourcesWithinRadius( pLooker, flRadius );
}
