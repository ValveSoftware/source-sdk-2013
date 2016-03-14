#ifndef SERVERBROWSERDIALOG_H
#define SERVERBROWSERDIALOG_H
#ifdef _WIN32
#pragma once
#endif

//extern class IRunGameEngine *g_pRunGameEngine;
//extern class IAppInformation *g_pAppInformation; // can be NULL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMapSelectorDialog : public vgui::Frame
{
    DECLARE_CLASS_SIMPLE(CMapSelectorDialog, vgui::Frame);

public:
    // Construction/destruction
    CMapSelectorDialog(vgui::VPANEL parent);
    ~CMapSelectorDialog(void);

    void		Initialize(void);

    // displays the dialog, moves it into focus, updates if it has to
    void		Open(void);
    void        Close(void)
    {
        ConVar *pCvar = g_pCVar->FindVar("cl_showmapselection");
        if (pCvar) pCvar->SetValue(0);
        BaseClass::Close();
    }

    // gets server info
    mapstruct_t *GetMap(unsigned int serverID);
    // called every frame
    virtual void OnTick();

    // updates status text at bottom of window
    void UpdateStatusText(const char *format, ...);

    // updates status text at bottom of window
    void UpdateStatusText(wchar_t *unicode);

    // context menu access
    CMapContextMenu *GetContextMenu(vgui::Panel *pParent);

    // returns a pointer to a static instance of this dialog
    // valid for use only in sort functions
    static CMapSelectorDialog *GetInstance();

    // begins the process of joining a server from a game list
    // the game info dialog it opens will also update the game list
    CDialogMapInfo *JoinGame(IMapList *gameList, unsigned int serverIndex);

    // joins a game by a specified IP, not attached to any game list
    CDialogMapInfo *JoinGame(int serverIP, int serverPort);

    // opens a game info dialog from a game list
    CDialogMapInfo *OpenMapInfoDialog(IMapList *gameList, KeyValues *pMap);

    // opens a game info dialog by a specified IP, not attached to any game list
    CDialogMapInfo *OpenMapInfoDialog(int serverIP, uint16 connPort, uint16 queryPort);

    // closes all the game info dialogs
    void CloseAllMapInfoDialogs();
    CDialogMapInfo *GetDialogGameInfoForFriend(uint64 ulSteamIDFriend);

    // accessor to the filter save data
    KeyValues *GetFilterSaveData(const char *filterSet);

    // load/saves filter & favorites settings from disk
    void		LoadUserData();
    void		SaveUserData();

    // forces the currently active page to refresh
    void		RefreshCurrentPage();

    virtual gameserveritem_t *GetCurrentConnectedServer()
    {
        return &m_CurrentConnection;
    }

private:

    // current game list change
    MESSAGE_FUNC(OnGameListChanged, "PageChanged");
    void ReloadFilterSettings();

    // notification that we connected / disconnected
    MESSAGE_FUNC_PARAMS(OnConnectToGame, "ConnectedToGame", kv);
    MESSAGE_FUNC(OnDisconnectFromGame, "DisconnectedFromGame");

    virtual bool GetDefaultScreenPosition(int &x, int &y, int &wide, int &tall);
    virtual void ActivateBuildMode();

private:
    // list of all open game info dialogs
    CUtlVector<vgui::DHANDLE<CDialogMapInfo> > m_vecMapInfoDialogs;

    // pointer to current game list
    IMapList *m_pGameList;

    // Status text
    vgui::Label	*m_pStatusLabel;

    // property sheet
    vgui::PropertySheet *m_pTabPanel;

    //Map tabs
    CLocalMaps *m_pLocal;
    COnlineMaps *m_pOnline;

    //Filter data
    KeyValues *m_pSavedData;//Saved on disk filter data
    KeyValues *m_pFilterData;//Current filter data in the Dialog

    // context menu
    CMapContextMenu *m_pContextMenu;

    // currently connected game
    bool m_bCurrentlyConnected;
    gameserveritem_t m_CurrentConnection;
};

// singleton accessor
extern CMapSelectorDialog &MapSelectorDialog();

// Used by the LAN tab and the add server dialog when trying to find servers without having
// been given any ports to look for servers on.
void GetMostCommonQueryPorts(CUtlVector<uint16> &ports);

#endif // SERVERBROWSERDIALOG_H