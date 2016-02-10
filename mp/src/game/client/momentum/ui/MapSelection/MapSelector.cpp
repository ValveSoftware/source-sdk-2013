#include "pch_mapselection.h"


static CMapSelector g_MapSelectorPanel;
IMapSelector* mapselector = (CMapSelector*)&g_MapSelectorPanel;

//
CMapSelector::CMapSelector()
{
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMapSelector::~CMapSelector()
{
}

static ConVar cl_showmapselection("cl_showmapselection", "0", FCVAR_CLIENTDLL | FCVAR_HIDDEN, "Sets the state of mapselection panel <state>");

CON_COMMAND_F(ToggleMapSelectionPanel, "Toggles MapSelectorPanel", FCVAR_CLIENTDLL | FCVAR_HIDDEN)
{
    if (!cl_showmapselection.GetBool())
    {
        mapselector->Activate();
        cl_showmapselection.SetValue(1);
    }
    else
    {
        cl_showmapselection.SetValue(0);
        mapselector->Deactivate();
    }
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapSelector::Create(vgui::VPANEL parent)
{
    if (!m_hMapsDlg.Get())
    {
        m_hMapsDlg = new CMapSelectorDialog(parent); // SetParent() call below fills this in
        m_hMapsDlg->Initialize();
    }
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapSelector::Activate()
{
    static bool m_bfirstTimeOpening = true;
    if (m_bfirstTimeOpening)
    {
        m_hMapsDlg->LoadUserData(); // reload the user data the first time the dialog is made visible, 
        //helps with the lag between module load and steamui getting Deactivate() call
        m_bfirstTimeOpening = false;
    }

    Open();
}

//-----------------------------------------------------------------------------
// Purpose: called when the server browser gets closed by the enduser
//-----------------------------------------------------------------------------
void CMapSelector::Deactivate()
{
    if (m_hMapsDlg.Get())
    {
        m_hMapsDlg->SaveUserData();
        m_hMapsDlg->Close();
        CloseAllMapInfoDialogs();
    }
}


//-----------------------------------------------------------------------------
// Purpose: called when the server browser is no longer being used in the game
//-----------------------------------------------------------------------------
/*void CMapSelector::Reactivate()
{
    if (m_hInternetDlg.Get())
    {
        m_hInternetDlg->LoadUserData();
        if (m_hInternetDlg->IsVisible())
        {
            m_hInternetDlg->RefreshCurrentPage();
        }
    }
}*/


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapSelector::Open()
{
    m_hMapsDlg->Open();
}


//-----------------------------------------------------------------------------
// Purpose: returns direct handle to main server browser dialog
//-----------------------------------------------------------------------------
/*vgui::VPANEL CMapSelector::GetPanel()
{
    return m_hInternetDlg.Get() ? m_hInternetDlg->GetVPanel() : NULL;
}


//-----------------------------------------------------------------------------
// Purpose: sets the parent panel of the main module panel
//-----------------------------------------------------------------------------
void CMapSelector::SetParent(vgui::VPANEL parent)
{
    if (m_hInternetDlg.Get())
    {
        m_hInternetDlg->SetParent(parent);
    }
}*/


//-----------------------------------------------------------------------------
// Purpose: Closes down the server browser for good
//-----------------------------------------------------------------------------
void CMapSelector::Destroy()
{
    if (m_hMapsDlg.Get())
    {
        m_hMapsDlg->Close();
        m_hMapsDlg->MarkForDeletion();
    }
}


//-----------------------------------------------------------------------------
// Purpose: opens a game info dialog to watch the specified server; associated with the friend 'userName'
//-----------------------------------------------------------------------------
/*bool CMapSelector::OpenGameInfoDialog(uint64 ulSteamIDFriend)
{
#if !defined( _X360 ) // X360TBD: SteamFriends()
    if (m_hInternetDlg.Get())
    {
        // activate an already-existing dialog
        CDialogGameInfo *pDialogGameInfo = m_hInternetDlg->GetDialogGameInfoForFriend(ulSteamIDFriend);
        if (pDialogGameInfo)
        {
            pDialogGameInfo->Activate();
            return true;
        }

        // none yet, create a new dialog
        uint64 nGameID;
        uint32 unGameIP;
        uint16 usGamePort;
        uint16 usQueryPort;
#ifndef NO_STEAM
        if (SteamFriends()->GetFriendGamePlayed(ulSteamIDFriend, &nGameID, &unGameIP, &usGamePort, &usQueryPort))
        {
            uint16 usConnPort = usGamePort;
            if (usQueryPort < QUERY_PORT_ERROR)
                usConnPort = usGamePort;
            CDialogGameInfo *pDialogGameInfo = m_hInternetDlg->OpenGameInfoDialog(unGameIP, usGamePort, usConnPort);
            pDialogGameInfo->SetFriend(ulSteamIDFriend);
            return true;
        }
#endif
    }
#endif
    return false;
}


//-----------------------------------------------------------------------------
// Purpose: joins a specified game - game info dialog will only be opened if the server is fully or passworded
//-----------------------------------------------------------------------------
bool CMapSelector::JoinGame(uint64 ulSteamIDFriend)
{
    if (OpenGameInfoDialog(ulSteamIDFriend))
    {
        CDialogGameInfo *pDialogGameInfo = m_hInternetDlg->GetDialogGameInfoForFriend(ulSteamIDFriend);
        pDialogGameInfo->Connect();
    }

    return false;
}


//-----------------------------------------------------------------------------
// Purpose: joins a game by IP/Port
//-----------------------------------------------------------------------------
bool CMapSelector::JoinGame(uint32 unGameIP, uint16 usGamePort)
{
    m_hInternetDlg->JoinGame(unGameIP, usGamePort);
    return true;
}


//-----------------------------------------------------------------------------
// Purpose: forces the game info dialog closed
//-----------------------------------------------------------------------------
void CMapSelector::CloseGameInfoDialog(uint64 ulSteamIDFriend)
{
    CDialogGameInfo *pDialogGameInfo = m_hInternetDlg->GetDialogGameInfoForFriend(ulSteamIDFriend);
    if (pDialogGameInfo)
    {
        pDialogGameInfo->Close();
    }
}*/


//-----------------------------------------------------------------------------
// Purpose: closes all the game info dialogs
//-----------------------------------------------------------------------------
void CMapSelector::CloseAllMapInfoDialogs()
{
    if (m_hMapsDlg.Get())
    {
        m_hMapsDlg->CloseAllMapInfoDialogs();
    }
}