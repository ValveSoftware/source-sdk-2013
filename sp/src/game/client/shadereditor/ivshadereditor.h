#ifndef IV_SHADEREDITOR
#define IV_SHADEREDITOR

#ifdef _WIN32
#pragma once
#endif // _WIN32

#define pFnClCallback( x ) void(* x )( float *pfl4 )
#define pFnClCallback_Declare( x ) void x( float *pfl4 )

#define pFnVrCallback( x ) void(* x )( bool * const pbOptions, int * const piOptions,\
										float * const pflOptions, char ** const pszOptions )
#define pFnVrCallback_Declare( x ) void x( bool * const pbOptions, int * const piOptions,\
										float * const pflOptions, char ** const pszOptions )

#ifndef PROCSHADER_DLL

#ifdef SHADER_EDITOR_DLL
#include "../public/tier1/interface.h"
#include "view_shared.h"
#else
#include "interface.h"
#include "ShaderEditor/ShaderEditorSystem.h"
#endif // NOT SHADER_EDITOR_DLL

enum SEDIT_SKYMASK_MODE
{
	SKYMASK_OFF = 0,
	SKYMASK_QUARTER, // render at 1/4 fb size where possible
	SKYMASK_FULL, // render at full fb size
};

class CViewSetup_SEdit_Shared
{
public:
	CViewSetup_SEdit_Shared()
	{
		Q_memset( this, 0, sizeof( CViewSetup_SEdit_Shared ) );
	};
	CViewSetup_SEdit_Shared( const CViewSetup &o )
	{
		x = o.x;
		y = o.y;
		width = o.width;
		height = o.height;
		fov = o.fov;
		fovViewmodel = o.fovViewmodel;
		origin = o.origin;
		angles = o.angles;
		zNear = o.zNear;
		zFar = o.zFar;
		zNearViewmodel = o.zNearViewmodel;
		zFarViewmodel = o.zFarViewmodel;
		m_flAspectRatio = o.m_flAspectRatio;
	};
	int			x,y,width,height;
	float		fov,fovViewmodel;
	Vector		origin;
	QAngle		angles;
	float		zNear,zFar,zNearViewmodel,zFarViewmodel;
	float		m_flAspectRatio;
};


class IVShaderEditor : public IBaseInterface
{
public:
	virtual bool Init( CreateInterfaceFn appSystemFactory, CGlobalVarsBase *pGlobals,
					void *pSEditMRender,
					bool bCreateEditor, bool bEnablePrimaryDebug, int iSkymaskMode ) = 0;
	virtual void Shutdown() = 0;
	virtual void PrecacheData() = 0;

	// call before Init() to overwrite any paths, pass in NULL for the ones that shouldn't be overwritten
	virtual void OverridePaths( const char *pszWorkingDirectory,
		const char *pszCompilePath = NULL,		// abs path to compiler binaries
		const char *pszLocalCompilePath = NULL,	// local path to compiler binaries, relative to shader source directory
		const char *pszGamePath = NULL,
		const char *pszCanvas = NULL,			// path to canvas files
		const char *pszShaderSource = NULL,		// path to shader source files
		const char *pszDumps = NULL,			// path to shader configuration files
		const char *pszUserFunctions = NULL,	// path to custom function bodies
		const char *pszEditorRoot = NULL ) = 0;	// path to 'shadereditorui' home directory

	// update the lib
	virtual void OnFrame( float frametime ) = 0;
	virtual void OnPreRender( void *viewsetup ) = 0;
	virtual void OnSceneRender() = 0;
	virtual void OnUpdateSkymask( bool bCombineMode ) = 0;
	virtual void OnPostRender( bool bUpdateFB ) = 0;

	// data callbacks for hlsl constants
	virtual void RegisterClientCallback( const char *name, pFnClCallback(callback), int numComponents ) = 0;
	virtual void LockClientCallbacks() = 0;

	// view render callbacks for post processing graphs
	virtual void RegisterViewRenderCallback( const char *pszVrCName, pFnVrCallback(callback),
		const char **pszBoolNames = NULL, const bool *pBoolDefaults = NULL, const int numBoolParams = 0,
		const char **pszIntNames = NULL, const int *pIntDefaults = NULL, const int numIntParams = 0,
		const char **pszFloatNames = NULL, const float *pFloatDefaults = NULL, const int numFloatParams = 0,
		const char **pszStringNames = NULL, const char **pStringDefaults = NULL, const int numStringParams = 0 ) = 0;
	virtual void LockViewRenderCallbacks() = 0;

	// post processing effect manipulation (precached effects accessible)
	// the index becomes invalid when editing the precache list
	virtual int			GetPPEIndex( const char *pszName ) = 0; // returns -1 when not found, case insensitive
	virtual bool		IsPPEEnabled( const int &index ) = 0;
	virtual void		SetPPEEnabled( const int &index, const bool &bEnabled ) = 0;
	virtual IMaterial	*GetPPEMaterial( const int &index, const char *pszNodeName ) = 0;

	// Draws a PPE graph right now or adds it to the render queue (r_queued_post_processing!)
	// Does not push a new RT but uses the current one
	// If you have 'during scene' nodes, make sure to call it twice in the appropriate places
	virtual void		DrawPPEOnDemand( const int &index, const bool bInScene = false ) = 0;
};

#define SHADEREDIT_INTERFACE_VERSION "ShaderEditor005"

#ifdef SHADER_EDITOR_DLL
class ShaderEditorInterface;
extern ShaderEditorInterface *shaderEdit;
#else
extern IVShaderEditor *shaderEdit;
#endif // NOT SHADER_EDITOR_DLL

#endif // NOT PROCSHADER_DLL

#endif // NOT IV_SHADEREDITOR