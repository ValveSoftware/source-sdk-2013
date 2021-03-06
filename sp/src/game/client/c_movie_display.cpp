//========= Copyright © 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//
//=====================================================================================//
#include "cbase.h"
#include "c_movie_display.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_MovieDisplay, DT_MovieDisplay, CMovieDisplay )
	RecvPropBool( RECVINFO( m_bEnabled ) ),
	RecvPropBool( RECVINFO( m_bLooping ) ),
	RecvPropString( RECVINFO( m_szMovieFilename ) ),
	RecvPropString( RECVINFO( m_szGroupName ) ),
END_RECV_TABLE()

C_MovieDisplay::C_MovieDisplay()
{
}

C_MovieDisplay::~C_MovieDisplay()
{
}
