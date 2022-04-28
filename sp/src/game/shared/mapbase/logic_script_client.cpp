//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Custom client-side equivalent of logic_script.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "vscript_shared.h"
#include "tier1/fmtstr.h"

#ifdef CLIENT_DLL
ConVar cl_script_think_interval( "cl_script_think_interval", "0.1" );
#endif

//-----------------------------------------------------------------------------
// Purpose: An entity that acts as a container for client-side game scripts.
//-----------------------------------------------------------------------------

#define MAX_SCRIPT_GROUP_CLIENT 8

class CLogicScriptClient : public CBaseEntity
{
public:
	DECLARE_CLASS( CLogicScriptClient, CBaseEntity );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

#ifdef CLIENT_DLL
	void OnDataChanged( DataUpdateType_t type )
	{
		BaseClass::OnDataChanged( type );
	
		if ( !m_ScriptScope.IsInitialized() )
		{
			RunVScripts();
		}
	}
#else
	int		UpdateTransmitState() { return SetTransmitState( FL_EDICT_ALWAYS ); }
#endif

	bool KeyValue( const char *szKeyName, const char *szValue )
	{
		if ( FStrEq( szKeyName, "vscripts" ) )
		{
			Q_strcpy( m_iszClientScripts.GetForModify(), szValue );
		}

		return BaseClass::KeyValue( szKeyName, szValue );
	}

	void RunVScripts()
	{
#ifdef CLIENT_DLL
		if (m_iszClientScripts == NULL_STRING)
		{
			CGMsg( 0, CON_GROUP_VSCRIPT, "%s has no client scripts", GetDebugName() );
			return;
		}

		if (g_pScriptVM == NULL)
		{
			return;
		}

		ValidateScriptScope();

		// All functions we want to have call chained instead of overwritten
		// by other scripts in this entities list.
		static const char* sCallChainFunctions[] =
		{
			"OnPostSpawn",
			"Precache"
		};

		ScriptLanguage_t language = g_pScriptVM->GetLanguage();

		// Make a call chainer for each in this entities scope
		for (int j = 0; j < ARRAYSIZE( sCallChainFunctions ); ++j)
		{

			if (language == SL_PYTHON)
			{
				// UNDONE - handle call chaining in python
				;
			}
			else if (language == SL_SQUIRREL)
			{
				//TODO: For perf, this should be precompiled and the %s should be passed as a parameter
				HSCRIPT hCreateChainScript = g_pScriptVM->CompileScript( CFmtStr( "%sCallChain <- CSimpleCallChainer(\"%s\", self.GetScriptScope(), true)", sCallChainFunctions[j], sCallChainFunctions[j] ) );
				g_pScriptVM->Run( hCreateChainScript, (HSCRIPT)m_ScriptScope );
			}
		}

		char szScriptsList[255];
		Q_strcpy( szScriptsList, m_iszClientScripts.Get() );
		CUtlStringList szScripts;

		V_SplitString( szScriptsList, " ", szScripts );

		for (int i = 0; i < szScripts.Count(); i++)
		{
			CGMsg( 0, CON_GROUP_VSCRIPT, "%s executing script: %s\n", GetDebugName(), szScripts[i] );

			RunScriptFile( szScripts[i], IsWorld() );

			for (int j = 0; j < ARRAYSIZE( sCallChainFunctions ); ++j)
			{
				if (language == SL_PYTHON)
				{
					// UNDONE - handle call chaining in python
					;
				}
				else if (language == SL_SQUIRREL)
				{
					//TODO: For perf, this should be precompiled and the %s should be passed as a parameter.
					HSCRIPT hRunPostScriptExecute = g_pScriptVM->CompileScript( CFmtStr( "%sCallChain.PostScriptExecute()", sCallChainFunctions[j] ) );
					g_pScriptVM->Run( hRunPostScriptExecute, (HSCRIPT)m_ScriptScope );
				}
			}
		}

		if (m_bClientThink)
		{
			SetNextClientThink( CLIENT_THINK_ALWAYS );
		}
#else
		// Avoids issues from having m_iszVScripts set without actually having a script scope
		ValidateScriptScope();

		if (m_bRunOnServer)
		{
			BaseClass::RunVScripts();
		}
#endif
	}

#ifdef CLIENT_DLL
	void ClientThink()
	{
		ScriptVariant_t varThinkRetVal;
		if (CallScriptFunction("ClientThink", &varThinkRetVal))
		{
			float flThinkFrequency = 0.0f;
			if (!varThinkRetVal.AssignTo(&flThinkFrequency))
			{
				// use default think interval if script think function doesn't provide one
				flThinkFrequency = cl_script_think_interval.GetFloat();
			}

			if (flThinkFrequency == CLIENT_THINK_ALWAYS)
				SetNextClientThink( CLIENT_THINK_ALWAYS );
			else
				SetNextClientThink( gpGlobals->curtime + flThinkFrequency );
		}
		else
		{
			DevWarning("%s FAILED to call client script think function!\n", GetDebugName());
		}

		BaseClass::ClientThink();
	}

