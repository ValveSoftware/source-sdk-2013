#include "pch_mapselection.h"

using namespace vgui;

static CMapSelectorDialog *s_InternetDlg = NULL;

CMapSelectorDialog &MapSelectorDialog()
{
    return *CMapSelectorDialog::GetInstance();
}


// Returns a list of the ports that we hit when looking for 
void GetMostCommonQueryPorts(CUtlVector<uint16> &ports)
{
    for (int i = 0; i <= 5; i++)
    {
        ports.AddToTail(27015 + i);
        ports.AddToTail(26900 + i);
    }

    ports.AddToTail(4242); //RDKF
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMapSelectorDialog::CMapSelectorDialog(vgui::VPANEL parent) : Frame(NULL, "CMapSelectorDialog")//"CServerBrowserDialog")
{
    SetParent(parent);
    s_InternetDlg = this;
    m_pSavedData = NULL;
    m_pFilterData = NULL;

    LoadUserData();

    m_pLocal = new CLocalMaps(this);
    //MOM_TODO: uncomment this: m_pOnline = new COnlineMaps(this);

    SetMinimumSize(640, 384);
    SetSize(640, 384);

    m_pGameList = (IMapList*) m_pLocal;

    m_pContextMenu = new CMapContextMenu(this);

    // property sheet
    m_pTabPanel = new PropertySheet(this, "MapTabs");
    m_pTabPanel->SetTabWidth(72);
    m_pTabPanel->AddPage(m_pLocal, "#MOM_MapSelector_LocalMaps");
    //MOM_TODO: uncomment: m_pTabPanel->AddPage(m_pOnline, "#MOM_MapSelector_OnlineMaps");

    m_pTabPanel->AddActionSignalTarget(this);

    m_pStatusLabel = new Label(this, "StatusLabel", "");

    LoadControlSettingsAndUserConfig("resource/ui/DialogMapSelector.res");

    m_pStatusLabel->SetText("");

    // load current tab
    const char *mapList = m_pSavedData->GetString("MapList", "local");
    if (!Q_stricmp(mapList, "local"))
    {
        m_pTabPanel->SetActivePage(m_pLocal);
    }
    else if (!Q_stricmp(mapList, "online"))
    {
        m_pTabPanel->SetActivePage(m_pOnline);
    }

    ivgui()->AddTickSignal(GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMapSelectorDialog::~CMapSelectorDialog()
{
    delete m_pContextMenu;

    SaveUserData();

    if (m_pSavedData)
    {
        m_pSavedData->deleteThis();
    }
}


//-----------------------------------------------------------------------------
// Purpose: Called once to set up
//-----------------------------------------------------------------------------
void CMapSelectorDialog::Initialize()
{
    SetTitle("#MOM_MapSelector_Maps", true);
    SetVisible(false);
}


//-----------------------------------------------------------------------------
// Purpose: returns a map in the list
//-----------------------------------------------------------------------------
mapstruct_t *CMapSelectorDialog::GetMap(unsigned int serverID)
{
    return m_pGameList->GetMap(serverID);
}


//-----------------------------------------------------------------------------
// Purpose: Activates and gives the tab focus
//-----------------------------------------------------------------------------
void CMapSelectorDialog::Open()
{
    BaseClass::Activate();
    m_pTabPanel->RequestFocus();
}


//-----------------------------------------------------------------------------
// Purpose: Called every frame, updates animations for this module
//-----------------------------------------------------------------------------
void CMapSelectorDialog::OnTick()
{
    BaseClass::OnTick();
    vgui::GetAnimationController()->UpdateAnimations(system()->GetFrameTime());
}


//-----------------------------------------------------------------------------
// Purpose: Loads filter settings from disk
//-----------------------------------------------------------------------------
void CMapSelectorDialog::LoadUserData()
{
    // free any old filters
    if (m_pSavedData)
    {
        m_pSavedData->deleteThis();
    }

    m_pSavedData = new KeyValues("Filters");
    if (!m_pSavedData->LoadFromFile(g_pFullFileSystem, "cfg/MapSelector.vdf", "MOD"))
    {
        // doesn't matter if the file is not found, defaults will work successfully and file will be created on exit
    }

    KeyValues *filters = m_pSavedData->FindKey("Filters", false);
    if (filters)
    {
        m_pFilterData = filters->MakeCopy();
        m_pSavedData->RemoveSubKey(filters);
    }
    else
    {
        m_pFilterData = new KeyValues("Filters");
    }

    int wide, tall;
    surface()->GetScreenSize(wide, tall);

    SetPos(wide / 2, tall / 3);

    InvalidateLayout();
    Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapSelectorDialog::SaveUserData()
{
    m_pSavedData->Clear();
    m_pSavedData->LoadFromFile(g_pFullFileSystem, "cfg/MapSelector.vdf", "MOD");

    // set the current tab
    if (m_pGameList == m_pLocal)
    {
        m_pSavedData->SetString("MapList", "local");
    }
    else if (m_pGameList == m_pOnline)
    {
        m_pSavedData->SetString("MapList", "online");//MOM_TODO
    }

    m_pSavedData->RemoveSubKey(m_pSavedData->FindKey("Filters")); // remove the saved subkey and add our subkey
    m_pSavedData->AddSubKey(m_pFilterData->MakeCopy());
    m_pSavedData->SaveToFile(g_pFullFileSystem, "cfg/MapSelector.vdf", "MOD");

    // save per-page config
    SaveUserConfig();
}

//-----------------------------------------------------------------------------
// Purpose: refreshes the page currently visible
//-----------------------------------------------------------------------------
void CMapSelectorDialog::RefreshCurrentPage()
{
    if (m_pGameList)
    {
        m_pGameList->StartRefresh();
    }
}

//-----------------------------------------------------------------------------
// Purpose: Updates status test at bottom of window
//-----------------------------------------------------------------------------
void CMapSelectorDialog::UpdateStatusText(const char *fmt, ...)
{
    if (!m_pStatusLabel)
        return;

    if (fmt && strlen(fmt) > 0)
    {
        char str[1024];
        va_list argptr;
        va_start(argptr, fmt);
        _vsnprintf(str, sizeof(str), fmt, argptr);
        va_end(argptr);

        m_pStatusLabel->SetText(str);
    }
    else
    {
        // clear
        m_pStatusLabel->SetText("");
    }
}

//-----------------------------------------------------------------------------
// Purpose: Updates status test at bottom of window
// Input  : wchar_t* (unicode string) - 
//-----------------------------------------------------------------------------
void CMapSelectorDialog::UpdateStatusText(wchar_t *unicode)
{
    if (!m_pStatusLabel)
        return;

    if (unicode && wcslen(unicode) > 0)
    {
        m_pStatusLabel->SetText(unicode);
    }
    else
    {
        // clear
        m_pStatusLabel->SetText("");
    }
}

//-----------------------------------------------------------------------------
// Purpose: Updates when the tabs are changed (online->Local and vice versa)
//-----------------------------------------------------------------------------
void CMapSelectorDialog::OnGameListChanged()
{
    m_pGameList = dynamic_cast<IMapList *>(m_pTabPanel->GetActivePage());

    UpdateStatusText("");

    InvalidateLayout();
    Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a static instance of this dialog
//-----------------------------------------------------------------------------
CMapSelectorDialog *CMapSelectorDialog::GetInstance()
{
    return s_InternetDlg;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapContextMenu *CMapSelectorDialog::GetContextMenu(vgui::Panel *pPanel)
{
    // create a drop down for this object's states
    if (m_pContextMenu)
        delete m_pContextMenu;
    m_pContextMenu = new CMapContextMenu(this);
    m_pContextMenu->SetAutoDelete(false);

    if (!pPanel)
    {
        m_pContextMenu->SetParent(this);
    }
    else
    {
        m_pContextMenu->SetParent(pPanel);
    }

    m_pContextMenu->SetVisible(false);
    return m_pContextMenu;
}

//-----------------------------------------------------------------------------
// Purpose: begins the process of joining a server from a game list
//			the game info dialog it opens will also update the game list
//-----------------------------------------------------------------------------
CDialogMapInfo *CMapSelectorDialog::JoinGame(IMapList *gameList, unsigned int serverIndex)
{
    // open the game info dialog, then mark it to attempt to connect right away
    //CDialogMapInfo *gameDialog = OpenMapInfoDialog(gameList, serverIndex);

    // set the dialog name to be the server name
    //gameDialog->Connect();

    // return gameDialog;
    return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: joins a game by a specified IP, not attached to any game list
//-----------------------------------------------------------------------------
CDialogMapInfo *CMapSelectorDialog::JoinGame(int serverIP, int serverPort)
{
    // open the game info dialog, then mark it to attempt to connect right away
    CDialogMapInfo *gameDialog = OpenMapInfoDialog(serverIP, serverPort, serverPort);

    // set the dialog name to be the server name
    gameDialog->Connect();

    return gameDialog;
}

//-----------------------------------------------------------------------------
// Purpose: opens a game info dialog from a game list
//-----------------------------------------------------------------------------
CDialogMapInfo *CMapSelectorDialog::OpenMapInfoDialog(IMapList *gameList, KeyValues *pMap)
{
    //mapstruct_t *pServer = gameList->GetMap(serverIndex);
    //if (!pServer)
    

    //MOM_TODO: complete the following so people can see information on the map 

    //We're going to send just the map name to the CDialogMapInfo() constructor,
    //then to the server and populate it with leaderboard times, replays, personal bests, etc
    const char *pMapName = pMap->GetString("name", "");
    CDialogMapInfo *gameDialog = new CDialogMapInfo(NULL, pMapName);
    gameDialog->SetParent(GetVParent());
    gameDialog->AddActionSignalTarget(this);
    gameDialog->Run(pMapName);
    int i = m_vecMapInfoDialogs.AddToTail();
    m_vecMapInfoDialogs[i] = gameDialog;
    return gameDialog;
    return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: opens a game info dialog by a specified IP, not attached to any game list
//-----------------------------------------------------------------------------
CDialogMapInfo *CMapSelectorDialog::OpenMapInfoDialog(int serverIP, uint16 connPort, uint16 queryPort)
{
    CDialogMapInfo *gameDialog = new CDialogMapInfo(NULL, "");
    gameDialog->AddActionSignalTarget(this);
    gameDialog->SetParent(GetVParent());
    gameDialog->Run("");
    int i = m_vecMapInfoDialogs.AddToTail();
    m_vecMapInfoDialogs[i] = gameDialog;
    return gameDialog;
}

//-----------------------------------------------------------------------------
// Purpose: closes all the game info dialogs
//-----------------------------------------------------------------------------
void CMapSelectorDialog::CloseAllMapInfoDialogs()
{
    for (int i = 0; i < m_vecMapInfoDialogs.Count(); i++)
    {
        vgui::Panel *dlg = m_vecMapInfoDialogs[i];
        if (dlg)
        {
            vgui::ivgui()->PostMessage(dlg->GetVPanel(), new KeyValues("Close"), NULL);
        }
    }
}


//-----------------------------------------------------------------------------
// Purpose: finds a dialog
//-----------------------------------------------------------------------------
CDialogMapInfo *CMapSelectorDialog::GetDialogGameInfoForFriend(uint64 ulSteamIDFriend)
{
    FOR_EACH_VEC(m_vecMapInfoDialogs, i)
    {
        CDialogMapInfo *pDlg = m_vecMapInfoDialogs[i];
        if (pDlg && pDlg->GetAssociatedFriend() == ulSteamIDFriend)
        {
            return pDlg;
        }
    }
    return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: accessor to the filter save data
//-----------------------------------------------------------------------------
KeyValues *CMapSelectorDialog::GetFilterSaveData(const char *filterSet)
{
    return m_pFilterData->FindKey(filterSet, true);
}

//-----------------------------------------------------------------------------
// Purpose: resets all pages filter settings
//-----------------------------------------------------------------------------
void CMapSelectorDialog::ReloadFilterSettings()
{
    m_pLocal->LoadFilterSettings();
    m_pOnline->LoadFilterSettings();
}

//-----------------------------------------------------------------------------
// Purpose: Adds server to the history, saves as currently connected server
//-----------------------------------------------------------------------------
void CMapSelectorDialog::OnConnectToGame(KeyValues *pMessageValues)
{
    //MOM_TODO: Make this OnStartMap/OnDownloadMap or similar

    int ip = pMessageValues->GetInt("ip");
    int connectionPort = pMessageValues->GetInt("connectionport");
    int queryPort = pMessageValues->GetInt("queryport");

    if (!ip || !queryPort)
        return;

    memset(&m_CurrentConnection, 0, sizeof(gameserveritem_t));
    m_CurrentConnection.m_NetAdr.SetIP(ip);
    m_CurrentConnection.m_NetAdr.SetQueryPort(queryPort);
    m_CurrentConnection.m_NetAdr.SetConnectionPort((unsigned short) connectionPort);
#ifndef NO_STEAM
    //if (m_pHistory && SteamMatchmaking())
    //{
    //    SteamMatchmaking()->AddFavoriteGame2(0, ::htonl(ip), connectionPort, queryPort, k_unFavoriteFlagHistory, time(NULL));
    //    m_pHistory->SetRefreshOnReload();
    //}
#endif
    // tell the game info dialogs, so they can cancel if we have connected
    // to a server they were auto-retrying
    for (int i = 0; i < m_vecMapInfoDialogs.Count(); i++)
    {
        vgui::Panel *dlg = m_vecMapInfoDialogs[i];
        if (dlg)
        {
            KeyValues *kv = new KeyValues("ConnectedToGame", "ip", ip, "connectionport", connectionPort);
            kv->SetInt("queryport", queryPort);
            vgui::ivgui()->PostMessage(dlg->GetVPanel(), kv, NULL);
        }
    }

    // forward to favorites
    //m_pFavorites->OnConnectToGame();

    m_bCurrentlyConnected = true;
}

//-----------------------------------------------------------------------------
// Purpose: Clears currently connected server
//-----------------------------------------------------------------------------
void CMapSelectorDialog::OnDisconnectFromGame(void)
{
    m_bCurrentlyConnected = false;
    memset(&m_CurrentConnection, 0, sizeof(gameserveritem_t));
}

//-----------------------------------------------------------------------------
// Purpose: Passes build mode activation down into the pages
//-----------------------------------------------------------------------------
void CMapSelectorDialog::ActivateBuildMode()
{
    // no subpanel, no build mode
    EditablePanel *panel = dynamic_cast<EditablePanel *>(m_pTabPanel->GetActivePage());
    if (!panel)
        return;

    panel->ActivateBuildMode();
}

//-----------------------------------------------------------------------------
// Purpose: gets the default position and size on the screen to appear the first time
//-----------------------------------------------------------------------------
bool CMapSelectorDialog::GetDefaultScreenPosition(int &x, int &y, int &wide, int &tall)
{
    int wx, wy, ww, wt;
    surface()->GetWorkspaceBounds(wx, wy, ww, wt);
    x = wx + (int) (ww * 0.05);
    y = wy + (int) (wt * 0.4);
    wide = (int) (ww * 0.5);
    tall = (int) (wt * 0.55);
    return true;
}