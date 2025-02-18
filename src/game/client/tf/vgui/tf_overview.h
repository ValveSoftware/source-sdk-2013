//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OVERVIEW_H
#define TF_OVERVIEW_H

#include <mapoverview.h>
#include "tf_shareddefs.h"

class CTFMapOverview : public CMapOverview
{
	DECLARE_CLASS_SIMPLE( CTFMapOverview, CMapOverview );

	CTFMapOverview( const char *pElementName );

	int m_CameraIcons[MAX_TEAMS];
	int m_CapturePoints[MAX_CONTROL_POINTS];

	void ShowLargeMap( void );
	void HideLargeMap( void );
	void ToggleZoom( void );

	void PlayerChat( int index );

	void DrawQuad( Vector pos, int scale, float angle, int textureID, int alpha );
	void DrawHorizontalSwipe( Vector pos, int scale, int textureID, float flCapPercentage, bool bSwipeLeft );
	bool DrawCapturePoint( int iCP, MapObject_t *obj );

	void SetDisabled( bool disabled ){ 	m_bDisabled = disabled; }
	bool IsDisabled( void ){ return m_bDisabled; }

	virtual void Paint();
	virtual bool ShouldDraw( void );
	virtual void VidInit( void );
	virtual void SetMap( const char * map );

protected:	
	virtual void SetMode(int mode);
	virtual void InitTeamColorsAndIcons();
	virtual void FireGameEvent( IGameEvent *event );
	virtual void DrawCamera();
	virtual void DrawMapPlayers();
	virtual void Update();
	virtual void UpdateSizeAndPosition();
	virtual void UpdateMapOverlayTexture();

	virtual void DrawMapOverlayTexture();

	// rules that define if you can see a player on the overview or not
	virtual bool CanPlayerBeSeen( MapPlayer_t *player );

	void DrawVoiceIconForPlayer( int playerIndex );

	virtual bool DrawIcon( MapObject_t *obj );

protected:
	void UpdateCapturePoints();

private:
	int m_iLastMode;

	int m_iVoiceIcon;
	int m_iChatIcon;

	bool m_bDisabled;

	float m_flPlayerChatTime[MAX_PLAYERS_ARRAY_SAFE];

	int m_nMapTextureOverlayID;	// texture id for current overlay image (shown over the current overview image)

#define TF_MAP_ZOOM_LEVELS	2
};

extern CTFMapOverview *GetTFOverview( void );

#endif // TF_OVERVIEW_H
