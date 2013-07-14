//====== Copyright @ Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy.h"
#include "filesystem.h"
#include "icommandline.h"
#include "srcpy_usermessage.h"
// TODO: #include "srcpy_gamerules.h"
#include "srcpy_entities.h"
#include "srcpy_networkvar.h"
#include "gamestringpool.h"

#ifdef CLIENT_DLL
	#include "networkstringtable_clientdll.h"
	// TODO #include "src_python_materials.h"
#else
	#include "networkstringtable_gamedll.h"
#endif // CLIENT_DLL

#include <winlite.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Shorter alias
namespace bp = boost::python;

#ifdef CLIENT_DLL
	// TODO: extern void DestroyPyPanels();
#endif // CLIENT_DLL

// Stubs for Python
const char *Py_GetBuildInfo(void) { return ""; }
const char *_Py_hgversion(void) { return ""; }
const char *_Py_hgidentifier(void) { return ""; }

extern "C" 
{
	char dllVersionBuffer[16] = ""; // a private buffer
	HMODULE PyWin_DLLhModule = NULL;
	const char *PyWin_DLLVersionString = dllVersionBuffer;
}

// For debugging
ConVar g_debug_python( "g_debug_python", "0", FCVAR_REPLICATED );

const Color g_PythonColor( 0, 255, 0, 255 );

// The thread ID in which python is initialized
unsigned int g_hPythonThreadID;

#if defined (PY_CHECK_LOG_OVERRIDES) || defined (_DEBUG)
	ConVar py_log_overrides("py_log_overrides", "0", FCVAR_REPLICATED);
#endif

// Global main space
boost::python::object mainmodule;
boost::python::object mainnamespace;

// Global module references.
bp::object builtins;
bp::object types;
bp::object sys;

bp::object weakref;
bp::object srcbuiltins;
bp::object _entitiesmisc;
bp::object _entities;
bp::object _particles;
bp::object _physics;
//bp::object matchmaking;

#ifdef CLIENT_DLL
	boost::python::object _vguicontrols;
#endif // CLIENT_DLL

boost::python::object fntype;

static CSrcPython g_SrcPythonSystem;

CSrcPython *SrcPySystem()
{
	return &g_SrcPythonSystem;
}

// Prevent python classes from initializing
bool g_bDoNotInitPythonClasses = true;

#ifdef CLIENT_DLL
extern void HookPyNetworkCls();
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Required by boost to be user defined if BOOST_NO_EXCEPTIONS is defined
//			http://www.boost.org/doc/libs/1_54_0/libs/exception/doc/throw_exception.html
//-----------------------------------------------------------------------------
namespace boost
{
	void throw_exception(std::exception const & e)
	{
		Warning("Boost Python Exception\n");

		FileHandle_t fh = filesystem->Open( "log.txt", "wb" );
		if ( !fh )
		{
			DevWarning( 2, "Couldn't create %s!\n", "log.txt" );
			return;
		}

		filesystem->Write( "Exception", strlen("Exception"), fh );
		filesystem->Close(fh);
	}
}

// Append functions
#ifdef CLIENT_DLL
extern void AppendClientModules();
#else
extern void AppendServerModules();
#endif // CLIENT_DLL
extern void AppendSharedModules();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSrcPython::CSrcPython()
{
	m_bPythonRunning = false;
	m_bPythonIsFinalizing = false;
	m_bPathProtected = true;

	double fStartTime = Plat_FloatTime();
	// Before the python interpreter is initialized, the modules must be appended
#ifdef CLIENT_DLL
	AppendClientModules();
#else
	AppendServerModules();
#endif // CLIENT_DLL
	AppendSharedModules();

#ifdef CLIENT_DLL
	DevMsg( "CLIENT: " );
#else
	DevMsg( "SERVER: " );
#endif // CLIENT_DLL
	DevMsg( "Added Python default modules... (%f seconds)\n", Plat_FloatTime() - fStartTime );
}

