#include <winlite.h>
#undef CreateDialog
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include "vstdlib/pch_vstdlib.h"
#include "vgui_controls/pch_vgui_controls.h"
#include "vgui_controls/Frame.h"

#include "tier3/tier3.h"

// steam3 API
//#include "steam/isteammasterserverupdater.h"
//#include "steam/steam_querypackets.h"
#include "steam/steam_api.h"
#include "steam/isteamuser.h"
#include "steam/isteammatchmaking.h"
#include "steam/isteamfriends.h"

#include "momentum/ui/MapSelection/IMapSelector.h"
//#include "ServerBrowser/IServerBrowser.h"
//#include "IVGuiModule.h"
//#include "vgui_controls/Controls.h"

//#include "tier1/netadr.h"
//#include "FileSystem.h"
//#include "iappinformation.h"
//#include "proto_oob.h"
//#include "modlist.h"
//#include "IRunGameEngine.h"
#include "momentum/mom_shareddefs.h"
#include "momentum/mom_gamerules.h"
#include "momentum/util/mom_util.h"
#include "OfflineMode.h"

//VGUI
#include <vgui_controls/pch_vgui_controls.h>

//MapSelection headers
#include "IMapList.h"
#include "MapSelector.h"
#include "MapContextMenu.h"
#include "MapInfoDialog.h"
#include "BaseMapsPage.h"
#include "LocalMaps.h"
#include "OnlineMaps.h"
#include "MapSelectorDialog.h"
#include "cbase.h"