//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MOUSEOVERHTMLBUTTON_H
#define MOUSEOVERHTMLBUTTON_H
#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Purpose: Triggers a new html page when the mouse goes over the button
//-----------------------------------------------------------------------------
class MouseOverHTMLButton : public vgui::Button
{
public:
	MouseOverHTMLButton(vgui::Panel *parent, const char *panelName, vgui::HTML *html, const char *page) :
					Button( parent, panelName, "MouseOverHTMLButton")
	{
		m_pHTML = html;
		m_iClass = 0;
		m_iIndex = -1;
		m_bAddShortCut = true;
		if ( page )
		{
			Q_strncpy( m_sPage, page, sizeof( m_sPage ) );
		}
		else
		{
			memset(m_sPage, 0x0, sizeof( m_sPage ) );
		}
	}

	void SetClass(int pClass, int index) { m_iClass = pClass; m_iIndex = index;}
	int GetClass() { return m_iClass; }
	
	void SetAddHotKey( bool state ) { m_bAddShortCut = state; }

	void SetPage( const char *page )
	{
		if ( page )
		{
			Q_strncpy( m_sPage, page, sizeof( m_sPage ) );
		}
		else
		{
			memset(m_sPage, 0x0, sizeof( m_sPage ) );
		}
	}

	void SetHTML( vgui::HTML *html) 
	{
		m_pHTML = html;
	}


private:

	virtual void OnCursorEntered() 
	{
		Button::OnCursorEntered();
		if ( m_pHTML && strlen(m_sPage) > 0 )
		{
			m_pHTML->OpenURL(m_sPage);
		}
	}

	virtual void SetText(const char *text)
	{
		if ( m_iIndex != -1 )
		{
			wchar_t newText[ 128 ];
			wchar_t localizeText[ 128 ];
			wchar_t *ansiLocal;
			if ( text[0] == '#' && ( ansiLocal = g_pVGuiLocalize->Find( text ) ) )
			{
				// wcsncpy will crash if ansiLocal is null... *sigh*
				wcsncpy(localizeText, ansiLocal, sizeof(localizeText)/sizeof(wchar_t));
			}
			else
			{
				g_pVGuiLocalize->ConvertANSIToUnicode( text, localizeText, sizeof( localizeText ) );
			}

			if ( m_bAddShortCut )
			{
#ifdef WIN32
				_snwprintf( newText, sizeof( newText )/ sizeof( wchar_t ), L"&%i %s", m_iIndex, localizeText);
#else
				_snwprintf( newText, sizeof( newText )/ sizeof( wchar_t ), L"&%i %S", m_iIndex, localizeText);
#endif

			}
			else
			{
				memcpy( newText, localizeText, sizeof( newText ) );
			}

			Button::SetText( newText );
		}
		else
		{
			Button::SetText( text );
		}
	}

	vgui::HTML *m_pHTML;
	char m_sPage[ 255 ];
	int m_iClass;
	int m_iIndex;
	bool m_bAddShortCut;
};


#endif // MOUSEOVERHTMLBUTTON_H
