//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <vgui/MouseCode.h>
#include "vgui_rotation_slider.h"


CRotationSlider::CRotationSlider( vgui::Panel *pParent, const char *pName ) :
	BaseClass( pParent, pName )
{
	AddActionSignalTarget( this );
	SetRange( -180, 180 );
	SetTickCaptions("-180", "180");
	SetValue( 0 );
	m_flYaw = 0;
}

void CRotationSlider::SetControlledObject( C_BaseObject *pObject )
{
	m_hObject.Set( pObject );
}

//-----------------------------------------------------------------------------
// When the slider is activated, deactivated, or moves
//-----------------------------------------------------------------------------
void CRotationSlider::OnMousePressed( vgui::MouseCode code )
{
	BaseClass::OnMousePressed( code );

	if (code != MOUSE_LEFT)
		return;

	C_BaseObject *pObj = m_hObject.Get();
	if (pObj)
	{
		m_flInitialYaw = pObj->GetAbsAngles().y;
		pObj->PreviewYaw( m_flInitialYaw );
		pObj->ActivateYawPreview( true );
	}
}

void CRotationSlider::OnSliderMoved( int position )
{
	C_BaseObject *pObj = m_hObject.Get();
	if (pObj && pObj->IsPreviewingYaw())
	{
		m_flYaw = anglemod(position);
		pObj->PreviewYaw( m_flInitialYaw - m_flYaw );
	}
}

void CRotationSlider::OnMouseReleased( vgui::MouseCode code )
{
	BaseClass::OnMouseReleased( code );

	if (code != MOUSE_LEFT)
		return;

	C_BaseObject *pObj = m_hObject.Get();
	if (pObj)
	{
		char szbuf[48];
		Q_snprintf( szbuf, sizeof( szbuf ), "yaw %0.2f\n",  m_flInitialYaw - m_flYaw );
		pObj->SendClientCommand( szbuf );
		pObj->ActivateYawPreview( false );
		SetValue(0);
		m_flYaw = 0;
	}
}
