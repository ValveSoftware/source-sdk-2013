//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CVARSLIDER_H
#define CVARSLIDER_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Slider.h>

class CCvarSlider : public vgui::Slider
{
	DECLARE_CLASS_SIMPLE( CCvarSlider, vgui::Slider );

public:

	CCvarSlider( vgui::Panel *parent, const char *panelName );
	CCvarSlider( vgui::Panel *parent, const char *panelName, char const *caption,
		float minValue, float maxValue, char const *cvarname, bool bAllowOutOfRange=false );
	~CCvarSlider();

	void			SetupSlider( float minValue, float maxValue, const char *cvarname, bool bAllowOutOfRange );

	void			SetCVarName( char const *cvarname );
	void			SetMinMaxValues( float minValue, float maxValue, bool bSetTickdisplay = true );
	void			SetTickColor( Color color );

	virtual void	Paint();

	virtual void	ApplySettings( KeyValues *inResourceData );
	virtual void	GetSettings( KeyValues *outResourceData );

	void			ApplyChanges();
	float			GetSliderValue();
    void            SetSliderValue(float fValue);
    void            Reset();
    bool            HasBeenModified();

private:
	MESSAGE_FUNC( OnSliderMoved, "SliderMoved" );
	MESSAGE_FUNC( OnSliderDragEnd, "SliderDragEnd" );

	CPanelAnimationVar( bool, m_bUseConVarMinMax, "use_convar_minmax", "0" );

    bool        m_bAllowOutOfRange;
    bool        m_bModifiedOnce;
    float       m_fStartValue;
    int         m_iStartValue;
    int         m_iLastSliderValue;
    float       m_fCurrentValue;
	char		m_szCvarName[ 64 ];

	bool		m_bCreatedInCode;
	float		m_flMinValue;
	float		m_flMaxValue;
};

#endif // CVARSLIDER_H
