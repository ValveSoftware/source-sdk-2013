#ifndef C_SCREEN_OVERLAY_MULTI_H
#define C_SCREEN_OVERLAY_MULTI_H

#include "cbase.h"

class CScreenoverlayMulti : public CBaseEntity
{
	DECLARE_CLASS( CScreenoverlayMulti, CBaseEntity );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:

	enum RENDERMODE
	{
		RENDERMODE_PRE_POSTPROCESSING = 0,
		RENDERMODE_POST_HDR,
		RENDERMODE_POST_POSTPROCESSING,
	};

	CScreenoverlayMulti();
	~CScreenoverlayMulti();

#ifdef GAME_DLL
	virtual void Spawn();
	virtual void Activate();

	virtual int UpdateTransmitState();

	void SetEnabled( bool bEnabled );

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	virtual int ObjectCaps( void ){
		return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	};
#else
	virtual void OnDataChanged( DataUpdateType_t t );

	virtual void UpdateOnRemove();

	void RenderOverlay( int x, int y, int w, int h );
#endif

	int GetOverlayRenderMode();

	int GetRenderIndex();

	bool IsEnabled();

private:

#ifdef GAME_DLL
	string_t m_strOverlayMaterial;
#else
	CMaterialReference m_matOverlay;
#endif

	CNetworkVar( int, m_iMaterialIndex );

	CNetworkVar( int, m_iRenderMode );
	CNetworkVar( int, m_iRenderIndex );

	CNetworkVar( bool, m_bEnabled );
};

#ifdef CLIENT_DLL
void DrawOverlaysForMode( CScreenoverlayMulti::RENDERMODE mode, int x, int y, int w, int h );
#endif

#endif