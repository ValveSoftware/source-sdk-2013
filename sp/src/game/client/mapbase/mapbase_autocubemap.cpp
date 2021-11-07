//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: A utility which automatically generates HDR and LDR cubemaps.
//			This has the following purposes:
// 
//            1. Allow both HDR and LDR cubemaps to be generated automatically after a map is compiled
//            2. Have a way to batch build cubemaps for several levels at once
// 
// Author: Blixibon
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tier0/icommandline.h"
#include "igamesystem.h"
#include "filesystem.h"
#include "utlbuffer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern const char *g_MapName;

ConVar autocubemap_hdr_do_both( "autocubemap_hdr_do_both", "1" );
ConVar autocubemap_hdr_value( "autocubemap_hdr_value", "2" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CAutoCubemapSystem : public CAutoGameSystem
{
public:
	CAutoCubemapSystem() : CAutoGameSystem( "CAutoCubemapSystem" )
	{
	}

	virtual bool Init()
	{
		const char *pszFile = NULL;
		if (CommandLine()->CheckParm( "-autocubemap", &pszFile ))
		{
			if (!pszFile || pszFile[0] == '\0')
			{
				// Assume that we just want to autocubemap the first map we load
				// (no code here for now)
			}
			else
			{
				LoadFile( pszFile );
			}

			// Begin autocubemap with the first level we load
			m_bAutoCubemapOnFirstLevel = true;
		}

		return true;
	}

	virtual void LevelInitPostEntity()
	{
		if (m_bAutoCubemapActive)
		{
			if (m_bAutoCubemapBuildingCubemaps)
			{
				// Check if we need to do the other HDR level
				if (autocubemap_hdr_do_both.GetBool() && !m_bAutoCubemapDoingBoth)
				{
					m_bAutoCubemapBuildingCubemaps = false;
					m_bAutoCubemapDoingBoth = true;

					// Change the HDR level and restart the map
					//ConVarRef mat_hdr_level( "mat_hdr_level" );
					engine->ClientCmd_Unrestricted( VarArgs( "toggle mat_hdr_level 0 %i; restart", autocubemap_hdr_value.GetInt() ) );
				}
				else
				{
					// Go to the next map
					m_bAutoCubemapBuildingCubemaps = false;
					m_bAutoCubemapDoingBoth = false;

					m_AutoCubemapMapsIndex++;
					if (m_AutoCubemapMapsIndex < m_AutoCubemapMaps.Count())
					{
						engine->ClientCmd_Unrestricted( VarArgs( "map %s", m_AutoCubemapMaps[m_AutoCubemapMapsIndex] ) );
					}
					else
					{
						// CUBEMAPPER FINISHED
						m_AutoCubemapMaps.PurgeAndDeleteElements();
						m_AutoCubemapMapsIndex = 0;
						m_bAutoCubemapActive = false;

						Msg( "CUBEMAPPER FINISHED\n" );

						if (autocubemap_hdr_do_both.GetBool())
						{
							engine->ClientCmd_Unrestricted( VarArgs( "mat_hdr_level %i", m_iAutoCubemapUserHDRLevel ) );
						}
					}
				}
			}
			else
			{
				// Build cubemaps for this map
				m_bAutoCubemapBuildingCubemaps = true;
				engine->ClientCmd_Unrestricted( "exec buildcubemaps_prep; buildcubemaps" );
			}
		}
		else if (m_bAutoCubemapOnFirstLevel)
		{
			// Start autocubemap now
			StartAutoCubemap();
			m_bAutoCubemapOnFirstLevel = false;
		}
	}

	//-------------------------------------------------------------------------------------

	void StartAutoCubemap()
	{
		if (m_AutoCubemapMaps.Count() <= 0)
		{
			//Msg("No maps to cubemap with!\n");
			//return;

			// Just do this map
			m_AutoCubemapMaps.AddToTail( strdup( g_MapName ) );
		}

		if (autocubemap_hdr_do_both.GetBool())
		{
			// Save the user's HDR level
			ConVarRef mat_hdr_level( "mat_hdr_level" );
			m_iAutoCubemapUserHDRLevel = mat_hdr_level.GetInt();
		}

		m_bAutoCubemapActive = true;
		m_AutoCubemapMapsIndex = 0;

		if (FStrEq( m_AutoCubemapMaps[m_AutoCubemapMapsIndex], g_MapName ))
		{
			// Build cubemaps right here, right now
			m_bAutoCubemapBuildingCubemaps = true;
			engine->ClientCmd_Unrestricted( "exec buildcubemaps_prep; buildcubemaps" );
		}
		else
		{
			// Go to that map
			engine->ClientCmd_Unrestricted( VarArgs( "map %s", m_AutoCubemapMaps[m_AutoCubemapMapsIndex] ) );
		}
	}

	void LoadFile( const char *pszFile )
	{
		KeyValues *pKV = new KeyValues( "AutoCubemap" );

		if ( pKV->LoadFromFile( filesystem, pszFile, NULL ) )
		{
			KeyValues *pSubKey = pKV->GetFirstSubKey();

			while ( pSubKey )
			{
				m_AutoCubemapMaps.AddToTail( strdup(pSubKey->GetName()) );
				pSubKey = pSubKey->GetNextKey();
			}

			Msg( "Initted autocubemap\n" );
		}
		else
		{
			Warning( "Unable to load autocubemap file \"%s\"\n", pszFile );
		}

		pKV->deleteThis();
	}

	void Clear()
	{
		m_bAutoCubemapActive = false;
		m_bAutoCubemapBuildingCubemaps = false;
		m_bAutoCubemapDoingBoth = false;

		m_AutoCubemapMaps.PurgeAndDeleteElements();
		m_AutoCubemapMapsIndex = 0;
	}

	void PrintState()
	{
		char szCmd[1024] = { 0 };

		if (m_AutoCubemapMaps.Count() > 0)
		{
			Q_strncpy( szCmd, "=== CUBEMAPPER MAP LIST ===\n", sizeof( szCmd ) );

			FOR_EACH_VEC( m_AutoCubemapMaps, i )
			{
				Q_snprintf( szCmd, sizeof( szCmd ), "%s%s\n", szCmd, m_AutoCubemapMaps[i] );
			}

			Q_strncat( szCmd, "========================", sizeof( szCmd ), COPY_ALL_CHARACTERS );

			Q_snprintf( szCmd, sizeof( szCmd ), "%s\nNumber of maps: %i (starting at %i)\n", szCmd, m_AutoCubemapMaps.Count(), m_AutoCubemapMapsIndex );
		}
		else
		{
			Q_strncat( szCmd, "========================\n", sizeof( szCmd ), COPY_ALL_CHARACTERS );
			Q_strncat( szCmd, "There are no maps selected. Use 'autocubemap_init' to load a map list.\nIf 'autocubemap_start' is executed while no maps are selected, only the current map will have cubemaps generated.\n", sizeof( szCmd ), COPY_ALL_CHARACTERS );
			Q_strncat( szCmd, "========================\n", sizeof( szCmd ), COPY_ALL_CHARACTERS );
		}

		Msg( "%s", szCmd );
	}

	//-------------------------------------------------------------------------------------

	bool m_bAutoCubemapActive = false;
	bool m_bAutoCubemapBuildingCubemaps = false;
	bool m_bAutoCubemapDoingBoth = false;
	int m_iAutoCubemapUserHDRLevel;		// For setting the user back to the right HDR level when we're finished

	// Start autocubemap with the first level we load (used for launch parameter)
	bool m_bAutoCubemapOnFirstLevel = false;

	CUtlVector<const char*> m_AutoCubemapMaps;
	int m_AutoCubemapMapsIndex;
};

CAutoCubemapSystem	g_AutoCubemapSystem;

CON_COMMAND( autocubemap_init, "Inits autocubemap" )
{
	if (gpGlobals->maxClients > 1)
	{
		Msg( "Can't run autocubemap in multiplayer\n" );
		return;
	}

	if (args.ArgC() <= 1)
	{
		Msg("Format: autocubemap_init <file name in the 'cfg' folder>\n");
		return;
	}

	g_AutoCubemapSystem.LoadFile( args.Arg( 1 ) );
}

CON_COMMAND( autocubemap_print, "Prints current autocubemap information" )
{
	if (gpGlobals->maxClients > 1)
	{
		Msg("Can't run autocubemap in multiplayer\n");
		return;
	}

	g_AutoCubemapSystem.PrintState();
}

CON_COMMAND( autocubemap_clear, "Clears autocubemap stuff" )
{
	if (gpGlobals->maxClients > 1)
	{
		Msg("Can't run autocubemap in multiplayer\n");
		return;
	}

	g_AutoCubemapSystem.Clear();
}

CON_COMMAND( autocubemap_start, "Begins the autocubemap (it's recommended to check 'autocubemap_print' before running this command)" )
{
	if (gpGlobals->maxClients > 1)
	{
		Msg("Can't run autocubemap in multiplayer\n");
		return;
	}

	g_AutoCubemapSystem.StartAutoCubemap();
}
