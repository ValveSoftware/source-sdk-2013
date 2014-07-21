//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: MiniMap.h: interface for the CMiniMap class.
//
// $NoKeywords: $
//=============================================================================//

#if !defined HLTVPANEL_H
#define HLTVPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <game/client/iviewport.h>
#include "mathlib/vector.h"
#include <igameevents.h>
#include <shareddefs.h>
#include <const.h>
#include "hudelement.h"

class IMapOverviewPanel
{
public:
	virtual void SetMode( int mode ) = 0;
	virtual int GetMode( void ) = 0;
	virtual void FlashEntity( int entityID ) = 0;
	virtual void SetPlayerPositions(int index, const Vector &position, const QAngle &angle) = 0;
	virtual void SetVisible(bool state) = 0;
	virtual float GetZoom( void ) = 0;
	virtual vgui::Panel *GetAsPanel() = 0;
	virtual bool AllowConCommandsWhileAlive() = 0;
	virtual void SetPlayerPreferredMode( int mode ) = 0;
	virtual void SetPlayerPreferredViewSize( float viewSize ) = 0;
	virtual bool IsVisible() = 0;
	virtual void GetBounds(int &x, int &y, int &wide, int &tall) = 0;
	virtual float GetFullZoom( void ) = 0;
	virtual float GetMapScale( void ) = 0;
};

#define MAX_TRAIL_LENGTH	30
#define OVERVIEW_MAP_SIZE	1024	// an overview map is 1024x1024 pixels

typedef bool ( *FnCustomMapOverviewObjectPaint )( int textureID, Vector pos, float scale, float angle, const char *text, Color *textColor, float status, Color *statusColor );


class CMapOverview : public CHudElement, public vgui::Panel, public IMapOverviewPanel
{
	DECLARE_CLASS_SIMPLE( CMapOverview, vgui::Panel );

public:	

	enum
	{
		MAP_MODE_OFF = 0,	// Totally off
		MAP_MODE_INSET,		// A little map up in a corner
		MAP_MODE_FULL,		// Full screen, full map
		MAP_MODE_RADAR		// In game radar, extra functionality
	};

	CMapOverview( const char *pElementName );
	virtual ~CMapOverview();

	virtual bool ShouldDraw( void );
	vgui::Panel *GetAsPanel(){ return this; }
	virtual bool AllowConCommandsWhileAlive(){return true;}
	virtual void SetPlayerPreferredMode( int mode ){}
	virtual void SetPlayerPreferredViewSize( float viewSize ){};

protected:	// private structures & types

	float GetViewAngle( void ); // The angle that determines the viewport twist from map texture to panel drawing.

	// list of game events the hLTV takes care of

	typedef struct {
		int		xpos;
		int		ypos;
	} FootStep_t;	

	typedef struct MapPlayer_s {
		int		index;		// player's index
		int		userid;		// user ID on server
		int		icon;		// players texture icon ID
		Color   color;		// players team color
		char	name[MAX_PLAYER_NAME_LENGTH];
		int		team;		// N,T,CT
		int		health;		// 0..100, 7 bit
		Vector	position;	// current x,y pos
		QAngle	angle;		// view origin 0..360
		Vector2D trail[MAX_TRAIL_LENGTH];	// save 1 footstep each second for 1 minute
	} MapPlayer_t;

	typedef struct MapObject_s {
		int		objectID;	// unique object ID
		int		index;		// entity index if any
		int		icon;		// players texture icon ID
		Color   color;		// players team color
		char	name[MAX_PLAYER_NAME_LENGTH];	// show text under icon
		Vector	position;	// current x,y pos
		QAngle	angle;		// view origin 0..360
		float	endtime;	// time stop showing object
		float	size;		// object size
		float	status;		// green status bar [0..1], -1 = disabled
		Color	statusColor;	// color of status bar
		int		flags;		// MAB_OBJECT_*
		const char *text;	// text to draw underneath the icon
	} MapObject_t;

#define MAP_OBJECT_ALIGN_TO_MAP	(1<<0)

public: // IViewPortPanel interface:

