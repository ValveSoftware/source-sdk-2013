#ifndef SHEDITSYSTEM_H
#define SHEDITSYSTEM_H

#include "cbase.h"

#include "datacache/imdlcache.h"

#include "iviewrender.h"
#include "view_shared.h"
#include "viewrender.h"


class ShaderEditorHandler : public CAutoGameSystemPerFrame
{
public:
	ShaderEditorHandler( char const *name );
	~ShaderEditorHandler();

	virtual bool Init();
	virtual void Shutdown();

	virtual void Update( float frametime );
	virtual void PreRender();
	virtual void PostRender();

#ifdef SOURCE_2006
	void CustomViewRender( int *viewId, const VisibleFogVolumeInfo_t &fogVolumeInfo );
#else
	void CustomViewRender( int *viewId, const VisibleFogVolumeInfo_t &fogVolumeInfo, const WaterRenderInfo_t &waterRenderInfo );
#endif
	void CustomPostRender();
	void UpdateSkymask( bool bCombineMode = false );

	const bool IsReady();
	int &GetViewIdForModify();
	const VisibleFogVolumeInfo_t &GetFogVolumeInfo();
#ifndef SOURCE_2006
	const WaterRenderInfo_t &GetWaterRenderInfo();
#endif

private:
	bool m_bReady;

	void RegisterCallbacks();
	void PrepareCallbackData();

	void RegisterViewRenderCallbacks();

	int *m_piCurrentViewId;
	VisibleFogVolumeInfo_t m_tFogVolumeInfo;
#ifndef SOURCE_2006
	WaterRenderInfo_t m_tWaterRenderInfo;
#endif
};

extern ShaderEditorHandler *g_ShaderEditorSystem;


#endif