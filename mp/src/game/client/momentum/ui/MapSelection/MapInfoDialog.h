#ifndef DIALOGGAMEINFO_H
#define DIALOGGAMEINFO_H
#ifdef _WIN32
#pragma once
#endif


/*struct challenge_s
{
    netadr_t addr;
    int challenge;
};*/

//-----------------------------------------------------------------------------
// Purpose: Dialog for displaying information about a game server
//-----------------------------------------------------------------------------
class CDialogMapInfo : public vgui::Frame//, public ISteamMatchmakingPlayersResponse, public ISteamMatchmakingPingResponse
{
    DECLARE_CLASS_SIMPLE(CDialogMapInfo, vgui::Frame);

public:
    CDialogMapInfo(vgui::Panel *parent, const char*);
    ~CDialogMapInfo();

    void Run(const char *titleName);
    void ChangeGame(int serverIP, int queryPort, unsigned short connectionPort);
    void SetFriend(uint64 ulSteamIDFriend);
    uint64 GetAssociatedFriend();

    // forces the dialog to attempt to connect to the server
    void Connect();

    // implementation of IServerRefreshResponse interface
    // called when the server has successfully responded
    virtual void ServerResponded(gameserveritem_t &server);

    // called when a server response has timed out
    virtual void ServerFailedToRespond();

    // on individual player added
    virtual void AddPlayerToList(const char *playerName, int score, float timePlayedSeconds);
    virtual void PlayersFailedToRespond() {}
    virtual void PlayersRefreshComplete() { m_hPlayersQuery = HSERVERQUERY_INVALID; }

    // called when the current refresh list is complete
    virtual void RefreshComplete(EMatchMakingServerResponse response);

    // player list received
    virtual void ClearPlayerList();

    //virtual void SendChallengeQuery( const netadr_t & to );
    virtual void SendPlayerQuery(uint32 unIP, uint16 usQueryPort);
    //virtual void InsertChallengeResponse( const netadr_t & to, int nChallenge );

protected:
    // message handlers
    MESSAGE_FUNC(OnConnect, "Connect");
    MESSAGE_FUNC(OnRefresh, "Refresh");
    MESSAGE_FUNC_PTR(OnButtonToggled, "ButtonToggled", panel);
    MESSAGE_FUNC_PTR(OnRadioButtonChecked, "RadioButtonChecked", panel)
    {
        OnButtonToggled(panel);
    }

    // response from the get password dialog
    MESSAGE_FUNC_CHARPTR(OnJoinServerWithPassword, "JoinServerWithPassword", password);

    MESSAGE_FUNC_INT_INT(OnConnectToGame, "ConnectedToGame", ip, port);

    // vgui overrides
    virtual void OnTick();
    virtual void PerformLayout();

private:
#ifndef NO_STEAM
    STEAM_CALLBACK(CDialogMapInfo, OnPersonaStateChange, PersonaStateChange_t, m_CallbackPersonaStateChange);
#endif

    long m_iRequestRetry;	// time at which to retry the request
    static int PlayerTimeColumnSortFunc(vgui::ListPanel *pPanel, const vgui::ListPanelItem &p1, const vgui::ListPanelItem &p2);

    // methods
    void RequestInfo();
    void ConnectToServer();
    void ShowAutoRetryOptions(bool state);
    void ConstructConnectArgs(char *pchOptions, int cchOptions, const gameserveritem_t &server);
    void ApplyConnectCommand(const gameserveritem_t &server);

    vgui::Button *m_pConnectButton;
    vgui::Button *m_pCloseButton;
    vgui::Button *m_pRefreshButton;
    vgui::Label *m_pInfoLabel;
    vgui::ToggleButton *m_pAutoRetry;
    vgui::RadioButton *m_pAutoRetryAlert;
    vgui::RadioButton *m_pAutoRetryJoin;
    vgui::ListPanel *m_pPlayerList;

    enum { PING_TIMES_MAX = 4 };

    // true if we should try connect to the server when it refreshes
    bool m_bConnecting;

    // password, if entered
    char m_szPassword[64];

    // state
    bool m_bServerNotResponding;
    bool m_bServerFull;
    bool m_bShowAutoRetryToggle;
    bool m_bShowingExtendedOptions;
    uint64 m_SteamIDFriend;

    gameserveritem_t m_Server;
    HServerQuery m_hPingQuery;
    HServerQuery m_hPlayersQuery;
    bool m_bPlayerListUpdatePending;
};

#endif // DIALOGGAMEINFO_H
