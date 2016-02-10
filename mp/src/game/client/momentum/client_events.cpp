#ifdef _WIN32
#include <windows.h>
#endif

#include "filesystem.h"
#include "movevars_shared.h"
#include "client_events.h"

#include "tier0/memdbgon.h"

extern IFileSystem *filesystem;

namespace Momentum {

	void OnClientDLLInit()
	{
		// enable console by default
		ConVarRef con_enable("con_enable");
		con_enable.SetValue(true);
		// mount CSS content even if it's on a different drive than SDK
#ifdef _WIN32
		HKEY hKey;
		if (VCRHook_RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 240",
			0,
			KEY_READ,
			&hKey) == ERROR_SUCCESS)
		{
			char installPath[MAX_PATH];
			DWORD len = sizeof(installPath);
			if (VCRHook_RegQueryValueEx(hKey,
				"InstallLocation",
				NULL,
				NULL,
				(LPBYTE)installPath,
				&len) == ERROR_SUCCESS)
			{
				char path[MAX_PATH];
				Q_strncpy(path, installPath, sizeof(path));

				Q_strncat(path, "\\cstrike", sizeof(path));
				filesystem->AddSearchPath(path, "GAME");

				Q_strncat(path, "\\download", sizeof(path));
				filesystem->AddSearchPath(path, "GAME");

				Q_strncpy(path, installPath, sizeof(path));
				Q_strncat(path, "\\cstrike\\cstrike_pak.vpk", sizeof(path));
				filesystem->AddSearchPath(path, "GAME");

				filesystem->PrintSearchPaths();
			}

			VCRHook_RegCloseKey(hKey);
		}
#endif
	}

} // namespace Momentum
