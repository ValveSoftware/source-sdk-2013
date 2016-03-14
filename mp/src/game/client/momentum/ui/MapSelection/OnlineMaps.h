#ifndef INTERNETGAMES_H
#define INTERNETGAMES_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: Internet games list
//-----------------------------------------------------------------------------
class COnlineMaps : public CBaseMapsPage
{

    DECLARE_CLASS_SIMPLE(COnlineMaps, CBaseMapsPage);

public:
    COnlineMaps(vgui::Panel *parent, const char *panelName = "OnlineMaps");
    ~COnlineMaps();

    // property page handlers
    virtual void OnPageShow();

    // returns true if the game list supports the specified ui elements
    virtual bool SupportsItem(IMapList::InterfaceItem_e item);

    // gets a new server list
    MESSAGE_FUNC(GetNewMapList, "GetNewMapList");

    // serverlist refresh responses
    virtual void ServerResponded(int iServer);
    virtual void ServerFailedToRespond(int iServer);
    virtual void RefreshComplete(EMatchMakingServerResponse response);
    MESSAGE_FUNC_INT(OnRefreshServer, "RefreshServer", serverID);

    virtual int GetRegionCodeToFilter();
    virtual bool CheckTagFilter(gameserveritem_t &server);
    //virtual void LoadFilterSettings() {};//MOM_TODO: make this filter online maps (by name/gametype/difficulty?)

protected:
    // vgui overrides
    virtual void PerformLayout();
    virtual void OnTick();

    virtual const char *GetStringNoUnfilteredServers() { return "#ServerBrowser_NoInternetGames"; }
    virtual const char *GetStringNoUnfilteredServersOnMaster() { return "#ServerBrowser_MasterServerHasNoServersListed"; }
    virtual const char *GetStringNoServersResponded() { return "#ServerBrowser_NoInternetGamesResponded"; }

private:
    // Called once per frame to see if sorting needs to occur again
    void CheckRedoSort();
    // Called once per frame to check re-send request to master server
    //void CheckRetryRequest(ESteamServerType serverType);
    // opens context menu (user right clicked on a server)
    MESSAGE_FUNC_INT(OnOpenContextMenu, "OpenContextMenu", itemID);

    struct regions_s
    {
        CUtlSymbol name;
        unsigned char code;
    };

    CUtlVector<struct regions_s> m_Regions;	// list of the different regions you can query for

    float				m_fLastSort;	// Time of last re-sort
    bool				m_bDirty;	// Has the list been modified, thereby needing re-sort
    bool				m_bRequireUpdate;	// checks whether we need an update upon opening

    // error cases for if no servers are listed
    bool				m_bAnyServersRetrievedFromMaster;
    bool				m_bAnyServersRespondedToQuery;
    bool				m_bNoServersListedOnMaster;

    bool m_bOfflineMode;
};

#endif // INTERNETGAMES_H