bool CSrcPython::Init( )
{
	return InitInterpreter();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::Shutdown( void )
{
	if( !m_bPythonRunning )
		return;

#ifdef CLIENT_DLL
	// TODO: PyShutdownProceduralMaterials();
#endif // CLIENT_DLL

	PyErr_Clear(); // Make sure it does not hold any references...
	GarbageCollect();

	ShutdownInterpreter();
}

#ifdef WIN32
extern "C" {
	void PyImport_FreeDynLibraries( void );
}
#endif // WIN32

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::ExtraShutdown( void )
{
	//ShutdownInterpreter();
}

static void VSPTCheckForParm( bp::list argv, const char *pParmToCheck )
{
	if( CommandLine()->FindParm( pParmToCheck ) == 0 )
		return;

	const char *value = CommandLine()->ParmValue( pParmToCheck, (const char *)0 );
	Msg("VSPT argument %s: %s\n", pParmToCheck, value ? value : "<null>" );
	argv.append( pParmToCheck );
	if( value )
		argv.append( value );
}

static void VSPTParmRemove( const char *pParmToCheck )
{
	if( CommandLine()->FindParm( pParmToCheck ) == 0 )
		return;

	CommandLine()->RemoveParm( pParmToCheck );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::InitInterpreter( void )
{
	const bool bEnabled = CommandLine() && CommandLine()->FindParm("-disablepython") == 0;
	const bool bRunInterpreter = CommandLine() && CommandLine()->FindParm("-interpreter") != 0;
	//const bool bToolsMode = CommandLine() && CommandLine()->FindParm("-tools") != 0;

	/*const char *pGameDir = COM_GetModDirectory();
	const char *pDevModDir = "hl2wars_asw_dev";
	if( Q_strncmp( pGameDir, pDevModDir, Q_strlen( pDevModDir ) ) != 0 )*/
	DevMsg("CommandLine: %s\n", CommandLine()->GetCmdLine());
	m_bPathProtected = CommandLine() ? CommandLine()->FindParm("-nopathprotection") == 0 : true;

	bool bNoChangeWorkingDirectory = CommandLine() ? CommandLine()->FindParm("-testnochangeworkingdir") != 0 : false;

	if( !bEnabled )
	{
	#ifdef CLIENT_DLL
		ConColorMsg( g_PythonColor, "CLIENT: " );
	#else
		ConColorMsg( g_PythonColor, "SERVER: " );
	#endif // CLIENT_DLL
		ConColorMsg( g_PythonColor, "Python is disabled.\n" );
		return true;
	}

	if( m_bPythonRunning )
		return true;

#if 1
	if( !bNoChangeWorkingDirectory )
	{
		// Change working directory	
		char moddir[_MAX_PATH];
		filesystem->RelativePathToFullPath(".", "MOD", moddir, _MAX_PATH);
		V_FixupPathName(moddir, _MAX_PATH, moddir);	
		V_SetCurrentDirectory(moddir);
	}
#endif // 0

	m_bPythonRunning = true;

	double fStartTime = Plat_FloatTime();

	char buf[MAX_PATH];
	char pythonpath[MAX_PATH];
	pythonpath[0] = '\0';

	filesystem->RelativePathToFullPath("python/Lib", "MOD", buf, _MAX_PATH);
	V_FixupPathName(buf, MAX_PATH, buf);
	V_strcat( pythonpath, buf, MAX_PATH );
	V_strcat( pythonpath, ";", MAX_PATH );
	filesystem->RelativePathToFullPath("python/Lib/DLLs", "MOD", buf, _MAX_PATH);
	V_FixupPathName(buf, MAX_PATH, buf);
	V_strcat( pythonpath, buf, MAX_PATH );

	::SetEnvironmentVariable( "PYTHONPATH", pythonpath );

	// Initialize an interpreter
	Py_InitializeEx( 0 );
#ifdef CLIENT_DLL
	ConColorMsg( g_PythonColor, "CLIENT: " );
#else
	ConColorMsg( g_PythonColor, "SERVER: " );
#endif // CLIENT_DLL
	ConColorMsg( g_PythonColor, "Initialized Python... (%f seconds)\n", Plat_FloatTime() - fStartTime );
	fStartTime = Plat_FloatTime();

	// Save our thread ID
#ifdef _WIN32
	g_hPythonThreadID = GetCurrentThreadId();
#endif // _WIN32

	// get our main space
	try 
	{
		mainmodule = bp::import("__main__");
		mainnamespace = mainmodule.attr("__dict__");
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to import main namespace!\n");
		PyErr_Print();
		return false;
	}

	// Import sys module
	Run( "import sys" );
	
	sys = Import("sys");

	// Redirect stdout/stderr to source print functions
	srcbuiltins = Import("srcbuiltins");
	sys.attr("stdout") = srcbuiltins.attr("SrcPyStdOut")();
	sys.attr("stderr") = srcbuiltins.attr("SrcPyStdErr")();

	weakref = Import("weakref");
	builtins = Import("builtins");

	// Set isclient and isserver globals to the right values
	try 
	{
#ifdef CLIENT_DLL
		builtins.attr("isclient") = true;
		builtins.attr("isserver") = false;
#else
		builtins.attr("isclient") = false;
		builtins.attr("isserver") = true;
#endif // CLIENT_DLL
		// TODO: builtins.attr("gpGlobals") = srcbase.attr("gpGlobals");
	} 
	catch( bp::error_already_set & ) 
	{
		PyErr_Print();
	}

	fntype = builtins.attr("type");

	// Add the maps directory to the modulse path
	SysAppendPath("maps");

	// Default imports
	// TODO: Run( "import vmath" );
	types = Import("types");
	// TODO: Run( "import sound" ); // Import _sound before _entitiesmisc (register converters)
	Run( "import _entitiesmisc" );
	_entitiesmisc = Import("_entitiesmisc");
	Run( "import _entities" );
	_entities = Import("_entities");
	// TODO: _particles = Import("_particles");
	// TODO: _physics = Import("_physics");
	// TODO: matchmaking = Import("matchmaking");
#ifdef CLIENT_DLL
	// TODO: Run( "import input" );		// Registers buttons
	// TODO: _vguicontrols = Import("_vguicontrols");
#endif	// CLIENT_DLL

#ifdef CLIENT_DLL
	DevMsg( "CLIENT: " );
#else
	DevMsg( "SERVER: " );
#endif // CLIENT_DLL
	DevMsg( "Initialized Python default modules... (%f seconds)\n", Plat_FloatTime() - fStartTime );

	// Support for Visual Studio Python Tools
	if( bRunInterpreter && CommandLine() )
	{
		char interpreterFile[MAX_PATH];
		interpreterFile[0] = 0;
		const char *pParmInterpreterFile = CommandLine()->ParmValue( "-interpreter" );
		if( pParmInterpreterFile )
			V_strncpy( interpreterFile, pParmInterpreterFile, MAX_PATH );

		if( interpreterFile[0] != 0 )
		{
			char vtptpath[MAX_PATH];
			V_strncpy( vtptpath, interpreterFile, MAX_PATH );
			V_StripFilename( vtptpath );
			this->SysAppendPath( vtptpath );

			VSPTParmRemove("-insecure");
			/*VSPTParmRemove("-dev");
			VSPTParmRemove("-textmode");
			VSPTParmRemove("-game");
			VSPTParmRemove("-interpreter");*/

			//bp::list argv = bp::list();
			//argv.append( interpreterFile );

			/*
			int n = CommandLine()->ParmCount();
			for( int i = 1; i < n; i++ )
			{
				Msg("VSPT Parm #%d: %s\n", i, CommandLine()->GetParm( i ) );
				argv.append( CommandLine()->GetParm( i ) );
			}*/

			// Check "visualstudio_py_repl.py" from visual studio python tools for all arguments
			//VSPTCheckForParm( argv, "--port" );
			//VSPTCheckForParm( argv, "--execution_mode" );
			//VSPTCheckForParm( argv, "--enable-attach" );
			//VSPTCheckForParm( argv, "--launch_file" );

			bp::str args( CommandLine()->GetCmdLine() );

			try
			{
				bp::str remainder( args.split( "-interpreter", 1 )[1] );

				bp::exec( "import shlex", mainnamespace, mainnamespace );
				bp::object shlex = Import("shlex");

				bp::str newCmd( /*"\"" + bp::str(interpreterFile) + "\" " +*/ remainder );
				newCmd = newCmd.replace( "\\\"", "\\\\\"" );

				builtins.attr("print")( newCmd );

				bp::list argv( shlex.attr("split")( newCmd ) );
				bp::setattr( sys, bp::object("argv"), argv );
				}
				catch( bp::error_already_set & ) 
				{
					PyErr_Print();
				}

				DevMsg("New CommandLine: %s\n", CommandLine()->GetCmdLine() );

				// Change working directory	
				V_SetCurrentDirectory( vtptpath );

				Msg("Running interpreter file: %s\n", interpreterFile );
				Run("import encodings.idna");
				this->ExecuteFile( interpreterFile );
		}

		::TerminateProcess( ::GetCurrentProcess(), 0 );
		return true;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::ShutdownInterpreter( void )
{
	if( !m_bPythonRunning )
		return false;

	PyErr_Clear(); // Make sure it does not hold any references...
	GarbageCollect();

#ifdef CLIENT_DLL
	// Clear python panels
	// TODO: DestroyPyPanels();
#endif // CLIENT_DLL

#if 0 // TODO
	// Clear Python gamerules
	ClearPyGameRules();
#endif // 0

	// Make sure these lists don't hold references
	m_deleteList.Purge();
	m_methodTickList.Purge();
	m_methodPerFrameList.Purge();

	// Disconnect redirecting stdout/stderr
	sys.attr("stdout") = bp::object();
	sys.attr("stderr") = bp::object();

	// Clear modules
	mainmodule = bp::object();
	mainnamespace = bp::object();

	builtins = bp::object();
	sys = bp::object();
	types = bp::object();
	weakref = bp::object();
	_entitiesmisc = bp::object();
	_entities = bp::object();
	_particles = bp::object();
	_physics = bp::object();
	//matchmaking = bp::object();
#ifdef CLIENT_DLL
	_vguicontrols = bp::object();
#endif // CLIENT_DLL

	// Finalize
	m_bPythonIsFinalizing = true;
	PyErr_Clear(); // Make sure it does not hold any references...
	GarbageCollect();
	Py_Finalize();
#ifdef WIN32
	// TODO/Check if still needed or can be solved otherwise
	//PyImport_FreeDynLibraries(); // IMPORTANT, otherwise it will crash.
#endif // WIN32
	m_bPythonIsFinalizing = false;
	m_bPythonRunning = false;

#ifdef CLIENT_DLL
	ConColorMsg( g_PythonColor, "CLIENT: " );
#else
	ConColorMsg( g_PythonColor, "SERVER: " );
#endif // CLIENT_DLL
	ConColorMsg( g_PythonColor, "Python is no longer running...\n" );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::PostInit()
{
	if( !IsPythonRunning() )
		return;

	// Hook PyMessage
#ifdef CLIENT_DLL
	HookPyMessage();
	HookPyNetworkCls();
	HookPyNetworkVar();
#endif // CLIENT_DLL

	// Autorun once
	ExecuteAllScriptsInPath("python/autorun_once/");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::LevelInitPreEntity()
{
	m_bActive = true;

	if( !IsPythonRunning() )
		return;

#ifdef CLIENT_DLL
	char pLevelName[_MAX_PATH];
	Q_FileBase(engine->GetLevelName(), pLevelName, _MAX_PATH);
#else
	const char *pLevelName = STRING(gpGlobals->mapname);
#endif

	m_LevelName = AllocPooledString(pLevelName);

	// BEFORE creating the entities setup the network tables
#ifndef CLIENT_DLL
	SetupNetworkTables();
#endif // CLIENT_DLL

	// Send prelevelinit signal
	try 
	{
		CallSignalNoArgs( Get("prelevelinit", "core.signals", true) );
		CallSignalNoArgs( Get("map_prelevelinit", "core.signals", true)[STRING(m_LevelName)] );
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to retrieve level signal:\n");
		PyErr_Print();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::LevelInitPostEntity()
{
	if( !IsPythonRunning() )
		return;

	// Send postlevelinit signal
	try 
	{
		CallSignalNoArgs( Get("postlevelinit", "core.signals", true) );
		CallSignalNoArgs( Get("map_postlevelinit", "core.signals", true)[STRING(m_LevelName)] );
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to retrieve level signal:\n");
		PyErr_Print();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::LevelShutdownPreEntity()
{
	if( !IsPythonRunning() )
		return;

	// Send prelevelshutdown signal
	try 
	{
		CallSignalNoArgs( Get("prelevelshutdown", "core.signals", true) );
		CallSignalNoArgs( Get("map_prelevelshutdown", "core.signals", true)[STRING(m_LevelName)] );
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to retrieve level signal:\n");
		PyErr_Print();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::LevelShutdownPostEntity()
{
	if( !IsPythonRunning() )
		return;

	// Send postlevelshutdown signal
	try 
	{
		CallSignalNoArgs( Get("postlevelshutdown", "core.signals", true) );
		CallSignalNoArgs( Get("map_postlevelshutdown", "core.signals", true)[STRING(m_LevelName)] );
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to retrieve level signal:\n");
		PyErr_Print();
	}

	// Reset all send/recv tables
	PyResetAllNetworkTables();

	// Clear all tick signals next times (level time based)
	for( int i = m_methodTickList.Count() - 1; i >= 0; i--)
	{
		m_methodTickList[i].m_fNextTickTime = 0;
	}

	m_bActive = false;
}

static ConVar py_disable_update("py_disable_update", "0", FCVAR_CHEAT|FCVAR_REPLICATED);

#ifdef CLIENT_DLL
void CSrcPython::Update( float frametime )
#else
void CSrcPython::FrameUpdatePostEntityThink( void )
#endif // CLIENT_DLL
{
	if( !IsPythonRunning() )
		return;

	// Update tick methods
	int i;
	for(i=m_methodTickList.Count()-1; i>=0; i--)
	{
		if( m_methodTickList[i].m_fNextTickTime < gpGlobals->curtime )
		{
			try 
			{
				m_methodTickList[i].method();

				// Method might have removed the method already
				if( !m_methodTickList.IsValidIndex(i) )
					continue;

				// Remove tick methods that are not looped (used to call back a function after a set time)
				if( !m_methodTickList[i].m_bLooped )
				{
					m_methodTickList.Remove(i);
					continue;
				}
			} 
			catch( bp::error_already_set & ) {
				Warning("Unregistering tick method due the following exception (catch exception if you don't want this): \n");
				PyErr_Print();
				m_methodTickList.Remove(i);
				continue;
			}
			m_methodTickList[i].m_fNextTickTime = gpGlobals->curtime + m_methodTickList[i].m_fTickSignal;
		}	
	}

	// Update frame methods
	for(i=m_methodPerFrameList.Count()-1; i>=0; i--)
	{
		try 
		{
			m_methodPerFrameList[i]();
		}
		catch( bp::error_already_set & ) {
			Warning("Unregistering per frame method due the following exception (catch exception if you don't want this): \n");
			PyErr_Print();
			m_methodPerFrameList.Remove(i);
			continue;
		}
	}

#ifdef CLIENT_DLL
	// TODO: PyUpdateProceduralMaterials();

	CleanupDelayedUpdateList();
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::object CSrcPython::Import( const char *pModule )
{
	// Import into the main space
	try
	{
		return bp::import(pModule);
	}
	catch( bp::error_already_set & )
	{
#ifdef CLIENT_DLL
		DevMsg("CLIENT: ImportPyModuleIntern failed -> mod: %s\n", pModule );
#else
		DevMsg("SERVER: ImportPyModuleIntern failed -> mod: %s\n", pModule );
#endif

		PyErr_Print();
	}

	return bp::object();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::object CSrcPython::ImportSilent( const char *pModule )
{
	// Import into the main space
	try
	{
		return bp::import(pModule);
	}
	catch( bp::error_already_set & )
	{
		PyErr_Clear();
	}

	return bp::object();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::object CSrcPython::Get( const char *pAttrName, bp::object obj, bool bReport )
{
	try 
	{
		return obj.attr(pAttrName);
	}
	catch(bp::error_already_set &) 
	{
		if( bReport )
			PyErr_Print();	
		else
			PyErr_Clear();
	}
	return bp::object();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::object CSrcPython::Get( const char *pAttrName, const char *pModule, bool bReport )
{
	return Get( pAttrName, Import(pModule), bReport );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::Run( bp::object method, bool report_errors )
{
	PYRUNMETHOD( method, report_errors )
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::Run( const char *pString, const char *pModule )
{
	// execute statement
	try
	{
		bp::object dict = pModule ? Import(pModule).attr("__dict__") : mainnamespace;
		bp::exec( pString, dict, dict );
	}
	catch( bp::error_already_set & )
	{
		PyErr_Print();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::ExecuteFile( const char* pScript )
{
	char char_filename[ _MAX_PATH ];
	char char_output_full_filename[ _MAX_PATH ];

	strcpy( char_filename, pScript);
	filesystem->RelativePathToFullPath(char_filename,"MOD",char_output_full_filename,sizeof(char_output_full_filename));

	const char *constcharpointer = reinterpret_cast<const char *>( char_output_full_filename );

	if (!filesystem->FileExists(constcharpointer))
	{
		Warning( "[Python] IFileSystem Cannot find the file: %s\n", constcharpointer);
		return false;
	}

	try
	{
		exec_file(constcharpointer, mainnamespace, mainnamespace );
	}
	catch( bp::error_already_set & )
	{
#ifdef CLIENT_DLL
		DevMsg("CLIENT: ");
#else
		DevMsg("SERVER: ");
#endif // CLIENT_DLL
		DevMsg("RunPythonFile failed -> file: %s\n", pScript );
		PyErr_Print();
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::Reload( const char *pModule )
{
	DevMsg("Reloading module %s\n", pModule);

	try
	{
		// import into the main space
		char command[MAX_PATH];
		Q_snprintf( command, sizeof( command ), "import %s", pModule );
		exec(command, mainnamespace, mainnamespace );

		// Reload
		Q_snprintf( command, sizeof( command ), "reload(%s)", pModule );
		exec(command, mainnamespace, mainnamespace );
	}
	catch( bp::error_already_set & )
	{
		PyErr_Print();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Collect garbage
//-----------------------------------------------------------------------------
void CSrcPython::GarbageCollect( void )
{
	PyGC_Collect();
}

//-----------------------------------------------------------------------------
// Purpose: Add a path
//-----------------------------------------------------------------------------
void CSrcPython::SysAppendPath( const char* path, bool inclsubdirs )
{
	// First retrieve the append method
	bp::object append = Get("append", Get("path", "sys", true), true);

	// Fixup path
	char char_output_full_filename[ _MAX_PATH ];
	char p_out[_MAX_PATH];
	filesystem->RelativePathToFullPath(path,"GAME",char_output_full_filename,sizeof(char_output_full_filename));
	V_FixupPathName(char_output_full_filename, _MAX_PATH, char_output_full_filename );
	V_StrSubst(char_output_full_filename, "\\", "//", p_out, _MAX_PATH ); 

	// Append
	Run<const char *>( append, p_out, true );

	// Check for sub dirs
	if( inclsubdirs )
	{
		char wildcard[MAX_PATH];
		FileFindHandle_t findHandle;
		
		Q_snprintf( wildcard, sizeof( wildcard ), "%s//*", path );
		const char *pFilename = filesystem->FindFirstEx( wildcard, "MOD", &findHandle );
		while ( pFilename != NULL )
		{

			if( Q_strncmp(pFilename, ".", 1) != 0 &&
				Q_strncmp(pFilename, "..", 2) != 0 &&
				filesystem->FindIsDirectory(findHandle) ) 
			{
				char path2[MAX_PATH];
				Q_snprintf( path2, sizeof( path2 ), "%s//%s", path, pFilename );
				SysAppendPath(path2, inclsubdirs);
			}
			pFilename = filesystem->FindNext( findHandle );
		}
		filesystem->FindClose( findHandle );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create a weakref using the weakref module
//-----------------------------------------------------------------------------
bp::object CSrcPython::CreateWeakRef( bp::object obj_ref )
{
	try
	{
		 return weakref.attr("ref")(obj_ref);
	}
	catch( bp::error_already_set & )
	{
		PyErr_Print();
	}
	return bp::object();
}

//-----------------------------------------------------------------------------
// Purpose: Execute all python files in a folder
//-----------------------------------------------------------------------------
void CSrcPython::ExecuteAllScriptsInPath( const char *pPath )
{
	char tempfile[MAX_PATH];
	char wildcard[MAX_PATH];

	Q_snprintf( wildcard, sizeof( wildcard ), "%s*.py", pPath );

	FileFindHandle_t findHandle;
	const char *pFilename = filesystem->FindFirstEx( wildcard, "GAME", &findHandle );
	while ( pFilename != NULL )
	{
		Q_snprintf( tempfile, sizeof( tempfile ), "%s/%s", pPath, pFilename );
		Msg("Executing %s\n", tempfile);
		ExecuteFile(tempfile);
		pFilename = filesystem->FindNext( findHandle );
	}
	filesystem->FindClose( findHandle );
}

#if 0 // TODO: Still used?
//-----------------------------------------------------------------------------
// Purpose: Identifier between server and client
//-----------------------------------------------------------------------------
int CSrcPython::GetModuleIndex( const char *pModule )
{
	if ( pModule )
	{
		int nIndex = g_pStringTablePyModules->FindStringIndex( pModule );
		if (nIndex != INVALID_STRING_INDEX ) 
			return nIndex;

		return g_pStringTablePyModules->AddString( CBaseEntity::IsServer(), pModule );
	}

	// This is the invalid string index
	return INVALID_STRING_INDEX;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char * CSrcPython::GetModuleNameFromIndex( int nModuleIndex )
{
	if ( nModuleIndex >= 0 && nModuleIndex < g_pStringTablePyModules->GetMaxStrings() )
		return g_pStringTablePyModules->GetString( nModuleIndex );
	return "error";
}
#endif // 0

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::CallSignalNoArgs( bp::object signal )
{
	// TODO
#if 0
	try 
	{
		srcmgr.attr("_CheckReponses")( 
			signal.attr("send_robust")( bp::object() )
		);
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to call signal:\n");
		PyErr_Print();
	}
#endif // 0
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::CallSignal( bp::object signal, bp::dict kwargs )
{
	// TODO
#if 0
	try 
	{
		srcmgr.attr("_CallSignal")( bp::object(signal.attr("send_robust")), kwargs );
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to call signal:\n");
		PyErr_Print();
	}
#endif // 0
}

//-----------------------------------------------------------------------------
// Purpose: Retrieving basic type values
//-----------------------------------------------------------------------------
int	CSrcPython::GetInt(const char *name, bp::object obj, int default_value, bool report_error )
{
	return Get<int>(name, obj, default_value, report_error);
}

float CSrcPython::GetFloat(const char *name, bp::object obj, float default_value, bool report_error )
{
	return Get<float>(name, obj, default_value, report_error);
}

const char *CSrcPython::GetString( const char *name, bp::object obj, const char *default_value, bool report_error )
{
	return Get<const char *>(name, obj, default_value, report_error);
}

Vector CSrcPython::GetVector( const char *name, bp::object obj, Vector default_value, bool report_error )
{
	return Get<Vector>(name, obj, default_value, report_error);
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void CSrcPython::AddToDelayedUpdateList( EHANDLE hEnt, char *name, bp::object data, bool callchanged )
{
	CSrcPython::py_delayed_data_update v;
	v.hEnt = hEnt;
	Q_snprintf(v.name, _MAX_PATH, name);
	v.data = data;
	v.callchanged = callchanged;
	py_delayed_data_update_list.AddToTail( v );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::CleanupDelayedUpdateList()
{
	for( int i=py_delayed_data_update_list.Count()-1; i >= 0; i-- )
	{
		EHANDLE h = py_delayed_data_update_list[i].hEnt;
		if( h != NULL )
		{	
			if( g_debug_pynetworkvar.GetBool() )
			{
				DevMsg("#%d Cleaning up delayed PyNetworkVar update %s (callback: %d)\n", 
					h.GetEntryIndex(),
					py_delayed_data_update_list[i].name,
					py_delayed_data_update_list[i].callchanged );
			}
			
			h->PyUpdateNetworkVar( py_delayed_data_update_list[i].name, 
				py_delayed_data_update_list[i].data, py_delayed_data_update_list[i].callchanged );

			py_delayed_data_update_list.Remove(i);
		}
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::RegisterTickMethod( bp::object method, float ticksignal, bool looped )
{
	int i;
	for(i=0; i<m_methodTickList.Count(); i++)
	{
		if( m_methodTickList[i].method == method )
		{
			PyErr_SetString(PyExc_Exception, "Method already registered" );
			throw boost::python::error_already_set(); 
		}
	}
	py_tick_methods tickmethod;
	tickmethod.method = method;
	tickmethod.m_fTickSignal = ticksignal;
	tickmethod.m_fNextTickTime = gpGlobals->curtime + ticksignal;
	tickmethod.m_bLooped = looped;
	m_methodTickList.AddToTail(tickmethod);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::UnregisterTickMethod( bp::object method )
{
	for( int i = 0; i < m_methodTickList.Count(); i++ )
	{
		if( m_methodTickList[i].method == method )
		{
			m_methodTickList.Remove(i);
			return;
		}
	}
	PyErr_SetString(PyExc_Exception, "Method not found" );
	throw boost::python::error_already_set(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
boost::python::list CSrcPython::GetRegisteredTickMethods()
{
	boost::python::list methodlist;
	for( int i = 0; i < m_methodTickList.Count(); i++ )
		methodlist.append(m_methodTickList[i].method);
	return methodlist;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::IsTickMethodRegistered( boost::python::object method )
{
	for( int i = 0; i < m_methodTickList.Count(); i++ )
	{
		if( m_methodTickList[i].method == method )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::RegisterPerFrameMethod( bp::object method )
{
	for( int i = 0; i < m_methodPerFrameList.Count(); i++ )
	{
		if( m_methodPerFrameList[i] == method )
		{
			PyErr_SetString(PyExc_Exception, "Method already registered" );
			throw boost::python::error_already_set(); 
		}
	}
	m_methodPerFrameList.AddToTail(method);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::UnregisterPerFrameMethod( bp::object method )
{
	for( int i = 0; i < m_methodPerFrameList.Count(); i++ )
	{
		if( m_methodPerFrameList[i] == method )
		{
			m_methodPerFrameList.Remove(i);
			return;
		}
	}
	PyErr_SetString(PyExc_Exception, "Method not found" );
	throw boost::python::error_already_set(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
boost::python::list CSrcPython::GetRegisteredPerFrameMethods()
{
	boost::python::list methodlist;
	for( int i = 0; i < m_methodPerFrameList.Count(); i++ )
		methodlist.append( m_methodPerFrameList[i] );
	return methodlist;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::IsPerFrameMethodRegistered( boost::python::object method )
{
	for( int i = 0; i < m_methodPerFrameList.Count(); i++ )
	{
		if( m_methodPerFrameList[i] == method )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Commands follow here
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
CON_COMMAND( py_runfile, "Run a python script")
#else
CON_COMMAND_F( cl_py_runfile, "Run a python script", FCVAR_CHEAT)
#endif // CLIENT_DLL
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
#ifndef CLIENT_DLL
	if( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL
	g_SrcPythonSystem.ExecuteFile( args.ArgS() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
CON_COMMAND( py_run, "Run a string on the python interpreter")
#else
CON_COMMAND_F( cl_py_run, "Run a string on the python interpreter", FCVAR_CHEAT)
#endif // CLIENT_DLL
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
#ifndef CLIENT_DLL
	if( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL
	g_SrcPythonSystem.Run( args.ArgS(), "consolespace" );
}

#ifndef CLIENT_DLL
CON_COMMAND( spy, "Run a string on the python interpreter")
#else
CON_COMMAND_F( cpy, "Run a string on the python interpreter", FCVAR_CHEAT)
#endif // CLIENT_DLL
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
#ifndef CLIENT_DLL
	if( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL
	g_SrcPythonSystem.Run( args.ArgS(), "consolespace" );
}

#ifndef CLIENT_DLL
CON_COMMAND( py_restart, "")
#else
CON_COMMAND_F( cl_py_restart, "", FCVAR_CHEAT)
#endif // CLIENT_DLL
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
#ifndef CLIENT_DLL
	if( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL
	g_SrcPythonSystem.ShutdownInterpreter();
	g_SrcPythonSystem.InitInterpreter();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static int PyModuleAutocomplete( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	int numMatches = 0;

	char newmodulepath[MAX_PATH];

	// Get command
	CUtlVector<char*, CUtlMemory<char*, int> > commandandrest;
	Q_SplitString( partial, " ", commandandrest );
	if( commandandrest.Count() == 0 )
		return numMatches;
	char *pCommand = commandandrest[0];

	char *pRest = "";
	if( commandandrest.Count() > 1 )
		pRest = commandandrest[1];
	bool bEndsWithDot = pRest[Q_strlen(pRest)-1] == '.';

	// Get modules typed so far
	CUtlVector<char*, CUtlMemory<char*, int> > modulesnames;
	Q_SplitString( pRest, ".", modulesnames );

	// Construct path
	char path[MAX_PATH];
	path[0] = '\0';
	Q_strcat( path, "python", MAX_PATH );
	char basemodulepath[MAX_PATH];
	basemodulepath[0] = '\0';

	// Add modules to path + remember base module path, stripping the last module
	for( int i = 0; i < modulesnames.Count(); i++ )
	{
		if( modulesnames.Count() - 1 == i && !bEndsWithDot )
			continue;

		Q_strcat( path, "\\", MAX_PATH );
		Q_strcat( path, modulesnames[i], MAX_PATH );

		Q_strcat( basemodulepath, modulesnames[i], MAX_PATH );
		Q_strcat( basemodulepath, ".", MAX_PATH );
	}

	char finalpath[MAX_PATH];
	V_FixupPathName( finalpath, MAX_PATH, path );
	
	// Create whildcar
	char wildcard[MAX_PATH];
	Q_snprintf( wildcard, MAX_PATH, "%s\\*", finalpath );

	// List directories/filenames
	FileFindHandle_t findHandle;
	const char *filename = filesystem->FindFirstEx( wildcard, "MOD", &findHandle );
	while ( filename )
	{
		char fullpath[MAX_PATH];
		Q_snprintf( fullpath, MAX_PATH, "%s/%s", finalpath, filename );

		if( Q_strncmp( filename, ".", 1 ) == 0 || Q_strncmp( filename, "..", 2 ) == 0 )
		{
			filename = filesystem->FindNext( findHandle );
			continue;
		}

		if( filesystem->IsDirectory( fullpath, "MOD" ) )
		{
			
			// Add directory if __init__.py inside
			char initpath[MAX_PATH];
			Q_snprintf( initpath, MAX_PATH, "%s\\__init__.py", fullpath );
			if( filesystem->FileExists( initpath, "MOD" ) )
			{
				
				
				Q_snprintf( newmodulepath, MAX_PATH, "%s%s", basemodulepath, filename );
				if( Q_strncmp( pRest, newmodulepath, Q_strlen(pRest) ) == 0 )
					Q_snprintf( commands[ numMatches++ ], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", pCommand, newmodulepath );
			}
		}
		else
		{
			// Check extension. If .py, strip and add
			char ext[5];
			Q_ExtractFileExtension( filename, ext, 5 );

			if( Q_strncmp( ext, "py", 5 ) == 0 )
			{
				char noextfilename[MAX_PATH]; 
				Q_StripExtension( filename, noextfilename, MAX_PATH );

				Q_snprintf( newmodulepath, MAX_PATH, "%s%s", basemodulepath, noextfilename );
				if( Q_strncmp( pRest, newmodulepath, Q_strlen(pRest) ) == 0 )
					Q_snprintf( commands[ numMatches++ ], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", pCommand, newmodulepath );
			}
		}

		if ( numMatches == COMMAND_COMPLETION_MAXITEMS )
			break;

		filename = filesystem->FindNext( findHandle );
	}
	filesystem->FindClose( findHandle );

	return numMatches;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
CON_COMMAND_F_COMPLETION( py_import, "Import a python module", 0, PyModuleAutocomplete )
#else
CON_COMMAND_F_COMPLETION( cl_py_import, "Import a python module", FCVAR_CHEAT, PyModuleAutocomplete )
#endif // CLIENT_DLL
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
#ifndef CLIENT_DLL
	if( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL
	char command[MAX_PATH];
	Q_snprintf( command, sizeof( command ), "import %s", args.ArgS() );
	g_SrcPythonSystem.Run( command, "consolespace" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
CON_COMMAND_F_COMPLETION( py_reload, "Reload a python module", 0, PyModuleAutocomplete )
#else
CON_COMMAND_F_COMPLETION( cl_py_reload, "Reload a python module", FCVAR_CHEAT, PyModuleAutocomplete )
#endif // CLIENT_DLL
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
#ifndef CLIENT_DLL
	if( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL
	g_SrcPythonSystem.Reload( args.ArgS() );
}

#ifdef CLIENT_DLL
#include "vgui_controls/Panel.h"


CON_COMMAND_F( test_shared_converters, "Test server converters", FCVAR_CHEAT)
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
	Msg("Testing keyvalues converter\n");
	KeyValues *pFromPython;
	KeyValues *pToPython = new KeyValues("DataC", "CName1", "CValue1");

	pFromPython = SrcPySystem()->RunT<KeyValues *, KeyValues &>
		( SrcPySystem()->Get("test_keyvalues", "test_converters", true), NULL, *pToPython );

	if( pFromPython )
		Msg("Got keyvalues from python. Name: %s, Value1: %s\n", pFromPython->GetName(), pFromPython->GetString("Name1", ""));
	else
		Msg("No data from python :(\n");

	Msg("Testing string_t converter\n");
	string_t str_t_toPython = MAKE_STRING("Hello there");
	const char *str_from_python;
	str_from_python = SrcPySystem()->RunT<const char *, string_t>
		( SrcPySystem()->Get("test_string_t", "test_converters", true), NULL, str_t_toPython );
	Msg("Return value: %s\n", str_from_python);
}

CON_COMMAND_F( test_client_converters, "Test client converters", FCVAR_CHEAT)
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
	// Panel
	vgui::Panel *pFromPython;
	vgui::Panel *pToPython = new vgui::Panel(NULL, "PanelBla");

	pFromPython = SrcPySystem()->RunT<vgui::Panel *, vgui::Panel *>
		( SrcPySystem()->Get("test_panel", "test_converters", true), NULL, pToPython );

	if( pFromPython )
		Msg("Got Panel from python\n", pFromPython->GetName() );
	else
		Msg("No data from python :(\n");
}

#endif // CLIENT_DLL
