//========= Copyright (C) 2018, CSProMod Team, All rights reserved. =========//
//
// Purpose: provide world light related functions to the client
//
// As the engine provides no access to brush/model data (brushdata_t, model_t),
// we hence have no access to dworldlight_t. Therefore, we manually extract the
// world light data from the BSP itself, before entities are initialised on map
// load.
//
// To find the brightest light at a point, all world lights are iterated.
// Lights whose radii do not encompass our sample point are quickly rejected,
// as are lights which are not in our PVS, or visible from the sample point.
// If the sky light is visible from the sample point, then it shall supersede
// all other world lights.
//
// Written: November 2011
// Author: Saul Rennison
//
//===========================================================================//

#include "cbase.h"
#include "worldlight.h"
#include "bspfile.h"
#include "filesystem.h"
#include "client_factorylist.h" // FactoryList_Retrieve
#include "eiface.h" // IVEngineServer

static IVEngineServer *g_pEngineServer = NULL;

#ifdef MAPBASE
ConVar cl_worldlight_use_new_method("cl_worldlight_use_new_method", "1", FCVAR_NONE, "Uses the new world light iteration method which splits lights into multiple lists for each cluster.");
#endif

//-----------------------------------------------------------------------------
// Singleton exposure
//-----------------------------------------------------------------------------
static CWorldLights s_WorldLights;
CWorldLights *g_pWorldLights = &s_WorldLights;

//-----------------------------------------------------------------------------
// Purpose: calculate intensity ratio for a worldlight by distance
// Author: Valve Software
//-----------------------------------------------------------------------------
static float Engine_WorldLightDistanceFalloff( const dworldlight_t *wl, const Vector& delta )
{
	float falloff;

	switch (wl->type)
	{
	case emit_surface:
		// Cull out stuff that's too far
		if(wl->radius != 0)
		{
			if(DotProduct( delta, delta ) > (wl->radius * wl->radius))
				return 0.0f;
		}

		return InvRSquared(delta);
		break;

	case emit_skylight:
		return 1.f;
		break;

	case emit_quakelight:
		// X - r;
		falloff = wl->linear_attn - FastSqrt( DotProduct( delta, delta ) );
		if(falloff < 0)
			return 0.f;

		return falloff;
		break;

	case emit_skyambient:
		return 1.f;
		break;

	case emit_point:
	case emit_spotlight:	// directional & positional
		{
			float dist2, dist;

			dist2 = DotProduct(delta, delta);
			dist = FastSqrt(dist2);

			// Cull out stuff that's too far
			if(wl->radius != 0 && dist > wl->radius)
				return 0.f;

			return 1.f / (wl->constant_attn + wl->linear_attn * dist + wl->quadratic_attn * dist2);
		}

		break;
	}

	return 1.f;
}

