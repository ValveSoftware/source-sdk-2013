#ifndef GIFMATERIAL_H
#define GIFMATERIAL_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utllinkedlist.h"

class IMaterial;
class ITexture;
class CJob;

#ifndef INCLUDED_GIFMATERIAL
#error You need to include gifmaterial_include.vpc in your project
#endif //!INCLUDED_GIFMATERIAL

// NOTE: Make sure you have connected at minimum Tier2 interfaces in your DLL before using

//-----------------------------------------------------------------------------
// Purpose: Utility for decoding GIFs and putting them on a material
//-----------------------------------------------------------------------------
class CGifMaterial
{
public:
	CGifMaterial( void );
	~CGifMaterial( void );

	bool		BInit( const char *pszFileName, const char *pszPathID = NULL );
	bool		BInit( const void *pubImage, int cubImage );
	void		Purge( void );

	bool		BIsProcessed( void ) const;

	void		SetFrame( int iFrame );
	int			GetFrame( void ) const;
	int			GetFrames( void ) const;
	bool		BAdvanceFrame( void ); // Iterates to the next frame if needed, returns true on success

	// Returns the ptr to the material. Note that the texture will take some time to load in, so
	// you might want to use a thumbnail or something until BIsProcessed returns true
	IMaterial  *GetMaterial( void );

private:
	void		Process( void *pvImage );

	struct rawframe_t
	{
		CUtlMemory< uint8 >	memTexture;
		float				flDuration;
		int					nDisposalMethod;
		int					iTransparentColor;
	};
	CUtlLinkedList< rawframe_t >	m_Frames;
	int								m_iSelectedFrame;

	float							m_flNextIterationTime;

	IMaterial					   *m_pMaterial;
	ITexture					   *m_pTexture;

	CThreadEvent					m_ProcessedEvent;
	::CJob						   *m_pAsyncProcessorJob;

	friend class CGifTextureRegen;
};

#endif //GIFMATERIAL_H