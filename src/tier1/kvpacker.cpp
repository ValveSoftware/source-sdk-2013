//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains a branch-neutral binary packer for KeyValues trees. 
//
// $NoKeywords: $
//
//=============================================================================//

#include <KeyValues.h>
#include "kvpacker.h"

#include "tier0/dbg.h"
#include "utlbuffer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define KEYVALUES_TOKEN_SIZE	1024

// writes KeyValue as binary data to buffer
bool KVPacker::WriteAsBinary( KeyValues *pNode, CUtlBuffer &buffer )
{
	if ( buffer.IsText() ) // must be a binary buffer
		return false;

	if ( !buffer.IsValid() ) // must be valid, no overflows etc
		return false;

	// Write subkeys:

	// loop through all our peers
	for ( KeyValues *dat = pNode; dat != NULL; dat = dat->GetNextKey() )
	{
		// write type
		switch ( dat->GetDataType() )
		{
		case KeyValues::TYPE_NONE:
			{
				buffer.PutUnsignedChar( PACKTYPE_NONE );
				break;
			}
		case KeyValues::TYPE_STRING:
			{
				buffer.PutUnsignedChar( PACKTYPE_STRING );
				break;
			}
		case KeyValues::TYPE_WSTRING:
			{
				buffer.PutUnsignedChar( PACKTYPE_WSTRING );
				break;
			}

		case KeyValues::TYPE_INT:
			{
				buffer.PutUnsignedChar( PACKTYPE_INT );
				break;
			}

		case KeyValues::TYPE_UINT64:
			{
				buffer.PutUnsignedChar( PACKTYPE_UINT64 );
				break;
			}

		case KeyValues::TYPE_FLOAT:
			{
				buffer.PutUnsignedChar( PACKTYPE_FLOAT );
				break;
			}
		case KeyValues::TYPE_COLOR:
			{
				buffer.PutUnsignedChar( PACKTYPE_COLOR );
				break;
			}
		case KeyValues::TYPE_PTR:
			{
				buffer.PutUnsignedChar( PACKTYPE_PTR );
				break;
			}

		default:
			break;
		}

		// write name
		buffer.PutString( dat->GetName() );

		// write value
		switch ( dat->GetDataType() )
		{
		case KeyValues::TYPE_NONE:
			{
				if( !WriteAsBinary( dat->GetFirstSubKey(), buffer ) )
					return false;
				break;
			}
		case KeyValues::TYPE_STRING:
			{
				if (dat->GetString() && *(dat->GetString()))
				{
					buffer.PutString( dat->GetString() );
				}
				else
				{
					buffer.PutString( "" );
				}
				break;
			}
		case KeyValues::TYPE_WSTRING:
			{
				int nLength = dat->GetWString() ? Q_wcslen( dat->GetWString() ) : 0;
				buffer.PutShort( nLength );
				for( int k = 0; k < nLength; ++ k )
				{
					buffer.PutShort( ( unsigned short ) dat->GetWString()[k] );
				}
				break;
			}

		case KeyValues::TYPE_INT:
			{
				buffer.PutInt( dat->GetInt() );				
				break;
			}

		case KeyValues::TYPE_UINT64:
			{
				buffer.PutInt64( dat->GetUint64() );
				break;
			}

		case KeyValues::TYPE_FLOAT:
			{
				buffer.PutFloat( dat->GetFloat() );
				break;
			}
		case KeyValues::TYPE_COLOR:
			{
				Color color = dat->GetColor();
				buffer.PutUnsignedChar( color[0] );
				buffer.PutUnsignedChar( color[1] );
				buffer.PutUnsignedChar( color[2] );
				buffer.PutUnsignedChar( color[3] );
				break;
			}
		case KeyValues::TYPE_PTR:
			{
				buffer.PutPtr( dat->GetPtr() );
				break;
			}

		default:
			break;
		}
	}

	// write tail, marks end of peers
	buffer.PutUnsignedChar( PACKTYPE_NULLMARKER ); 

	return buffer.IsValid();
}

// read KeyValues from binary buffer, returns true if parsing was successful
bool KVPacker::ReadAsBinary( KeyValues *pNode, CUtlBuffer &buffer )
{
	if ( buffer.IsText() ) // must be a binary buffer
		return false;

	if ( !buffer.IsValid() ) // must be valid, no overflows etc
		return false;

	pNode->Clear();

	char		token[KEYVALUES_TOKEN_SIZE];
	KeyValues	*dat = pNode;
	EPackType		ePackType = (EPackType)buffer.GetUnsignedChar();

	// loop through all our peers
	while ( true )
	{
		if ( ePackType == PACKTYPE_NULLMARKER )
			break; // no more peers

		buffer.GetString( token );
		token[KEYVALUES_TOKEN_SIZE-1] = 0;

		dat->SetName( token );

		switch ( ePackType )
		{
		case PACKTYPE_NONE:
			{
				KeyValues *pNewNode = new KeyValues("");
				dat->AddSubKey( pNewNode );
				if( !ReadAsBinary( pNewNode, buffer ) )
					return false;
				break;
			}
		case PACKTYPE_STRING:
			{
				buffer.GetString( token );
				token[KEYVALUES_TOKEN_SIZE-1] = 0;
				dat->SetStringValue( token );
				break;
			}
		case PACKTYPE_WSTRING:
			{
				int nLength = buffer.GetShort();
				if ( nLength >= 0 && nLength*sizeof( uint16 ) <= (uint)buffer.GetBytesRemaining() )
				{
					if ( nLength > 0 )
					{
						wchar_t *pTemp = (wchar_t *)malloc( sizeof( wchar_t ) * (1 + nLength) );

						for ( int k = 0; k < nLength; ++k )
						{
							pTemp[k] = buffer.GetShort(); // ugly, but preserving existing behavior
						}

						pTemp[nLength] = 0;
						dat->SetWString( NULL, pTemp );

						free( pTemp );
					}
					else
						dat->SetWString( NULL, L"" );

				}
				break;
			}

		case PACKTYPE_INT:
			{
				dat->SetInt( NULL, buffer.GetInt() );
				break;
			}

		case PACKTYPE_UINT64:
			{
				dat->SetUint64( NULL, (uint64)buffer.GetInt64() );
				break;
			}

		case PACKTYPE_FLOAT:
			{
				dat->SetFloat( NULL, buffer.GetFloat() );
				break;
			}
		case PACKTYPE_COLOR:
			{
				Color color( 
					buffer.GetUnsignedChar(),
					buffer.GetUnsignedChar(),
					buffer.GetUnsignedChar(),
					buffer.GetUnsignedChar() );
				dat->SetColor( NULL, color );
				break;
			}
		case PACKTYPE_PTR:
			{
				dat->SetPtr( NULL, buffer.GetPtr() );
				break;
			}

		default:
			break;
		}

		if ( !buffer.IsValid() ) // error occured
			return false;

		ePackType = (EPackType)buffer.GetUnsignedChar();

		if ( ePackType == PACKTYPE_NULLMARKER )
			break;

		// new peer follows
		KeyValues *pNewPeer = new KeyValues("");
		dat->SetNextKey( pNewPeer );
		dat = pNewPeer;
	}

	return buffer.IsValid();
}

