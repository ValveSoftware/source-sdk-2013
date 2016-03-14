#ifndef TIMERTRIGGERS_H
#define TIMERTRIGGERS_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "filters.h"
#include "func_break.h"

// spawnflags 
enum {
    //CTriggerTimerStart
    SF_LIMIT_LEAVE_SPEED = 0x0001,          // Limit max leave speed to m_fMaxLeaveSpeed?
    SF_USE_LOOKANGLES = 0x0002,             // Use look angles?
    SF_LIMIT_LEAVE_SPEED_ONLYXY = 0x0004,   // Limit speed without taking into account hvel (Z axis)
    SF_LIMIT_LEAVE_SPEED_BHOP = 0x0008,     // Limit bhop in start zone?
    //CTriggerOneHop
    SF_TELEPORT_RESET_ONEHOP = 0x0010,      // Reset hop state if player hops onto another different onehop
    //CTriggerLimitMove
    LIMIT_JUMP = 0x0020,                    //prevent player from jumping
    LIMIT_CROUCH = 0x0040,                  //prevent player from croching
    LIMIT_BHOP = 0x0080,                    //prevent player from bhopping
    //CFuncShootBost and CTriggerMomentumPush
    SF_PUSH_DIRECTION_AS_FINAL_FORCE = 0x0100,  // Use the direction vector as final force instead of calculating it by force amount
    //CTriggerMomentumPush
    SF_PUSH_ONETOUCH = 0x0200,               // Only allow for one touch
    SF_PUSH_ONSTART = 0x0400,                // Modify player velocity on StartTouch
    SF_PUSH_ONEND = 0x0800,                  // Modify player velocity on EndTouch
};
// CBaseMomentumTrigger
class CBaseMomentumTrigger : public CTriggerMultiple
{
    DECLARE_CLASS(CBaseMomentumTrigger, CTriggerMultiple);

public:
    virtual void Spawn();
};

// CTriggerTimerStop
class CTriggerTimerStop : public CBaseMomentumTrigger
{
    DECLARE_CLASS(CTriggerTimerStop, CBaseMomentumTrigger);

public:
    void StartTouch(CBaseEntity*);
};

// CTriggerTeleportEnt
class CTriggerTeleportEnt : public CBaseMomentumTrigger
{
    DECLARE_CLASS(CTriggerTeleportEnt, CBaseMomentumTrigger);
    DECLARE_DATADESC();

public:
    //This void teleports the touching entity!
    void StartTouch(CBaseEntity*);
    // Used by children classes to define what ent to teleport to (see CTriggerOneHop)
    void SetDestinationEnt(CBaseEntity *ent) { pDestinationEnt = ent; }
    bool ShouldStopPlayer() { return m_bResetVelocity; }
    bool ShouldResetAngles() { return m_bResetAngles; }
    void SetShouldStopPlayer(bool newB) { m_bResetVelocity = newB; }
    void SetShouldResetAngles(bool newB) { m_bResetAngles = newB; }

    virtual void AfterTeleport() {};//base class does nothing

private:
    bool m_bResetVelocity;
    bool m_bResetAngles;
    CBaseEntity *pDestinationEnt;
};

// CTriggerCheckpoint, used by mappers for teleporting 
class CTriggerCheckpoint : public CBaseMomentumTrigger
{
    DECLARE_CLASS(CTriggerCheckpoint, CBaseMomentumTrigger);
    DECLARE_DATADESC();

public:
    void StartTouch(CBaseEntity*);
    // the following is only used by CFilterCheckpoint
    virtual int GetCheckpointNumber() { return m_iCheckpointNumber; }
    // The following is used by mapzones.cpp
    void SetCheckpointNumber(int newInt) { m_iCheckpointNumber = newInt; }

private:
    int m_iCheckpointNumber;
};

