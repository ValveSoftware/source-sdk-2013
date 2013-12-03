//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PREDICTABLEID_H
#define PREDICTABLEID_H
#ifdef _WIN32
#pragma once
#endif

#if !defined( NO_ENTITY_PREDICTION )
//-----------------------------------------------------------------------------
// Purpose: Wraps 32bit predictID to allow access and creation
//-----------------------------------------------------------------------------
class CPredictableId
{
public:
	// Construction
					CPredictableId( void );

	static void		ResetInstanceCounters( void );

	// Is the Id being used
	bool			IsActive( void ) const;

	// Call this to set from data
	void			Init( int player, int command, const char *classname, const char *module, int line );

	// Get player index
	int				GetPlayer( void ) const;
	// Get hash value
	int				GetHash( void ) const;
	// Get index number
	int				GetInstanceNumber( void ) const;
	// Get command number
	int				GetCommandNumber( void ) const;

	// Check command number
//	bool			IsCommandNumberEqual( int testNumber ) const;

	// Client only
	void			SetAcknowledged( bool ack );
	bool			GetAcknowledged( void ) const;

	// For conversion to/from integer
	int				GetRaw( void ) const;
	void			SetRaw( int raw );

	char const		*Describe( void ) const;

	// Equality test
	bool operator ==( const CPredictableId& other ) const;
	bool operator !=( const CPredictableId& other ) const;
private:
	void			SetCommandNumber( int commandNumber );
	void			SetPlayer( int playerIndex );
	void			SetInstanceNumber( int counter );

	// Encoding bits, should total 32
	struct bitfields
	{
		 unsigned int ack		: 1;	// 1
		 unsigned int player	: 5;	// 6
		 unsigned int command	: 10;	// 16
		 unsigned int hash		: 12;	// 28
		 unsigned int instance	: 4;	// 32
	} m_PredictableID;
};

// This can be empty, the class has a proper constructor
FORCEINLINE void NetworkVarConstruct( CPredictableId &x ) {}

#endif

#endif // PREDICTABLEID_H