//-----------------------------------------------------------------------------
// Purpose: initialise game system and members
//-----------------------------------------------------------------------------
CWorldLights::CWorldLights() : CAutoGameSystem("World lights")
{
	m_nWorldLights = 0;
	m_pWorldLights = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: clear worldlights, free memory
//-----------------------------------------------------------------------------
void CWorldLights::Clear()
{
	m_nWorldLights = 0;

	if(m_pWorldLights)
	{
		delete [] m_pWorldLights;
		m_pWorldLights = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: get the IVEngineServer, we need this for the PVS functions
//-----------------------------------------------------------------------------
bool CWorldLights::Init()
{
#ifdef MAPBASE
	// Moved to its own clientside interface after I found out it was possible
	// (still have no idea how or why this works)
	if ((g_pEngineServer = serverengine) == NULL)
		return false;

	return true;
#else
	factorylist_t factories;
	FactoryList_Retrieve(factories);

	if((g_pEngineServer = (IVEngineServer*)factories.appSystemFactory(INTERFACEVERSION_VENGINESERVER, NULL)) == NULL)
		return false;

	return true;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: get all world lights from the BSP
//-----------------------------------------------------------------------------
void CWorldLights::LevelInitPreEntity()
{
	// Get the map path
	const char *pszMapName = modelinfo->GetModelName(modelinfo->GetModel(1));

	// Open map
	FileHandle_t hFile = g_pFullFileSystem->Open(pszMapName, "rb");
	if(!hFile)
	{
		Warning("CWorldLights: unable to open map\n");
		return;
	}

	// Read the BSP header. We don't need to do any version checks, etc. as we
	// can safely assume that the engine did this for us
	dheader_t hdr;
	g_pFullFileSystem->Read(&hdr, sizeof(hdr), hFile);

	// Grab the light lump and seek to it
	lump_t &lightLump = hdr.lumps[LUMP_WORLDLIGHTS];

	// INSOLENCE: If the worldlights lump is empty, that means theres no normal, LDR lights to extract
	//			  This can happen when, for example, the map is compiled in HDR mode only
	//			  So move on to the HDR worldlights lump
	if (lightLump.filelen == 0)
	{
		lightLump = hdr.lumps[LUMP_WORLDLIGHTS_HDR];
	}

	// If we can't divide the lump data into a whole number of worldlights,
	// then the BSP format changed and we're unaware
	if(lightLump.filelen % sizeof(dworldlight_t))
	{
		Warning("CWorldLights: unknown world light lump\n");

		// Close file
		g_pFullFileSystem->Close(hFile);
		return;
	}

	g_pFullFileSystem->Seek(hFile, lightLump.fileofs, FILESYSTEM_SEEK_HEAD);

	// Allocate memory for the worldlights
	m_nWorldLights = lightLump.filelen / sizeof(dworldlight_t);
	m_pWorldLights = new dworldlight_t[m_nWorldLights];

	// Read worldlights then close
	g_pFullFileSystem->Read(m_pWorldLights, lightLump.filelen, hFile);
	g_pFullFileSystem->Close(hFile);

	DevMsg("CWorldLights: load successful (%d lights at 0x%p)\n", m_nWorldLights, m_pWorldLights);

#ifdef MAPBASE
	// Now that the lights have been gathered, begin separating them into lists for each PVS cluster.
	// This code is adapted from the soundscape cluster list code (see soundscape_system.cpp) and is intended to
	// reduce frame drops in large maps which use dynamic RTT shadow angles.
	CUtlVector<bbox_t> clusterbounds;
	int clusterCount = g_pEngineServer->GetClusterCount();
	clusterbounds.SetCount( clusterCount );
	g_pEngineServer->GetAllClusterBounds( clusterbounds.Base(), clusterCount );
	m_WorldLightsInCluster.SetCount(clusterCount);
	for ( int i = 0; i < clusterCount; i++ )
	{
		m_WorldLightsInCluster[i].lightCount = 0;
		m_WorldLightsInCluster[i].firstLight = 0;
	}
	unsigned char myPVS[16 * 1024];
	CUtlVector<short> clusterIndexList;
	CUtlVector<short> lightIndexList;

	// Find the clusters visible from each light, then add it to those clusters' light lists
	// (Also try to clip for radius if possible)
	for (int i = 0; i < m_nWorldLights; ++i)
	{
		dworldlight_t *light = &m_pWorldLights[i];

		// Assign the sun to its own pointer
		if (light->type == emit_skylight)
		{
			m_iSunIndex = i;
			continue;
		}

		float radiusSq = light->radius * light->radius;
		if (radiusSq == 0.0f)
		{
			// TODO: Use intensity instead?
			radiusSq = FLT_MAX;
		}

		g_pEngineServer->GetPVSForCluster( light->cluster, sizeof( myPVS ), myPVS );
		for ( int j = 0; j < clusterCount; j++ )
		{
			if ( myPVS[ j >> 3 ] & (1<<(j&7)) )
			{
				float distSq = CalcSqrDistanceToAABB( clusterbounds[j].mins, clusterbounds[j].maxs, light->origin );
				if ( distSq < radiusSq )
				{
					m_WorldLightsInCluster[j].lightCount++;
					clusterIndexList.AddToTail(j);
					lightIndexList.AddToTail(i);
				}
			}
		}
	}

	m_WorldLightsIndexList.SetCount(lightIndexList.Count());

	// Compute the starting index of each cluster
	int firstLight = 0;
	for ( int i = 0; i < clusterCount; i++ )
	{
		m_WorldLightsInCluster[i].firstLight = firstLight;
		firstLight += m_WorldLightsInCluster[i].lightCount;
		m_WorldLightsInCluster[i].lightCount = 0;
	}

	// Now add each light index to the appropriate cluster's list
	for ( int i = 0; i < lightIndexList.Count(); i++ )
	{
		int cluster = clusterIndexList[i];
		int outIndex = m_WorldLightsInCluster[cluster].lightCount + m_WorldLightsInCluster[cluster].firstLight;
		m_WorldLightsInCluster[cluster].lightCount++;
		m_WorldLightsIndexList[outIndex] = lightIndexList[i];
	}

	//DevMsg( "CWorldLights: Light clusters list has %i elements; Light index list has %i\n", m_WorldLightsInCluster.Count(), m_WorldLightsIndexList.Count() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: find the brightest light source at a point
//-----------------------------------------------------------------------------
bool CWorldLights::GetBrightestLightSource(const Vector &vecPosition, Vector &vecLightPos, Vector &vecLightBrightness)
{
	if(!m_nWorldLights || !m_pWorldLights)
		return false;

	// Default light position and brightness to zero
	vecLightBrightness.Init();
	vecLightPos.Init();

	// Find the size of the PVS for our current position
	int nCluster = g_pEngineServer->GetClusterForOrigin(vecPosition);

#ifdef MAPBASE
	if (cl_worldlight_use_new_method.GetBool())
	{
		FindBrightestLightSourceNew( vecPosition, vecLightPos, vecLightBrightness, nCluster );
	}
	else
#endif
	{
		FindBrightestLightSourceOld( vecPosition, vecLightPos, vecLightBrightness, nCluster );
	}

	//engine->Con_NPrintf(m_nWorldLights, "result: %d", !vecLightBrightness.IsZero());
	return !vecLightBrightness.IsZero();
}

void CWorldLights::FindBrightestLightSourceOld( const Vector &vecPosition, Vector &vecLightPos, Vector &vecLightBrightness, int nCluster )
{
	// Find the size of the PVS for our current position
	int nPVSSize = g_pEngineServer->GetPVSForCluster(nCluster, 0, NULL);

	// Get the PVS at our position
	byte *pvs = new byte[nPVSSize];
	g_pEngineServer->GetPVSForCluster(nCluster, nPVSSize, pvs);

	// Iterate through all the worldlights
	for(int i = 0; i < m_nWorldLights; ++i)
	{
		dworldlight_t *light = &m_pWorldLights[i];

		// Skip skyambient
		if(light->type == emit_skyambient)
		{
			//engine->Con_NPrintf(i, "%d: skyambient", i);
			continue;
		}

		// Handle sun
		if(light->type == emit_skylight)
		{
			// Calculate sun position
			Vector vecAbsStart = vecPosition + Vector(0,0,30);
			Vector vecAbsEnd = vecAbsStart - (light->normal * MAX_TRACE_LENGTH);

			trace_t tr;
			UTIL_TraceLine(vecPosition, vecAbsEnd, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr);

			// If we didn't hit anything then we have a problem
			if(!tr.DidHit())
			{
				//engine->Con_NPrintf(i, "%d: skylight: couldn't touch sky", i);
				continue;
			}

			// If we did hit something, and it wasn't the skybox, then skip
			// this worldlight
			if(!(tr.surface.flags & SURF_SKY) && !(tr.surface.flags & SURF_SKY2D))
			{
				//engine->Con_NPrintf(i, "%d: skylight: no sight to sun", i);
				continue;
			}

			// Act like we didn't find any valid worldlights, so the shadow
			// manager uses the default shadow direction instead (should be the
			// sun direction)

			delete[] pvs;

			return;
		}

		// Calculate square distance to this worldlight
		Vector vecDelta = light->origin - vecPosition;
		float flDistSqr = vecDelta.LengthSqr();
		float flRadiusSqr = light->radius * light->radius;

		// Skip lights that are out of our radius
		if(flRadiusSqr > 0 && flDistSqr >= flRadiusSqr)
		{
			//engine->Con_NPrintf(i, "%d: out-of-radius (dist: %d, radius: %d)", i, sqrt(flDistSqr), light->radius);
			continue;
		}

		// Is it out of our PVS?
		if(!g_pEngineServer->CheckOriginInPVS(light->origin, pvs, nPVSSize))
		{
			//engine->Con_NPrintf(i, "%d: out of PVS", i);
			continue;
		}

		// Calculate intensity at our position
		float flRatio = Engine_WorldLightDistanceFalloff(light, vecDelta);
		Vector vecIntensity = light->intensity * flRatio;

		// Is this light more intense than the one we already found?
		if(vecIntensity.LengthSqr() <= vecLightBrightness.LengthSqr())
		{
			//engine->Con_NPrintf(i, "%d: too dim", i);
			continue;
		}

		// Can we see the light?
		trace_t tr;
		Vector vecAbsStart = vecPosition + Vector(0,0,30);
		UTIL_TraceLine(vecAbsStart, light->origin, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr);

		if(tr.DidHit())
		{
			//engine->Con_NPrintf(i, "%d: trace failed", i);
			continue;
		}

		vecLightPos = light->origin;
		vecLightBrightness = vecIntensity;

		//engine->Con_NPrintf(i, "%d: set (%.2f)", i, vecIntensity.Length());
	}

	delete[] pvs;
}

#ifdef MAPBASE
void CWorldLights::FindBrightestLightSourceNew( const Vector &vecPosition, Vector &vecLightPos, Vector &vecLightBrightness, int nCluster )
{
	// Handle sun
	if (m_iSunIndex != -1)
	{
		dworldlight_t *light = &m_pWorldLights[m_iSunIndex];

		// Calculate sun position
		Vector vecAbsStart = vecPosition + Vector(0,0,30);
		Vector vecAbsEnd = vecAbsStart - (light->normal * MAX_TRACE_LENGTH);

		trace_t tr;
		UTIL_TraceLine(vecPosition, vecAbsEnd, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr);

		// If we didn't hit anything then we have a problem
		if(tr.DidHit())
		{
			// If we did hit something, and it wasn't the skybox, then skip
			// this worldlight
			if((tr.surface.flags & SURF_SKY) && (tr.surface.flags & SURF_SKY2D))
			{
				// Act like we didn't find any valid worldlights, so the shadow
				// manager uses the default shadow direction instead (should be the
				// sun direction)

				return;
			}
		}
	}

	// Iterate through all the worldlights
	if ( nCluster >= 0 && nCluster < m_WorldLightsInCluster.Count() )
	{
		// find all soundscapes that could possibly attach to this player and update them
		for ( int j = 0; j < m_WorldLightsInCluster[nCluster].lightCount; j++ )
		{
			int ssIndex = m_WorldLightsIndexList[m_WorldLightsInCluster[nCluster].firstLight + j];
			dworldlight_t *light = &m_pWorldLights[ssIndex];

			// Calculate square distance to this worldlight
			Vector vecDelta = light->origin - vecPosition;
			float flDistSqr = vecDelta.LengthSqr();
			float flRadiusSqr = light->radius * light->radius;

			// Skip lights that are out of our radius
			if(flRadiusSqr > 0 && flDistSqr >= flRadiusSqr)
			{
				//engine->Con_NPrintf(i, "%d: out-of-radius (dist: %d, radius: %d)", i, sqrt(flDistSqr), light->radius);
				continue;
			}

			// Calculate intensity at our position
			float flRatio = Engine_WorldLightDistanceFalloff(light, vecDelta);
			Vector vecIntensity = light->intensity * flRatio;

			// Is this light more intense than the one we already found?
			if(vecIntensity.LengthSqr() <= vecLightBrightness.LengthSqr())
			{
				//engine->Con_NPrintf(i, "%d: too dim", i);
				continue;
			}

			// Can we see the light?
			trace_t tr;
			Vector vecAbsStart = vecPosition + Vector(0,0,30);
			UTIL_TraceLine(vecAbsStart, light->origin, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr);

			if(tr.DidHit())
			{
				//engine->Con_NPrintf(i, "%d: trace failed", i);
				continue;
			}

			vecLightPos = light->origin;
			vecLightBrightness = vecIntensity;

			//engine->Con_NPrintf(i, "%d: set (%.2f)", i, vecIntensity.Length());
		}
	}
}
#endif
