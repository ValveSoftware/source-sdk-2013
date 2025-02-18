//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef SUBRECTIMAGE_H
#define SUBRECTIMAGE_H

#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Image.h"
#include "vgui/VGUI.h"


//-----------------------------------------------------------------------------
// Purpose: Check box image
//-----------------------------------------------------------------------------
class CSubRectImage : public vgui::Image
{
public:
	CSubRectImage( const char *filename, bool hardwareFiltered, int subx, int suby, int subw, int subh );
	virtual ~CSubRectImage();

	void GetSize( int &wide, int &tall );
	void GetContentSize( int &wide, int &tall );
	void SetSize( int x, int y );
	void SetPos( int x, int y );
	void SetColor( Color col );
	const char *GetName();
	void Paint();
	void ForceUpload();
	vgui::HTexture GetID();
	bool IsValid();

private:
	vgui::HTexture _id;
	int			sub[ 4 ];
	char		*_filename;
	int			_pos[2];
	int			_wide,_tall;
	Color		_color;
	bool        _uploaded;
	bool		_valid;
	bool		_filtered;
};


#endif // SUBRECTIMAGE_H
