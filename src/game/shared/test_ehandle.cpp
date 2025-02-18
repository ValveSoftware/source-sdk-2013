//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// In this test, the server makes an entity (call it A) that has an EHANDLE to another 
// entity (call it B). Intitially, A is sent to the client but B is not. Later, 
// the server decides to send B to the client too. At that point, without resending A's EHANDLE,
// the client's EHANDLE should access B.

#if defined( GAME_DLL )

	// ------------------------------------------------------------------------------------ //
	// The main entity class (class A).
	// ------------------------------------------------------------------------------------ //
	class CHandleTest : public CBaseEntity
	{
	public:
		DECLARE_CLASS( CHandleTest, CBaseEntity );
		DECLARE_SERVERCLASS();

		CHandleTest()
		{
			m_bSendHandle = false;
		}
		
		virtual int UpdateTransmitState()
		{
			// ALWAYS transmit to all clients.
			return SetTransmitState( FL_EDICT_ALWAYS );
		}

		virtual void SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
		{
			if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
				return;

			BaseClass::SetTransmit( pInfo, bAlways );
			
			// Force the thing we're pointing at to be sent too?
			if ( m_bSendHandle )
				m_Handle->SetTransmit( pInfo, bAlways );
		}

		CNetworkHandle( CBaseEntity, m_Handle );
		CNetworkVar( bool, m_bSendHandle );
	};

	IMPLEMENT_SERVERCLASS_ST( CHandleTest, DT_HandleTest )
		SendPropEHandle( SENDINFO( m_Handle ) ),
		SendPropInt( SENDINFO( m_bSendHandle ) )
	END_SEND_TABLE()
	
	LINK_ENTITY_TO_CLASS( handle_test, CHandleTest );


	// ------------------------------------------------------------------------------------ //
	// The class pointed to by the handle.
	// ------------------------------------------------------------------------------------ //
	class CHandleDummy : public CBaseEntity
	{
		DECLARE_CLASS( CHandleDummy, CBaseEntity );
	public:
	};
	LINK_ENTITY_TO_CLASS( handle_dummy, CHandleDummy );

	CHandle<CHandleTest> g_HandleTest;

	// The test runs this command.
	void CC_Test_EHandle()
	{
		if ( g_HandleTest.Get() )
		{
			g_HandleTest->m_bSendHandle = !g_HandleTest->m_bSendHandle;
		}
		else
		{
			CHandleTest *pHolder = CREATE_ENTITY( CHandleTest, "handle_test" );
			pHolder->m_Handle = CREATE_ENTITY( CHandleDummy, "handle_dummy" );
			pHolder->Spawn();
			g_HandleTest = pHolder;
			Msg( "Created EHANDLE test entity. Run this command again to transmit the second ent.\n" );
		}
	}
	ConCommand Test_EHandle( "Test_EHandle", CC_Test_EHandle, 0, FCVAR_CHEAT );
	

#else

	class C_HandleTest : public C_BaseEntity
	{
	public:
		DECLARE_CLASS( C_HandleTest, C_BaseEntity );
		DECLARE_CLIENTCLASS();
		
		C_HandleTest()
		{
		}

		virtual void OnDataChanged( DataUpdateType_t type )
		{
			Msg( "m_bSendHandle: %d, m_Handle.Get: 0x%p\n", m_bSendHandle, m_Handle.Get() );
		}

		EHANDLE m_Handle;
		bool m_bSendHandle;
	};

	IMPLEMENT_CLIENTCLASS_DT( C_HandleTest, DT_HandleTest, CHandleTest )
		RecvPropEHandle( RECVINFO( m_Handle ) ),
		RecvPropInt( RECVINFO( m_bSendHandle ) )
	END_RECV_TABLE()


#endif


