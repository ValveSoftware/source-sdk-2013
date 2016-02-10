
#include "gamemovement.h"
#include "func_ladder.h"
#include "mom_player_shared.h"
#include "baseplayer_shared.h"


struct surface_data_t;

class CMomentumPlayer;

#define NO_REFL_NORMAL_CHANGE -2.0f

class CMomentumGameMovement : public CGameMovement
{
    typedef CGameMovement BaseClass;
public:

    CMomentumGameMovement();

    // Overrides
    virtual bool LadderMove(void); // REPLACED
    virtual bool OnLadder(trace_t &trace); // REPLACED
    virtual void SetGroundEntity(trace_t *pm);
    virtual bool CanAccelerate(void) { BaseClass::CanAccelerate(); return true; }//C+P from HL2GM
    virtual bool CheckJumpButton(void);
    virtual void PlayerMove(void);
    virtual void AirMove(void);//Overridden for rampboost fix
    virtual void WalkMove(void);
    virtual void CheckForLadders(bool);

    virtual void CategorizeGroundSurface(trace_t&);


    //added ladder
    virtual float LadderDistance(void) const
    {
        if (player->GetMoveType() == MOVETYPE_LADDER)
            return 10.0f;
        return 2.0f;
    }

    virtual unsigned int LadderMask(void) const
    {
        return MASK_PLAYERSOLID & (~CONTENTS_PLAYERCLIP);
    }

    virtual float ClimbSpeed(void) const;
    virtual float LadderLateralMultiplier(void) const;
    const float DuckSpeedMultiplier = 0.34f;

    //Overrides for fixing rampboost
    virtual int TryPlayerMove(Vector *pFirstDest = NULL, trace_t *pFirstTrace = NULL);
    virtual void FullWalkMove();
    void DoLateReflect();
    void CategorizePosition();


    // Duck
    virtual void Duck(void);
    virtual void FinishUnDuck(void);
    virtual void FinishDuck(void);
    virtual bool CanUnduck();
    virtual void HandleDuckingSpeedCrop();
    virtual void CheckParameters(void) { BaseClass::CheckParameters(); }

private:

    float m_flReflectNormal = NO_REFL_NORMAL_CHANGE;//Used by rampboost fix

    // Given a list of nearby ladders, find the best ladder and the "mount" origin
    void		Findladder(float maxdist, CFuncLadder **ppLadder, Vector& ladderOrigin, const CFuncLadder *skipLadder);

    // Debounce the +USE key
    void		SwallowUseKey();
    CMomentumPlayer *GetMomentumPlayer();
};
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CMomentumPlayer	*CMomentumGameMovement::GetMomentumPlayer()
{
    return static_cast<CMomentumPlayer *>(player);
}