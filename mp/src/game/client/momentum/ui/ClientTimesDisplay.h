//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CLIENTTIMESDISPLAY_H
#define CLIENTTIMESDISPLAY_H
#ifdef _WIN32
#pragma once
#endif

#include "GameEventListener.h"
#include "cbase.h"
#include "momentum/mom_shareddefs.h"
#include <KeyValues.h>
#include <game/client/iviewport.h>
#include <vgui_controls/EditablePanel.h>

#define TYPE_NOTEAM			0	// NOTEAM must be zero :)
#define TYPE_TEAM			1	// a section for a single team	
#define TYPE_PLAYERS		2
#define TYPE_SPECTATORS		3	// a section for a spectator group
#define TYPE_BLANK			4


//-----------------------------------------------------------------------------
// Purpose: Game ScoreBoard
//-----------------------------------------------------------------------------
class CClientTimesDisplay : public vgui::EditablePanel, public IViewPortPanel, public CGameEventListener
{
private:
    DECLARE_CLASS_SIMPLE(CClientTimesDisplay, vgui::EditablePanel);

protected:
    // column widths at 640
    enum { NAME_WIDTH = 160, SCORE_WIDTH = 60, DEATH_WIDTH = 60, PING_WIDTH = 80, VOICE_WIDTH = 0, FRIENDS_WIDTH = 0 };
    // total = 340

public:
    CClientTimesDisplay(IViewPort *pViewPort);
    ~CClientTimesDisplay();

    virtual const char *GetName(void) { return PANEL_TIMES; }
    virtual void SetData(KeyValues *data) {};
    virtual void Reset();
    virtual void Update();
    void Update(bool pFullUpdate);
    void Reset(bool pFullReset);
    virtual bool NeedsUpdate(void);
    virtual bool HasInputElements(void) { return true; }
    virtual void ShowPanel(bool bShow);

    virtual bool ShowAvatars()
    {
        return IsPC();
    }

    // both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
    vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
    virtual bool IsVisible() { return BaseClass::IsVisible(); }
    virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

    // IGameEventListener interface:
    virtual void FireGameEvent(IGameEvent *event);

    virtual void UpdatePlayerAvatar(int playerIndex, KeyValues *kv);

protected:
    MESSAGE_FUNC_INT(OnPollHideCode, "PollHideCode", code);

    // functions to override
    virtual bool GetPlayerTimes(KeyValues *outPlayerInfo);
    virtual void InitScoreboardSections();
    virtual void UpdateTeamInfo();
    virtual void UpdatePlayerInfo(KeyValues *outPlayerInfo);
    virtual void OnThink();
    virtual void AddHeader(vgui::Label *pMapSummary); // add the start header of the scoreboard
    virtual void AddSection(int teamType, int teamNumber); // add a new section header for a team
    virtual int GetAdditionalHeight() { return 0; }

    // sorts players within a section
    static bool StaticLocalTimeSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2);
    static bool StaticOnlineTimeSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2);

    virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

    virtual void PostApplySchemeSettings(vgui::IScheme *pScheme);

    // finds the player in the scoreboard
    int FindItemIDForPlayerIndex(int playerIndex);
    // finds a local time in the scoreboard
    int FindItemIDForLocalTime(KeyValues *kvRef);

    int m_iNumTeams;

    vgui::SectionedListPanel *m_pPlayerList;
    int				m_iSectionId; // the current section we are entering into

    float m_fNextUpdateTime;

    void MoveLabelToFront(const char *textEntryName);
    void MoveToCenterOfScreen();

    vgui::ImageList	*m_pImageList;
    vgui::Panel *m_pHeader;
    vgui::Panel *m_pPlayerStats;
    vgui::Panel *m_pLeaderboards;
    vgui::Label *m_lMapSummary;
    vgui::Label *m_lPlayerName;
    vgui::Label *m_lPlayerMapRank;
    vgui::Label *m_lPlayerGlobalRank;
    vgui::SectionedListPanel *m_pOnlineLeaderboards;
    vgui::SectionedListPanel *m_pLocalLeaderboards;
    vgui::SectionedListPanel *m_pFriendsLeaderboards;
    vgui::ImagePanel *m_pPlayerAvatar;
    vgui::ImagePanel *m_pMomentumLogo;


    CUtlMap<CSteamID, int>		m_mapAvatarsToImageList;

    CPanelAnimationVar(int, m_iAvatarWidth, "avatar_width", "34");		// Avatar width doesn't scale with resolution
    CPanelAnimationVarAliasType(int, m_iNameWidth, "name_width", "136", "proportional_int");
    CPanelAnimationVarAliasType(int, m_iClassWidth, "class_width", "35", "proportional_int");
    CPanelAnimationVarAliasType(int, m_iScoreWidth, "score_width", "35", "proportional_int");
    CPanelAnimationVarAliasType(int, m_iDeathWidth, "death_width", "35", "proportional_int");
    CPanelAnimationVarAliasType(int, m_iPingWidth, "ping_width", "23", "proportional_int");

private:
    int			m_iPlayerIndexSymbol;
    int			m_iDesiredHeight;
    IViewPort	*m_pViewPort;
    ButtonCode_t m_nCloseKey;
    struct Time
    {
        int ticks;
        float rate;
        time_t date;

        Time(KeyValues* kv)
        {
            ticks = Q_atoi(kv->GetName());
            rate = kv->GetFloat("rate", 0.015);
            date = (time_t) kv->GetInt("date", 0);
        };
    };
    CUtlVector<Time> m_vLocalTimes;
    // MOM_TODO: CUtlVector<SomeStruct> m_vOnlineTimes;

    bool bLocalTimesLoaded = false;
    bool bLocalTimesNeedUpdate = false;
    //widths[0] == WIDTH FOR DATE
    //widths[1] == WIDTH FOR RANK
    //widths[2] == WIDTH FOR TIME
    int m_aiColumnWidths[3];

    // methods
    void FillScoreBoard();
    void FillScoreBoard(bool pFullUpdate);
    void LoadLocalTimes(KeyValues *kv);
    void ConvertLocalTimes(KeyValues*);
};


#endif // CLIENTSCOREBOARDDIALOG_H