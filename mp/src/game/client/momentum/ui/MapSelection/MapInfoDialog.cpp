#include "pch_mapselection.h"

using namespace vgui;
//extern class IAppInformation *g_pAppInformation; // may be NULL

static const long RETRY_TIME = 10000;		// refresh server every 10 seconds
static const long CHALLENGE_ENTRIES = 1024;

extern "C"
{
    DLL_EXPORT bool JoiningSecureServerCall()
    {
        return true;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Comparison function used in query redblack tree
//-----------------------------------------------------------------------------
/*bool QueryLessFunc(const struct challenge_s &item1, const struct challenge_s &item2)
{
    return false;
    // compare port then ip
    if (item1.addr.GetPort() < item2.addr.GetPort())
        return true;
    else if (item1.addr.GetPort() > item2.addr.GetPort())
        return false;

    //int ip1 = item1.addr.GetIP();
    //int ip2 = item2.addr.GetIP();
    //  return ip1 < ip2;
}*/


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CDialogMapInfo::CDialogMapInfo(vgui::Panel *parent, const char *mapname) :
Frame(parent, "DialogMapInfo")
#ifndef NO_STEAM
, m_CallbackPersonaStateChange(this, &CDialogMapInfo::OnPersonaStateChange)
#endif
{
    SetBounds(0, 0, 512, 512);
    SetMinimumSize(416, 340);
    SetDeleteSelfOnClose(true);
    m_bConnecting = false;
    m_bServerFull = false;
    m_bShowAutoRetryToggle = false;
    m_bServerNotResponding = false;
    m_bShowingExtendedOptions = false;
    m_SteamIDFriend = 0;
    m_hPingQuery = HSERVERQUERY_INVALID;
    m_hPlayersQuery = HSERVERQUERY_INVALID;
    m_bPlayerListUpdatePending = false;

    m_szPassword[0] = 0;

    m_pConnectButton = new Button(this, "Connect", "#ServerBrowser_JoinGame");
    m_pCloseButton = new Button(this, "Close", "#ServerBrowser_Close");
    m_pRefreshButton = new Button(this, "Refresh", "#ServerBrowser_Refresh");
    m_pInfoLabel = new Label(this, "InfoLabel", "");
    m_pAutoRetry = new ToggleButton(this, "AutoRetry", "#ServerBrowser_AutoRetry");
    m_pAutoRetry->AddActionSignalTarget(this);

    m_pAutoRetryAlert = new RadioButton(this, "AutoRetryAlert", "#ServerBrowser_AlertMeWhenSlotOpens");
    m_pAutoRetryJoin = new RadioButton(this, "AutoRetryJoin", "#ServerBrowser_JoinWhenSlotOpens");
    m_pPlayerList = new ListPanel(this, "PlayerList");
    m_pPlayerList->AddColumnHeader(0, "PlayerName", "#ServerBrowser_PlayerName", 156);
    m_pPlayerList->AddColumnHeader(1, "Score", "#ServerBrowser_Score", 64);
    m_pPlayerList->AddColumnHeader(2, "Time", "#ServerBrowser_Time", 64);

    m_pPlayerList->SetSortFunc(2, &PlayerTimeColumnSortFunc);

    // set the defaults for sorting
    // hack, need to make this more explicit functions in ListPanel
    PostMessage(m_pPlayerList, new KeyValues("SetSortColumn", "column", 2));
    PostMessage(m_pPlayerList, new KeyValues("SetSortColumn", "column", 1));
    PostMessage(m_pPlayerList, new KeyValues("SetSortColumn", "column", 1));

    m_pAutoRetryAlert->SetSelected(true);

    m_pConnectButton->SetCommand(new KeyValues("Connect"));
    m_pCloseButton->SetCommand(new KeyValues("Close"));
    m_pRefreshButton->SetCommand(new KeyValues("Refresh"));

    m_iRequestRetry = 0;

    // create a new server to watch
    //memset(&m_Server, 0, sizeof(m_Server));
    //m_Server.m_NetAdr.Init(serverIP, queryPort, connectionPort);

    // refresh immediately
    RequestInfo();

    // let us be ticked every frame
    ivgui()->AddTickSignal(this->GetVPanel());

    LoadControlSettings("resource/ui/DialogMapInfo.res");
    //LoadControlSettings("Servers/DialogGameInfo.res");
    //RegisterControlSettingsFile("Servers/DialogGameInfo_SinglePlayer.res");
    //RegisterControlSettingsFile("Servers/DialogGameInfo_AutoRetry.res");
    MoveToCenterOfScreen();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDialogMapInfo::~CDialogMapInfo()
{
    //MOM_TODO: Cancel any queries sent out
/*#ifndef NO_STEAM
    if (!SteamMatchmakingServers())
        return;

    if (m_hPingQuery != HSERVERQUERY_INVALID)
        SteamMatchmakingServers()->CancelServerQuery(m_hPingQuery);
    if (m_hPlayersQuery != HSERVERQUERY_INVALID)
        SteamMatchmakingServers()->CancelServerQuery(m_hPlayersQuery);
#endif*/
}

//-----------------------------------------------------------------------------
// Purpose: send a player query to a server
//-----------------------------------------------------------------------------
void CDialogMapInfo::SendPlayerQuery(uint32 unIP, uint16 usQueryPort)
{
    //MOM_TODO: 
    /*
#ifndef NO_STEAM
    if (!SteamMatchmakingServers())
        return;

    if (m_hPlayersQuery != HSERVERQUERY_INVALID)
        SteamMatchmakingServers()->CancelServerQuery(m_hPlayersQuery);
    m_hPlayersQuery = SteamMatchmakingServers()->PlayerDetails(unIP, usQueryPort, this);
    m_bPlayerListUpdatePending = true;
#endif*/
}


//-----------------------------------------------------------------------------
// Purpose: Activates the dialog
//-----------------------------------------------------------------------------
void CDialogMapInfo::Run(const char *titleName)
{
    if (titleName)
    {
        SetTitle("#ServerBrowser_GameInfoWithNameTitle", true);
    }
    else
    {
        SetTitle("#ServerBrowser_GameInfoWithNameTitle", true);
    }
    SetDialogVariable("game", titleName);

    // get the info from the user
    //RequestInfo();
    //MOM_TODO: LoadLocalInfo(); //Loads the local information (local PBs, local replays)
    Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Changes which server to watch
//-----------------------------------------------------------------------------
void CDialogMapInfo::ChangeGame(int serverIP, int queryPort, unsigned short connectionPort)
{
    memset(&m_Server, 0x0, sizeof(m_Server));

    m_Server.m_NetAdr.Init(serverIP, queryPort, connectionPort);

    // remember the dialogs position so we can keep it the same
    int x, y;
    GetPos(x, y);

    // see if we need to change dialog state
    if (!m_Server.m_NetAdr.GetIP() || !m_Server.m_NetAdr.GetQueryPort())
    {
        // not in a server, load the simple settings dialog
        SetMinimumSize(0, 0);
        SetSizeable(false);
        LoadControlSettings("Servers/DialogGameInfo_SinglePlayer.res");
    }
    else
    {
        // moving from a single-player game -> multiplayer, reset dialog
        SetMinimumSize(416, 340);
        SetSizeable(true);
        LoadControlSettings("Servers/DialogGameInfo.res");
    }
    SetPos(x, y);

    // Start refresh immediately
    m_iRequestRetry = 0;
    RequestInfo();
    InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: updates the dialog if it's watching a friend who changes servers
//-----------------------------------------------------------------------------
#ifndef NO_STEAM
void CDialogMapInfo::OnPersonaStateChange(PersonaStateChange_t *pPersonaStateChange)
{
#if 0 // TBD delete this func
    if (m_SteamIDFriend && m_SteamIDFriend == pPersonaStateChange->m_ulSteamID)
    {
        // friend may have changed servers
        uint64 nGameID;
        uint32 unGameIP;
        uint16 usGamePort;
        uint16 usQueryPort;

        if (SteamFriends()->GetFriendGamePlayed(m_SteamIDFriend, &nGameID, &unGameIP, &usGamePort, &usQueryPort))
        {
            if (pPersonaStateChange->m_nGameIDPrevious != nGameID
                || pPersonaStateChange->m_unGameServerIPPrevious != unGameIP
                || pPersonaStateChange->m_usGameServerPortPrevious != usGamePort
                || pPersonaStateChange->m_usGameServerQueryPortPrevious != usQueryPort)
            {
                ChangeGame(unGameIP, usQueryPort, usGamePort);
            }
        }
    }
#endif
}
#endif // NO_STEAM

//-----------------------------------------------------------------------------
// Purpose: Associates a user with this dialog
//-----------------------------------------------------------------------------
void CDialogMapInfo::SetFriend(uint64 ulSteamIDFriend)
{
    //MOM_TODO: What is this void?
/*#ifndef NO_STEAM
    // set the title to include the friends name
    SetTitle("#ServerBrowser_GameInfoWithNameTitle", true);

    SetDialogVariable("game", SteamFriends()->GetFriendPersonaName(ulSteamIDFriend));
    SetDialogVariable("friend", SteamFriends()->GetFriendPersonaName(ulSteamIDFriend));

    // store the friend we're associated with
    m_SteamIDFriend = ulSteamIDFriend;

    uint64 nGameID;
    uint32 unGameIP;
    uint16 usGamePort;
    uint16 usQueryPort;
    if (SteamFriends()->GetFriendGamePlayed(ulSteamIDFriend, &nGameID, &unGameIP, &usGamePort, &usQueryPort))
    {
        uint16 usConnPort = usGamePort;
        if (usQueryPort < QUERY_PORT_ERROR)
            usConnPort = usGamePort;
        ChangeGame(unGameIP, usConnPort, usGamePort);
    }
#endif*/
}


//-----------------------------------------------------------------------------
// Purpose: data access
//-----------------------------------------------------------------------------
uint64 CDialogMapInfo::GetAssociatedFriend()
{
    return m_SteamIDFriend;
}


//-----------------------------------------------------------------------------
// Purpose: lays out the data
//-----------------------------------------------------------------------------
void CDialogMapInfo::PerformLayout()
{
    BaseClass::PerformLayout();

    SetControlString("ServerText", m_Server.GetName());
    SetControlString("GameText", m_Server.m_szGameDescription);
    SetControlString("MapText", m_Server.m_szMap);

    /*if (!Q_strlen(m_Server.m_szGameDescription) && m_SteamIDFriend && g_pAppInformation)
    {
        // no game description set yet
        // get the game from the friends info if we can
        uint64 nGameID = 0;
#ifndef NO_STEAM
        //SteamFriends()->GetFriendGamePlayed(m_SteamIDFriend, &nGameID, NULL, NULL, NULL);
#endif
        if (nGameID)
        {
            //SetControlString("GameText", g_pAppInformation->GetAppName(g_pAppInformation->GetIAppForAppID(nGameID)));
        }
        else
        {
            SetControlString("GameText", "#ServerBrowser_NotInGame");
        }
    }*/

    if (!m_Server.m_bHadSuccessfulResponse)
    {
        SetControlString("SecureText", "");
    }
    else if (m_Server.m_bSecure)
    {
        SetControlString("SecureText", "#ServerBrowser_Secure");
    }
    else
    {
        SetControlString("SecureText", "#ServerBrowser_NotSecure");
    }

    char buf[128];
    if (m_Server.m_nMaxPlayers > 0)
    {
        Q_snprintf(buf, sizeof(buf), "%d / %d", m_Server.m_nPlayers, m_Server.m_nMaxPlayers);
    }
    else
    {
        buf[0] = 0;
    }
    SetControlString("PlayersText", buf);

    if (m_Server.m_NetAdr.GetIP() && m_Server.m_NetAdr.GetQueryPort())
    {
        SetControlString("ServerIPText", m_Server.m_NetAdr.GetConnectionAddressString());
        m_pConnectButton->SetEnabled(true);
        if (m_pAutoRetry->IsSelected())
        {
            m_pAutoRetryAlert->SetVisible(true);
            m_pAutoRetryJoin->SetVisible(true);
        }
        else
        {
            m_pAutoRetryAlert->SetVisible(false);
            m_pAutoRetryJoin->SetVisible(false);
        }
    }
    else
    {
        SetControlString("ServerIPText", "");
        m_pConnectButton->SetEnabled(false);
    }

    if (m_Server.m_bHadSuccessfulResponse)
    {
        Q_snprintf(buf, sizeof(buf), "%d", m_Server.m_nPing);
        SetControlString("PingText", buf);
    }
    else
    {
        SetControlString("PingText", "");
    }

    // set the info text
    if (m_pAutoRetry->IsSelected())
    {
        if (m_Server.m_nPlayers < m_Server.m_nMaxPlayers)
        {
            m_pInfoLabel->SetText("#ServerBrowser_PressJoinToConnect");
        }
        else if (m_pAutoRetryJoin->IsSelected())
        {
            m_pInfoLabel->SetText("#ServerBrowser_JoinWhenSlotIsFree");
        }
        else
        {
            m_pInfoLabel->SetText("#ServerBrowser_AlertWhenSlotIsFree");
        }
    }
    else if (m_bServerFull)
    {
        m_pInfoLabel->SetText("#ServerBrowser_CouldNotConnectServerFull");
    }
    else if (m_bServerNotResponding)
    {
        m_pInfoLabel->SetText("#ServerBrowser_ServerNotResponding");
    }
    else
    {
        // clear the status
        m_pInfoLabel->SetText("");
    }

    if (m_Server.m_bHadSuccessfulResponse && !(m_Server.m_nPlayers + m_Server.m_nBotPlayers))
    {
        m_pPlayerList->SetEmptyListText("#ServerBrowser_ServerHasNoPlayers");
    }
    else
    {
        m_pPlayerList->SetEmptyListText("#ServerBrowser_ServerNotResponding");
    }

    // auto-retry layout
    m_pAutoRetry->SetVisible(m_bShowAutoRetryToggle);

    Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Forces the game info dialog to try and connect
//-----------------------------------------------------------------------------
void CDialogMapInfo::Connect()
{
    OnConnect();
}

//-----------------------------------------------------------------------------
// Purpose: Connects the user to this game
//-----------------------------------------------------------------------------
void CDialogMapInfo::OnConnect()
{
    // flag that we are attempting connection
    m_bConnecting = true;

    // reset state
    m_bServerFull = false;
    m_bServerNotResponding = false;

    InvalidateLayout();

    // need to refresh server before attempting to connect, to make sure there is enough room on the server
    m_iRequestRetry = 0;
    RequestInfo();
}

//-----------------------------------------------------------------------------
// Purpose: Cancel auto-retry if we connect to the game by other means
//-----------------------------------------------------------------------------
void CDialogMapInfo::OnConnectToGame(int ip, int port)
{
    // if we just connected to the server we were looking at, close the dialog
    // important so that we don't auto-retry a server that we are already on
    if (m_Server.m_NetAdr.GetIP() == (uint32) ip && m_Server.m_NetAdr.GetConnectionPort() == (uint16) port)
    {
        // close this dialog
        Close();
    }
}

//-----------------------------------------------------------------------------
// Purpose: Handles Refresh button press, starts a re-ping of the server
//-----------------------------------------------------------------------------
void CDialogMapInfo::OnRefresh()
{
    m_iRequestRetry = 0;
    // re-ask the server for the game info
    RequestInfo();
}

//-----------------------------------------------------------------------------
// Purpose: Forces the whole dialog to redraw when the auto-retry button is toggled
//-----------------------------------------------------------------------------
void CDialogMapInfo::OnButtonToggled(Panel *panel)
{
    if (panel == m_pAutoRetry)
    {
        ShowAutoRetryOptions(m_pAutoRetry->IsSelected());
    }

    InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether the extended auto-retry options are visible or not
//-----------------------------------------------------------------------------
void CDialogMapInfo::ShowAutoRetryOptions(bool state)
{
    // we need to extend the dialog
    int growSize = 60;
    if (!state)
    {
        growSize = -growSize;
    }

    // alter the dialog size accordingly
    int x, y, wide, tall;
    GetBounds(x, y, wide, tall);

    // load a new layout file depending on the state
    SetMinimumSize(416, 340);
    if (state)
        LoadControlSettings("Servers/DialogGameInfo_AutoRetry.res");
    else
        LoadControlSettings("Servers/DialogGameInfo.res");

    // restore size and position as 
    // load control settings will override them
    SetBounds(x, y, wide, tall + growSize);

    // restore other properties of the dialog
    PerformLayout();

    m_pAutoRetryAlert->SetSelected(true);

    InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Requests the right info from the server
//-----------------------------------------------------------------------------
void CDialogMapInfo::RequestInfo()
{
    //MOM_TODO: Request info from our website/API
/*#ifndef NO_STEAM
    if (!SteamMatchmakingServers())
        return;

    if (m_iRequestRetry == 0)
    {
        // reset the time at which we auto-refresh
        m_iRequestRetry = system()->GetTimeMillis() + RETRY_TIME;
        if (m_hPingQuery != HSERVERQUERY_INVALID)
            SteamMatchmakingServers()->CancelServerQuery(m_hPingQuery);
        m_hPingQuery = SteamMatchmakingServers()->PingServer(m_Server.m_NetAdr.GetIP(), m_Server.m_NetAdr.GetQueryPort(), this);
    }
#endif*/
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame, handles resending network messages
//-----------------------------------------------------------------------------
void CDialogMapInfo::OnTick()
{
    // check to see if we should perform an auto-refresh
    if (m_iRequestRetry && m_iRequestRetry < system()->GetTimeMillis())
    {
        m_iRequestRetry = 0;
        RequestInfo();
    }
}

//-----------------------------------------------------------------------------
// Purpose: called when the server has successfully responded
//-----------------------------------------------------------------------------
void CDialogMapInfo::ServerResponded(gameserveritem_t &server)
{
    m_hPingQuery = HSERVERQUERY_INVALID;
    m_Server = server;

    if (m_bConnecting)
    {
        ConnectToServer();
    }
    else if (m_pAutoRetry->IsSelected() && server.m_nPlayers < server.m_nMaxPlayers)
    {
        // there is a slot free, we can join

        // make the sound
        surface()->PlaySound("Servers/game_ready.wav");

        // flash this window
        FlashWindow();

        // if it's set, connect right away
        if (m_pAutoRetryJoin->IsSelected())
        {
            ConnectToServer();
        }
    }
    else
    {
        SendPlayerQuery(server.m_NetAdr.GetIP(), server.m_NetAdr.GetQueryPort());
    }

    m_bServerNotResponding = false;

    InvalidateLayout();
    Repaint();
}


//-----------------------------------------------------------------------------
// Purpose: called when a server response has timed out
//-----------------------------------------------------------------------------
void CDialogMapInfo::ServerFailedToRespond()
{
    // the server didn't respond, mark that in the UI
    // only mark if we haven't ever received a response
    if (!m_Server.m_bHadSuccessfulResponse)
    {
        m_bServerNotResponding = true;
    }

    InvalidateLayout();
    Repaint();
}



//-----------------------------------------------------------------------------
// Purpose: Constructs a command to send a running game to connect to a server,
// based on the server type
//
// TODO it would be nice to push this logic into the IRunGameEngine interface; that
// way we could ask the engine itself to construct arguments in ways that fit.
// Might be worth the effort as we start to add more engines.
//-----------------------------------------------------------------------------
void CDialogMapInfo::ApplyConnectCommand(const gameserveritem_t &server)
{
    char command[256];
    // set the server password, if any
    if (m_szPassword[0])
    {
        Q_snprintf(command, Q_ARRAYSIZE(command), "password \"%s\"\n", m_szPassword);
        //engine->ExecuteClientCmd(command);
        //g_pRunGameEngine->AddTextCommand(command);
    }

    // send engine command to change servers
    Q_snprintf(command, Q_ARRAYSIZE(command), "connect %s\n", server.m_NetAdr.GetConnectionAddressString());
    //engine->ExecuteClientCmd(command);
    //g_pRunGameEngine->AddTextCommand(command);
}


//-----------------------------------------------------------------------------
// Purpose: Constructs game options to use when running a game to connect to a server
//-----------------------------------------------------------------------------
void CDialogMapInfo::ConstructConnectArgs(char *pchOptions, int cchOptions, const gameserveritem_t &server)
{
    Q_snprintf(pchOptions, cchOptions, " +connect %s", server.m_NetAdr.GetConnectionAddressString());
    if (m_szPassword[0])
    {
        Q_strcat(pchOptions, " +password \"", cchOptions);
        Q_strcat(pchOptions, m_szPassword, cchOptions);
        Q_strcat(pchOptions, "\"", cchOptions);
    }
}


//-----------------------------------------------------------------------------
// Purpose: Connects to the server
//-----------------------------------------------------------------------------
void CDialogMapInfo::ConnectToServer()
{
    m_bConnecting = false;

    // check VAC status
    /*if (m_Server.m_bSecure && ServerBrowser().IsVACBannedFromGame(m_Server.m_nAppID))
    {
        // refuse the user
        CVACBannedConnRefusedDialog *pDlg = new CVACBannedConnRefusedDialog(GetVParent(), "VACBannedConnRefusedDialog");
        pDlg->Activate();
        Close();
        return;
    }*/


    // check to see if we need a password
    /*if (m_Server.m_bPassword && !m_szPassword[0])
    {
        CDialogServerPassword *box = new CDialogServerPassword(this);
        box->AddActionSignalTarget(this);
        box->Activate(m_Server.GetName(), 0);
        return;
    }*/

    // check the player count
    if (m_Server.m_nPlayers >= m_Server.m_nMaxPlayers)
    {
        // mark why we cannot connect
        m_bServerFull = true;
        // give them access to auto-retry options
        m_bShowAutoRetryToggle = true;
        InvalidateLayout();
        return;
    }

    // tell the engine to connect
    //const char *gameDir = m_Server.m_szGameDir;
    //if (/*engine->IsActiveApp()*/)
    {
        ApplyConnectCommand(m_Server);
    }
    //else
    {
        //Log("ENGINE IS NOT RUNNING!\n");
        /*char connectArgs[256];
        ConstructConnectArgs(connectArgs, Q_ARRAYSIZE(connectArgs), m_Server);

        if ((m_Server.m_bSecure && JoiningSecureServerCall()) || !m_Server.m_bSecure)
        {
            switch (g_pRunGameEngine->RunEngine(m_Server.m_nAppID, gameDir, connectArgs))
            {
            case IRunGameEngine::k_ERunResultModNotInstalled:
            {
                MessageBox *dlg = new MessageBox("#ServerBrowser_GameInfoTitle", "#ServerBrowser_ModNotInstalled");
                dlg->DoModal();
                SetVisible(false);
                return;
            }
            break;
            case IRunGameEngine::k_ERunResultAppNotFound:
            {
                MessageBox *dlg = new MessageBox("#ServerBrowser_GameInfoTitle", "#ServerBrowser_AppNotFound");
                dlg->DoModal();
                SetVisible(false);
                return;
            }
            break;
            case IRunGameEngine::k_ERunResultNotInitialized:
            {
                MessageBox *dlg = new MessageBox("#ServerBrowser_GameInfoTitle", "#ServerBrowser_NotInitialized");
                dlg->DoModal();
                SetVisible(false);
                return;
            }
            break;
            case IRunGameEngine::k_ERunResultOkay:
            default:
                break;
            };
        }*/
    }

    // close this dialog
    PostMessage(this, new KeyValues("Close"));
}

//-----------------------------------------------------------------------------
// Purpose: called when the current refresh list is complete
//-----------------------------------------------------------------------------
void CDialogMapInfo::RefreshComplete(EMatchMakingServerResponse response)
{
}

//-----------------------------------------------------------------------------
// Purpose: handles response from the get password dialog
//-----------------------------------------------------------------------------
void CDialogMapInfo::OnJoinServerWithPassword(const char *password)
{
    // copy out the password
    Q_strncpy(m_szPassword, password, sizeof(m_szPassword));

    // retry connecting to the server again
    OnConnect();
}

//-----------------------------------------------------------------------------
// Purpose: player list received
//-----------------------------------------------------------------------------
void CDialogMapInfo::ClearPlayerList()
{
    m_pPlayerList->DeleteAllItems();
    Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: on individual player added
//-----------------------------------------------------------------------------
void CDialogMapInfo::AddPlayerToList(const char *playerName, int score, float timePlayedSeconds)
{
    if (m_bPlayerListUpdatePending)
    {
        m_bPlayerListUpdatePending = false;
        m_pPlayerList->RemoveAll();
    }

    KeyValues *player = new KeyValues("player");
    player->SetString("PlayerName", playerName);
    player->SetInt("Score", score);
    player->SetInt("TimeSec", (int) timePlayedSeconds);

    // construct a time string
    int seconds = (int) timePlayedSeconds;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    seconds %= 60;
    minutes %= 60;
    char buf[64];
    buf[0] = 0;
    if (hours)
    {
        Q_snprintf(buf, sizeof(buf), "%dh %dm %ds", hours, minutes, seconds);
    }
    else if (minutes)
    {
        Q_snprintf(buf, sizeof(buf), "%dm %ds", minutes, seconds);
    }
    else
    {
        Q_snprintf(buf, sizeof(buf), "%ds", seconds);
    }
    player->SetString("Time", buf);

    m_pPlayerList->AddItem(player, 0, false, true);
    player->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: Sorting function for time column
//-----------------------------------------------------------------------------
int CDialogMapInfo::PlayerTimeColumnSortFunc(ListPanel *pPanel, const ListPanelItem &p1, const ListPanelItem &p2)
{
    int p1time = p1.kv->GetInt("TimeSec");
    int p2time = p2.kv->GetInt("TimeSec");

    if (p1time > p2time)
        return -1;
    if (p1time < p2time)
        return 1;

    return 0;
}