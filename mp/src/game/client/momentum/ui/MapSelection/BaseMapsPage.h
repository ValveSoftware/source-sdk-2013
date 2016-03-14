//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BASEGAMESPAGE_H
#define BASEGAMESPAGE_H
#ifdef _WIN32
#pragma once
#endif

class CBaseMapsPage;

//-----------------------------------------------------------------------------
// Purpose: Acts like a regular ListPanel but forwards enter key presses
// to its outer control.
//-----------------------------------------------------------------------------
class CGameListPanel : public vgui::ListPanel
{
public:
    DECLARE_CLASS_SIMPLE(CGameListPanel, vgui::ListPanel);

    CGameListPanel(CBaseMapsPage *pOuter, const char *pName);

    virtual void OnKeyCodeTyped(vgui::KeyCode code);

private:
    CBaseMapsPage *m_pOuter;
};


//-----------------------------------------------------------------------------
// Purpose: Base property page for all the games lists (internet/favorites/lan/etc.)
//-----------------------------------------------------------------------------
class CBaseMapsPage : public vgui::PropertyPage, public IMapList
{
    DECLARE_CLASS_SIMPLE(CBaseMapsPage, vgui::PropertyPage);

public:
    CBaseMapsPage(vgui::Panel *parent, const char *name, const char *pCustomResFilename = NULL);
    ~CBaseMapsPage();

    virtual void PerformLayout();
    virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

    // gets information about specified map
    //MOM_TODO: consider how we're going to ID maps for handling 
    //(creating CMapInfoDialog and actually starting/downloading map)
    //PropertyPages have some sort of unsigned int IDs assigned to
    //things added into the list (see , and ServerBrowser used the Matchmaking ID of the
    //server as the handle, but since we don't have that, all we have is map name?
    virtual mapstruct_t *GetMap(unsigned int serverID);

    //uint32 GetServerFilters(MatchMakingKeyValuePair_t **pFilters); Used by server browser, this will translate
    //into API call filters

    

    // loads filter settings from disk
    virtual void LoadFilterSettings();

    // Called by CGameList when the enter key is pressed.
    // This is overridden in the add server dialog - since there is no Connect button, the message
    // never gets handled, but we want to add a server when they dbl-click or press enter.
    virtual bool OnGameListEnterPressed();

    int GetSelectedItemsCount();

    

    virtual void UpdateDerivedLayouts(void);
    //STEAM_CALLBACK(CBaseMapsPage, OnFavoritesMsg, FavoritesListChanged_t, m_CallbackFavoritesMsg);
    //MOM_TODO: STEAM_CALLBACK for the HTTP requests for maps
protected:
    virtual void OnCommand(const char *command);
    virtual void OnKeyCodePressed(vgui::KeyCode code);
    virtual int GetRegionCodeToFilter() { return -1; }
    
    MESSAGE_FUNC(OnItemSelected, "ItemSelected");

    // applies games filters to current list
    void ApplyGameFilters();
    // updates server count UI
    void UpdateStatus();

    //MOM_TODO: Look into custom HTTP callbacks for the below

    // ISteamMatchmakingServerListResponse callbacks
    /*virtual void ServerResponded(HServerListRequest hReq, int iServer);
    virtual void ServerFailedToRespond(HServerListRequest hRequest, int iServer);
    virtual void RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response) = 0;

    virtual void ServerResponded(int iServer, gameserveritem_t *pServerItem);

    // ISteamMatchmakingPingResponse callbacks
    virtual void ServerResponded(gameserveritem_t &server);
    virtual void ServerFailedToRespond() {}*/

    // Removes map from list
    void RemoveMap(mapdisplay_t&);

    //MOM_TODO: Correlate this to online maps
    virtual bool BShowMap(mapdisplay_t &server) { return server.m_bDoNotRefresh; }

    //Clears the list of maps
    void ClearMapList();

    // filtering methods
    // returns true if filters passed; false if failed
    virtual bool CheckPrimaryFilters(mapstruct_t &);
    virtual bool CheckSecondaryFilters(mapstruct_t &);
    virtual bool CheckTagFilter(mapstruct_t &) { return true; }
    virtual int GetInvalidMapListID();

    virtual void OnSaveFilter(KeyValues *filter);
    virtual void OnLoadFilter(KeyValues *filter);
    virtual void UpdateFilterSettings();

    virtual void GetNewMapList();
    //MOM_TODO: Make these methods "search" for maps based on filter data
    virtual void StartRefresh();
    virtual void StopRefresh();
    virtual bool IsRefreshing();
    virtual void SetRefreshing(bool state);
    virtual void OnPageShow();
    virtual void OnPageHide();

    // called when Connect button is pressed
    MESSAGE_FUNC(OnMapStart, "StartMap");
    // called to look at game info
    MESSAGE_FUNC(OnViewMapInfo, "ViewMapInfo");
    // refreshes a single server
    MESSAGE_FUNC_INT(OnRefreshServer, "RefreshServer", serverID);

    // If true, then we automatically select the first item that comes into the games list.
    bool m_bAutoSelectFirstItemInGameList;

    CGameListPanel *m_pGameList;

    // command buttons
    vgui::Button *m_pStartMap;
    vgui::Button *m_pRefreshAll;//MOM_TODO: change to "m_pSearchMaps"
    vgui::Button *m_pRefreshQuick;
    vgui::ToggleButton *m_pFilter;

    CUtlVector<mapdisplay_t> m_vecMaps;

    int m_iServerRefreshCount;//MOM_TODO: change this to "maps found online" ?


protected:
    virtual void CreateFilters();

    MESSAGE_FUNC_PTR_CHARPTR(OnTextChanged, "TextChanged", panel, text);
    MESSAGE_FUNC_PTR_INT(OnButtonToggled, "ButtonToggled", panel, state);

private:
    void RequestServersResponse(int iServer, EMatchMakingServerResponse response, bool bLastServer); // callback for matchmaking interface

    void RecalculateFilterString();

    // If set, it uses the specified resfile name instead of its default one.
    const char *m_pCustomResFilename;

    // filter controls
    vgui::ComboBox *m_pGameModeFilter;
    vgui::TextEntry *m_pMapFilter;
    vgui::ComboBox *m_pDifficultyFilter;
    vgui::CheckButton *m_pHideCompletedFilterCheck;//Used for local maps only
    vgui::ComboBox *m_pMapLayoutFilter;//0 = ALL, 1 = LINEAR ONLY, 2 = STAGED ONLY
    vgui::Label *m_pFilterString;//MOM_TODO: determine what this is and if we need it
    char m_szComboAllText[64];

    KeyValues *m_pFilters; // base filter data
    bool m_bFiltersVisible;	// true if filter section is currently visible
    vgui::HFont m_hFont;

    // filter data
    int m_iGameModeFilter;
    char m_szMapFilter[32];
    int	m_iDifficultyFilter;//What tier the map should be under
    bool m_bFilterHideCompleted;//Hide completed maps
    int m_iMapLayoutFilter;//Map is non-linear (has stages)

    typedef enum
    {
        HEADER_COMPLETED =0,
        HEADER_MAPLAYOUT,
        //HEADER_STAGEDMAP,
        HEADER_MAPNAME,
        HEADER_GAMEMODE,
        HEADER_DIFFICULTY,
        HEADER_BESTTIME
    } HEADERS;

};

#endif // BASEGAMESPAGE_H
