#ifndef TIMER_H
#define TIMER_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "momentum/tickset.h"
#include "KeyValues.h"
#include "momentum/util/mom_util.h"
#include "filesystem.h"
#include "mom_triggers.h"
#include "GameEventListener.h"
#include "tier1/checksum_sha1.h"
#include "momentum/mom_shareddefs.h"
#include "momentum/mom_gamerules.h"
#include "movevars_shared.h"
#include <ctime>

class CTriggerTimerStart;
class CTriggerCheckpoint;
class CTriggerOnehop;
class CTriggerStage;

class CTimer
{
public:

    //-------- HUD Messages --------------------
    void DispatchStateMessage();
    void DispatchResetMessage();
    void DispatchCheckpointMessage();
    void DispatchStageMessage();
    void DispatchStageCountMessage();


    // ------------- Timer state related messages --------------------------
    // Strats the timer for the given starting tick
    void Start(int startTick);
    // Stops the timer
    void Stop(bool = false);
    // Is the timer running?
    bool IsRunning() { return m_bIsRunning; }
    // Set the running status of the timer
    void SetRunning(bool running) { m_bIsRunning = running; }

    // ------------- Timer trigger related methods ----------------------------
    // Gets the current starting trigger
    CTriggerTimerStart *GetStartTrigger() { return m_pStartTrigger.Get(); }
    // Gets the current checkpoint
    CTriggerCheckpoint *GetCurrentCheckpoint() { return m_pCurrentCheckpoint.Get(); }

    // Sets the given trigger as the start trigger
    void SetStartTrigger(CTriggerTimerStart *pTrigger) { m_pStartTrigger.Set(pTrigger); }

    // Sets the current checkpoint
    void SetCurrentCheckpointTrigger(CTriggerCheckpoint *pTrigger) { m_pCurrentCheckpoint.Set(pTrigger); }

    void SetCurrentStage(CTriggerStage *pTrigger)
    {
        m_pCurrentStage.Set(pTrigger);
        DispatchStageMessage();
    }
    CTriggerStage *GetCurrentStage() { return m_pCurrentStage.Get(); }

    // Calculates the stage count
    // Stores the result on m_iStageCount
    void RequestStageCount();
    // Gets the total stage count
    int GetStageCount() { return m_iStageCount; };

    //--------- CheckpointMenu stuff --------------------------------
    // Gets the current menu checkpoint index
    int GetCurrentCPMenuStep() { return m_iCurrentStepCP; }
    // MOM_TODO: For leaderboard use later on
    bool IsUsingCPMenu() { return m_bUsingCPMenu; }
    // Creates a checkpoint (menu) on the location of the given Entity
    void CreateCheckpoint(CBasePlayer*);
    // Removes last checkpoint (menu) form the checkpoint lists
    void RemoveLastCheckpoint();
    // Removes every checkpoint (menu) on the checkpoint list
    void RemoveAllCheckpoints()
    {
        checkpoints.RemoveAll();
        m_iCurrentStepCP = -1;
        //SetUsingCPMenu(false);
        DispatchCheckpointMessage();
    }
    // Teleports the entity to the checkpoint (menu) with the given index
    void TeleportToCP(CBasePlayer*, int);
    // Sets the current checkpoint (menu) to the desired one with that index
    void SetCurrentCPMenuStep(int pNewNum);
    // Gets the total amount of menu checkpoints
    int GetCPCount() { return checkpoints.Size(); }
    // Sets wheter or not we're using the CPMenu
    // WARNING! No verification is done. It is up to the caller to don't give false information
    void SetUsingCPMenu(bool pIsUsingCPMenu);

    //----- Trigger_Onehop stuff -----------------------------------------
    // Removes the given Onehop form the hopped list.
    // Returns: True if deleted, False if not found.
    bool RemoveOnehopFromList(CTriggerOnehop* pTrigger);
    // Adds the give Onehop to the hopped list.
    // Returns: Its new index.
    int AddOnehopToListTail(CTriggerOnehop* pTrigger);
    // Finds a Onehop on the hopped list.
    // Returns: Its index. -1 if not found
    int FindOnehopOnList(CTriggerOnehop* pTrigger);
    // Removes all onehops from the list
    void RemoveAllOnehopsFromList() { onehops.RemoveAll(); }
    // Returns the count for the onehop list
    int GetOnehopListCount() { return onehops.Count(); }
    // Finds the onehop with the given index on the list
    CTriggerOnehop* FindOnehopOnList(int pIndexOnList);


    //-------- Online-related timer commands -----------------------------
    // Tries to post the current time.
    void PostTime();
    //MOM_TODO: void LoadOnlineTimes();


    //------- Local-related timer commands -------------------------------
    // Loads local times from given map name
    void LoadLocalTimes(const char*);
    // Saves current time to a local file
    void SaveTime();
    void OnMapEnd(const char *);
    void OnMapStart(const char *);

    // Practice mode- noclip mode that stops timer
    void PracticeMove();
    void EnablePractice(CBasePlayer *pPlayer);
    void DisablePractice(CBasePlayer *pPlayer);
    bool IsPracticeMode(CBaseEntity *pOther);

    // Have the cheats been turned on in this session?
    bool GotCaughtCheating() { return m_bWereCheatsActivated; };
    void SetCheating(bool newBool)
    {
        UTIL_ShowMessage("CHEATER", UTIL_GetLocalPlayer());
        Stop(false);
        m_bWereCheatsActivated = newBool;
    }

    void SetGameModeConVars();

private:

    int m_iStageCount;
    int m_iStartTick;
    bool m_bIsRunning;
    bool m_bWereCheatsActivated;
    CHandle<CTriggerTimerStart> m_pStartTrigger;
    CHandle<CTriggerCheckpoint> m_pCurrentCheckpoint;
    CHandle<CTriggerStage> m_pCurrentStage;

    struct Time
    {
        //The amount of ticks took to complete
        int ticks;
        //Tickrate the run was done on
        float tickrate;
        //Date achieved
        time_t date;
    };

    struct Checkpoint
    {
        Vector pos;
        Vector vel;
        QAngle ang;
    };
    CUtlVector<Checkpoint> checkpoints;
    CUtlVector<CTriggerOnehop*> onehops;
    CUtlVector<Time> localTimes;
    //MOM_TODO: CUtlVector<OnlineTime> onlineTimes;

    int m_iCurrentStepCP = 0;
    bool m_bUsingCPMenu = false;

    const char* c_mapDir = "maps/";
    // Extension used for storing local map times
    const char* c_timesExt = ".tim";
};

extern CTimer g_Timer;

#endif // TIMER_H