	virtual const char *GetName( void ) { return PANEL_OVERVIEW; }
	virtual void SetData(KeyValues *data);
	virtual void Reset();
	virtual void OnThink();
	virtual void Update();
	virtual bool NeedsUpdate( void );
	virtual bool HasInputElements( void ) { return false; }
	virtual void ShowPanel( bool bShow );
	virtual void Init( void );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void GetBounds(int &x, int &y, int &wide, int &tall) { BaseClass::GetBounds(x, y, wide, tall); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

public: // IGameEventListener

	virtual void FireGameEvent( IGameEvent *event);

public:	// VGUI overrides

	virtual void Paint();
	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void SetVisible(bool state){BaseClass::SetVisible(state);}

public:

	virtual float GetZoom( void );
	virtual int GetMode( void );
	virtual float GetFullZoom( void ){ return m_fFullZoom; }
	virtual float GetMapScale( void ){ return m_fMapScale; }

	// Player settings:
	virtual void ShowPlayerNames(bool state);
	virtual void ShowPlayerHealth(bool state);
	virtual void ShowPlayerTracks(float seconds); 
	virtual void SetPlayerPositions(int index, const Vector &position, const QAngle &angle);

	// general settings:
	virtual void SetMap(const char * map);
	virtual void SetTime( float time );
	virtual void SetMode( int mode );
	virtual bool SetTeamColor(int team, Color color);
	virtual void SetFollowAngle(bool state);
	virtual void SetFollowEntity(int entindex); // 0 = off
	virtual void SetCenter( const Vector2D &mappos); 
	virtual void SetAngle( float angle);
	virtual Vector2D WorldToMap( const Vector &worldpos );

	// Object settings
	virtual int		AddObject( const char *icon, int entity, float timeToLive ); // returns object ID, 0 = no entity, -1 = forever
	virtual void	SetObjectIcon( int objectID, const char *icon, float size );  // icon world size
	virtual void	SetObjectText( int objectID, const char *text, Color color ); // text under icon
	virtual void	SetObjectStatus( int objectID, float value, Color statusColor ); // status bar under icon
	virtual void	SetObjectPosition( int objectID, const Vector &position, const QAngle &angle ); // world pos/angles
	virtual void 	AddObjectFlags( int objectID, int flags );
	virtual void 	SetObjectFlags( int objectID, int flags );
	virtual void	RemoveObject( int objectID );
	virtual void	RemoveObjectByIndex( int index );
	virtual void	FlashEntity( int entityID ){}

	// rules that define if you can see a player on the overview or not
	virtual bool CanPlayerBeSeen(MapPlayer_t *player);

	/// allows mods to restrict health
	virtual bool CanPlayerHealthBeSeen(MapPlayer_t *player);

	/// allows mods to restrict names (e.g. CS when mp_playerid is non-zero)
	virtual bool CanPlayerNameBeSeen(MapPlayer_t *player);

	virtual int GetIconNumberFromTeamNumber( int teamNumber ){return teamNumber;}

protected:

	virtual void	DrawCamera();
	virtual void	DrawObjects();
	virtual void	DrawMapTexture();
	virtual void	DrawMapPlayers();
	virtual void	DrawMapPlayerTrails();
	virtual void	UpdatePlayerTrails();
	virtual void	ResetRound();
	virtual void	InitTeamColorsAndIcons();
	virtual void	UpdateSizeAndPosition();
	virtual bool	RunHudAnimations(){ return true; }

	bool			IsInPanel(Vector2D &pos);
	MapPlayer_t*	GetPlayerByUserID( int userID );
	int				AddIconTexture(const char *filename);
	Vector2D		MapToPanel( const Vector2D &mappos );
	int				GetPixelOffset( float height );
	void			UpdateFollowEntity();
	virtual void	UpdatePlayers();
	void			UpdateObjects(); // objects bound to entities 
	MapObject_t*	FindObjectByID(int objectID);
	virtual bool	IsRadarLocked() {return false;}

	virtual bool	DrawIcon( MapObject_t *obj );

	/*virtual bool	DrawIcon(	int textureID,
								int offscreenTextureID,
								Vector pos,
								float scale,
								float angle,
								int alpha = 255,
								const char *text = NULL,
								Color *textColor = NULL,
								float status = -1,
								Color *statusColor = NULL,
								int objectType = OBJECT_TYPE_NORMAL );*/
	
	int				m_nMode;
	Vector2D		m_vPosition;
	Vector2D		m_vSize;
	float			m_flChangeSpeed;
	float			m_flIconSize;


	IViewPort *		m_pViewPort;
	MapPlayer_t		m_Players[MAX_PLAYERS];
	CUtlDict< int, int> m_TextureIDs;
	CUtlVector<MapObject_t>	m_Objects;

	Color	m_TeamColors[MAX_TEAMS];
	int		m_TeamIcons[MAX_TEAMS];
	int		m_ObjectIcons[64];
	int		m_ObjectCounterID;
	vgui::HFont	m_hIconFont;


	bool m_bShowNames;
	bool m_bShowTrails;
	bool m_bShowHealth;

	int	 m_nMapTextureID;		// texture id for current overview image

	KeyValues * m_MapKeyValues; // keyvalues describing overview parameters

	Vector	m_MapOrigin;	// read from KeyValues files
	float	m_fMapScale;	// origin and scale used when screenshot was made
	bool	m_bRotateMap;	// if true roatate map around 90 degress, so it fits better to 4:3 screen ratio

	int		m_nFollowEntity;// entity number to follow, 0 = off
	CPanelAnimationVar( float, m_fZoom, "zoom", "1.0" );	// current zoom n = overview panel shows 1/n^2 of whole map'
	float	m_fFullZoom;	// best zoom factor for full map view (1.0 is map is a square) 
	Vector2D m_ViewOrigin;	// map coordinates that are in the center of the pverview panel
	Vector2D m_MapCenter;	// map coordinates that are in the center of the pverview panel

	float	m_fNextUpdateTime;
	float	m_fViewAngle;	// rotation of overview map
	float	m_fWorldTime;	// current world time
	float   m_fNextTrailUpdate; // next time to update player trails
	float	m_fTrailUpdateInterval; // if -1 don't show trails
	bool	m_bFollowAngle;	// if true, map rotates with view angle


};

extern IMapOverviewPanel *g_pMapOverview;

#endif //
