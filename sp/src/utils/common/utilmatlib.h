//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef UTILMATLIB_H
#define UTILMATLIB_H

#ifdef _WIN32
#pragma once
#endif

#define MATERIAL_NOT_FOUND NULL
	
class IMaterialSystem;
extern IMaterialSystem *g_pMaterialSystem;

typedef void *MaterialSystemMaterial_t;

#define UTILMATLIB_NEEDS_BUMPED_LIGHTMAPS 0
#define UTILMATLIB_NEEDS_LIGHTMAP 1
#define UTILMATLIB_OPACITY 2

enum { UTILMATLIB_ALPHATEST = 0, UTILMATLIB_OPAQUE, UTILMATLIB_TRANSLUCENT };

void InitMaterialSystem( const char *materialBaseDirPath, CreateInterfaceFn fileSystemFactory );
void ShutdownMaterialSystem( );
MaterialSystemMaterial_t FindMaterial( const char *materialName, bool *pFound, bool bComplain = true );
void GetMaterialDimensions( MaterialSystemMaterial_t materialHandle, int *width, int *height );
int GetMaterialShaderPropertyBool( MaterialSystemMaterial_t materialHandle, int propID );
int GetMaterialShaderPropertyInt( MaterialSystemMaterial_t materialHandle, int propID );
const char *GetMaterialVar( MaterialSystemMaterial_t materialHandle, const char *propertyName );
void GetMaterialReflectivity( MaterialSystemMaterial_t materialHandle, float *reflectivityVect );
const char *GetMaterialShaderName( MaterialSystemMaterial_t materialHandle );


#endif // UTILMATLIB_H
