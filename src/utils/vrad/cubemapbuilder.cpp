//========= ------------------------------------------------------ ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "vrad.h"
#include "filesystem_init.h"
#include "KeyValues.h"
#include "utlbuffer.h"
#include "cubemapbuilder.h"
#include "steam/steam_api.h"


#define MAX_CMD_BUFFER_SIZE 8192 // 8KB

//-----------------------------------------------------------------------------
// Purpose: Defines CubemapBuilder KeyValues. (NOTE: sync this with VBSP!)
//-----------------------------------------------------------------------------
#define CUBEMAPBUILDER_KV				"CubemapBuilder"
#define CUBEMAPBUILDER_KV_MAX_GRAPHICS	"SetGameToMaximumGraphic"
#define CUBEMAPBUILDER_KV_32BITS_EXE	"GameExecutableName32bits"
#define CUBEMAPBUILDER_KV_64BITS_EXE	"GameExecutableName64bits"
#define	CUBEMAPBUILDER_KV_HDR			"Hdr"
#define	CUBEMAPBUILDER_KV_LDR			"Ldr"
#define CUBEMAPBUILDER_KV_PARAMSTRING	"BuildParams"
#define CUBEMAPBUILDER_MAX_GRAPH_CVAR	"+r_lightmap_bicubic 1 +r_waterforceexpensive 1 +mat_antialias 8 +mat_picmip -10 +mat_forceaniso 16"


