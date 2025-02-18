//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_HUD_MEDICCALLERS_H
#define TF_HUD_MEDICCALLERS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "tf_imagepanel.h"
#include "c_tf_player.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFMedicCallerPanel : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFMedicCallerPanel, vgui::EditablePanel );
public:
	CTFMedicCallerPanel( vgui::Panel *parent, const char *name );
	~CTFMedicCallerPanel( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnTick( void );
	virtual void PaintBackground( void );
	virtual void Paint( void );

	virtual const char *GetControlSettingFile() const { return "resource/UI/MedicCallerPanel.res"; }
	
	void	GetCallerPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation );
	void	SetEntity( C_BaseEntity *pEntity, float flDuration, Vector &vecOffset );
	void	SetMedicCallerType( MedicCallerType nType );
	static void AddMedicCaller( C_BaseEntity *pEntity, float flDuration, Vector &vecOffset, MedicCallerType nType = CALLER_TYPE_NORMAL );
	
	virtual void FireGameEvent( IGameEvent *event );

protected:
	C_BaseEntity	*GetEntity() const { return m_hEntity; }

private:
	IMaterial		*m_pArrowMaterial;
	float			m_flRemoveAt;
	Vector			m_vecOffset;
	CHandle<C_BaseEntity> m_hEntity;
	int				m_iDrawArrow;
	bool			m_bOnscreen;
	bool			m_bBurning;
	bool			m_bBleeding;
	float			m_flPanelScale;
	int				m_nCallerType;
};

#endif // TF_HUD_MEDICCALLERS_H
