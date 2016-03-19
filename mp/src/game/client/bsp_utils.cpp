//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Exposes bsp tools to game for e.g. workshop use
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include <tier2/tier2.h>
#include "filesystem.h"
#include "bsp_utils.h"
#include "utlbuffer.h"
#include "igamesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool BSP_SyncRepack( const char *pszInputMapFile,
                     const char *pszOutputMapFile,
                     IBSPPack::eRepackBSPFlags eRepackFlags )
{
	// load the bsppack dll
	IBSPPack *libBSPPack = NULL;
	CSysModule *pModule = g_pFullFileSystem->LoadModule( "bsppack" );
	if ( pModule )
	{
		CreateInterfaceFn BSPPackFactory = Sys_GetFactory( pModule );
		if ( BSPPackFactory )
		{
			libBSPPack = ( IBSPPack * )BSPPackFactory( IBSPPACK_VERSION_STRING, NULL );
		}
	}
	if( !libBSPPack )
	{
		Warning( "Can't load bsppack library - unable to compress bsp\n" );
		return false;
	}

	Msg( "Repacking %s -> %s\n", pszInputMapFile, pszOutputMapFile );

	if ( !g_pFullFileSystem->FileExists( pszInputMapFile ) )
	{
		Warning( "Couldn't open input file %s - BSP recompress failed\n", pszInputMapFile );
		return false;
	}

	CUtlBuffer inputBuffer;
	if ( !g_pFullFileSystem->ReadFile( pszInputMapFile, NULL, inputBuffer ) )
	{
		Warning( "Couldn't read file %s - BSP compression failed\n", pszInputMapFile );
		return false;
	}

	CUtlBuffer outputBuffer;

	if ( !libBSPPack->RepackBSP( inputBuffer, outputBuffer, eRepackFlags ) )
	{
		Warning( "Internal error compressing BSP\n" );
		return false;
	}

	g_pFullFileSystem->WriteFile( pszOutputMapFile, NULL, outputBuffer );

	Msg( "Successfully repacked %s as %s -- %u -> %u bytes\n",
	     pszInputMapFile, pszOutputMapFile, inputBuffer.TellPut(), outputBuffer.TellPut() );

	return true;
}

// Helper to create a thread that calls SyncCompressMap, and clean it up when it exists
void BSP_BackgroundRepack( const char *pszInputMapFile,
                           const char *pszOutputMapFile,
                           IBSPPack::eRepackBSPFlags eRepackFlags )
{
	// Make this a gamesystem and thread, so it can check for completion each frame and clean itself up. Run() is the
	// background thread, Update() is the main thread tick.
	class BackgroundBSPRepackThread : public CThread, public CAutoGameSystemPerFrame
	{
	public:
		BackgroundBSPRepackThread( const char *pszInputFile, const char *pszOutputFile, IBSPPack::eRepackBSPFlags eRepackFlags )
			: m_strInput( pszInputFile )
			, m_strOutput( pszOutputFile )
			, m_eRepackFlags( eRepackFlags )
		{
			Start();
		}

		// CThread job - returns 0 for success
		virtual int Run() OVERRIDE
		{
			return BSP_SyncRepack( m_strInput.Get(), m_strOutput.Get(), m_eRepackFlags ) ? 0 : 1;
		}

		// GameSystem
		virtual const char* Name( void ) OVERRIDE { return "BackgroundBSPRepackThread"; }

		// Runs on main thread
		void CheckFinished()
		{
			if ( !IsAlive() )
			{
				// Thread finished
				if ( GetResult() != 0 )
				{
					Warning( "Map compression thread failed :(\n" );
				}

				// AutoGameSystem deregisters itself on destruction, we're done
				delete this;
			}
		}

		#ifdef CLIENT_DLL
		virtual void Update( float frametime ) OVERRIDE { CheckFinished(); }
        #else // GAME DLL
		virtual void FrameUpdatePostEntityThink() OVERRIDE { CheckFinished(); }
		#endif
	private:
		CUtlString                m_strInput;
		CUtlString                m_strOutput;
		IBSPPack::eRepackBSPFlags m_eRepackFlags;
	};

	Msg( "Starting BSP repack job %s -> %s\n", pszInputMapFile, pszOutputMapFile );

	// Deletes itself up when done
	new BackgroundBSPRepackThread( pszInputMapFile, pszOutputMapFile, eRepackFlags );
}

CON_COMMAND( bsp_repack, "Repack and output a (re)compressed version of a bsp file" )
{
#ifdef GAME_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	// Handle -nocompress
	bool bCompress = true;
	const char *szInFilename = NULL;
	const char *szOutFilename = NULL;

	if ( args.ArgC() == 4 && V_strcasecmp( args.Arg( 1 ), "-nocompress" ) == 0 )
	{
		bCompress = false;
		szInFilename = args.Arg( 2 );
		szOutFilename = args.Arg( 3 );
	}
	else if ( args.ArgC() == 3 )
	{
		szInFilename = args.Arg( 1 );
		szOutFilename = args.Arg( 2 );
	}

	if ( !szInFilename || !szOutFilename || !strlen( szInFilename ) || !strlen( szOutFilename ) )
	{
		Msg( "Usage: bsp_repack [-nocompress] map.bsp output_map.bsp\n" );
		return;
	}

	if ( bCompress )
	{
		// Use default compress flags
		BSP_BackgroundRepack( szInFilename, szOutFilename );
	}
	else
	{
		// No compression
		BSP_BackgroundRepack( szInFilename, szOutFilename, (IBSPPack::eRepackBSPFlags)0 );
	}
}
