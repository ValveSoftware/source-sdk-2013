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
// Purpose:
//-----------------------------------------------------------------------------
void CSceneImageBuilderApp::PrintHeader()
{
    Msg("Valve Software - sceneimagebuilder.exe (Build: %s %s)\n", __DATE__, __TIME__);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSceneImageBuilderApp::PrintUsage()
{
    Msg("\n"
        "Usage: sceneimagebuilder.exe [options] -game <path>\n"
        "Example: D:\\dota 2\\bin\\win64\\sceneimagebuilder.exe -l -pause -game \"D:\\dota 2\"\n"
        "   -game <path>:    Path of the game folder to \'gameinfo.txt\'. (e.g: C:\\Half Life 2\\hl2)\n"
        "   -? or -help:     Prints help.\n"
        "   -v or -verbose:  Turn on verbose output.\n"
        "   -l:              log to file log.txt\n"
        "   -pause:          Pause at end of processing.\n"
        "   -quiet:          Prints only essential messages.\n"
        "\n"
    );
    HitKeyToContinue();
    exit(-1);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSceneImageBuilderApp::HitKeyToContinue()
{
    if (m_bPause)
        system("pause");
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSceneImageBuilderApp::ParseCommandline()
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
            case 'h': 
                PrintUsage();
                break;

            case '?': 
                PrintUsage();
                break;

            case 'v': 
                if (!Q_stricmp(pParm, "-v") || !Q_stricmp(pParm, "-verbose"))
                {
                    qprintf("Verbose mode enabled\n");
                    verbose = true;
                    m_bQuiet = false;
                }
                break;

            case 'p': 
                if (!Q_stricmp(pParm, "-pause"))
                {
                    qprintf("No pause enabled.\n");
                    m_bPause = true;
                }
                break;
            case 'l':
                if (!Q_stricmp(pParm, "-l"))
                {
                    qprintf("log mode enabled.\n");
                    m_bLog = true;
                }
            case 'q':
                if (!Q_stricmp(pParm, "-quiet"))
                {
                    qprintf("Quiet mode enabled.\n");
                    verbose = false;
                    m_bQuiet = true;
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
                            V_strcpy_safe(gamedir, gamePath);
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
    V_strcpy_safe(szTempGameDir, gamedir);
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
    
    if(m_bLog)
    {
        char szLogFile[MAX_PATH];
        V_sprintf_safe(szLogFile, "%s\\%s\\%s.log", szTempGameDir, SCENESRC_DIR, "sceneimagebuilder");
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
    bool bSuccess = m_SceneImage.CreateSceneImageFile(targetBuffer, pchModPath, bLittleEndian, bQuiet, Status);
    return bSuccess;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSceneImageBuilderApp::SceneBuild()
{
    CUtlBuffer	targetBuffer;
    char szSceneCompiledPath[MAX_PATH], szSceneFile[128], szTempGameDir[1024]; /*gamedir without the blackslash*/
    
    V_sprintf_safe(szSceneFile, "%s\\%s", SCENESRC_DIR, SCENEIMAGE_FILE);
    V_strcpy_safe(szTempGameDir, gamedir);
    V_StripTrailingSlash(szTempGameDir);

    V_sprintf_safe(szSceneCompiledPath, "%s\\%s", szTempGameDir, szSceneFile);

    qprintf("Game path: %s\n", szTempGameDir);
    qprintf("Scene source (.vcd): %s\\%s\\*.vcd\n", szTempGameDir, SCENESRC_DIR);
    qprintf("Scene compiled (.image): %s\n", szSceneCompiledPath);

    if (g_pSceneImage->CreateSceneImageFile(targetBuffer, szTempGameDir, true, m_bQuiet, NULL))
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

    return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Main entry point 
//-----------------------------------------------------------------------------
DEFINE_CONSOLE_STEAM_APPLICATION_OBJECT(CSceneImageBuilderApp)

