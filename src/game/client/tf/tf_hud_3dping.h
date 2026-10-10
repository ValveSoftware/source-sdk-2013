#ifndef TF_HUD_3DPING_H
#define TF_HUD_3DPING_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include "tf_3dping_shared.h"

enum ETF3DPingIcon
{
	TF_3DPING_ICON_DEFAULT = 0,
	TF_3DPING_ICON_FRIENDLY_PLAYER,
	TF_3DPING_ICON_FRIENDLY_BUILDING,
	TF_3DPING_ICON_ENEMY_PLAYER,
	TF_3DPING_ICON_ENEMY_BUILDING,
	TF_3DPING_ICON_PICKUP_HEALTH,
	TF_3DPING_ICON_PICKUP_AMMO,
	TF_3DPING_ICON_MVM_ROBOT,
	TF_3DPING_ICON_MVM_SENTRYBUSTER,
	TF_3DPING_ICON_MVM_TANK,

	TF_3DPING_ICON_COUNT
};

class CHud3DPing : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHud3DPing, vgui::Panel );

public:
	CHud3DPing( const char *pElementName );
	virtual ~CHud3DPing();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool ShouldDraw( void );
	virtual void Paint( void );
	virtual void OnThink( void );

	bool IsLocalPlayerStealthLocked( void ) const;

	void OnPingReceived( ETF3DPingType eType, int iSenderEntIndex, const Vector &vecPosition, float flLifetime, bool bHostile, bool bFromPartyScope );

private:
	struct ActivePing_t
	{
		bool				bActive;
		ETF3DPingType		eType;
		int					iSenderEntIndex;
		Vector				vecPosition;
		float				flSpawnTime;
		float				flExpireTime;
		bool				bHostile;
		bool				bFromPartyScope;
		float				flOcclusionAmount;
	};

	void LoadIcons( void );
	ETF3DPingIcon GetIconForPing( const ActivePing_t &ping ) const;
	void DrawPing( ActivePing_t &ping, bool bDying );

	ActivePing_t	m_Pings[ MAX_PLAYERS + 1 ];

	// Replaced markers use flExpireTime as the fade start time.
	ActivePing_t	m_DyingPings[ MAX_PLAYERS + 1 ];

	int				m_iFillTextureID[ TF_3DPING_ICON_COUNT ];
	int				m_iBorderTextureID[ TF_3DPING_ICON_COUNT ];
	bool			m_bFillValid[ TF_3DPING_ICON_COUNT ];
	bool			m_bBorderValid[ TF_3DPING_ICON_COUNT ];

	bool			m_bWasStealthed;
	float			m_flLastUnstealthTime;

	int				m_iLastObservedTeam;
};

CHud3DPing *GetHud3DPing( void );

namespace vgui { class PanelListPanel; class Label; }

#define TF_3DPING_PREVIEW_COUNT	10

class CTF3DPingIconPreview : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CTF3DPingIconPreview, vgui::Panel );

public:
	CTF3DPingIconPreview( vgui::Panel *pParent, const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual void Paint();

private:
	int				m_iFillTextureID[ TF_3DPING_ICON_COUNT ];
	int				m_iBorderTextureID[ TF_3DPING_ICON_COUNT ];
	bool			m_bFillValid[ TF_3DPING_ICON_COUNT ];
	bool			m_bBorderValid[ TF_3DPING_ICON_COUNT ];
	vgui::Label		*m_pLabels[ TF_3DPING_PREVIEW_COUNT ];
};

void TF3DPing_MaybeInsertPreviewRow( vgui::PanelListPanel *pList, const char *pszCategoryPrompt );

#endif // TF_HUD_3DPING_H
