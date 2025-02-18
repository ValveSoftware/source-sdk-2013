//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// MessageBuffer - handy for serializing/unserializing
// structures to be sent as messages  
// dal - 9/2002
//
#include <stdlib.h>
#include <string.h>
#include "messbuf.h"
#include "tier1/strtools.h"


///////////////////////////
//
//
//
MessageBuffer::MessageBuffer()
{
	size = DEFAULT_MESSAGE_BUFFER_SIZE;
	data = (char *) malloc(size);
	len = 0;
	offset = 0;
}

///////////////////////////
//
//
//
MessageBuffer::MessageBuffer(int minsize)
{
	size = minsize;
	data = (char *) malloc(size);
	len = 0;
	offset = 0;
}

///////////////////////////
//
//
//
MessageBuffer::~MessageBuffer()
{
	free(data);
}


///////////////////////////
//
//
//
int
MessageBuffer::getSize()
{
	return size;
}

///////////////////////////
//
//
//
int
MessageBuffer::getLen()
{
	return len;
}

///////////////////////////
//
//
//
int
MessageBuffer::setLen(int nlen)
{
	if (nlen < 0) return -1;
	if (nlen > size) {
		resize(nlen);
	}

	int res = len;
	len = nlen;

	return res;
}

///////////////////////////
//
//
//
int
MessageBuffer::getOffset()
{
	return offset;
}

///////////////////////////
//
//
//
int
MessageBuffer::setOffset(int noffset)
{
	if (noffset < 0 || noffset > len) return -1;
	int res = offset;
	offset = noffset;

	return res;
}


///////////////////////////
//
//
//
int
MessageBuffer::write(void const * p, int bytes)
{
	if (bytes + len > size) {
		resize(bytes + len);
	}
	memcpy(data + len, p, bytes);
	int res = len;
	len += bytes;

	return res;
}

///////////////////////////
//
//
//
int
MessageBuffer::update(int loc, void const * p, int bytes)
{
	if (loc + bytes > size) {
		resize(loc + bytes);
	}
	memcpy(data + loc, p, bytes);

	if (len < loc + bytes) {
		len = loc + bytes;
	}

	return len;
}

///////////////////////////
//
//
//
int
MessageBuffer::extract(int loc, void * p, int bytes)
{
	if (loc + bytes > len) return -1;
	memcpy(p, data + loc, bytes);

	return loc + bytes;
}

///////////////////////////
//
//
//
int
MessageBuffer::read(void * p, int bytes)
{
	if (offset + bytes > len) return -1;
	memcpy(p, data + offset, bytes);

	offset += bytes;
	return offset;
}

int MessageBuffer::WriteString( const char *pString )
{
	return write( pString, V_strlen( pString ) + 1 );
}

int MessageBuffer::ReadString( char *pOut, int bufferLength )
{
	int nChars = 0;
	while ( 1 )
	{
		char ch;
		if ( read( &ch, sizeof( ch ) ) == -1 )
		{
			pOut[0] = 0;
			return -1;
		}
		
		if ( ch == 0 || nChars >= (bufferLength-1) )
			break;
			
		pOut[nChars] = ch;
		++nChars;
	}
	pOut[nChars] = 0;
	return nChars + 1;
}


///////////////////////////
//
//
//
void
MessageBuffer::clear()
{
	memset(data, 0, size);
	offset = 0;
	len = 0;
}

///////////////////////////
//
//
//
void
MessageBuffer::clear(int minsize)
{
	if (minsize > size) {
		resize(minsize);
    }
	memset(data, 0, size);
	offset = 0;
	len = 0;
}

///////////////////////////
//
//
//
void
MessageBuffer::reset(int minsize)
{
	if (minsize > size) {
		resize(minsize);
    }
	len = 0;
	offset = 0;
}

///////////////////////////
//
//
//
void
MessageBuffer::resize(int minsize)
{
	if (minsize < size) return;

	if (size * 2 > minsize) minsize = size * 2;

	char * odata = data;
	data = (char *) malloc(minsize);
	memcpy(data, odata, len);
	size = minsize;
	free(odata);
}


///////////////////////////
//
//
void
MessageBuffer::print(FILE * ofile, int num)
{
	fprintf(ofile, "Len: %d Offset: %d Size: %d\n", len, offset, size);
    if (num > size) num = size;
	for (int i=0; i<num; ++i) {
		fprintf(ofile, "%02x ", (unsigned char) data[i]);
	}
	fprintf(ofile, "\n");
}