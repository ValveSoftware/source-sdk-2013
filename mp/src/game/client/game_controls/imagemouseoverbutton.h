//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IMAGE_MOUSE_OVER_BUTTON_H
#define IMAGE_MOUSE_OVER_BUTTON_H

#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "mouseoverpanelbutton.h"

//===============================================
// CImageMouseOverButton - used for class images
//===============================================

template <class T>
class CImageMouseOverButton : public MouseOverButton<T>
{
private:
	//DECLARE_CLASS_SIMPLE( CImageMouseOverButton, MouseOverButton );

public:
	CImageMouseOverButton( vgui::Panel *parent, const char *panelName, T *templatePanel );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnSizeChanged( int newWide, int newTall );

	void RecalculateImageSizes( void );
	void SetActiveImage( const char *imagename );
	void SetInactiveImage( const char *imagename );
	void SetActiveImage( vgui::IImage *image );
	void SetInactiveImage( vgui::IImage *image );

public:
	virtual void Paint();
	virtual void ShowPage( void );
	virtual void HidePage( void );

private:
	vgui::IImage *m_pActiveImage;	
	char *m_pszActiveImageName;

	vgui::IImage *m_pInactiveImage;
	char *m_pszInactiveImageName;

	bool m_bScaleImage;
};

template <class T>
CImageMouseOverButton<T>::CImageMouseOverButton( vgui::Panel *parent, const char *panelName, T *templatePanel ) :
	MouseOverButton<T>( parent, panelName, templatePanel )
{
	m_pszActiveImageName = NULL;
	m_pszInactiveImageName = NULL;

	m_pActiveImage = NULL;
	m_pInactiveImage = NULL;
}

template <class T>
void CImageMouseOverButton<T>::ApplySettings( KeyValues *inResourceData )
{
	m_bScaleImage = inResourceData->GetInt( "scaleImage", 0 );

	// Active Image
	delete [] m_pszActiveImageName;
	m_pszActiveImageName = NULL;

	const char *activeImageName = inResourceData->GetString( "activeimage", "" );
	if ( *activeImageName )
	{
		this->SetActiveImage( activeImageName );
	}

	// Inactive Image
	delete [] m_pszInactiveImageName;
	m_pszInactiveImageName = NULL;

	const char *inactiveImageName = inResourceData->GetString( "inactiveimage", "" );
	if ( *inactiveImageName )
	{
		this->SetInactiveImage( inactiveImageName );
	}

	MouseOverButton<T>::ApplySettings( inResourceData );

	this->InvalidateLayout( false, true ); // force applyschemesettings to run
}

template <class T>
void CImageMouseOverButton<T>::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	MouseOverButton<T>::ApplySchemeSettings( pScheme );

	if ( m_pszActiveImageName && strlen( m_pszActiveImageName ) > 0 )
	{
		this->SetActiveImage( vgui::scheme()->GetImage( m_pszActiveImageName, m_bScaleImage ) );
	}

	if ( m_pszInactiveImageName && strlen( m_pszInactiveImageName ) > 0 )
	{
		this->SetInactiveImage( vgui::scheme()->GetImage( m_pszInactiveImageName, m_bScaleImage ) );
	}

	vgui::IBorder *pBorder = pScheme->GetBorder( "NoBorder" );
	this->SetDefaultBorder( pBorder);
	this->SetDepressedBorder( pBorder );
	this->SetKeyFocusBorder( pBorder );

	Color blank(0,0,0,0);
	this->SetDefaultColor( this->GetButtonFgColor(), blank );
	this->SetArmedColor( this->GetButtonArmedFgColor(), blank );
	this->SetDepressedColor( this->GetButtonDepressedFgColor(), blank );
}

template <class T>
void CImageMouseOverButton<T>::RecalculateImageSizes( void )
{
	// Reset our images, which will force them to recalculate their size.
	// Necessary for images shared with other scaling buttons.
	this->SetActiveImage( m_pActiveImage );
	this->SetInactiveImage( m_pInactiveImage );
}

template <class T>
void CImageMouseOverButton<T>::SetActiveImage( const char *imagename )
{
	int len = Q_strlen( imagename ) + 1;
	m_pszActiveImageName = new char[ len ];
	Q_strncpy( m_pszActiveImageName, imagename, len );
}

template <class T>
void CImageMouseOverButton<T>::SetInactiveImage( const char *imagename )
{
	int len = Q_strlen( imagename ) + 1;
	m_pszInactiveImageName = new char[ len ];
	Q_strncpy( m_pszInactiveImageName, imagename, len );
}

template <class T>
void CImageMouseOverButton<T>::SetActiveImage( vgui::IImage *image )
{
	m_pActiveImage = image;

	if ( m_pActiveImage )
	{
		int wide, tall;
		if ( m_bScaleImage )
		{
			// scaling, force the image size to be our size
			this->GetSize( wide, tall );
			m_pActiveImage->SetSize( wide, tall );
		}
		else
		{
			// not scaling, so set our size to the image size
			m_pActiveImage->GetSize( wide, tall );
			this->SetSize( wide, tall );
		}
	}

	this->Repaint();
}

template <class T>
void CImageMouseOverButton<T>::SetInactiveImage( vgui::IImage *image )
{
	m_pInactiveImage = image;

	if ( m_pInactiveImage )
	{
		int wide, tall;
		if ( m_bScaleImage)
		{
			// scaling, force the image size to be our size
			this->GetSize( wide, tall );
			m_pInactiveImage->SetSize( wide, tall );
		}
		else
		{
			// not scaling, so set our size to the image size
			m_pInactiveImage->GetSize( wide, tall );
			this->SetSize( wide, tall );
		}
	}

	this->Repaint();
}

template <class T>
void CImageMouseOverButton<T>::OnSizeChanged( int newWide, int newTall )
{
	if ( m_bScaleImage )
	{
		// scaling, force the image size to be our size

		if ( m_pActiveImage )
			m_pActiveImage->SetSize( newWide, newTall );

		if ( m_pInactiveImage )
			m_pInactiveImage->SetSize( newWide, newTall );
	}
	MouseOverButton<T>::OnSizeChanged( newWide, newTall );
}

template <class T>
void CImageMouseOverButton<T>::Paint()
{
	this->SetActiveImage( m_pActiveImage );
	this->SetInactiveImage( m_pInactiveImage );

	if ( this->IsArmed() )
	{
		// draw the active image
		if ( m_pActiveImage )
		{
			vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
			m_pActiveImage->SetPos( 0, 0 );
			m_pActiveImage->Paint();
		}
	}
	else 
	{
		// draw the inactive image
		if ( m_pInactiveImage )
		{
			vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
			m_pInactiveImage->SetPos( 0, 0 );
			m_pInactiveImage->Paint();
		}
	}
	
	MouseOverButton<T>::Paint();
}

template <class T>
void CImageMouseOverButton<T>::ShowPage( void )
{
	MouseOverButton<T>::ShowPage();

	// send message to parent that we triggered something
	this->PostActionSignal( new KeyValues( "ShowPage", "page", this->GetName() ) );
}

template <class T>
void CImageMouseOverButton<T>::HidePage( void )
{
	MouseOverButton<T>::HidePage();
}

#endif //IMAGE_MOUSE_OVER_BUTTON_H