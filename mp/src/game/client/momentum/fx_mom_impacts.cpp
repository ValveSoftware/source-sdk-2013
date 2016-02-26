//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================//
#include "cbase.h"
#include "fx_impact.h"
#include "fx.h"
#include "decals.h"
#include "fx_quad.h"
#include "fx_sparks.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Handle gauss impacts
//-----------------------------------------------------------------------------
void ImpactGaussCallback(const CEffectData &data)
{
    trace_t tr;
    Vector vecOrigin, vecStart, vecShotDir;
    int iMaterial, iDamageType, iHitbox;
    short nSurfaceProp;
    C_BaseEntity *pEntity = ParseImpactData(data, &vecOrigin, &vecStart, &vecShotDir, nSurfaceProp, iMaterial, iDamageType, iHitbox);

    if (!pEntity)
    {
        // This happens for impacts that occur on an object that's then destroyed.
        // Clear out the fraction so it uses the server's data
        tr.fraction = 1.0;
        PlayImpactSound(pEntity, tr, vecOrigin, nSurfaceProp);
        return;
    }

    // If we hit, perform our custom effects and play the sound
    if (Impact(vecOrigin, vecStart, iMaterial, iDamageType, iHitbox, pEntity, tr))
    {
        // Check for custom effects based on the Decal index
        PerformCustomEffects(vecOrigin, tr, vecShotDir, iMaterial, 2);
    }

    PlayImpactSound(pEntity, tr, vecOrigin, nSurfaceProp);
}

DECLARE_CLIENT_EFFECT("ImpactGauss", ImpactGaussCallback);


//-----------------------------------------------------------------------------
// Purpose: Handle weapon impacts
//-----------------------------------------------------------------------------
void ImpactCallback(const CEffectData &data)
{
    VPROF_BUDGET("ImpactCallback", VPROF_BUDGETGROUP_PARTICLE_RENDERING);
    trace_t tr;
    Vector vecOrigin, vecStart, vecShotDir;
    int iMaterial, iDamageType, iHitbox;
    short nSurfaceProp;
    C_BaseEntity *pEntity = ParseImpactData(data, &vecOrigin, &vecStart, &vecShotDir, nSurfaceProp, iMaterial, iDamageType, iHitbox);
    if (!pEntity)
    {
        // This happens for impacts that occur on an object that's then destroyed.
        // Clear out the fraction so it uses the server's data
        tr.fraction = 1.0;
        PlayImpactSound(pEntity, tr, vecOrigin, nSurfaceProp);
        return;
    }
    // If we hit, perform our custom effects and play the sound
    if (Impact(vecOrigin, vecStart, iMaterial, iDamageType, iHitbox, pEntity, tr))
    {
        // Check for custom effects based on the Decal index
        PerformCustomEffects(vecOrigin, tr, vecShotDir, iMaterial, 1.0);
    }
    PlayImpactSound(pEntity, tr, vecOrigin, nSurfaceProp);
}
DECLARE_CLIENT_EFFECT("Impact", ImpactCallback);