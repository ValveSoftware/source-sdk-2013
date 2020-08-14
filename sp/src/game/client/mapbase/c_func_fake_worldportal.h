//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Recreates Portal 2 linked_portal_door functionality using SDK code only.
//			(basically a combination of point_camera and func_reflective_glass)
//
// $NoKeywords: $
//===========================================================================//

#ifndef C_FUNC_FAKE_WORLDPORTAL
#define C_FUNC_FAKE_WORLDPORTAL

#ifdef _WIN32
#pragma once
#endif

struct cplane_t;
class CViewSetup;

class C_FuncFakeWorldPortal : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_FuncFakeWorldPortal, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_FuncFakeWorldPortal();
	virtual ~C_FuncFakeWorldPortal();

	virtual bool	ShouldDraw();
	virtual void	OnDataChanged( DataUpdateType_t type );

	SkyboxVisibility_t	SkyMode() { return m_iSkyMode; }

	ITexture		*RenderTarget();

	fogparams_t		*GetFog();

public:

	EHANDLE		m_hTargetPlane;
	QAngle		m_PlaneAngles;
	SkyboxVisibility_t m_iSkyMode;
	float		m_flScale;

	EHANDLE		m_hFogController;
	fogparams_t *m_pFog;

	char m_iszRenderTarget[64];
	ITexture *m_pRenderTarget;

	C_FuncFakeWorldPortal	*m_pNext;
};

//-----------------------------------------------------------------------------
// Do we have reflective glass in view? If so, what's the reflection plane?
//-----------------------------------------------------------------------------
C_FuncFakeWorldPortal *IsFakeWorldPortalInView( const CViewSetup& view, cplane_t &plane );

C_FuncFakeWorldPortal *NextFakeWorldPortal( C_FuncFakeWorldPortal *pStart, const CViewSetup& view, cplane_t &plane,
	const Frustum_t &frustum );


#endif // C_FUNC_FAKE_WORLDPORTAL


