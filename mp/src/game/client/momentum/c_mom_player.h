#ifndef C_MOMPLAYER_H
#define C_MOMPLAYER_H
#ifdef WIN32
#pragma once
#endif

#include "cbase.h"
#include "momentum/mom_shareddefs.h"

class C_MomentumPlayer : public C_BasePlayer
{
public:
    DECLARE_CLASS(C_MomentumPlayer, C_BasePlayer);
    
    C_MomentumPlayer();
    ~C_MomentumPlayer();

    DECLARE_CLIENTCLASS();

    Vector m_lastStandingPos; // used by the gamemovement code for finding ladders

    void SurpressLadderChecks(const Vector& pos, const Vector& normal);
    bool CanGrabLadder(const Vector& pos, const Vector& normal);
    bool HasAutoBhop();
    bool DidPlayerBhop() { return m_bDidPlayerBhop; }

    int m_iShotsFired;
    int m_iDirection;
    bool m_bResumeZoom;
    int m_iLastZoom;
    bool m_bAutoBhop;
    bool m_bDidPlayerBhop;

    void GetBulletTypeParameters(
        int iBulletType,
        float &fPenetrationPower,
        float &flPenetrationDistance);

    void FireBullet(
        Vector vecSrc,
        const QAngle &shootAngles,
        float vecSpread,
        float flDistance,
        int iPenetration,
        int iBulletType,
        int iDamage,
        float flRangeModifier,
        CBaseEntity *pevAttacker,
        bool bDoEffects,
        float x,
        float y);

    void KickBack(
        float up_base,
        float lateral_base,
        float up_modifier,
        float lateral_modifier,
        float up_max,
        float lateral_max,
        int direction_change);

private:
    CountdownTimer m_ladderSurpressionTimer;
    Vector m_lastLadderNormal;
    Vector m_lastLadderPos;

    bool m_duckUntilOnGround;
    float m_flStamina;

    friend class CMomentumGameMovement;
};

#endif