//**********************************************//
//						//
// Installation instructions for grass clusters //
//						//
//**********************************************//

*** ONLY TESTED WITH SOURCE 2007, SO DON'T EXPECT SWARM COMPATIBILITY ***

The grass clusters are meant to replace the existing detail props. They'll use
the lighting and positioning information from them to generate their geometry.
Do not use a huge texture atlas for them from which you only read some parts but
use actual unique materials for each type of grass (PERFO), note that you have
to add any map dependent functionality yourself for that matter!
Also, don't use alpha blending on them, it WON'T WORK. And that won't change because
grass objects should stay batched. If you can't stand alpha testing use alpha to coverage,
which requires more performance.



Make sure to have the shader editor and its example materials and shaders installed too
or export them and update all references accordingly. VERIFY THAT YOU HAVE THE SHADER 'detail_prop_shader'
COMPILED AND IN YOUR PRECACHE LIST.



In case you're experiencing errors that you don't understand, go there: http://www.learncpp.com/
and READ it. 'Reading' in a sense of actually understanding what's written there. That includes the possibility
of iteratively examining a passage for a variable number of times until comprehension has been acquired.



If you're using the source sdk base 2007 you can apply the subversion patch (dprop_and_shadow_patch.patch)
and skip to step 5 now, other branches MIGHT NOT BE COMPATIBLE.



Step 1.

client/clientshadowmgr.cpp


Find this line:

ShadowType_t GetActualShadowCastType( ClientShadowHandle_t handle ) const;


Replace it with these:

public:
	ShadowHandle_t GetShadowHandle( ClientShadowHandle_t clienthandle ){ return m_Shadows[ clienthandle ].m_ShadowHandle; };
	int GetNumShadowDepthtextures(){ return m_DepthTextureCache.Count(); };
	CTextureReference GetShadowDepthTex( int num ){ return m_DepthTextureCache[num]; };

	ShadowType_t GetActualShadowCastType( ClientShadowHandle_t handle ) const;
private:


Step 2.

client/detailobjectsystem.cpp


Find this line:

#include "c_world.h"


Add this line below:

#include "ShaderEditor/Grass/CGrassCluster.h"


Find this line:

pSpritex4->m_pSpriteDefs[nSubField] = pSDef;


Add these lines below:

	_grassClusterInfo clusterHint;
	clusterHint.orig = pos;
	clusterHint.color.Init( color[0], color[1], color[2], 1 );
	clusterHint.uv_min = pSDef->m_TexLR;
	clusterHint.uv_max = pSDef->m_TexUL;
	CGrassClusterManager::GetInstance()->AddClusterHint( clusterHint );


Step 3.

client/iclientshadowmgr.h


Find this line:

virtual void ComputeShadowDepthTextures( const CViewSetup &pView ) = 0;


Add these lines below:

	virtual ShadowHandle_t GetShadowHandle( ClientShadowHandle_t clienthandle ) = 0;
	virtual ShadowType_t GetActualShadowCastType( ClientShadowHandle_t handle ) const = 0;
	virtual int GetNumShadowDepthtextures() = 0;
	virtual CTextureReference GetShadowDepthTex( int num ) = 0;


Step 4.

client/viewrender.cpp


Find this line:

#include "con_nprint.h"


Add this line below:

#include "ShaderEditor/Grass/CGrassCluster.h"


Find this line in the function void CRendering3dView::DrawOpaqueRenderables( bool bShadowDepth ):

g_pParticleSystemMgr->DrawRenderCache( bShadowDepth );


Add this line below for 2007 / swarm:

CGrassClusterManager::GetInstance()->RenderClusters( bShadowDepth );

OR this line for 2013:

CGrassClusterManager::GetInstance()->RenderClusters( DepthMode == DEPTH_MODE_SHADOW );


Step 5.

Copy the folder 'Grass' and both files to /client/ShaderEditor/. If you want to put them elsewhere,
you'll have to change the include paths in the respective files.


Step 6.

Add both files in /Grass/ to your client project and compile it. Grass clusters will now be
created wherever you placed standard simple detail sprites.