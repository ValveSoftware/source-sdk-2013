//========= -------------------------------------------------------------- ============//
//
// Purpose: 
//
//=====================================================================================//
#include "appframework/AppFramework.h"
#include "tier1/strtools.h"
#include "tier0/icommandline.h"
#include "tools_minidump.h"
#include "filesystem_init.h"
#include "filesystem_tools.h"
#include "filesystem.h"
#include "loadcmdline.h"
#include "scriplib.h"
#include "utlbuffer.h"
#include "cmdlib.h"
#include "sceneimage.h"
#include "choreoevent.h"

#include "sceneimagebuilder.h"


//-----------------------------------------------------------------------------
// Purpose: Global vars 
//-----------------------------------------------------------------------------
bool g_bBuilVcd  = false;
bool g_bPause    = true;
bool g_bQuiet    = false;
bool g_bLog      = false;


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void HitKeyToContinue()
{
    if (g_bPause)
    {
        system("pause");
    }
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void PrintHeader()
{
    Msg("Valve Software - sceneimagebuilder.exe (Build: %s %s)\n", __DATE__, __TIME__);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void PrintUsage()
{
    Msg("\n"
        "Usage: sceneimagebuilder.exe [options] -game <path>\n"
        "Example: D:\\dota 2\\bin\\win64\\sceneimagebuilder.exe -l -nopause -game \"D:\\dota 2\"\n"
        "   -game <path>:    Path of the game folder to \'gameinfo.txt\'. (e.g: C:\\Half Life 2\\hl2)\n"
        "   -? or -help:     Prints help.\n"
        "   -v or -verbose:  Turn on verbose output.\n"
        "   -l:              log to file log.txt\n"
        "   -nopause:        Dont pause at end of processing.\n"
        "   -quiet:          Prints only escensial messagues.\n"
        "\n"
    );
    HitKeyToContinue();
    exit(-1);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void ParseCommandline()
{
    if (CommandLine()->ParmCount() < 2)
    {
        PrintUsage();
    }

    for (int i = 1; i < CommandLine()->ParmCount(); ++i)
    {
        const char* pParm = CommandLine()->GetParm(i);

        if (pParm[0] == '-')
        {
            switch (pParm[1])
            {
            case 'h' || '?': 
                PrintUsage();
                break;

            case 'v': 
                if (!Q_stricmp(pParm, "-v") || !Q_stricmp(pParm, "-verbose"))
                {
                    qprintf("Verbose mode enabled\n");
                    verbose = true;
                    g_bQuiet = false;
                }
                break;

            case 'n': 
                if (!Q_stricmp(pParm, "-nopause"))
                {
                    qprintf("No pause enabled.\n");
                    g_bPause = false;
                }
                break;
            case 'l':
                if (!Q_stricmp(pParm, "-l"))
                {
                    qprintf("log mode enabled.\n");
                    g_bLog = true;
                }
            case 'q':
                if (!Q_stricmp(pParm, "-quiet"))
                {
                    qprintf("Quiet mode enabled.\n");
                    verbose = false;
                    g_bQuiet = true;
                }
                break;

            case 'g':
                if (!Q_stricmp(pParm, "-game"))
                {
                    ++i;
                    if (i < CommandLine()->ParmCount())
                    {
                        const char* gamePath = CommandLine()->GetParm(i);
                        if (gamePath[0] == '-')
                        {
                            Error("Error: -game requires a valid path argument.\n");
                        }
                        else
                        {
                            V_strcpy(gamedir, gamePath);
                        }
                    }
                    else
                    {
                        Error("Error: -game requires a valid path argument.\n");
                    }
                }
                break;

            default:
                Warning("Warning: Unknown option '%s'\n", pParm);
                PrintUsage();
            }
        }
        else
        {
            Warning("Warning: Unknown non-option argument '%s'\n", pParm);
            PrintUsage();
        }
    }
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CSceneImageBuilderApp::SetupSearchPaths()
{
    if (!BaseClass::SetupSearchPaths(NULL, false, true))
        return false;

    // Set gamedir.
    V_AppendSlash(gamedir, sizeof(gamedir));

    return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CSceneImageBuilderApp::Create()
{
    AppSystemInfo_t appSystems[] =
    {
        { "", "" }	// Required to terminate the list
    };

    return AddSystems(appSystems);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSceneImageBuilderApp::Destroy()
{
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CSceneImageBuilderApp::PreInit()
{
    char szTempGameDir[MAX_PATH];
    V_strcpy(szTempGameDir, gamedir);
    V_StripTrailingSlash(szTempGameDir);

    if (!CTier3SteamApp::PreInit())
    {
        return false;
    }

    g_pFileSystem = g_pFullFileSystem;
    if (!g_pFileSystem)
    {
        Error("Unable to load required library interface! (FileSystem)\n");
        return false;
    }

    g_pFullFileSystem->SetWarningFunc(Warning);
    if (!SetupSearchPaths())
    {
        return false;
    }
    
    if(g_bLog)
    {
        char szLogFile[MAX_PATH];
        V_snprintf(szLogFile, sizeof(szLogFile), "%s\\%s\\%s.log", szTempGameDir, SCENESRC_DIR, "sceneimagebuilder");
        remove(szLogFile);
        SetSpewFunctionLogFile(szLogFile);
    }

    return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSceneImageBuilderApp::PostShutdown()
{
    g_pFileSystem = NULL;
    BaseClass::PostShutdown();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CSceneImageBuilderApp::CreateSceneImageFile(CUtlBuffer& targetBuffer, char const* pchModPath, bool bLittleEndian, bool bQuiet, ISceneCompileStatus* Status)
{
    bool bSuccess = sceneImage.CreateSceneImageFile(targetBuffer, pchModPath, bLittleEndian, bQuiet, Status);
    return bSuccess;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSceneImageBuilderApp::SceneBuild()
{
    CUtlBuffer	targetBuffer;
    char szSceneCompiledPath[MAX_PATH], szSceneFile[128], szTempGameDir[1024]; /*gamedir without the blackslash*/
    
    V_snprintf(szSceneFile, sizeof(szSceneFile), "%s\\%s", SCENESRC_DIR, SCENEIMAGE_FILE);
    V_strcpy(szTempGameDir, gamedir);
    V_StripTrailingSlash(szTempGameDir);

    V_snprintf(szSceneCompiledPath, sizeof(szSceneCompiledPath), "%s\\%s", szTempGameDir, szSceneFile);

    qprintf("Game path: %s\n", szTempGameDir);
    qprintf("Scene source (.vcd): %s\\%s\\*.vcd\n", szTempGameDir, SCENESRC_DIR);
    qprintf("Scene compiled (.image): %s\n", szSceneCompiledPath);

    if (g_pSceneImage->CreateSceneImageFile(targetBuffer, szTempGameDir, true, g_bQuiet, NULL))
    {
        Msg("Writting compiled Scene file: %s... ", szSceneCompiledPath);

        if (scriptlib->WriteBufferToFile(szSceneCompiledPath, targetBuffer, WRITE_TO_DISK_ALWAYS))
        {
            Msg("done\n");
        }
        else
        {
            Error("scriptlib->WriteBufferToFile Failed!!\n");
        }
    }
    else
    {
        Error("\nScene Compile failed!\n");
    }
}


//-----------------------------------------------------------------------------
// Purpose: Main funtion
//-----------------------------------------------------------------------------
int CSceneImageBuilderApp::Main()
{
    float start = Plat_FloatTime();

    InstallSpewFunction();
    SetupDefaultToolsMinidumpHandler();
    PrintHeader();
    ParseCommandline();
    Create();
    PreInit();

    SceneBuild();

    Destroy();
    PostShutdown();

    Msg("--> Done in %.2f seconds.\n", Plat_FloatTime() - start);
    HitKeyToContinue();

    return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Main entry point 
//-----------------------------------------------------------------------------
DEFINE_CONSOLE_STEAM_APPLICATION_OBJECT(CSceneImageBuilderApp)
