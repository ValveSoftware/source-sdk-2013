//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//
#if !defined( FX_H )
#define FX_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "particles_simple.h"
#include "c_pixel_visibility.h"

class Vector;
class CGameTrace;
typedef CGameTrace trace_t;
//struct trace_t;

enum
{
	FX_ENERGYSPLASH_EXPLOSIVE		= 0x1,
	FX_ENERGYSPLASH_SMOKE			= 0x2,
	FX_ENERGYSPLASH_LITTLESPARKS	= 0x4,
	FX_ENERGYSPLASH_BIGSPARKS		= 0x8,
	FX_ENERGYSPLASH_BIGSPARKSCOLLIDE = 0x10,
	FX_ENERGYSPLASH_ENERGYBALLS		= 0x20,
	FX_ENERGYSPLASH_DLIGHT			= 0x40,
	
	FX_ENERGYSPLASH_DEFAULT = ~FX_ENERGYSPLASH_EXPLOSIVE,
	FX_ENERGYSPLASH_DEFAULT_EXPLOSIVE = ~0,
};

bool FX_GetAttachmentTransform( ClientEntityHandle_t hEntity, int attachmentIndex, matrix3x4_t &transform );
bool FX_GetAttachmentTransform( ClientEntityHandle_t hEntity, int attachmentIndex, Vector *origin, QAngle *angles );

void FX_RicochetSound( const Vector& pos );

void FX_AntlionImpact( const Vector &pos, trace_t *tr );
void FX_DebrisFlecks( const Vector& origin, trace_t *trace, char materialType, int iScale, bool bNoFlecks = false );
void FX_Tracer( Vector& start, Vector& end, int velocity, bool makeWhiz = true );
void FX_GunshipTracer( Vector& start, Vector& end, int velocity, bool makeWhiz = true );
void FX_StriderTracer( Vector& start, Vector& end, int velocity, bool makeWhiz = true );
void FX_HunterTracer( Vector& start, Vector& end, int velocity, bool makeWhiz = true );
void FX_PlayerTracer( Vector& start, Vector& end );
void FX_BulletPass( Vector& start, Vector& end );
void FX_MetalSpark( const Vector &position, const Vector &direction, const Vector &surfaceNormal, int iScale = 1 );
void FX_MetalScrape( Vector &position, Vector &normal );
void FX_Sparks( const Vector &pos, int nMagnitude, int nTrailLength, const Vector &vecDir, float flWidth, float flMinSpeed, float flMaxSpeed, char *pSparkMaterial = NULL );
void FX_ElectricSpark( const Vector &pos, int nMagnitude, int nTrailLength, const Vector *vecDir );
void FX_BugBlood( Vector &pos, Vector &dir, Vector &vWorldMins, Vector &vWorldMaxs );
void FX_Blood( Vector &pos, Vector &dir, float r, float g, float b, float a );
void FX_CreateImpactDust( Vector &origin, Vector &normal );
void FX_EnergySplash( const Vector &pos, const Vector &normal, int nFlags = FX_ENERGYSPLASH_DEFAULT );
void FX_MicroExplosion( Vector &position, Vector &normal );
void FX_Explosion( Vector& origin, Vector& normal, char materialType );
void FX_ConcussiveExplosion( Vector& origin, Vector& normal ); 
void FX_DustImpact( const Vector &origin, trace_t *tr, int iScale );
void FX_DustImpact( const Vector &origin, trace_t *tr, float flScale );
void FX_MuzzleEffect( const Vector &origin, const QAngle &angles, float scale, ClientEntityHandle_t hEntity, unsigned char *pFlashColor = NULL, bool bOneFrame = false );
void FX_MuzzleEffectAttached( float scale, ClientEntityHandle_t hEntity, int attachmentIndex, unsigned char *pFlashColor = NULL, bool bOneFrame = false  );
void FX_StriderMuzzleEffect( const Vector &origin, const QAngle &angles, float scale, ClientEntityHandle_t hEntity, unsigned char *pFlashColor = NULL );
void FX_GunshipMuzzleEffect( const Vector &origin, const QAngle &angles, float scale, ClientEntityHandle_t hEntity, unsigned char *pFlashColor = NULL );
CSmartPtr<CSimpleEmitter> FX_Smoke( const Vector &origin, const Vector &velocity, float scale, int numParticles, float flDietime, unsigned char *pColor, int iAlpha, const char *pMaterial, float flRoll, float flRollDelta );
void FX_Smoke( const Vector &origin, const QAngle &angles, float scale, int numParticles, unsigned char *pColor = NULL, int iAlpha = -1 );
void FX_Dust( const Vector &vecOrigin, const Vector &vecDirection, float flSize, float flSpeed );
void FX_CreateGaussExplosion( const Vector &pos, const Vector &dir, int type );
void FX_GaussTracer( Vector& start, Vector& end, int velocity, bool makeWhiz = true );
void FX_TracerSound( const Vector &start, const Vector &end, int iTracerType );

// Lighting information utility
void UTIL_GetNormalizedColorTintAndLuminosity( const Vector &color, Vector *tint = NULL, float *luminosity = NULL );

// Useful function for testing whether to draw noZ effects
bool EffectOccluded( const Vector &pos, pixelvis_handle_t *queryHandle = 0 );

class CTeslaInfo
{
public:
	Vector			m_vPos;
	QAngle			m_vAngles;
	int				m_nEntIndex;
	const char		*m_pszSpriteName;
	float			m_flBeamWidth;
	int				m_nBeams;
	Vector			m_vColor;
	float			m_flTimeVisible;
	float			m_flRadius;
};

void FX_Tesla( const CTeslaInfo &teslaInfo );
extern ConVar r_decals;

extern void FX_CacheMaterialHandles( void );

extern PMaterialHandle g_Mat_Fleck_Wood[2];
extern PMaterialHandle g_Mat_Fleck_Cement[2];
extern PMaterialHandle g_Mat_Fleck_Antlion[2];
extern PMaterialHandle g_Mat_Fleck_Tile[2];
extern PMaterialHandle g_Mat_DustPuff[2];
extern PMaterialHandle g_Mat_BloodPuff[2];
extern PMaterialHandle g_Mat_Fleck_Glass[2];
extern PMaterialHandle g_Mat_SMG_Muzzleflash[4];
extern PMaterialHandle g_Mat_Combine_Muzzleflash[3];
#endif // FX_H
