#include "cbase.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include "menu.h"

using namespace vgui;

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/AnimationController.h>

#include "vgui_helpers.h"
#include "momentum/util/mom_util.h"

#include "tier0/memdbgon.h"

#define BUFSIZETIME (sizeof("00:00:00.0000")+1)
#define BUFSIZELOCL (73)

static ConVar mom_timer("mom_timer", "1",
    FCVAR_DONTRECORD | FCVAR_CLIENTDLL | FCVAR_ARCHIVE,
    "Turn the timer display on/off\n");

static ConVar timer_mode("mom_timer_mode", "0", FCVAR_DONTRECORD | FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_REPLICATED,
    "Set what type of timer you want.\n0 = Generic Timer (no splits)\n1 = Splits by Checkpoint\n");

class C_Timer : public CHudElement, public Panel
{
    DECLARE_CLASS_SIMPLE(C_Timer, Panel);
public:
    C_Timer();
    C_Timer(const char *pElementName);
    virtual void Init();
    virtual void Reset();
    virtual void OnThink();
    virtual bool ShouldDraw()
    {
        return mom_timer.GetBool() && CHudElement::ShouldDraw();
    }
    void MsgFunc_Timer_State(bf_read &msg);
    void MsgFunc_Timer_Reset(bf_read &msg);
    void MsgFunc_Timer_Checkpoint(bf_read &msg);
    void MsgFunc_Timer_Stage(bf_read &msg);
    void MsgFunc_Timer_StageCount(bf_read&);
    virtual void Paint();
    int GetCurrentTime();
    bool m_bIsRunning;
    int m_iStartTick;

private:
    int m_iStageCurrent;
    int m_iStageCount;
    int initialTall;
    wchar_t m_pwCurrentTime[BUFSIZETIME];
    char m_pszString[BUFSIZETIME];
    wchar_t m_pwCurrentCheckpoints[BUFSIZELOCL];
    char m_pszStringCps[BUFSIZELOCL];
    wchar_t m_pwCurrentStages[BUFSIZELOCL];
    char m_pszStringStages[BUFSIZELOCL];
    CUtlMap<const char*, float> map;
    int m_iTotalTicks;
    bool m_bWereCheatsActivated = false;
    bool m_bShowCheckpoints;
    int m_iCheckpointCount;
    int m_iCheckpointCurrent;

protected:
    CPanelAnimationVar(float, m_flBlur, "Blur", "0");
    CPanelAnimationVar(Color, m_TextColor, "TextColor", "FgColor");
    CPanelAnimationVar(Color, m_Ammo2Color, "Ammo2Color", "FgColor");

    CPanelAnimationVar(HFont, m_hNumberFont, "NumberFont", "HudNumbers");
    CPanelAnimationVar(HFont, m_hNumberGlowFont, "NumberGlowFont",
        "HudNumbersGlow");
    CPanelAnimationVar(HFont, m_hSmallNumberFont, "SmallNumberFont",
        "HudNumbersSmall");
    CPanelAnimationVar(HFont, m_hTextFont, "TextFont", "Default");

    CPanelAnimationVarAliasType(bool, center_time, "centerTime", "1",
        "BOOL");
    CPanelAnimationVarAliasType(float, time_xpos, "time_xpos", "50",
        "proportional_float");
    CPanelAnimationVarAliasType(float, time_ypos, "time_ypos", "2",
        "proportional_float");
    CPanelAnimationVarAliasType(bool, center_cps, "centerCps", "1",
        "BOOL");
    CPanelAnimationVarAliasType(float, cps_xpos, "cps_xpos", "50",
        "proportional_float");
    CPanelAnimationVarAliasType(float, cps_ypos, "cps_ypos", "25",
        "proportional_float");
    CPanelAnimationVarAliasType(bool, center_stage, "centerStage", "1",
        "BOOL");
    CPanelAnimationVarAliasType(float, stage_xpos, "stage_xpos", "50",
        "proportional_float");
    CPanelAnimationVarAliasType(float, stage_ypos, "stage_ypos", "40",
        "proportional_float");
};

DECLARE_HUDELEMENT(C_Timer);
// MOM_TODO add more for checkpoints and ending
DECLARE_HUD_MESSAGE(C_Timer, Timer_State);
DECLARE_HUD_MESSAGE(C_Timer, Timer_Reset);
DECLARE_HUD_MESSAGE(C_Timer, Timer_Checkpoint);
DECLARE_HUD_MESSAGE(C_Timer, Timer_Stage);
DECLARE_HUD_MESSAGE(C_Timer, Timer_StageCount);