//-----------------------------------------------------------------------------
// Purpose: Copies the BSP file to the game directory
//-----------------------------------------------------------------------------
static bool CopyBspToGameDir(const char* pszSrcBsp, const char* pszDstBsp)
{
	CUtlBuffer utlBuffer;

	Msg("Copying bsp file: %s to %s... ", pszSrcBsp, pszDstBsp);

	if (!g_pFileSystem->ReadFile(pszSrcBsp, "MOD", utlBuffer))
	{
		Warning("\n"
				"Failed to read .bsp file: %s\n"
				"Skipping cubemap compile!\n", pszSrcBsp);
		return false;
	}

	g_pFullFileSystem->CreateDirHierarchy("maps/", "MOD");

	if (!g_pFileSystem->WriteFile(pszDstBsp, "MOD", utlBuffer))
	{
		Warning("\n"
				"Failed to write .bsp file: %s\n"
				"Skipping cubemap compile!\n", pszDstBsp);
		return false;
	}

	Msg("done\n");

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Copies the BSP file from the game directory back to the original path
//          (e.g., game/maps/foo.bsp -> path/foo.bsp).
//-----------------------------------------------------------------------------
static bool CopyGameDirBspToOrignalBspDir(const char* pszSrcBsp, const char* pszDstBsp)
{
	CUtlBuffer utlBuffer;

	Msg("Copying bsp file: %s to %s... ", pszDstBsp, pszSrcBsp);

	if (!g_pFileSystem->ReadFile(pszDstBsp, "MOD", utlBuffer))
	{
		Warning("\n"
				"Failed to read .bsp file: %s\n"
				"Skipping cubemap compile!\n", pszDstBsp);
		return false;
	}

	if (!g_pFileSystem->WriteFile(pszSrcBsp, "MOD", utlBuffer))
	{
		Warning("\n"
				"Failed to write .bsp file: %s\n"
				"Skipping cubemap compile!\n", pszSrcBsp);
		return false;
	}

	Msg("done\n");

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Loads ConVars or command-line parameters found in 'CubemapBuilder' 
//          within gameinfo.txt.
//-----------------------------------------------------------------------------
static bool LoadGameInfoConvar(const char* pGameInfoPath, char* pExeName, const std::size_t ExeNameBufferSize,  char* pConvar, const std::size_t uiConvarBufferSize, const bool bHdrMode)
{
	float start = Plat_FloatTime();

	qprintf("Loading KeyValues from gameinfo.txt... ");

	KeyValues* pKvGameInfo = new KeyValues("");
	if (!pKvGameInfo->LoadFromFile(g_pFullFileSystem, "gameinfo.txt", "MOD"))
	{
		Warning("Failed to load KeyValues from: %s\n"
				"Skipping cubemap compile, FAIL!"
				,pGameInfoPath);

		pKvGameInfo->deleteThis();
		return false;
	}

	const char* pGameExe = pKvGameInfo->GetString(IsPlatform64Bits() ? CUBEMAPBUILDER_KV_64BITS_EXE : CUBEMAPBUILDER_KV_32BITS_EXE);
	if (!pGameExe)
	{
		Warning("\n"
			"Warning: Could not locate \'%s\' key in gameinfo.txt\n"
			"Warning: Skipping cubemap compile, FAIL!\n",
			IsPlatform64Bits() ? CUBEMAPBUILDER_KV_64BITS_EXE : CUBEMAPBUILDER_KV_32BITS_EXE,
			pGameExe);
		pKvGameInfo->deleteThis();
		return false;
	}
	V_snprintf(pExeName, ExeNameBufferSize, "%s", pGameExe);

	KeyValues* pKvCubemapBuilder = pKvGameInfo->FindKey(CUBEMAPBUILDER_KV);
	if(!pKvCubemapBuilder)
	{
		Warning("\n"
				"Warning: Could not load KeyValues for %s!\n"
				"Warning: Cubemaps might not look right!\n",
				CUBEMAPBUILDER_KV);

		pKvGameInfo->deleteThis();
		return false;
	}
	
	const char* pSetGameToMaximumGraphic = pKvCubemapBuilder->GetString(CUBEMAPBUILDER_KV_MAX_GRAPHICS, "0");
	const bool bSetGameToMaximumGraphic = atoi(pSetGameToMaximumGraphic) == 1;
	KeyValues* pKvLightingMode = pKvCubemapBuilder->FindKey(bHdrMode ? CUBEMAPBUILDER_KV_HDR : CUBEMAPBUILDER_KV_LDR);
	if (!pKvLightingMode)
	{
		Warning("\n"
				"Warning: Could not load KeyValues for %s!\n"
				"Warning: Cubemap compile might not look right!\n"
				,bHdrMode ? "Hdr" : "Ldr");
		
		pKvGameInfo->deleteThis();
		return false;
	}

	const char* pBuildParam = pKvLightingMode->GetString(CUBEMAPBUILDER_KV_PARAMSTRING);
	if(!pBuildParam)
	{
		Warning("Failed to get %s KeyValue inside %s Key!\n", CUBEMAPBUILDER_KV_PARAMSTRING, bHdrMode ? CUBEMAPBUILDER_KV_HDR : CUBEMAPBUILDER_KV_LDR);
		pBuildParam = "\0";
	}
	V_snprintf(pConvar, uiConvarBufferSize, "%s %s", pBuildParam, bSetGameToMaximumGraphic ? CUBEMAPBUILDER_MAX_GRAPH_CVAR : "");


	pKvGameInfo->deleteThis();

	qprintf("done (%.2f)\n", Plat_FloatTime() - start);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Automatically builds cubemaps.
//-----------------------------------------------------------------------------
void BuildCubemaps(const bool bHdrMode)
{
	float	start = Plat_FloatTime();
	char	szExeName[64];
	char	szGameInfoPath[MAX_PATH];
	char	szGameInfoFile[MAX_PATH];
	char	szSourceBspPath[MAX_PATH];
	char	szGameBspPath[MAX_PATH];
	char	szKvFromGameInfo[MAX_PATH];
	char	szGameExecutablePath[MAX_PATH];
	char	szCmdCommandLine[MAX_CMD_BUFFER_SIZE];
	char	szBuildCubemapsCommandLine[MAX_CMD_BUFFER_SIZE];

	Msg("\n\n");
	Msg("====== Building cubemaps (%s) ======\n", bHdrMode ? "Hdr" : "Ldr");

	V_strcpy(szGameInfoPath, gamedir);
	V_StripTrailingSlash(szGameInfoPath);

	V_snprintf(szGameInfoFile, sizeof(szGameInfoFile), "%s\\gameinfo.txt", szGameInfoPath);

	V_strcpy(szSourceBspPath, source);
	V_snprintf(szGameBspPath, sizeof(szGameBspPath), "%s\\maps\\%s", szGameInfoPath, V_strrchr(source, '\\') + 1);

	if (!LoadGameInfoConvar(szGameInfoPath, szExeName, sizeof(szExeName), szKvFromGameInfo, sizeof(szKvFromGameInfo), bHdrMode))
		return;

	V_snprintf(szBuildCubemapsCommandLine, sizeof(szBuildCubemapsCommandLine),
	" -sw -w %d -h %d -dev -novid -insecure -console -buildcubemaps %d -game \"%s\" +map %s %s ",
			GetSystemMetrics(SM_CXSCREEN), 
			GetSystemMetrics(SM_CYSCREEN), 
			bHdrMode ? g_iBuildHdrCubemapPasses : g_iBuildLdrCubemapPasses,
			szGameInfoPath,
			level_name,
			szKvFromGameInfo);

	if (!SteamAPI_Init())
	{
		Error("SteamAPI_Init() failed! Possible causes:\n"
			"  - Steam is not open.\n"
			"  - Could not find steam_appid.txt\n"
#ifdef PLATFORM_64BITS
			"  - Could not find steam_api64.dll\n"
#else
			"  - Could not find steam_api.dll\n"
#endif // PLATFORM_64BITS
		);
	}

	AppId_t appID = SteamUtils()->GetAppID();
	if (appID == 0)
	{
		Warning("Failed to get AppID!");
		return;
	}

	uint32 result = SteamApps()->GetAppInstallDir(appID, szGameExecutablePath, sizeof(szGameExecutablePath));
	if (result == 0)
	{
		Error("Failed to get App Install Directory!");
	}

	V_snprintf(szCmdCommandLine, sizeof(szCmdCommandLine), "%s\\%s.exe %s", szGameExecutablePath, szExeName, szBuildCubemapsCommandLine);

	if (!CopyBspToGameDir(szSourceBspPath, szGameBspPath))
		return;

	Msg("Starting the executable (%s), Comamnd line:%s \n", szExeName, szBuildCubemapsCommandLine);
	
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	if (!CreateProcessA(NULL, szCmdCommandLine, NULL, NULL, false, 0x00000000, NULL, NULL, &si, &pi))
	{
		Warning("Warning: %s could not start!\n"
				"Warning: Skipping cubemap compile for %s, FAIL!\n"
				, szExeName, szGameExecutablePath);
		return;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	DWORD exitCode = 0;
	if (!GetExitCodeProcess(pi.hProcess, &exitCode))
	{
		if (exitCode > 0)
		{
			Warning("Warning: cubemaps compile failed: %d!\n"
					"Warning: The resulting .bsp file:  %s, may have issues!\n"
					"Warning: It is highly recomended to delete the game bsp file and re run cubemapbuilder!\n",
					exitCode, szGameBspPath);
			return;
		}
	}
	else
	{
		Warning("Warning: GetExitCodeProcess() failed!\n");
	}

	// Once the cubemap compile is complete we will copy the file again to the original bsp dir. This is done 
	// to not break older workflows (e.g: if the user wants use bspzip, vbspinfo or a postcompiler)
	CopyGameDirBspToOrignalBspDir(szSourceBspPath, szGameBspPath);

	Msg("--> Cubemap builder complete in %.2f seconds\n", Plat_FloatTime() - start);
	Msg("\n\n");
}

