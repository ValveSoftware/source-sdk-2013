//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// MessageBuffer - handy for packing and upacking
// structures to be sent as messages  
//
#ifndef _MESSAGEBUFFER
#define _MESSAGEBUFFER

#include <stdio.h>
#define DEFAULT_MESSAGE_BUFFER_SIZE 2048

class MessageBuffer {
	public:
		char * data;

		MessageBuffer();
		MessageBuffer(int size);
		~MessageBuffer();

		int		getSize();
		int		getLen();
		int		setLen(int len);
		int		getOffset();
		int		setOffset(int offset);
		
		int		write(void const * p, int bytes);
		int		update(int loc, void const * p, int bytes);
		int		extract(int loc, void * p, int bytes);
		int		read(void * p, int bytes);

		int		WriteString( const char *pString );
		int		ReadString( char *pOut, int bufferLength );

		void	clear();
		void	clear(int minsize);
		void	reset(int minsize);
		void	print(FILE * ofile, int num);	

		void *GetReadPointer()
		{
			return ( void * )( data + offset );
		}

		int GetReadBytesLeft()
		{
			return len - offset;
		}

	private:
		void	resize(int minsize);
		int		size;
		int		offset;
		int		len;
};

#endif
