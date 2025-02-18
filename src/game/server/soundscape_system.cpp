//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//


#include "cbase.h"
#include "soundscape_system.h"
#include "soundscape.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "game.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SOUNDSCAPE_MANIFEST_FILE				"scripts/soundscapes_manifest.txt"

CON_COMMAND(soundscape_flush, "Flushes the server & client side soundscapes")
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if ( engine->IsDedicatedServer() )
	{
		// If it's a dedicated server, only the server console can run this.
		if ( pPlayer )
			return;
	}
	else
	{
		// If it's a listen server, only the listen server host can run this.
		if ( !pPlayer || pPlayer != UTIL_GetListenServerHost() )
			return;
	}

	g_SoundscapeSystem.FlushSoundscapes();	// don't bother forgetting about the entities
	g_SoundscapeSystem.Init();

	
	if ( engine->IsDedicatedServer() )
	{
		// If the ds console typed it, send it to everyone.
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer	*pSendToPlayer = UTIL_PlayerByIndex( i );
			if ( pSendToPlayer )
				engine->ClientCommand( pSendToPlayer->edict(), "cl_soundscape_flush\n" );
		}
	}
	else
	{
		engine->ClientCommand( pPlayer->edict(), "cl_soundscape_flush\n" );
	}
}

CSoundscapeSystem g_SoundscapeSystem( "CSoundscapeSystem" );

extern ConVar soundscape_debug;

void CSoundscapeSystem::AddSoundscapeFile( const char *filename )
{
	MEM_ALLOC_CREDIT();
	// Open the soundscape data file, and abort if we can't
	KeyValues *pKeyValuesData = new KeyValues( filename );
	if ( filesystem->LoadKeyValues( *pKeyValuesData, IFileSystem::TYPE_SOUNDSCAPE, filename, "GAME" ) )
	{
		// parse out all of the top level sections and save their names
		KeyValues *pKeys = pKeyValuesData;
		while ( pKeys )
		{
			if ( pKeys->GetFirstSubKey() )
			{
				if ( g_pDeveloper->GetBool() )
				{
					if ( strstr( pKeys->GetName(), "{" ) )
					{
						Msg("Error parsing soundscape file %s after %s\n", filename, m_soundscapeCount>0 ?m_soundscapes.GetStringText( m_soundscapeCount-1 ) : "FIRST" );
					}
				}
				m_soundscapes.AddString( pKeys->GetName(), m_soundscapeCount );

				if ( IsX360() )
				{
					AddSoundscapeSounds( pKeys, m_soundscapeCount );
				}
				m_soundscapeCount++;
			}
			pKeys = pKeys->GetNextKey();
		}
	}
	pKeyValuesData->deleteThis();
}

CON_COMMAND_F( sv_soundscape_printdebuginfo, "print soundscapes", FCVAR_DEVELOPMENTONLY )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	g_SoundscapeSystem.PrintDebugInfo();
}


void CSoundscapeSystem::PrintDebugInfo()
{
	Msg( "\n------- SERVER SOUNDSCAPES -------\n" );
	for ( int key=m_soundscapes.First(); key != m_soundscapes.InvalidIndex(); key = m_soundscapes.Next( key ) )
	{
		int id = m_soundscapes.GetIDForKey( key );
		const char *pName = m_soundscapes.GetStringForKey( key );

		Msg( "- %d: %s\n", id, pName );
	}
	Msg( "-------- SOUNDSCAPE ENTITIES -----\n" );
	for( int entityIndex = 0; entityIndex < m_soundscapeEntities.Size(); ++entityIndex )
	{
		CEnvSoundscape *currentSoundscape = m_soundscapeEntities[entityIndex];
		Msg("- %d: %s x:%.4f y:%.4f z:%.4f\n", 
			entityIndex, 
			STRING(currentSoundscape->GetSoundscapeName()), 
			currentSoundscape->GetAbsOrigin().x,
			currentSoundscape->GetAbsOrigin().y,
			currentSoundscape->GetAbsOrigin().z
			);
	}
	Msg( "----------------------------------\n\n" );
}