C_Timer::C_Timer(const char *pElementName) :
CHudElement(pElementName), Panel(g_pClientMode->GetViewport(), "HudTimer")
{
    // This is already set for HUD elements, but still...
    SetProportional(true);
    SetKeyBoardInputEnabled(false);
    SetMouseInputEnabled(false);
    SetHiddenBits(HIDEHUD_WEAPONSELECTION);
}

void C_Timer::Init()
{
    HOOK_HUD_MESSAGE(C_Timer, Timer_State);
    HOOK_HUD_MESSAGE(C_Timer, Timer_Reset);
    HOOK_HUD_MESSAGE(C_Timer, Timer_Checkpoint);
    HOOK_HUD_MESSAGE(C_Timer, Timer_Stage);
    HOOK_HUD_MESSAGE(C_Timer, Timer_StageCount);
    initialTall = 48;
    m_iTotalTicks = 0;
    //Reset();
}

void C_Timer::Reset()
{
    m_bIsRunning = false;
    m_iTotalTicks = 0;
    m_iStageCount = 0;
    m_iStageCurrent = 0;
    m_bShowCheckpoints = false;
    m_iCheckpointCount = 0;
    m_iCheckpointCurrent = 0;
}

void C_Timer::OnThink()
{
    if (m_iStageCount == 0)
        engine->ServerCmd("hud_timer_request_stages");
    // Cheat detection moved to server Timer.cpp
}

void C_Timer::MsgFunc_Timer_State(bf_read &msg)
{
    bool started = msg.ReadOneBit();
    m_bIsRunning = started;
    m_iStartTick = (int) msg.ReadLong();
    C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
    if (!pPlayer)
        return;
    // MOM_TODO: Create HUD animations for states
    if (started)
    {
        //VGUI_ANIMATE("TimerStart");
        // Checking again, even if we just checked 8 lines before
        if (pPlayer != NULL)
        {
            pPlayer->EmitSound("Momentum.StartTimer");
        }
    }
    else // stopped
    {
        // Compare times.
        if (m_bWereCheatsActivated) //EY, CHEATER, STOP
        {
            Msg("sv_cheats was set to 1, thus making the run not valid \n");
        }
        else //He didn't cheat, we can carry on
        {
            //m_iTotalTicks = gpGlobals->tickcount - m_iStartTick;
            //DevMsg("Ticks upon exit: %i and total seconds: %f\n", m_iTotalTicks, gpGlobals->interval_per_tick);
            //Paint();
            //DevMsg("%s \n", m_pszString);
        }

        //VGUI_ANIMATE("TimerStop");
        if (pPlayer != NULL)
        {
            pPlayer->EmitSound("Momentum.StopTimer");
        }

        //MOM_TODO: (Beta+) show scoreboard animation with new position on leaderboards?
    }
}

void C_Timer::MsgFunc_Timer_Reset(bf_read &msg)
{
    Reset();
}

void C_Timer::MsgFunc_Timer_Checkpoint(bf_read &msg)
{
    m_bShowCheckpoints = msg.ReadOneBit();
    m_iCheckpointCurrent = (int) msg.ReadLong();
    m_iCheckpointCount = (int) msg.ReadLong();
}

void C_Timer::MsgFunc_Timer_Stage(bf_read &msg)
{
    m_iStageCurrent = (int) msg.ReadLong();
    //g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuPulse");
}

void C_Timer::MsgFunc_Timer_StageCount(bf_read &msg)
{
    m_iStageCount = (int) msg.ReadLong();
}

int C_Timer::GetCurrentTime()
{
    if (m_bIsRunning) m_iTotalTicks = gpGlobals->tickcount - m_iStartTick;
    return m_iTotalTicks;
}

