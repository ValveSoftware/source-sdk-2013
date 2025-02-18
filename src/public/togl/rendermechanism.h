//========= Copyright Valve Corporation, All rights reserved. ============//
//                       TOGL CODE LICENSE
//
//  Copyright 2011-2014 Valve Corporation
//  All Rights Reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//
#ifndef RENDERMECHANISM_H
#define RENDERMECHANISM_H

#if defined(DX_TO_GL_ABSTRACTION) && !defined(USE_DXVK)

#undef PROTECTED_THINGS_ENABLE

#include <GL/gl.h>
#include <GL/glext.h>

#include "tier0/basetypes.h"
#include "tier0/platform.h"

#include "togl/dxformats.h"

#include "togl/linuxwin/glmdebug.h"
#include "togl/linuxwin/glbase.h"
#include "togl/linuxwin/glentrypoints.h"
#include "togl/linuxwin/glmdisplay.h"
#include "togl/linuxwin/glmdisplaydb.h"
#include "togl/linuxwin/glmgrbasics.h"
#include "togl/linuxwin/glmgrext.h"
#include "togl/linuxwin/cglmbuffer.h"
#include "togl/linuxwin/cglmtex.h"
#include "togl/linuxwin/cglmfbo.h"
#include "togl/linuxwin/cglmprogram.h"
#include "togl/linuxwin/cglmquery.h"
#include "togl/linuxwin/glmgr.h"
#include "togl/linuxwin/dxabstract_types.h"
#include "togl/linuxwin/dxabstract.h"

#include "togl/d3dx_impl.h"

#else
	//USE_ACTUAL_DX
	#ifdef WIN32
		#ifdef _X360
			#include "d3d9.h"
			#include "d3dx9.h"
		#else
			#include <WinSock2.h>
			#include <windows.h>
			#if __has_include( "../../dx9sdk/include/d3d9.h" )
				#include "../../dx9sdk/include/d3d9.h"
			#else
				#include <d3d9.h>		
			#endif
			#include "togl/d3dx_impl.h"
		#endif
		typedef HWND VD3DHWND;
	#else
		#include <d3d9.h>
		#include "togl/d3dx_impl.h"
		typedef HWND VD3DHWND;
	#endif

	#define	GLMPRINTF(args)	
	#define	GLMPRINTSTR(args)
	#define	GLMPRINTTEXT(args)
#endif // defined(DX_TO_GL_ABSTRACTION)

#endif // RENDERMECHANISM_H