bool CSoundscapeSystem::Init()
{
	m_soundscapeCount = 0;

	const char *mapname = STRING( gpGlobals->mapname );
	const char *mapSoundscapeFilename = NULL;
	if ( mapname && *mapname )
	{
		mapSoundscapeFilename = UTIL_VarArgs( "scripts/soundscapes_%s.txt", mapname );
	}

	KeyValues *manifest = new KeyValues( SOUNDSCAPE_MANIFEST_FILE );
	if ( filesystem->LoadKeyValues( *manifest, IFileSystem::TYPE_SOUNDSCAPE, SOUNDSCAPE_MANIFEST_FILE, "GAME" ) )
	{
		for ( KeyValues *sub = manifest->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey() )
		{
			if ( !Q_stricmp( sub->GetName(), "file" ) )
			{
				// Add
				AddSoundscapeFile( sub->GetString() );
				if ( mapSoundscapeFilename && FStrEq( sub->GetString(), mapSoundscapeFilename ) )
				{
					mapSoundscapeFilename = NULL; // we've already loaded the map's soundscape
				}
				continue;
			}

			Warning( "CSoundscapeSystem::Init:  Manifest '%s' with bogus file type '%s', expecting 'file'\n", 
				SOUNDSCAPE_MANIFEST_FILE, sub->GetName() );
		}

		if ( mapSoundscapeFilename && filesystem->FileExists( mapSoundscapeFilename ) )
		{
			AddSoundscapeFile( mapSoundscapeFilename );
		}
	}
	else
	{
		Error( "Unable to load manifest file '%s'\n", SOUNDSCAPE_MANIFEST_FILE );
	}
	manifest->deleteThis();
	m_activeIndex = 0;

	return true;
}

void CSoundscapeSystem::FlushSoundscapes( void )
{
	m_soundscapeCount = 0;
	m_soundscapes.ClearStrings();
}

void CSoundscapeSystem::Shutdown()
{
	FlushSoundscapes();
	m_soundscapeEntities.RemoveAll();
	m_activeIndex = 0;

	if ( IsX360() )
	{
		m_soundscapeSounds.Purge();
	}
}

void CSoundscapeSystem::LevelInitPreEntity()
{
	g_SoundscapeSystem.Shutdown();
	g_SoundscapeSystem.Init();
}

void CSoundscapeSystem::LevelInitPostEntity()
{
	if ( IsX360() )
	{
		m_soundscapeSounds.Purge();
	}
	CUtlVector<bbox_t> clusterbounds;
	int clusterCount = engine->GetClusterCount();
	clusterbounds.SetCount( clusterCount );
	engine->GetAllClusterBounds( clusterbounds.Base(), clusterCount );
	m_soundscapesInCluster.SetCount(clusterCount);
	for ( int i = 0; i < clusterCount; i++ )
	{
		m_soundscapesInCluster[i].soundscapeCount = 0;
		m_soundscapesInCluster[i].firstSoundscape = 0;
	}
	unsigned char myPVS[16 * 1024];
	CUtlVector<short> clusterIndexList;
	CUtlVector<short> soundscapeIndexList;

	// find the clusters visible from each soundscape
	// add this soundscape to the list of soundscapes for that cluster, clip cluster bounds to radius
	for ( int i = 0; i < m_soundscapeEntities.Count(); i++ )
	{
		Vector position = m_soundscapeEntities[i]->GetAbsOrigin();
		float radius = m_soundscapeEntities[i]->m_flRadius;
		float radiusSq = radius * radius;
		engine->GetPVSForCluster( engine->GetClusterForOrigin( position ), sizeof( myPVS ), myPVS );
		for ( int j = 0; j < clusterCount; j++ )
		{
			if ( myPVS[ j >> 3 ] & (1<<(j&7)) )
			{
				float distSq = CalcSqrDistanceToAABB( clusterbounds[j].mins, clusterbounds[j].maxs, position );
				if ( distSq < radiusSq || radius < 0 )
				{
					m_soundscapesInCluster[j].soundscapeCount++;
					clusterIndexList.AddToTail(j);
					// UNDONE: Technically you just need a soundscape index and a count for this list.
					soundscapeIndexList.AddToTail(i);
				}
			}
		}
	}

	// basically this part is like a radix sort
	// this is how many entries we need in the soundscape index list
	m_soundscapeIndexList.SetCount(soundscapeIndexList.Count());

	// now compute the starting index of each cluster
	int firstSoundscape = 0;
	for ( int i = 0; i < clusterCount; i++ )
	{
		m_soundscapesInCluster[i].firstSoundscape = firstSoundscape;
		firstSoundscape += m_soundscapesInCluster[i].soundscapeCount;
		m_soundscapesInCluster[i].soundscapeCount = 0;
	}
	// now add each soundscape index to the appropriate cluster's list
	// The resulting list is precomputing all soundscapes that need to be checked for a player
	// in each cluster.  This is used to accelerate the per-frame operations
	for ( int i = 0; i < soundscapeIndexList.Count(); i++ )
	{
		int cluster = clusterIndexList[i];
		int outIndex = m_soundscapesInCluster[cluster].soundscapeCount + m_soundscapesInCluster[cluster].firstSoundscape;
		m_soundscapesInCluster[cluster].soundscapeCount++;
		m_soundscapeIndexList[outIndex] = soundscapeIndexList[i];
	}
}

