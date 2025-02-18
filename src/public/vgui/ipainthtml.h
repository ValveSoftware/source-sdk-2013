//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef IPAINTHTML_H
#define IPAINTHTML_H

class IPaintHTML
{
public:
	enum EPaintTarget
	{
		ePaintBrowser,
		ePaintPopup,
		ePaintMAX
	};
	// returns the texture id used, pass in -1 to create a new texture
	virtual int DrawSubTextureRGBA( EPaintTarget eTarget, int textureID, int x, int y, const unsigned char *pRGBA, int wide, int tall ) = 0;
	virtual void DeleteTexture( EPaintTarget eTarget, int textureID ) = 0;
};

class IInputEventHTML
{
public:
	enum EMouseButton
	{
		eButtonLeft,
		eButtonMiddle,
		eButtonRight
	};

	virtual bool ChromeHandleMouseClick( EMouseButton eButton, bool bUp, int nClickCount ) = 0;
	virtual bool ChromeHandleMouseMove( int x, int y ) = 0;
	virtual bool ChromeHandleMouseWheel( int delta ) = 0;

	enum EKeyType
	{
		KeyDown,
		KeyUp,
		Char
	};
	enum EKeyModifier
	{
		AltDown = 1,
		CrtlDown = 2,
		ShiftDown = 4,
	};

	virtual bool ChromeHandleKeyEvent( EKeyType type, int key, int modifiers, bool bKeyUp ) = 0;
};

#endif // IPAINTHTML_H
