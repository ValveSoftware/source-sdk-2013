//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Swap a compiled caption file.
//
// $NoKeywords: $
//=============================================================================//

#include "utlbuffer.h"
#include "byteswap.h"
#include "filesystem.h"
#include "tier2/fileutils.h"
#include "captioncompiler.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_BYTESWAP_DATADESC( CompiledCaptionHeader_t )
	DEFINE_FIELD( magic, FIELD_INTEGER ),
	DEFINE_FIELD( version, FIELD_INTEGER ),
	DEFINE_FIELD( numblocks, FIELD_INTEGER ),
	DEFINE_FIELD( blocksize, FIELD_INTEGER ),
	DEFINE_FIELD( directorysize, FIELD_INTEGER ),
	DEFINE_FIELD( dataoffset, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( CaptionLookup_t )
	DEFINE_FIELD( hash, FIELD_INTEGER ),
	DEFINE_FIELD( blockNum, FIELD_INTEGER ),
	DEFINE_FIELD( offset, FIELD_SHORT ),
	DEFINE_FIELD( length, FIELD_SHORT ),
END_BYTESWAP_DATADESC()

//-----------------------------------------------------------------------------
// Swap a compiled closecaption file 
//-----------------------------------------------------------------------------
bool SwapClosecaptionFile( void *pData )
{
	CByteswap swap;
	swap.ActivateByteSwapping( true );

	CompiledCaptionHeader_t *pHdr = (CompiledCaptionHeader_t*)pData;

	if ( IsX360() )
	{
		// pre-swap file header
		swap.SwapFieldsToTargetEndian( pHdr );
	}

	if ( pHdr->magic != COMPILED_CAPTION_FILEID || pHdr->version != COMPILED_CAPTION_VERSION )
	{
		// bad data
		return false;
	}

	// lookup headers
	pData = (byte*)pData + sizeof(CompiledCaptionHeader_t);
	swap.SwapFieldsToTargetEndian( (CaptionLookup_t*)pData, pHdr->directorysize );

	// unicode data
	pData = (byte*)pHdr + pHdr->dataoffset;
	swap.SwapBufferToTargetEndian( (wchar_t*)pData, (wchar_t*)pData, pHdr->numblocks * pHdr->blocksize / sizeof(wchar_t) );

	if ( IsPC() )
	{
		// post-swap file header
		swap.SwapFieldsToTargetEndian( pHdr );
	}

	return true;
}

#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Callback for UpdateOrCreate - generates .360 file
//-----------------------------------------------------------------------------
static bool CaptionCreateCallback( const char *pSourceName, const char *pTargetName, const char *pPathID, void *pExtraData )
{
	// Generate the file
	CUtlBuffer buf;
	bool bOk = g_pFullFileSystem->ReadFile( pSourceName, pPathID, buf );
	if ( bOk )
	{
		bOk = SwapClosecaptionFile( buf.Base() );
		if ( bOk )
		{
			bOk = g_pFullFileSystem->WriteFile( pTargetName, pPathID, buf );
		}
		else
		{
			Warning( "Failed to create %s\n", pTargetName );
		}
	}
	return bOk;
}

//-----------------------------------------------------------------------------
// Calls utility function UpdateOrCreate
//-----------------------------------------------------------------------------
int UpdateOrCreateCaptionFile( const char *pSourceName, char *pTargetName, int maxLen, bool bForce )
{
	return ::UpdateOrCreate( pSourceName, pTargetName, maxLen, "GAME", CaptionCreateCallback, bForce );
}
#endif