// CTriggerStage
// used to declare which major part of the map the player has gotten to
class CTriggerStage : public CTriggerCheckpoint
{
    DECLARE_CLASS(CTriggerStage, CTriggerCheckpoint);
    DECLARE_DATADESC();

public:
    void StartTouch(CBaseEntity*);
    void Spawn()
    {
        SetCheckpointNumber(-1);
        BaseClass::Spawn();
    }
    //Used by CTimer and CStageFilter
    virtual int GetStageNumber() { return m_iStageNumber; }
    void SetStageNumber(int newInt) { m_iStageNumber = newInt; }
    int GetCheckpointNumber() { return -1; }//Override, use GetStageNumber()

private:
    int m_iStageNumber;
};

// CTriggerTimerStart
class CTriggerTimerStart : public CTriggerStage
{
    DECLARE_CLASS(CTriggerTimerStart, CTriggerStage);
    DECLARE_DATADESC();

public:
    void EndTouch(CBaseEntity*);
    void StartTouch(CBaseEntity*);
    void Spawn();
    void Think();

    // The start is always the first stage/checkpoint
    int GetCheckpointNumber() { return -1; }//Override
    int GetStageNumber() { return 1; }
    float GetMaxLeaveSpeed() { return m_fMaxLeaveSpeed; }
    void SetMaxLeaveSpeed(float pMaxSpeed);
    float GetBhopLeaveSpeed() { return m_fBhopLeaveSpeed; }
    void SetBhopLeaveSpeed(float pBhopLeaveSpeed);
    void SetLookAngles(QAngle newang);
    QAngle GetLookAngles() { return m_angLook; }

    //spawnflags
    bool IsLimitingSpeed() { return HasSpawnFlags(SF_LIMIT_LEAVE_SPEED); }
    void SetIsLimitingSpeed(bool pIsLimitingSpeed);
    void SetHasLookAngles(bool bHasLook);
    bool GetHasLookAngles() { return HasSpawnFlags(SF_USE_LOOKANGLES); }
    bool IsLimitingSpeedOnlyXY() { return HasSpawnFlags(SF_LIMIT_LEAVE_SPEED_ONLYXY); }
    void SetIsLimitingSpeedOnlyXY(bool pIsLimitingSpeedOnlyXY);
    bool IsLimitingBhop() { return HasSpawnFlags(SF_LIMIT_LEAVE_SPEED_BHOP); }
    void SetIsLimitingBhop(bool bIsLimitBhop);

private:
    QAngle m_angLook = QAngle(0, 0, 0);

    // How fast can the player leave the start trigger?
    float m_fMaxLeaveSpeed = 290;

    //limitbhop stuff
    float m_fBhopLeaveSpeed = 250;
    //default false, so if SF_LIMIT_BHOP is not set we don't do any bhop limit stuff
    bool m_bDidPlayerBhop {false};
};

// CFilterCheckpoint
class CFilterCheckpoint : public CBaseFilter
{
    DECLARE_CLASS(CFilterCheckpoint, CBaseFilter);
    DECLARE_DATADESC();

public:
    bool PassesFilterImpl(CBaseEntity*, CBaseEntity*);

private:
    int m_iCheckpointNumber;

};

// CTriggerTeleportCheckpoint
class CTriggerTeleportCheckpoint : public CTriggerTeleportEnt
{
    DECLARE_CLASS(CTriggerTeleportCheckpoint, CTriggerTeleportEnt);

public:
    void StartTouch(CBaseEntity*);
};

// CTriggerOnehop
class CTriggerOnehop : public CTriggerTeleportEnt
{
    DECLARE_CLASS(CTriggerOnehop, CTriggerTeleportEnt);
    DECLARE_DATADESC();

public:
    void StartTouch(CBaseEntity*);
    float GetHoldTeleportTime() { return m_fMaxHoldSeconds; }
    void SetHoldTeleportTime(float pHoldTime) { m_fMaxHoldSeconds = pHoldTime; }
    void Think();
    void AfterTeleport() { m_fStartTouchedTime = -1.0f; SetDestinationEnt(NULL); }

private:
    // The time that the player initally touched the trigger
    float m_fStartTouchedTime = 0.0f;
    // Seconds to hold before activating the teleport
    float m_fMaxHoldSeconds = 1;

};

// CTriggerResetOnehop
class CTriggerResetOnehop : public CBaseMomentumTrigger
{
    DECLARE_CLASS(CTriggerResetOnehop, CBaseMomentumTrigger);

public:
    void StartTouch(CBaseEntity*);

};