	void OnSave()
	{
		// HACKHACK: Save the next think in the VM since the VM is saved
		if (m_bClientThink)
		{
			g_pScriptVM->SetValue( m_ScriptScope, "__c_think", GetNextThink() );
		}

		BaseClass::OnSave();
	}

	void OnRestore()
	{
		// HACKHACK: See OnSave()
		if (m_bClientThink)
		{
			ScriptVariant_t flNextThink;
			if (g_pScriptVM->GetValue( m_ScriptScope, "__c_think", &flNextThink ))
			{
				SetNextClientThink( flNextThink );
			}
		}

		BaseClass::OnRestore();
	}

	void ReceiveMessage( int classID, bf_read &msg )
	{
		if ( classID != GetClientClass()->m_ClassID )
		{
			BaseClass::ReceiveMessage( classID, msg );
			return;
		}

		char szFunction[64];
		msg.ReadString( szFunction, sizeof( szFunction ) );

		if ( m_ScriptScope.IsInitialized() )
		{
			CallScriptFunction( szFunction, NULL );
		}
		else
		{
			CGMsg( 0, CON_GROUP_VSCRIPT, "%s script scope not initialized!\n", GetDebugName() );
		}
	}
#endif

#ifdef GAME_DLL
	void InputCallScriptFunctionClient( inputdata_t &inputdata )
	{
		const char *pszFunction = inputdata.value.String();
		if ( V_strlen( pszFunction ) >= 64 )
		{
			Msg( "%s CallScriptFunctionClient: \"%s\" is too long at %i characters, must be 64 or less\n", GetDebugName(), pszFunction, V_strlen(pszFunction)+1 );
			return;
		}

		EntityMessageBegin( this, true );
			WRITE_STRING( pszFunction );
		MessageEnd();
	}
#endif

	//CNetworkArray( string_t, m_iszGroupMembers, MAX_SCRIPT_GROUP_CLIENT );
	CNetworkString( m_iszClientScripts, 128 );
	CNetworkVar( bool, m_bClientThink );

#ifndef CLIENT_DLL
	bool m_bRunOnServer;
#endif
};

LINK_ENTITY_TO_CLASS( logic_script_client, CLogicScriptClient );

BEGIN_DATADESC( CLogicScriptClient )

	// TODO: Does this need to be saved?
	//DEFINE_AUTO_ARRAY( m_iszClientScripts, FIELD_CHARACTER ),

	//DEFINE_KEYFIELD( m_iszGroupMembers[0], FIELD_STRING, "Group00"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[1], FIELD_STRING, "Group01"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[2], FIELD_STRING, "Group02"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[3], FIELD_STRING, "Group03"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[4], FIELD_STRING, "Group04"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[5], FIELD_STRING, "Group05"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[6], FIELD_STRING, "Group06"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[7], FIELD_STRING, "Group07"),

	DEFINE_KEYFIELD( m_bClientThink, FIELD_BOOLEAN, "ClientThink" ),

#ifndef CLIENT_DLL
	DEFINE_KEYFIELD( m_bRunOnServer, FIELD_BOOLEAN, "RunOnServer" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "CallScriptFunctionClient", InputCallScriptFunctionClient ),
#endif

END_DATADESC()

IMPLEMENT_NETWORKCLASS_DT( CLogicScriptClient, DT_LogicScriptClient )

#ifdef CLIENT_DLL
	//RecvPropArray( RecvPropString( RECVINFO( m_iszGroupMembers[0] ) ), m_iszGroupMembers ),
	RecvPropString( RECVINFO( m_iszClientScripts ) ),
	RecvPropBool( RECVINFO( m_bClientThink ) ),
#else
	//SendPropArray( SendPropStringT( SENDINFO_ARRAY( m_iszGroupMembers ) ), m_iszGroupMembers ),
	SendPropString( SENDINFO( m_iszClientScripts ) ),
	SendPropBool( SENDINFO( m_bClientThink ) ),
#endif

END_NETWORK_TABLE()