int	CSoundscapeSystem::GetSoundscapeIndex( const char *pName )
{
	return m_soundscapes.GetStringID( pName );
}

bool CSoundscapeSystem::IsValidIndex( int index )
{
	if ( index >= 0 && index < m_soundscapeCount )
		return true;
	return false;
}

void CSoundscapeSystem::AddSoundscapeEntity( CEnvSoundscape *pSoundscape )
{
	if ( m_soundscapeEntities.Find( pSoundscape ) == -1 )
	{
		int index = m_soundscapeEntities.AddToTail( pSoundscape );
		pSoundscape->m_soundscapeEntityId = index + 1;
	}
}

void CSoundscapeSystem::RemoveSoundscapeEntity( CEnvSoundscape *pSoundscape )
{
	m_soundscapeEntities.FindAndRemove( pSoundscape );
	pSoundscape->m_soundscapeEntityId = -1;
}

void CSoundscapeSystem::FrameUpdatePostEntityThink()
{
	int total = m_soundscapeEntities.Count();
	if ( total > 0 )
	{
		int traceCount = 0;
		int playerCount = 0;
		// budget tuned for TF.  Do a max of 20 traces.  That's going to happen anyway because a bunch of the maps
		// use radius -1 for all soundscapes.  So to trace one player you'll often need that many and this code must
		// always trace one player's soundscapes.
		// If the map has been optimized, then allow more players to update per frame.
		int maxPlayers = gpGlobals->maxClients / 2;
		// maxPlayers has to be at least 1
		maxPlayers = MAX( 1, maxPlayers );
		int maxTraces = 20;
		if ( soundscape_debug.GetBool() )
		{
			maxTraces = 9999;
			maxPlayers = MAX_PLAYERS;
		}

		// load balance across server ticks a bit by limiting the numbers of players (get cluster for origin)
		// and traces processed in a single tick.  In single player this will update the player every tick
		// because it always does at least one player's full load of work
		for ( int i = 0; i < gpGlobals->maxClients && traceCount <= maxTraces && playerCount <= maxPlayers; i++ )
		{
			m_activeIndex = (m_activeIndex+1) % gpGlobals->maxClients;
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( m_activeIndex + 1 );
			if ( pPlayer && pPlayer->IsNetClient() )
			{
				// check to see if this is the sound entity that is 
				// currently affecting this player
				audioparams_t &audio = pPlayer->GetAudioParams();

				// if we got this far, we're looking at an entity that is contending
				// for current player sound. the closest entity to player wins.
				CEnvSoundscape *pCurrent = (CEnvSoundscape *)( audio.ent.Get() );
				if ( pCurrent )
				{
					int nEntIndex = pCurrent->m_soundscapeEntityId - 1;
					NOTE_UNUSED( nEntIndex );
					Assert( m_soundscapeEntities[nEntIndex] == pCurrent );
				}
				ss_update_t update;
				update.pPlayer = pPlayer;
				update.pCurrentSoundscape = pCurrent;
				update.playerPosition = pPlayer->EarPosition();
				update.bInRange = false;
				update.currentDistance = 0;
				update.traceCount = 0;
				if ( pCurrent )
				{
					pCurrent->UpdateForPlayer(update);
				}

				int clusterIndex = engine->GetClusterForOrigin( update.playerPosition );
			
				if ( clusterIndex >= 0 && clusterIndex < m_soundscapesInCluster.Count() )
				{
					// find all soundscapes that could possibly attach to this player and update them
					for ( int j = 0; j < m_soundscapesInCluster[clusterIndex].soundscapeCount; j++ )
					{
						int ssIndex = m_soundscapeIndexList[m_soundscapesInCluster[clusterIndex].firstSoundscape + j];
						if ( m_soundscapeEntities[ssIndex] == update.pCurrentSoundscape )
							continue;
						m_soundscapeEntities[ssIndex]->UpdateForPlayer( update );
					}
				}
				playerCount++;
				traceCount += update.traceCount;
			}
		}
	}
}

