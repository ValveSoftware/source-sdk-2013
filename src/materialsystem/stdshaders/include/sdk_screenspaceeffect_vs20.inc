// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#pragma once
#include "shaderlib/cshader.h"
class sdk_screenspaceeffect_vs20_Static_Index
{
	unsigned int m_nX360APPCHOOSER : 2;
public:
	void SetX360APPCHOOSER( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nX360APPCHOOSER = i;
	}

	sdk_screenspaceeffect_vs20_Static_Index(  )
	{
		m_nX360APPCHOOSER = 0;
	}

	int GetIndex() const
	{
		return ( 1 * m_nX360APPCHOOSER ) + 0;
	}
};

#define shaderStaticTest_sdk_screenspaceeffect_vs20 1


class sdk_screenspaceeffect_vs20_Dynamic_Index
{
public:
	sdk_screenspaceeffect_vs20_Dynamic_Index(  )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_sdk_screenspaceeffect_vs20 1

