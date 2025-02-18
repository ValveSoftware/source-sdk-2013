//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds the result of an AsyncCreateTextureFromRenderTarget for VGUI. Don't reuse these.
//
//=============================================================================
#ifndef ICON_RENDER_RECEIVER_H
#define ICON_RENDER_RECEIVER_H
#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/itexture.h"

class CIconRenderReceiver : public IAsyncTextureOperationReceiver
{
public:
	CIconRenderReceiver() : m_pTex( NULL ) { }

protected:
	virtual ~CIconRenderReceiver() { SafeRelease( &m_pTex ); }

public:
	virtual int AddRef() OVERRIDE 
	{
		return ++m_nReferenceCount;
	}

	virtual int Release() OVERRIDE 
	{
		int retVal = --m_nReferenceCount;
		if ( retVal == 0 )
			delete this;
		return retVal;
	}

	virtual void OnAsyncCreateComplete( ITexture* pTex, void *pRtSrc ) OVERRIDE 
	{
		SafeAssign( &m_pTex, pTex );
	}

	virtual void OnAsyncFindComplete( ITexture* pTex, void * ) OVERRIDE  { Assert( !"Should never be called." ); }
	virtual void OnAsyncMapComplete( ITexture* pTex, void* pExtraArgs, void* pMemory, int pPitch ) OVERRIDE { Assert( !"Should never be called." ); }
	virtual void OnAsyncReadbackBegin( ITexture* pDst, ITexture* pSrc, void* pExtraArgs ) OVERRIDE { Assert( !"Should never be called." ); }

	virtual int GetRefCount() const OVERRIDE { return m_nReferenceCount; } 

	ITexture* GetTexture() const { return m_pTex; }

private:
	CInterlockedInt m_nReferenceCount; 
	ITexture* m_pTex;
};

#endif // ICON_RENDER_RECEIVER_H
