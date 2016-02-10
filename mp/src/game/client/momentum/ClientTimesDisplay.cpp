#include "cbase.h"
//#include "ClientTimesDisplay.h"
//#include <stdio.h>
//
//#include <cdll_client_int.h>
//#include <cdll_util.h>
//#include <globalvars_base.h>
//#include <igameresources.h>
//#include "IGameUIFuncs.h" // for key bindings
//#include "inputsystem/iinputsystem.h"
//#include <voice_status.h>
//
//#include <vgui/IScheme.h>
//#include <vgui/ILocalize.h>
//#include <vgui/ISurface.h>
//#include <vgui/IVGui.h>
//#include <vstdlib/IKeyValuesSystem.h>
//
//#include <KeyValues.h>
//#include <vgui_controls/ImageList.h>
//#include <vgui_controls/Label.h>
//#include <vgui_controls/SectionedListPanel.h>
//
//#include <game/client/iviewport.h>
//#include <igameresources.h>
//
//#include "vgui_avatarimage.h"
//
//// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h"
//
//using namespace vgui;
//
////-----------------------------------------------------------------------------
//// Purpose: Constructor
////-----------------------------------------------------------------------------
//CClientTimesDisplay::CClientTimesDisplay(IViewPort *pViewPort) : EditablePanel(NULL, PANEL_SCOREBOARD)
//{
//    
//    m_nCloseKey = BUTTON_CODE_INVALID;
//
//    //memset(s_VoiceImage, 0x0, sizeof( s_VoiceImage ));
//    TrackerImage = 0;
//    m_pViewPort = pViewPort;
//
//    //initialize dialog
//    SetProportional(true);
//    SetKeyBoardInputEnabled(false);
//    SetMouseInputEnabled(false);
//
//    //set the scheme before any child control is created
//    SetScheme("ClientScheme");
//    
//    // load scoreboard components
//
//    LoadControlSettings("Resource/UI/timesdisplay.res");
//
//    m_pHeader = FindControl<Panel>("Header", true);
//    m_lMapSummary = FindControl<Label>("MapSummary", true);
//    m_pPlayerStats = FindControl<Panel>("PlayerStats", true);
//    m_pPlayerAvatar = FindControl<ImagePanel>("PlayerAvatar",true);
//    m_lPlayerName = FindControl<Label>("PlayerName", true);
//    m_lPlayerMapRank = FindControl<Label>("PlayerMapRank", true);
//    m_lPlayerGlobalRank = FindControl<Label>("PlayerGlobalRank", true);
//    m_pLeaderboards = FindControl<Panel>("Leaderboards", true);
//    m_pOnlineLeaderboards = FindControl<SectionedListPanel>("OnlineNearbyLeaderboard", true);
//    m_pLocalBests = FindControl<SectionedListPanel>("LocalPersonalBest", true);
//    
//    if (!m_pHeader || !m_lMapSummary || !m_pPlayerStats || !m_pPlayerAvatar || !m_lPlayerName ||
//        !m_lPlayerMapRank || !m_lPlayerGlobalRank || !m_pLeaderboards || !m_pOnlineLeaderboards || !m_pLocalBests)
//    {
//        Assert("Null pointer(s) on scoreboards");
//    }
//    
//
//    m_lMapSummary->SetParent(m_pHeader);
//    m_pPlayerAvatar->SetParent(m_pPlayerStats);
//    m_lPlayerName->SetParent(m_pPlayerStats);
//    m_lPlayerMapRank->SetParent(m_pPlayerStats);
//    m_lPlayerGlobalRank->SetParent(m_pPlayerStats);
//    m_pOnlineLeaderboards->SetParent(m_pLeaderboards);
//    m_pLocalBests->SetParent(m_pLeaderboards);
//
//    m_pOnlineLeaderboards->SetVerticalScrollbar(false);
//    m_pLocalBests->SetVerticalScrollbar(false);
//
//    m_iDesiredHeight = GetTall();
//    m_pHeader->SetVisible(false); // hide this until we load everything in applyschemesettings
//    m_pPlayerStats->SetVisible(false); // hide this until we load everything in applyschemesettings
//    m_pLeaderboards->SetVisible(false); // hide this until we load everything in applyschemesettings
//
//    m_HLTVSpectators = 0;
//    m_ReplaySpectators = 0;
//
//    // update scoreboard instantly if on of these events occure
//    ListenForGameEvent("hltv_status");
//    ListenForGameEvent("server_spawn");
//    //Momentum specific
//    ListenForGameEvent("runtime_posted");
//
//    m_pImageList = NULL;
//
//    m_mapAvatarsToImageList.SetLessFunc(DefLessFunc(CSteamID));
//    m_mapAvatarsToImageList.RemoveAll();
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Constructor
////-----------------------------------------------------------------------------
//CClientTimesDisplay::~CClientTimesDisplay()
//{
//    if (NULL != m_pImageList)
//    {
//        delete m_pImageList;
//        m_pImageList = NULL;
//    }
//
//    //// Kill children before parents
//
//    //if (NULL != m_lMapSummary)
//    //{
//    //    delete m_lMapSummary;
//    //    m_lMapSummary = NULL;
//    //}
//    //if (NULL != m_pHeader)
//    //{
//    //    delete m_pHeader;
//    //    m_pHeader = NULL;
//    //}
//
//    //if (NULL != m_lPlayerName)
//    //{
//    //    delete m_lPlayerName;
//    //    m_lPlayerName = NULL;
//    //}
//    //if (NULL != m_lPlayerMapRank)
//    //{
//    //    delete m_lPlayerMapRank;
//    //    m_lPlayerMapRank = NULL;
//    //}
//    //if (NULL != m_lPlayerGlobalRank)
//    //{
//    //    delete m_lPlayerGlobalRank;
//    //    m_lPlayerGlobalRank = NULL;
//    //}
//    //if (NULL != m_pPlayerStats)
//    //{
//    //    delete m_pPlayerStats;
//    //    m_pPlayerStats = NULL;
//    //}
//
//    //if (NULL != m_pOnlineLeaderboards)
//    //{
//    //    delete m_pOnlineLeaderboards;
//    //    m_pOnlineLeaderboards = NULL;
//    //    
//    //}
//    //if (NULL != m_pLocalBests)
//    //{
//    //    delete m_pLocalBests;
//    //    m_pLocalBests = NULL;
//    //}
//    //if (NULL != m_pLeaderboards)
//    //{
//    //    delete m_pLeaderboards;
//    //    m_pLeaderboards = NULL;
//    //}
//
//    //// And now the KV for the data.
//    //if (NULL != m_kvPlayerData)
//    //{
//    //    m_kvPlayerData->deleteThis();
//    //    m_kvPlayerData = NULL;
//    //}
//}
//
////-----------------------------------------------------------------------------
//// Call every frame
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::OnThink()
//{
//    BaseClass::OnThink();
//
//    // NOTE: this is necessary because of the way input works.
//    // If a key down message is sent to vgui, then it will get the key up message
//    // Sometimes the scoreboard is activated by other vgui menus, 
//    // sometimes by console commands. In the case where it's activated by
//    // other vgui menus, we lose the key up message because this panel
//    // doesn't accept keyboard input. It *can't* accept keyboard input
//    // because another feature of the dialog is that if it's triggered
//    // from within the game, you should be able to still run around while
//    // the scoreboard is up. That feature is impossible if this panel accepts input.
//    // because if a vgui panel is up that accepts input, it prevents the engine from
//    // receiving that input. So, I'm stuck with a polling solution.
//    // 
//    // Close key is set to non-invalid when something other than a keybind
//    // brings the scoreboard up, and it's set to invalid as soon as the 
//    // dialog becomes hidden.
//    if (m_nCloseKey != BUTTON_CODE_INVALID)
//    {
//        if (!g_pInputSystem->IsButtonDown(m_nCloseKey))
//        {
//            m_nCloseKey = BUTTON_CODE_INVALID;
//            gViewPortInterface->ShowPanel(PANEL_SCOREBOARD, false);
//            GetClientVoiceMgr()->StopSquelchMode();
//        }
//    }
//}
//
////-----------------------------------------------------------------------------
//// Called by vgui panels that activate the client scoreboard
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::OnPollHideCode(int code)
//{
//    m_nCloseKey = (ButtonCode_t)code;
//}
//
////-----------------------------------------------------------------------------
//// Purpose: clears everything in the scoreboard and all it's state
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::Reset(bool pUpdateLists)
//{
//    if (pUpdateLists)
//    {
//        // clear
//        if (m_pOnlineLeaderboards)
//        {
//            m_pOnlineLeaderboards->DeleteAllItems();
//            m_pOnlineLeaderboards->RemoveAll();
//        }
//        if (m_pLocalBests)
//        {
//            m_pLocalBests->DeleteAllItems();
//            m_pLocalBests->RemoveAll();
//        }
//        // add all the sections
//        InitScoreboardSections();
//    }
//    m_iSectionId = 0;
//    m_fNextUpdateTime = 0;
//}
//
//void CClientTimesDisplay::Reset()
//{
//    Reset(false);
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Fills the leaderboards lists
//// MOM_TODO: Implement
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::InitScoreboardSections()
//{
//
//}
//
////-----------------------------------------------------------------------------
//// Purpose: sets up screen
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::ApplySchemeSettings(IScheme *pScheme)
//{
//    
//    BaseClass::ApplySchemeSettings(pScheme);
//    
//    if (m_pImageList)
//        delete m_pImageList;
//    m_pImageList = new ImageList(false);
//    
//    m_mapAvatarsToImageList.RemoveAll();
//    
//    PostApplySchemeSettings(pScheme);
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Does dialog-specific customization after applying scheme settings.
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::PostApplySchemeSettings(IScheme *pScheme)
//{
//    // resize the images to our resolution
//    for (int i = 0; i < m_pImageList->GetImageCount(); i++)
//    {
//        int wide, tall;
//        m_pImageList->GetImage(i)->GetSize(wide, tall);
//        m_pImageList->GetImage(i)->SetSize(scheme()->GetProportionalScaledValueEx(GetScheme(), wide), scheme()->GetProportionalScaledValueEx(GetScheme(), tall));
//    }
//    // MOM_TODO: Which one is the Player's avatar?
//    m_pPlayerAvatar->SetImage(m_pImageList->GetImage(0));
//    // Now that the image is loaded, we display the whole thing
//    if (m_pHeader)
//     m_pHeader->SetVisible(true); // hide this until we load everything in applyschemesettings
//    if (m_pPlayerStats)
//        m_pPlayerStats->SetVisible(true); // hide this until we load everything in applyschemesettings
//    if (m_pLeaderboards)
//        m_pLeaderboards->SetVisible(true); // hide this until we load everything in applyschemesettings
//    
//    // light up scoreboard a bit
//    SetBgColor(Color(0, 0, 0, 0));
//}
//
////-----------------------------------------------------------------------------
//// Purpose: 
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::ShowPanel(bool bShow)
//{
//    // Catch the case where we call ShowPanel before ApplySchemeSettings, eg when
//    // going from windowed <-> fullscreen
//    if (m_pImageList == NULL)
//    {
//        InvalidateLayout(true, true);
//    }
//
//    if (!bShow)
//    {
//        m_nCloseKey = BUTTON_CODE_INVALID;
//    }
//
//    if (BaseClass::IsVisible() == bShow)
//        return;
//
//    if (bShow)
//    {
//        Reset(true);
//        Update(false);
//        SetVisible(true);
//        MoveToFront();
//    }
//    else
//    {
//        BaseClass::SetVisible(false);
//        SetMouseInputEnabled(false);
//        SetKeyBoardInputEnabled(false);
//    }
//}
//
//void CClientTimesDisplay::FireGameEvent(IGameEvent *event)
//{
//    const char * type = event->GetName();
//
//    if (Q_strcmp(type, "hltv_status") == 0)
//    {
//        // spectators = clients - proxies
//        m_HLTVSpectators = event->GetInt("clients");
//        m_HLTVSpectators -= event->GetInt("proxies");
//    }
//    else if (Q_strcmp(type, "server_spawn") == 0)
//    {
//        // We'll post the message ourselves instead of using SetControlString()
//        // so we don't try to translate the hostname.
//        const char *hostname = event->GetString("hostname");
//        Panel *control = FindChildByName("ServerName");
//        if (control)
//        {
//            PostMessage(control, new KeyValues("SetText", "text", hostname));
//            control->MoveToFront();
//        }
//    }
//    else if (Q_strcmp(type, "runtime_posted") == 0)
//    {
//        // MOM_TODO(ish): Update the scoreboard when a new run is submited
//    }
//
//    if (IsVisible())
//        Update(true);
//
//}
//
//bool CClientTimesDisplay::NeedsUpdate(void)
//{
//    return (m_fNextUpdateTime < gpGlobals->curtime);
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Recalculate the internal scoreboard data
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::Update(bool pFullUpdate)
//{
//    Reset(pFullUpdate);
//
//    FillScoreBoard(pFullUpdate);
//
//    // grow the scoreboard to fit all the players
//   /* int wide, tall;
//    m_pPlayerList->GetContentSize(wide, tall);
//    tall += GetAdditionalHeight();
//    wide = GetWide();
//    if (m_iDesiredHeight < tall)
//    {
//        SetSize(wide, tall);
//        m_pPlayerList->SetSize(wide, tall);
//    }
//    else
//    {
//        SetSize(wide, m_iDesiredHeight);
//        m_pPlayerList->SetSize(wide, m_iDesiredHeight);
//    }*/
//
//    MoveToCenterOfScreen();
//
//    // update every 2 seconds.
//    // Maybe we can increase this number (We don't really need to update too often)
//    // MOM_TODO: Think about update interval
//    m_fNextUpdateTime = gpGlobals->curtime + 2.0f;
//}
//
//void CClientTimesDisplay::Update()
//{
//    Update(false);
//}
//
////-----------------------------------------------------------------------------
//// Purpose: 
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::UpdatePlayerInfo()
//{
//    for (int i = 1; i <= gpGlobals->maxClients; ++i)
//    {
//        IGameResources *gr = GameResources();
//
//        if (gr && gr->IsConnected(i))
//        {
//            // add the player to the list
//            KeyValues *playerData = new KeyValues("data");
//            UpdatePlayerAvatar(i, playerData);
//
//            const char *oldName = playerData->GetString("name", "");
//            char newName[MAX_PLAYER_NAME_LENGTH];
//            UTIL_MakeSafeName(oldName, newName, MAX_PLAYER_NAME_LENGTH);
//            playerData->SetString("name", newName);
//
//            playerData->SetInt("globalRank", -1);
//            playerData->SetInt("globalCount", -1);
//            playerData->SetInt("mapRank", -1);
//            playerData->SetInt("mapCount", -1);
//
//            m_kvPlayerData = playerData;
//            playerData->deleteThis();
//        }// else, we're not connected. We simply can't be here and disconnected unless something is fucked up
//    }
//}
//
////-----------------------------------------------------------------------------
//// Purpose: adds the top header of the scoreboars
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::AddHeader()
//{
//    //// MOM_TODO: Implement map & gamemode detection
//    //Q_strcat(ch, "(", 512);
//    //Q_strcat(ch, "Gamemode", 512);
//    //Q_strcat(ch, ")", 512);
//    if (m_lMapSummary)
//    {
//        char *ch = "mapname";
//        m_lMapSummary->SetText(ch);
//        // add the top header
//        m_lMapSummary->SetVisible(true);
//    }
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Adds a new section to the scoreboard (i.e the team header)
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::AddSection(int teamType, int teamNumber)
//{
//    //if (teamType == TYPE_TEAM)
//    //{
//    //    IGameResources *gr = GameResources();
//
//    //    if (!gr)
//    //        return;
//
//    //    // setup the team name
//    //    wchar_t *teamName = g_pVGuiLocalize->Find(gr->GetTeamName(teamNumber));
//    //    wchar_t name[64];
//    //    wchar_t string1[1024];
//
//    //    if (!teamName)
//    //    {
//    //        g_pVGuiLocalize->ConvertANSIToUnicode(gr->GetTeamName(teamNumber), name, sizeof(name));
//    //        teamName = name;
//    //    }
//
//    //    g_pVGuiLocalize->ConstructString(string1, sizeof(string1), g_pVGuiLocalize->Find("#Player"), 2, teamName);
//
//    //    m_pPlayerList->AddSection(m_iSectionId, "", StaticPlayerSortFunc);
//
//    //    // Avatars are always displayed at 32x32 regardless of resolution
//    //    if (ShowAvatars())
//    //    {
//    //        m_pPlayerList->AddColumnToSection(m_iSectionId, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, m_iAvatarWidth);
//    //    }
//
//    //    m_pPlayerList->AddColumnToSection(m_iSectionId, "name", string1, 0, scheme()->GetProportionalScaledValueEx(GetScheme(), NAME_WIDTH) - m_iAvatarWidth);
//    //    m_pPlayerList->AddColumnToSection(m_iSectionId, "frags", "", 0, scheme()->GetProportionalScaledValueEx(GetScheme(), SCORE_WIDTH));
//    //    m_pPlayerList->AddColumnToSection(m_iSectionId, "deaths", "", 0, scheme()->GetProportionalScaledValueEx(GetScheme(), DEATH_WIDTH));
//    //    m_pPlayerList->AddColumnToSection(m_iSectionId, "ping", "", 0, scheme()->GetProportionalScaledValueEx(GetScheme(), PING_WIDTH));
//    //}
//    //else if (teamType == TYPE_SPECTATORS)
//    //{
//    //    m_pPlayerList->AddSection(m_iSectionId, "");
//
//    //    // Avatars are always displayed at 32x32 regardless of resolution
//    //    if (ShowAvatars())
//    //    {
//    //        m_pPlayerList->AddColumnToSection(m_iSectionId, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, m_iAvatarWidth);
//    //    }
//    //    m_pPlayerList->AddColumnToSection(m_iSectionId, "name", "#Spectators", 0, scheme()->GetProportionalScaledValueEx(GetScheme(), NAME_WIDTH) - m_iAvatarWidth);
//    //    m_pPlayerList->AddColumnToSection(m_iSectionId, "frags", "", 0, scheme()->GetProportionalScaledValueEx(GetScheme(), SCORE_WIDTH));
//    //}
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Used for sorting players
////-----------------------------------------------------------------------------
//bool CClientTimesDisplay::StaticPlayerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2)
//{
//    //KeyValues *it1 = list->GetItemData(itemID1);
//    //KeyValues *it2 = list->GetItemData(itemID2);
//    //Assert(it1 && it2);
//
//    //// first compare frags
//    //int v1 = it1->GetInt("frags");
//    //int v2 = it2->GetInt("frags");
//    //if (v1 > v2)
//    //    return true;
//    //else if (v1 < v2)
//    //    return false;
//
//    //// next compare deaths
//    //v1 = it1->GetInt("deaths");
//    //v2 = it2->GetInt("deaths");
//    //if (v1 > v2)
//    //    return false;
//    //else if (v1 < v2)
//    //    return true;
//
//    //// the same, so compare itemID's (as a sentinel value to get deterministic sorts)
//    return itemID1 < itemID2;
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Adds a new row to the scoreboard, from the playerinfo structure
////-----------------------------------------------------------------------------
//bool CClientTimesDisplay::GetPlayerScoreInfo(int playerIndex, KeyValues *kv)
//{
//    //IGameResources *gr = GameResources();
//
//    //if (!gr)
//    //    return false;
//
//    //kv->SetInt("deaths", gr->GetDeaths(playerIndex));
//    //kv->SetInt("frags", gr->GetFrags(playerIndex));
//    //kv->SetInt("ping", gr->GetPing(playerIndex));
//    //kv->SetString("name", gr->GetPlayerName(playerIndex));
//    //kv->SetInt("playerIndex", playerIndex);
//
//    return true;
//}
//
////-----------------------------------------------------------------------------
//// Purpose: 
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::UpdatePlayerAvatar(int playerIndex, KeyValues *kv)
//{
//    // Update their avatar
//    if (kv && ShowAvatars() && steamapicontext->SteamFriends() && steamapicontext->SteamUtils())
//    {
//        player_info_t pi;
//        if (engine->GetPlayerInfo(playerIndex, &pi))
//        {
//            if (pi.friendsID)
//            {
//                CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
//
//                // See if we already have that avatar in our list
//                int iMapIndex = m_mapAvatarsToImageList.Find(steamIDForPlayer);
//                int iImageIndex;
//                if (iMapIndex == m_mapAvatarsToImageList.InvalidIndex())
//                {
//                    CAvatarImage *pImage = new CAvatarImage();
//                    pImage->SetAvatarSteamID(steamIDForPlayer);
//                    pImage->SetAvatarSize(32, 32);	// Deliberately non scaling
//                    iImageIndex = m_pImageList->AddImage(pImage);
//
//                    m_mapAvatarsToImageList.Insert(steamIDForPlayer, iImageIndex);
//                }
//                else
//                {
//                    iImageIndex = m_mapAvatarsToImageList[iMapIndex];
//                }
//
//                kv->SetInt("avatar", iImageIndex);
//
//                CAvatarImage *pAvIm = (CAvatarImage *)m_pImageList->GetImage(iImageIndex);
//                pAvIm->UpdateFriendStatus();
//            }
//        }
//    }
//}
//
////-----------------------------------------------------------------------------
//// Purpose: reload the player list on the scoreboard
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::FillScoreBoard(bool pFullUpdate)
//{
//    // update player info
//    if (pFullUpdate)
//        UpdatePlayerInfo();
//    if (m_lPlayerName)
//        m_lPlayerName->SetText(m_kvPlayerData->GetString("name","Undefined"));
//
//    if (m_lPlayerMapRank)
//    {
//        char *mapRank = "Map rank:";
//        /* Q_strcat(mapRank,(const char *)m_kvPlayerData->GetInt("mapRank", -1), 50);
//        Q_strcat(mapRank, "/", 50);
//        Q_strcat(mapRank, (const char *)m_kvPlayerData->GetInt("mapCount", -1), 50);*/
//        m_lPlayerMapRank->SetText(mapRank);
//    }
//
//    if (m_lPlayerGlobalRank)
//    {
//        char *globalRank = "Global rank:";
//        /*Q_strcat(mapRank, (const char *)m_kvPlayerData->GetInt("globalRank", -1), 50);
//        Q_strcat(mapRank, "/", 50);
//        Q_strcat(mapRank, (const char *)m_kvPlayerData->GetInt("globalCount", -1), 50);*/
//        m_lPlayerGlobalRank->SetText(globalRank);
//    }
//    if (pFullUpdate)
//        Reset(true);
//}
//
////-----------------------------------------------------------------------------
//// Purpose: searches for the player in the scoreboard
////-----------------------------------------------------------------------------
//int CClientTimesDisplay::FindItemIDForPlayerIndex(int playerIndex)
//{
//    return 0;
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Sets the text of a control by name
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::MoveLabelToFront(const char *textEntryName)
//{
//    Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
//    if (entry)
//    {
//        entry->MoveToFront();
//    }
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Center the dialog on the screen.  (vgui has this method on
////			Frame, but we're an EditablePanel, need to roll our own.)
////-----------------------------------------------------------------------------
//void CClientTimesDisplay::MoveToCenterOfScreen()
//{
//    int wx, wy, ww, wt;
//    surface()->GetWorkspaceBounds(wx, wy, ww, wt);
//    SetPos((ww - GetWide()) / 2, (wt - GetTall()) / 2);
//}