void CSoundscapeSystem::AddSoundscapeSounds( KeyValues *pSoundscape, int soundscapeIndex )
{
	if ( !IsX360() )
	{
		return;
	}

	int i = m_soundscapeSounds.AddToTail();
	Assert( i == soundscapeIndex );

	KeyValues *pKey = pSoundscape->GetFirstSubKey();
	while ( pKey )
	{
		if ( !Q_strcasecmp( pKey->GetName(), "playlooping" ) )
		{
			KeyValues *pAmbientKey = pKey->GetFirstSubKey();
			while ( pAmbientKey )
			{
				if ( !Q_strcasecmp( pAmbientKey->GetName(), "wave" ) )
				{
					char const *pSoundName = pAmbientKey->GetString();
					m_soundscapeSounds[i].AddToTail( pSoundName );
				}
				pAmbientKey = pAmbientKey->GetNextKey();
			}
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "playrandom" ) )
		{
			KeyValues *pRandomKey = pKey->GetFirstSubKey();
			while ( pRandomKey )
			{
				if ( !Q_strcasecmp( pRandomKey->GetName(), "rndwave" ) )
				{
					KeyValues *pRndWaveKey = pRandomKey->GetFirstSubKey();
					while ( pRndWaveKey )
					{
						if ( !Q_strcasecmp( pRndWaveKey->GetName(), "wave" ) )
						{
							char const *pSoundName = pRndWaveKey->GetString();
							m_soundscapeSounds[i].AddToTail( pSoundName );
						}
						pRndWaveKey = pRndWaveKey->GetNextKey();
					}
				}
				pRandomKey = pRandomKey->GetNextKey();
			}
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "playsoundscape" ) )
		{
			KeyValues *pPlayKey = pKey->GetFirstSubKey();
			while ( pPlayKey )
			{
				if ( !Q_strcasecmp( pPlayKey->GetName(), "name" ) )
				{
					char const *pSoundName = pPlayKey->GetString();
					m_soundscapeSounds[i].AddToTail( pSoundName );
				}
				pPlayKey = pPlayKey->GetNextKey();
			}
		}
		pKey = pKey->GetNextKey();
	}
}

void CSoundscapeSystem::PrecacheSounds( int soundscapeIndex )
{
	if ( !IsX360() )
	{
		return;
	}

	if ( !IsValidIndex( soundscapeIndex ) )
	{
		return;
	}

	int count = m_soundscapeSounds[soundscapeIndex].Count();
	for ( int i=0; i<count; i++ )
	{
		const char *pSound = m_soundscapeSounds[soundscapeIndex][i];
		if ( Q_stristr( pSound, ".wav" ) )
		{
			CBaseEntity::PrecacheSound( pSound );
		}
		else
		{
			// recurse into new soundscape
			PrecacheSounds( GetSoundscapeIndex( pSound ) );
		}
	}
}
