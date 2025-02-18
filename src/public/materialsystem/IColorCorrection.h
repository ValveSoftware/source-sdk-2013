//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef ICOLORCORRECTION_H
#define ICOLORCORRECTION_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"
#include "bitmap/imageformat.h"

typedef uintp ColorCorrectionHandle_t;
struct ShaderColorCorrectionInfo_t;

#define COLORCORRECTION_INTERFACE_VERSION "COLORCORRECTION_VERSION_1"

abstract_class IColorCorrectionSystem
{
public:
	virtual void Init() = 0;
	virtual void Shutdown() = 0;

	virtual ColorCorrectionHandle_t AddLookup( const char *pName ) = 0;
	virtual bool RemoveLookup( ColorCorrectionHandle_t handle ) = 0;

	virtual void  SetLookupWeight( ColorCorrectionHandle_t handle, float flWeight ) = 0;
	virtual float GetLookupWeight( ColorCorrectionHandle_t handle ) = 0;
	virtual float GetLookupWeight( int i ) = 0;

	virtual void LockLookup() = 0;
	virtual void LockLookup( ColorCorrectionHandle_t handle ) = 0;

	virtual void UnlockLookup() = 0;
	virtual void UnlockLookup( ColorCorrectionHandle_t handle ) = 0;

	virtual void SetLookup( RGBX5551_t inColor, color24 outColor ) = 0;
	virtual void SetLookup( ColorCorrectionHandle_t handle, RGBX5551_t inColor, color24 outColor ) = 0;

	virtual color24 GetLookup( RGBX5551_t inColor ) = 0;
	virtual color24 GetLookup( ColorCorrectionHandle_t handle, RGBX5551_t inColor ) = 0;

	virtual void LoadLookup( const char *pLookupName ) = 0;
	virtual void LoadLookup( ColorCorrectionHandle_t handle, const char *pLookupName ) = 0;

	virtual void CopyLookup( const color24 *pSrcColorCorrection ) = 0;
	virtual void CopyLookup( ColorCorrectionHandle_t handle, const color24 *pSrcColorCorrection ) = 0;

	virtual void ResetLookup( ColorCorrectionHandle_t handle ) = 0;
	virtual void ResetLookup( ) = 0;

	virtual void ReleaseTextures( ) = 0;
	virtual void RestoreTextures( ) = 0;

	virtual void ResetLookupWeights( ) = 0;

	virtual int GetNumLookups( ) = 0;

	virtual color24 ConvertToColor24( RGBX5551_t inColor ) = 0;

	virtual void SetResetable( ColorCorrectionHandle_t handle, bool bResetable ) = 0;

	virtual void EnableColorCorrection( bool bEnable ) = 0;

	// FIXME: Move this to a private interface only the material system can see?
	virtual void GetCurrentColorCorrection( ShaderColorCorrectionInfo_t* pInfo ) = 0;
};

#endif
