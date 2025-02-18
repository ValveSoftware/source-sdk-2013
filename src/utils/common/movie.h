//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef _MOVIE_H_
#define _MOVIE_H_

/*
	movie.h

	definitions and such for dumping screen shots to make a movie
*/

typedef struct
{
	unsigned long tag;
	unsigned long size;
} movieblockheader_t;


typedef struct	
{
	short width;
	short height;
	short depth;
} movieframe_t;



#endif _MOVIE_H_