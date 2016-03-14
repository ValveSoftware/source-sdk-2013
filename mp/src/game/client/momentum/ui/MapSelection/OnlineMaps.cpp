#include "pch_mapselection.h"

using namespace vgui;

// How often to re-sort the server list
const float MINIMUM_SORT_TIME = 1.5f;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//			NOTE:	m_Servers can not use more than 96 sockets, else it will
//					cause internet explorer to Stop working under win98 SE!
//-----------------------------------------------------------------------------
COnlineMaps::COnlineMaps(vgui::Panel *parent, const char *panelName) : CBaseMapsPage(parent, panelName)
{
    m_fLastSort = 0.0f;
    m_bDirty = false;
    m_bRequireUpdate = true;
    m_bOfflineMode = !IsSteamGameServerBrowsingEnabled();

    m_bAnyServersRetrievedFromMaster = false;
    m_bNoServersListedOnMaster = false;
    m_bAnyServersRespondedToQuery = false;

    //m_pLocationFilter->DeleteAllItems();
    KeyValues *kv = new KeyValues("Regions");
    if (kv->LoadFromFile(g_pFullFileSystem, "servers/Regions.vdf", NULL))
    {
        // iterate the list loading all the servers
        for (KeyValues *srv = kv->GetFirstSubKey(); srv != NULL; srv = srv->GetNextKey())
        {
            struct regions_s region;

            region.name = srv->GetString("text");
            region.code = srv->GetInt("code");
            KeyValues *regionKV = new KeyValues("region", "code", region.code);
            //m_pLocationFilter->AddItem(region.name.String(), regionKV);
            regionKV->deleteThis();
            m_Regions.AddToTail(region);
        }
    }
    else
    {
        Assert(!("Could not load file servers/Regions.vdf; server browser will not function."));
    }
    kv->deleteThis();

    LoadFilterSettings();
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
COnlineMaps::~COnlineMaps()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COnlineMaps::PerformLayout()
{
    if (!m_bOfflineMode && m_bRequireUpdate && MapSelectorDialog().IsVisible())
    {
        PostMessage(this, new KeyValues("GetNewServerList"), 0.1f);
        m_bRequireUpdate = false;
    }

    if (m_bOfflineMode)
    {
        m_pGameList->SetEmptyListText("#ServerBrowser_OfflineMode");
        m_pStartMap->SetEnabled(false);
        m_pRefreshAll->SetEnabled(false);
        m_pRefreshQuick->SetEnabled(false);
        m_pFilter->SetEnabled(false);
    }

    BaseClass::PerformLayout();
    //m_pLocationFilter->SetEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose: Activates the page, starts refresh if needed
//-----------------------------------------------------------------------------
void COnlineMaps::OnPageShow()
{
}


//-----------------------------------------------------------------------------
// Purpose: Called every frame, maintains sockets and runs refreshes
//-----------------------------------------------------------------------------
void COnlineMaps::OnTick()
{
    if (m_bOfflineMode)
    {
        BaseClass::OnTick();
        return;
    }

    BaseClass::OnTick();

    CheckRedoSort();
}


//-----------------------------------------------------------------------------
// Purpose: Handles incoming server refresh data
//			updates the server browser with the refreshed information from the server itself
//-----------------------------------------------------------------------------
void COnlineMaps::ServerResponded(int iServer)
{
    m_bDirty = true;
    //BaseClass::ServerResponded(iServer);
    m_bAnyServersRespondedToQuery = true;
    m_bAnyServersRetrievedFromMaster = true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COnlineMaps::ServerFailedToRespond(int iServer)
{
    /*
#ifndef NO_STEAM
    m_bDirty = true;
    gameserveritem_t *pServer = SteamMatchmakingServers()->GetServerDetails(m_eMatchMakingType, iServer);
    Assert(pServer);

    if (pServer->m_bHadSuccessfulResponse)
    {
        // if it's had a successful response in the past, leave it on
        ServerResponded(iServer);
    }
    else
    {
        int iServerMap = m_mapServers.Find(iServer);
        if (iServerMap != m_mapServers.InvalidIndex())
            RemoveServer(m_mapServers[iServerMap]);
        // we've never had a good response from this server, remove it from the list
        m_iServerRefreshCount++;
    }
#endif*/
}


//-----------------------------------------------------------------------------
// Purpose: Called when server refresh has been completed
//-----------------------------------------------------------------------------
void COnlineMaps::RefreshComplete(EMatchMakingServerResponse response)
{
    SetRefreshing(false);
    UpdateFilterSettings();

    if (response != eServerFailedToRespond)
    {
        if (m_bAnyServersRespondedToQuery)
        {
            m_pGameList->SetEmptyListText(GetStringNoUnfilteredServers());
        }
        else if (response == eNoServersListedOnMasterServer)
        {
            m_pGameList->SetEmptyListText(GetStringNoUnfilteredServersOnMaster());
        }
        else
        {
            m_pGameList->SetEmptyListText(GetStringNoServersResponded());
        }
    }
    else
    {
        m_pGameList->SetEmptyListText("#ServerBrowser_MasterServerNotResponsive");
    }

    // perform last sort
    m_bDirty = false;
    m_fLastSort = Plat_FloatTime();
    if (IsVisible())
    {
        m_pGameList->SortList();
    }

    UpdateStatus();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COnlineMaps::GetNewMapList()
{
    BaseClass::GetNewMapList();
    UpdateStatus();

    m_bRequireUpdate = false;
    m_bAnyServersRetrievedFromMaster = false;
    m_bAnyServersRespondedToQuery = false;

    m_pGameList->DeleteAllItems();
}


//-----------------------------------------------------------------------------
// Purpose: returns true if the game list supports the specified ui elements
//-----------------------------------------------------------------------------
bool COnlineMaps::SupportsItem(IMapList::InterfaceItem_e item)
{
    switch (item)
    {
    case FILTERS:
    case GETNEWLIST:
        return true;

    default:
        return false;
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COnlineMaps::CheckRedoSort(void)
{
    float fCurTime;

    // No changes detected
    if (!m_bDirty)
        return;

    fCurTime = Plat_FloatTime();
    // Not time yet
    if (fCurTime - m_fLastSort < MINIMUM_SORT_TIME)
        return;

    // postpone sort if mouse button is down
    if (input()->IsMouseDown(MOUSE_LEFT) || input()->IsMouseDown(MOUSE_RIGHT))
    {
        // don't sort for at least another second
        m_fLastSort = fCurTime - MINIMUM_SORT_TIME + 1.0f;
        return;
    }

    // Reset timer
    m_bDirty = false;
    m_fLastSort = fCurTime;

    // Force sort to occur now!
    m_pGameList->SortList();
}


//-----------------------------------------------------------------------------
// Purpose: opens context menu (user right clicked on a server)
//-----------------------------------------------------------------------------
void COnlineMaps::OnOpenContextMenu(int itemID)
{
    if (!m_pGameList->GetSelectedItemsCount())
        return;

    // get the server
    //int serverID = m_pGameList->GetItemData(m_pGameList->GetSelectedItem(0))->userData;

    // Activate context menu
    CMapContextMenu *menu = MapSelectorDialog().GetContextMenu(m_pGameList);
    menu->ShowMenu(this, true, true);
}

//-----------------------------------------------------------------------------
// Purpose: refreshes a single server
//-----------------------------------------------------------------------------
void COnlineMaps::OnRefreshServer(int serverID)
{
    BaseClass::OnRefreshServer(serverID);

    MapSelectorDialog().UpdateStatusText("#ServerBrowser_GettingNewServerList");
}


//-----------------------------------------------------------------------------
// Purpose: get the region code selected in the ui
// Output: returns the region code the user wants to filter by
//-----------------------------------------------------------------------------
int COnlineMaps::GetRegionCodeToFilter()
{
    //KeyValues *kv = m_pLocationFilter->GetActiveItemUserData();
    //if (kv)
     //   return kv->GetInt("code");
    //else
        return 255;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool COnlineMaps::CheckTagFilter(gameserveritem_t &server)
{
    // Servers without tags go in the official games, servers with tags go in custom games
    bool bOfficialServer = !(server.m_szGameTags && server.m_szGameTags[0]);
    if (!bOfficialServer)
        return false;

    return true;
}