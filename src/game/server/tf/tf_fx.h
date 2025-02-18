//========= Copyright Valve Corporation, All rights reserved. ============//
//
//  
//
//=============================================================================

#ifndef TF_FX_H
#define TF_FX_H
#ifdef _WIN32
#pragma once
#endif

#include "particle_parse.h"
#include "networkstringtabledefs.h"
void TE_FireBullets( int iPlayerIndex, const Vector &vOrigin, const QAngle &vAngles, int iWeaponID, int	iMode, int iSeed, float flSpread, bool bCritical );

// Historically -1 was used for an nEntIndex when we wanted the client to reconstruct
// INVALID_EHANDLE_INDEX, however that forced us to transmit nEntIndex as signed which
// meant that half of the edict values triggered asserts and did not transmit correctly.
// Now we use 2047 as the magic value. This results in the same bit patterns being
// transmitted, but no asserts, and it makes it more explicit that we have stolen one
// of the valid nEntIndex values for a special purpose.
const int kInvalidEHandleExplosion = MAX_EDICTS - 1;
const int kInvalidEHandleParticleEffect = MAX_EDICTS - 1;
void TE_TFExplosion( IRecipientFilter &filter, float flDelay, const Vector &vecOrigin, const Vector &vecNormal, int iWeaponID, int nEntIndex, int nDefID = -1, int nSound = SPECIAL1, int iCustomParticle = INVALID_STRING_INDEX );
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, const char *pszAttachmentName, bool bResetAllParticlesOnEntity = false );
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity = NULL, int iAttachmentPoint = -1, bool bResetAllParticlesOnEntity = false );
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity = NULL, ParticleAttachment_t iAttachType = PATTACH_CUSTOMORIGIN );
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CBaseEntity *pEntity = NULL );
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, int iEffectIndex, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CBaseEntity *pEntity = NULL );

void TE_TFParticleEffectComplex
(
	IRecipientFilter &filter,
	float flDelay,
	const char *pszParticleName,
	Vector vecOrigin,
	QAngle vecAngles,
	te_tf_particle_effects_colors_t *pOptionalColors = NULL,
	te_tf_particle_effects_control_point_t *pOptionalControlPoint1 = NULL,
	CBaseEntity *pEntity = NULL,
	ParticleAttachment_t eAttachType = PATTACH_CUSTOMORIGIN,
	Vector vecStart = vec3_origin
);

#endif	// TF_FX_H
