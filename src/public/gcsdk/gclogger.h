//========= Copyright (c), Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef GCLOGGER_H
#define GCLOGGER_H




namespace GCSDK
{
	//a class that defines an output group for messages that can be output to the console or to a log. This allows for individual filtering of different groups to different
	//outputs to control log spamming
	class CGCEmitGroup
	{
	public:

		//the different severity levels for a message
		enum EMsgLevel
		{
			kMsg_Error = 1,
			kMsg_Warning = 2,
			kMsg_Msg = 3,
			kMsg_Verbose = 4
		};

		//constructs a group given a static string for the name of the group, and the name for the console variables that control the console and log output levels. Note
		//that to help with consistency, DECLARE_GC_EMIT_GROUP should be used for declaring these objects over manually providing names for all fields
		CGCEmitGroup( const char* pszGroupName, const char* pszConsoleVar, const char* pszLogVar, const char* pszDefaultConsole, const char* pszDefaultLog )	
			: m_pszGroupName( pszGroupName )
		{}

		//get the name of the group and current filter levels
		const char*	GetName() const				{ return m_pszGroupName; }
			int		GetConsoleLevel() const			{ return 3; }
			int		GetLogLevel() const				{ return 3; }

		//these will output text for each of the appropriate severities, from Verbose (defaulted to filtered out), to critical error. These are not intended to be called directly, as the preparation
		//of all the command line arguments can be costly, and the EG_XXXX macros should be used instead to avoid this cost.
		void	Internal_AssertError( PRINTF_FORMAT_STRING const char *pchMsg, ... ) const FMTFUNCTION( 2, 3 );
		void	Internal_Error( PRINTF_FORMAT_STRING const char *pchMsg, ... ) const FMTFUNCTION( 2, 3 );
		void	Internal_Warning( PRINTF_FORMAT_STRING const char *pchMsg, ... ) const FMTFUNCTION( 2, 3 );
		void	Internal_Msg( PRINTF_FORMAT_STRING const char *pchMsg, ... ) const FMTFUNCTION( 2, 3 );
		void	Internal_Verbose( PRINTF_FORMAT_STRING const char *pchMsg, ... ) const FMTFUNCTION( 2, 3 );
		void	Internal_Emit( EMsgLevel eLvl, PRINTF_FORMAT_STRING const char *pchMsg, ... ) const FMTFUNCTION( 3, 4 );
		void	Internal_BoldMsg( PRINTF_FORMAT_STRING const char *pchMsg, ... ) const FMTFUNCTION( 2, 3 );

		//same as the above, but takes a var args structure
		void	AssertErrorV( PRINTF_FORMAT_STRING const char *pchMsg, va_list vaArgs ) const;		
		void	ErrorV( PRINTF_FORMAT_STRING const char *pchMsg, va_list vaArgs ) const;
		void	WarningV( PRINTF_FORMAT_STRING const char *pchMsg, va_list vaArgs ) const;		
		void	MsgV( PRINTF_FORMAT_STRING const char *pchMsg, va_list vaArgs ) const;		
		void	VerboseV( PRINTF_FORMAT_STRING const char *pchMsg, va_list vaArgs ) const;
		void	EmitV( EMsgLevel eLvl, PRINTF_FORMAT_STRING const char *pchMsg, va_list vaArgs ) const;
		void	BoldMsgV( PRINTF_FORMAT_STRING const char *pchMsg, va_list vaArgs ) const;		

	private:
		//the display name of this group, must be a static string
		const char*		m_pszGroupName;
	};