// CTriggerMultihop
class CTriggerMultihop : public CTriggerTeleportEnt
{
    DECLARE_CLASS(CTriggerMultihop, CTriggerTeleportEnt);
    DECLARE_DATADESC();

public:
    void StartTouch(CBaseEntity*);
    void EndTouch(CBaseEntity*);
    float GetHoldTeleportTime() { return m_fMaxHoldSeconds; }
    void SetHoldTeleportTime(float pHoldTime) { m_fMaxHoldSeconds = pHoldTime; }
    void Think();
    void AfterTeleport() { m_fStartTouchedTime = -1.0f; SetDestinationEnt(NULL); }

private:
    // The time that the player initally touched the trigger. -1 if not checking for teleport
    float m_fStartTouchedTime = 0.0f;
    // Seconds to hold before activating the teleport
    float m_fMaxHoldSeconds = 1;

};

// CTriggerUserInput
class CTriggerUserInput : public CBaseMomentumTrigger
{
    DECLARE_CLASS(CTriggerUserInput, CBaseMomentumTrigger);
    DECLARE_DATADESC();

public:
    enum key { forward, back, moveleft, moveright, jump, duck, attack, attack2, reload };
    key m_eKey;
    void Think();
    void Spawn();
    COutputEvent m_OnKeyPressed;

private:
    int m_ButtonRep;

};

// CTriggerLimitMovement
class CTriggerLimitMovement : public CBaseMomentumTrigger
{
    DECLARE_CLASS(CTriggerLimitMovement, CBaseMomentumTrigger);

public:
    void Think();
    void StartTouch(CBaseEntity *pOther);
    void EndTouch(CBaseEntity *pOther);

private:
    CountdownTimer m_BhopTimer;
    const float FL_BHOP_TIMER = 0.15f;
};


// CFuncShootBoost
class CFuncShootBoost : public CBreakable
{
    DECLARE_CLASS(CFuncShootBoost, CBreakable);
    DECLARE_DATADESC();

public:
    void Spawn();
    int OnTakeDamage(const CTakeDamageInfo &info);
    // Force in units per seconds applied to the player
    float m_fPushForce;
    // 0: No
    // 1: Yes
    // 2: Only if the player's velocity is lower than the push velocity, set player's velocity to final push velocity
    // 3: Only if the player's velocity is lower than the push velocity, increase player's velocity by final push velocity
    int m_iIncrease;
    // Dictates the direction of push
    Vector m_vPushDir;
    // If not null, dictates which entity the attacker must be touching for the func to work
    CBaseEntity *m_Destination;
};

// CTriggerMomentumPush
class CTriggerMomentumPush : public CTriggerTeleportEnt
{
    DECLARE_CLASS(CTriggerMomentumPush, CTriggerTeleportEnt);
    DECLARE_DATADESC();

public:
    void StartTouch(CBaseEntity*);
    void EndTouch(CBaseEntity*);
    // Called when (and by) either a StartTouch() or EndTouch() event happens and their requisites are met
    void OnSuccessfulTouch(CBaseEntity*);
    float GetHoldTeleportTime() { return m_fMaxHoldSeconds; }
    void SetHoldTeleportTime(float pHoldTime) { m_fMaxHoldSeconds = pHoldTime; }
    void AfterTeleport() { m_fStartTouchedTime = -1.0f; SetDestinationEnt(NULL); }

private:
    // The time that the player initally touched the trigger
    float m_fStartTouchedTime = 0.0f;
    // Seconds to hold before activating the teleport
    float m_fMaxHoldSeconds = 1;
    // Force in units per seconds applied to the player
    float m_fPushForce;
    // 1: SetPlayerVelocity to final push force
    // 2: Increase player's current velocity by push final foce ammount // This is almost like the default trigger_push behaviour
    // 3: Only set the player's velocity to the final push velocity if player's velocity is lower than final push velocity
    int m_iIncrease;
    // Dictates the direction of push
    Vector m_vPushDir;
    // Pointer to the destination entity if a teleport is needed
    CBaseEntity *m_Destination;
};
#endif // TIMERTRIGGERS_H
