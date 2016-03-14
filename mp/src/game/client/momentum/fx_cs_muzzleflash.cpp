#include "cbase.h"
#include "particles_simple.h"
#include "particles_localspace.h"
#include "c_te_effect_dispatch.h"
#include "clienteffectprecachesystem.h"

// Precache our effects
CLIENTEFFECT_REGISTER_BEGIN(PrecacheEffect_CS_MuzzleFlash)
CLIENTEFFECT_MATERIAL("effects/muzzleflashX")	//.vmt
CLIENTEFFECT_MATERIAL("sprites/muzzleflash4")	//.vmt
CLIENTEFFECT_REGISTER_END()

void TE_DynamicLight(IRecipientFilter& filter, float delay,
const Vector* org, int r, int g, int b, int exponent, float radius, float time, float decay, int nLightIndex = LIGHT_INDEX_TE_DYNAMIC);

void CS_MuzzleFlashCallback(const CEffectData &data)
{
    CSmartPtr<CLocalSpaceEmitter> pEmitter =
        CLocalSpaceEmitter::Create("CS_MuzzleFlash", data.m_hEntity, data.m_nAttachmentIndex, 0);

    if (!pEmitter)
        return;

    // SetBBox() manually on the particle system so it doesn't have to be recalculated more than once.
    Vector vCenter(0.0f, 0.0f, 0.0f);
    C_BaseEntity *pEnt = data.GetEntity();
    if (pEnt)
    {
        vCenter = pEnt->WorldSpaceCenter();
    }
    else
    {
        IClientRenderable *pRenderable = data.GetRenderable();
        if (pRenderable)
        {
            Vector vecMins, vecMaxs;
            pRenderable->GetRenderBoundsWorldspace(vecMins, vecMaxs);
            VectorAdd(vecMins, vecMaxs, vCenter);
            vCenter *= 0.5f;
        }
    }

    Assert(pEmitter);
    pEmitter->GetBinding().SetBBox(vCenter - Vector(3, 3, 3), vCenter + Vector(3, 3, 3));

    // haxors - make the clip much shorter so the alpha is not 
    // changed based on large clip distances
    pEmitter->SetNearClip(0, 5);

    PMaterialHandle hFlashMaterial = pEmitter->GetPMaterial("sprites/muzzleflash4");

    for (int i = 0; i<3; i++)
    {
        SimpleParticle *pParticle = (SimpleParticle *) pEmitter->AddParticle(sizeof(SimpleParticle),
            hFlashMaterial,
            vec3_origin);
        if (pParticle)
        {
            pParticle->m_flLifetime = 0.0f;
            pParticle->m_flDieTime = 0.08f;

            pParticle->m_vecVelocity = vec3_origin;

            pParticle->m_uchColor[0] = 255;
            pParticle->m_uchColor[1] = 255;
            pParticle->m_uchColor[2] = 255;

            pParticle->m_uchStartAlpha = 80;
            pParticle->m_uchEndAlpha = 30;

            pParticle->m_uchStartSize = (3.0 + 3.0*i) * data.m_flScale;

            pParticle->m_uchEndSize = pParticle->m_uchStartSize * 0.8;
            pParticle->m_flRoll = random->RandomInt(0, 3);

            pParticle->m_flRollDelta = 0.0f;
        }
    }

    // dynamic light temporary entity for the muzzle flash
    CPVSFilter filter(pEmitter->GetSortOrigin());
    TE_DynamicLight(filter, 0.0, &(pEmitter->GetSortOrigin()), 255, 192, 64, 5, 70, 0.05, 768);
}

DECLARE_CLIENT_EFFECT("CS_MuzzleFlash", CS_MuzzleFlashCallback);


// 'X' shaped muzzleflash used by certain weapons
void CS_MuzzleFlashXCallback(const CEffectData &data)
{
    CSmartPtr<CLocalSpaceEmitter> pEmitter =
        CLocalSpaceEmitter::Create("CS_MuzzleFlashX", data.m_hEntity, data.m_nAttachmentIndex, 0);

    Assert(pEmitter);

    // haxors - make the clip much shorter so the alpha is not 
    // changed based on large clip distances
    pEmitter->SetNearClip(0, 5);

    PMaterialHandle hFlashMaterial = pEmitter->GetPMaterial("effects/muzzleflashX");

    SimpleParticle *pParticle = (SimpleParticle *) pEmitter->AddParticle(sizeof(SimpleParticle),
        hFlashMaterial,
        vec3_origin);
    if (pParticle)
    {
        pParticle->m_flLifetime = 0.0f;
        pParticle->m_flDieTime = 0.08f;

        pParticle->m_vecVelocity = vec3_origin;

        pParticle->m_uchColor[0] = 255;
        pParticle->m_uchColor[1] = 255;
        pParticle->m_uchColor[2] = 255;

        pParticle->m_uchStartAlpha = 130;
        pParticle->m_uchEndAlpha = 80;

        pParticle->m_uchStartSize = 6.0f * data.m_flScale * random->RandomFloat(0.9, 1.1);

        pParticle->m_uchEndSize = pParticle->m_uchStartSize * 0.8;

        pParticle->m_flRoll = random->RandomFloat(-0.25, 0.25);

        pParticle->m_flRollDelta = 0.0f;
    }

    // dynamic light temporary entity for the muzzle flash
    CPVSFilter filter(pEmitter->GetSortOrigin());
    TE_DynamicLight(filter, 0.0, &(pEmitter->GetSortOrigin()), 255, 192, 64, 5, 70, 0.05, 768);
}

DECLARE_CLIENT_EFFECT("CS_MuzzleFlash_X", CS_MuzzleFlashXCallback);