void C_Timer::Paint(void)
{
    mom_UTIL.FormatTime(GetCurrentTime(), gpGlobals->interval_per_tick, m_pszString);
    
    //float m_flSecondsTime = ((float) GetCurrentTime()) * gpGlobals->interval_per_tick;

    /*int hours = m_flSecondsTime / (60.0f * 60.0f);
    int minutes = fmod(m_flSecondsTime / 60.0f, 60.0f);
    int seconds = fmod(m_flSecondsTime, 60.0f);
    int millis = fmod(m_flSecondsTime, 1.0f) * 1000.0f;

    Q_snprintf(m_pszString, sizeof(m_pszString), "%02d:%02d:%02d.%03d",
        hours, //hours
        minutes, //minutes
        seconds, //seconds
        millis //millis
        );*/


    g_pVGuiLocalize->ConvertANSIToUnicode(
        m_pszString, m_pwCurrentTime, sizeof(m_pwCurrentTime));

    if (m_bShowCheckpoints)
    {
        char cpLocalized[BUFSIZELOCL];
        wchar_t *uCPUnicode = g_pVGuiLocalize->Find("#MOM_Checkpoint");
        g_pVGuiLocalize->ConvertUnicodeToANSI(uCPUnicode ? uCPUnicode : L"#MOM_Checkpoint", cpLocalized, BUFSIZELOCL);

        Q_snprintf(m_pszStringCps, sizeof(m_pszStringCps), "%s %i/%i",
            cpLocalized, // Checkpoint localization
            m_iCheckpointCurrent, //CurrentCP
            m_iCheckpointCount //CPCount
            );
        g_pVGuiLocalize->ConvertANSIToUnicode(
            m_pszStringCps, m_pwCurrentCheckpoints, sizeof(m_pwCurrentCheckpoints));
    }
    if (m_iStageCount > 1)
    {
        char stLocalized[BUFSIZELOCL];
        wchar_t *uStageUnicode = g_pVGuiLocalize->Find("#MOM_Stage");
        g_pVGuiLocalize->ConvertUnicodeToANSI(uStageUnicode ? uStageUnicode : L"#MOM_Stage", stLocalized, BUFSIZELOCL);
        Q_snprintf(m_pszStringStages, sizeof(m_pszStringStages), "%s %i/%i",
            stLocalized, // Stage localization
            m_iStageCurrent, // Current Stage
            m_iStageCount // Total number of stages
            );
    }
    else //it's a linear map
    {
        char linearLocalized[25];
        wchar_t *uLinearUnicode = g_pVGuiLocalize->Find("#MOM_Linear");
        g_pVGuiLocalize->ConvertUnicodeToANSI(uLinearUnicode ? uLinearUnicode : L"#MOM_Linear", linearLocalized, 25);
        Q_snprintf(m_pszStringStages, sizeof(m_pszStringStages), linearLocalized);
    }

    g_pVGuiLocalize->ConvertANSIToUnicode(
        m_pszStringStages, m_pwCurrentStages, sizeof(m_pwCurrentStages));

    // Draw the text label.
    surface()->DrawSetTextFont(m_hTextFont);
    surface()->DrawSetTextColor(GetFgColor());

    // Draw current time.
    int dummy, totalWide;

    GetSize(totalWide, dummy);

    if (center_time)
    {
        int timeWide;
        surface()->GetTextSize(m_hTextFont, m_pwCurrentTime, timeWide, dummy);
        int offsetToCenter = ((totalWide - timeWide) / 2);
        surface()->DrawSetTextPos(offsetToCenter, time_ypos);
    }
    else
    {
        surface()->DrawSetTextPos(time_xpos, time_ypos);
    }
    surface()->DrawPrintText(m_pwCurrentTime, wcslen(m_pwCurrentTime));

    if (m_bShowCheckpoints)
    {
        if (center_cps)
        {
            int cpsWide;
            surface()->GetTextSize(m_hTextFont, m_pwCurrentCheckpoints, cpsWide, dummy);
            int offsetToCenter = ((totalWide - cpsWide) / 2);
            surface()->DrawSetTextPos(offsetToCenter, cps_ypos);
        }
        else
            surface()->DrawSetTextPos(cps_xpos, cps_ypos);

        surface()->DrawPrintText(m_pwCurrentCheckpoints, wcslen(m_pwCurrentCheckpoints));
    }

    // MOM_TODO: Print this only if map gamemode is supported
    if (center_stage)
    {
        int stageWide;
        surface()->GetTextSize(m_hTextFont, m_pwCurrentStages, stageWide, dummy);
        int offsetToCenter = ((totalWide - stageWide) / 2);
        surface()->DrawSetTextPos(offsetToCenter, stage_ypos);
    }
    else
        surface()->DrawSetTextPos(stage_xpos, stage_ypos);

    surface()->DrawPrintText(m_pwCurrentStages, wcslen(m_pwCurrentStages));

}