	//macros to emit to an emit group. These should be used so that you don't have to pay the cost of formatting parameters that won't be used if the output isn't turned on anyway.
	//Note the use of do{}while, this to prevent the if from pairing with a following else if they don't use braces around the print
	#define EG_ASSERTERROR( EmitGroup, ... )	do{ if( EmitGroup.GetConsoleLevel() >= CGCEmitGroup::kMsg_Error || EmitGroup.GetLogLevel() >= CGCEmitGroup::kMsg_Error ) EmitGroup.Internal_AssertError( __VA_ARGS__ ); } while(0)
	#define EG_ERROR( EmitGroup, ... )			do{ if( EmitGroup.GetConsoleLevel() >= CGCEmitGroup::kMsg_Error || EmitGroup.GetLogLevel() >= CGCEmitGroup::kMsg_Error ) EmitGroup.Internal_Error( __VA_ARGS__ ); } while(0)
	#define EG_WARNING( EmitGroup, ... )		do{ if( EmitGroup.GetConsoleLevel() >= CGCEmitGroup::kMsg_Warning || EmitGroup.GetLogLevel() >= CGCEmitGroup::kMsg_Warning ) EmitGroup.Internal_Warning( __VA_ARGS__ ); } while(0)
	#define EG_MSG( EmitGroup, ... )			do{ if( EmitGroup.GetConsoleLevel() >= CGCEmitGroup::kMsg_Msg || EmitGroup.GetLogLevel() >= CGCEmitGroup::kMsg_Msg ) EmitGroup.Internal_Msg( __VA_ARGS__ ); } while(0)
	#define EG_VERBOSE( EmitGroup, ... )		do{ if( EmitGroup.GetConsoleLevel() >= CGCEmitGroup::kMsg_Verbose || EmitGroup.GetLogLevel() >= CGCEmitGroup::kMsg_Verbose ) EmitGroup.Internal_Verbose( __VA_ARGS__ ); } while(0)
	#define EG_EMIT( EmitGroup, eLvl, ... )		do{ if( EmitGroup.GetConsoleLevel() >= eLvl || EmitGroup.GetLogLevel() >= eLvl ) EmitGroup.Internal_Emit( eLvl, __VA_ARGS__ ); } while(0)
	#define EG_BOLDMSG( EmitGroup, ... )		do{ if( EmitGroup.GetConsoleLevel() >= CGCEmitGroup::kMsg_Error || EmitGroup.GetLogLevel() >= CGCEmitGroup::kMsg_Error ) EmitGroup.Internal_BoldMsg( __VA_ARGS__ ); } while(0)

	//----------------------------------------
	//Legacy interface support - Where possible, use the CGCEmitGroup above for more robust functionality
	//----------------------------------------
	extern CGCEmitGroup SPEW_SYSTEM_MISC;
	extern CGCEmitGroup SPEW_JOB;
	extern CGCEmitGroup SPEW_CONSOLE;
	extern CGCEmitGroup SPEW_GC;
	extern CGCEmitGroup SPEW_SQL;
	extern CGCEmitGroup SPEW_NETWORK;
	extern CGCEmitGroup SPEW_SHAREDOBJ;
	extern CGCEmitGroup SPEW_MICROTXN;
	extern CGCEmitGroup SPEW_PROMO;
	extern CGCEmitGroup SPEW_PKGITEM;
	extern CGCEmitGroup SPEW_ECONOMY;
	extern CGCEmitGroup SPEW_THREADS;

	//this one is a macro since it is called in many places and can add considerable overhead to the formatting
	void EGInternal_EmitInfo( const CGCEmitGroup& Group, int iLevel, int iLevelLog, PRINTF_FORMAT_STRING const char *pchMsg, ... ) FMTFUNCTION( 4, 5 );
	#define EmitInfo( EmitGroup, ConsoleLevel, LogLevel, ... )		do{ if( EmitGroup.GetConsoleLevel() >= ( ConsoleLevel ) || EmitGroup.GetLogLevel() >= ( LogLevel ) ) EGInternal_EmitInfo( EmitGroup, ConsoleLevel, LogLevel, __VA_ARGS__ ); } while(0)

	void EmitInfoV( const CGCEmitGroup& Group, int iLevel, int iLevelLog, PRINTF_FORMAT_STRING const char *pchMsg, va_list vaArgs ); 

	void EmitWarning( const CGCEmitGroup& Group, int iLevel, PRINTF_FORMAT_STRING const char *pchMsg, ... ) FMTFUNCTION( 3, 4 );
	void EmitError( const CGCEmitGroup& Group, PRINTF_FORMAT_STRING const char *pchMsg, ... ) FMTFUNCTION( 2, 3 );
	// Emit an assert-like error, generating a minidump
	void EmitAssertError( const CGCEmitGroup& Group, PRINTF_FORMAT_STRING const char *pchMsg, ... ) FMTFUNCTION( 2, 3 );

} // namespace GCSDK

//the use of the typedef is to catch issues with people putting quotes around the group name. Note that the emit group comes first in case they precede this with 'static'
#define DECLARE_GC_EMIT_GROUP_DEFAULTS( VarName, GroupName, ConsoleLevel, LogLevel )	\
	GCSDK::CGCEmitGroup VarName( #GroupName, "console_level_" #GroupName, "log_level_" #GroupName, #ConsoleLevel, #LogLevel ); \
	typedef uint32 __TEmitGroupSanityCheck_##GroupName_##ConsoleLevel_##LogLevel;	

//a utility macro that assists in creating emit groups in a name consistent manner
#define DECLARE_GC_EMIT_GROUP( VarName, GroupName )										DECLARE_GC_EMIT_GROUP_DEFAULTS( VarName, GroupName, 3, 3 )

#endif // GCLOGGER_H
