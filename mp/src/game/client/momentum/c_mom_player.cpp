#include "cbase.h"
#include "c_mom_player.h"

#include "tier0/memdbgon.h"



IMPLEMENT_CLIENTCLASS_DT(C_MomentumPlayer, DT_MOM_Player, CMomentumPlayer)
RecvPropInt(RECVINFO(m_iShotsFired)),
RecvPropInt(RECVINFO(m_iDirection)),
RecvPropBool(RECVINFO(m_bResumeZoom)),
RecvPropInt(RECVINFO(m_iLastZoom)),
//RecvPropDataTable(RECVINFO_DT(m_HL2Local), 0, &REFERENCE_RECV_TABLE(DT_HL2Local)),
//RecvPropBool(RECVINFO(m_fIsSprinting)),
END_RECV_TABLE()


C_MomentumPlayer::C_MomentumPlayer()
{

}

C_MomentumPlayer::~C_MomentumPlayer()
{

}

void C_MomentumPlayer::SurpressLadderChecks(const Vector& pos, const Vector& normal)
{
    m_ladderSurpressionTimer.Start(1.0f);
    m_lastLadderPos = pos;
    m_lastLadderNormal = normal;
}

bool C_MomentumPlayer::CanGrabLadder(const Vector& pos, const Vector& normal)
{
    if (m_ladderSurpressionTimer.GetRemainingTime() <= 0.0f)
    {
        return true;
    }

    const float MaxDist = 64.0f;
    if (pos.AsVector2D().DistToSqr(m_lastLadderPos.AsVector2D()) < MaxDist * MaxDist)
    {
        return false;
    }

    if (normal != m_lastLadderNormal)
    {
        return true;
    }

    return false;
}