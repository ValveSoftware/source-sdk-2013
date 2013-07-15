from srcpy.module_generators import SemiSharedModuleGenerator
import settings

from pygccxml import declarations 
from pyplusplus.module_builder import call_policies
from pyplusplus import function_transformers as FT
from pyplusplus import code_creators
from pygccxml.declarations import matchers, pointer_t, reference_t, declarated_t

class GameInterface(SemiSharedModuleGenerator):
    module_name = '_gameinterface'
    
    clientfiles = [
        'cbase.h',
        'gamerules.h',
        'multiplay_gamerules.h',
        'teamplay_gamerules.h',
        # TODO: 'srcpy_gamerules.h',
        'c_recipientfilter.h',
        'tier0/icommandline.h',
    ]
    
    serverfiles = [
        'cbase.h',
        'mathlib/vmatrix.h', 
        'utlvector.h', 
        'shareddefs.h',
        'util.h',
        'iservernetworkable.h',
        #'enginecallback.h',
        'recipientfilter.h',
        'srcpy_usermessage.h',
        'mapentities.h',
        'tier0/icommandline.h',
    ]
    
    files = [
        'convar.h',
        'igameevents.h',
        'irecipientfilter.h',
        'srcpy_gameinterface.h',
        'cdll_int.h',
    ]

    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude() 
        
        # Linux model_t fix ( correct? )
        '''mb.add_declaration_code( '#ifdef _LINUX\r\n' + \
                             'typedef struct model_t {};\r\n' + \
                             '#endif // _LINUX\r\n'
                           )'''
                           
        # Filesystem functions
        # TODO: mb.free_function('PyRemoveFile').include()
        # TODO: mb.free_function('PyRemoveFile').rename('RemoveFile')
        # TODO: mb.free_function('PyRemoveDirectory').include()
        # TODO: mb.free_function('PyRemoveDirectory').rename('RemoveDirectory')
        mb.free_function('PyGetModPath').include()
        mb.free_function('PyGetModPath').rename('GetModPath')
        mb.free_function('PyAsyncFinishAllWrites').include()
        mb.free_function('PyAsyncFinishAllWrites').rename('AsyncFinishAllWrites')
        
        # Engine functions
        mb.free_function('GetLevelName').include()    
        
        # Time
        mb.free_function('Plat_FloatTime').include()
        mb.free_function('Plat_MSTime').include()
        
        # Precache functions
        mb.free_function('PrecacheMaterial').include()
        
        # ConVar wrapper
        cls = mb.class_('PyConVar')
        cls.include()
        cls.rename('ConVar')
        cls.mem_funs().virtuality = 'not virtual'
        cls.mem_fun('Shutdown').exclude()
        
        mb.free_function('PyShutdownConVar').include()
        mb.free_function('PyShutdownConCommand').include()

        # Don't want to include ConVar, so add methods manually...
        # Can't this be done automatically in py++?
        cls.add_registration_code(
            '''def( 
            "AddFlags"
            , (void ( ::ConVar::* )( int ) )( &::ConVar::AddFlags )
            , ( bp::arg("flags") ) )''')
        cls.add_registration_code(
            '''def( 
            "GetBool"
            , (bool ( ::ConVar::* )(  ) const)( &::ConVar::GetBool ) )''')
        cls.add_registration_code(
            '''def( 
            "GetDefault"
            , (char const * ( ::ConVar::* )(  ) const)( &::ConVar::GetDefault ) )''')
        cls.add_registration_code(
            '''def( 
            "GetFloat"
            , (float ( ::ConVar::* )(  ) const)( &::ConVar::GetFloat ) )''')
        cls.add_registration_code(
            '''def( 
            "GetHelpText"
            , (char const * ( ::ConVar::* )(  ) const)( &::ConVar::GetHelpText ) )''')
        cls.add_registration_code(
            '''def( 
            "GetInt"
            , (int ( ::ConVar::* )(  ) const)( &::ConVar::GetInt ) )''')
        cls.add_registration_code(
            '''def( 
            "GetMax"
            , (bool ( ::ConVar::* )( float & ) const)( &::ConVar::GetMax )
            , ( bp::arg("maxVal") ) )''')
        cls.add_registration_code(
            '''def( 
            "GetMin"
            , (bool ( ::ConVar::* )( float & ) const)( &::ConVar::GetMin )
            , ( bp::arg("minVal") ) )''')
        cls.add_registration_code(
            '''def( 
            "GetName"
            , (char const * ( ::ConVar::* )(  ) const)( &::ConVar::GetName ) )''')
        cls.add_registration_code(
            '''def( 
            "GetString"
            , (char const * ( ::ConVar::* )(  ) const)( &::ConVar::GetString ) )''')
        cls.add_registration_code(
            '''def( 
            "IsCommand"
            , (bool ( ::ConVar::* )(  ) const)( &::ConVar::IsCommand ) )''')
        cls.add_registration_code(
            '''def( 
            "IsFlagSet"
            , (bool ( ::ConVar::* )( int ) const)( &::ConVar::IsFlagSet )
            , ( bp::arg("flag") ) )''')
        cls.add_registration_code(
            '''def( 
            "IsRegistered"
            , (bool ( ::ConVar::* )(  ) const)( &::ConVar::IsRegistered ) )''')
        cls.add_registration_code(
            '''def( 
            "Revert"
            , (void ( ::ConVar::* )(  ) )( &::ConVar::Revert ) )''')

        # ConVarRef
        mb.class_('ConVarRef').include()
        mb.mem_funs('GetLinkedConVar').exclude()
        
        # CCommand
        cls = mb.class_('CCommand')
        cls.include()
        cls.mem_funs('Tokenize').exclude()
        cls.mem_funs('ArgV').exclude()
        cls.mem_funs('DefaultBreakSet').exclude()
        
        cls.add_registration_code(
         '''def( 
            "__len__"
            , (int ( ::CCommand::* )(  ) const)( &::CCommand::ArgC ) )''')
        
        # PyConCommand
        cls = mb.class_('PyConCommand')
        cls.include()
        cls.rename('ConCommand')
        cls.var('m_pyCommandCallback').exclude()         # Must be excluded, or else things get broken without errors/warnings!
        cls.mem_funs('Dispatch').exclude()               # Must be excluded, or else things get broken without errors/warnings!
        cls.mem_funs('AutoCompleteSuggest').exclude()
        
        # Virtuality screws up ConCommand. 
        cls.mem_funs().virtuality = 'not virtual' 
        
        # Sending messages
        if self.isserver:
            mb.free_functions('PySendUserMessage').include()
            mb.free_functions('PySendUserMessage').rename('SendUserMessage')
            
        # filters
        mb.class_('IRecipientFilter').include()
        mb.class_('IRecipientFilter').mem_funs().virtuality = 'not virtual' 
        if self.isserver:
            mb.class_('CRecipientFilter').include()
            mb.class_('CRecipientFilter').mem_funs().virtuality = 'not virtual' 
        else:
            mb.class_('C_RecipientFilter').include()
            mb.class_('C_RecipientFilter').mem_funs().virtuality = 'not virtual' 
            
            mb.class_('CLocalPlayerFilter').include()
            #mb.class_('CLocalPlayerFilter').mem_funs().virtuality = 'not virtual' 
            
        # Shared filters
        mb.class_('CSingleUserRecipientFilter').include()
        mb.class_('CBroadcastRecipientFilter').include()
        mb.class_('CReliableBroadcastRecipientFilter').include()
        mb.class_('CPASFilter').include()
        mb.class_('CPASAttenuationFilter').include()
        mb.class_('CPVSFilter').include()
            
        # Gameevents
        mb.class_('PyGameEventListener').include()
        mb.class_('PyGameEventListener').rename('GameEventListener')
        mb.class_('PyGameEventListener').mem_fun('PyFireGameEvent').rename('FireGameEvent')
        mb.class_('PyGameEventListener').add_registration_code('def( "ListenForGameEvent", (void ( ::PyGameEventListener::* )( char const * ) )( &::PyGameEventListener::ListenForGameEvent ), bp::arg("name") ) ')
        mb.class_('PyGameEventListener').add_registration_code('def( "StopListeningForAllEvents", (void ( ::PyGameEventListener::* )() )( &::PyGameEventListener::StopListeningForAllEvents ) ) ')
        
        mb.class_('PyGameEvent').include()
        mb.class_('PyGameEvent').rename('GameEvent')
        mb.class_('PyGameEvent').mem_fun('Init').exclude()
        mb.class_('PyGameEvent').mem_fun('GetEvent').exclude()
        
        if self.isserver:
            mb.free_function('PyFireGameEvent').include()
            mb.free_function('PyFireGameEvent').rename('FireGameEvent')
        else:
            mb.free_function('PyFireGameEventClientSide').include()
            mb.free_function('PyFireGameEventClientSide').rename('FireGameEventClientSide')
                
        # Player info
        mb.class_('py_player_info_s').include()
        mb.class_('py_player_info_s').rename('PlayerInfo')
        
        # Get map header
        mb.free_function('PyGetMapHeader').include()
        mb.free_function('PyGetMapHeader').rename('GetMapHeader')
        mb.class_('dheader_t').include()
        mb.class_('lump_t').include()
        mb.mem_funs('GetBaseMap').exclude()
        #mb.mem_funs('DataMapAccess').include()
        #mb.mem_funs('DataMapInit').include()
        mb.vars('m_DataMap').exclude()
        
        # Content mounting
        mb.free_function('PyAddSearchPath').include()
        mb.free_function('PyAddSearchPath').rename('AddSearchPath')
        mb.free_function('PyRemoveSearchPath').include()
        mb.free_function('PyRemoveSearchPath').rename('RemoveSearchPath')
        mb.free_function('PyGetSearchPath').include()
        mb.free_function('PyGetSearchPath').rename('GetSearchPath')
        
        mb.enum('SearchPathAdd_t').include()
        mb.enum('FilesystemMountRetval_t').include()
        
        # GameSystem
        mb.class_('CBaseGameSystem').include()
        mb.class_('CBaseGameSystemPerFrame').include()
        mb.class_('CAutoGameSystem').include()
        mb.class_('CAutoGameSystemPerFrame').include()
        mb.mem_funs('IsPerFrame').virtuality = 'not virtual' 
        mb.mem_funs('SafeRemoveIfDesired').virtuality = 'not virtual'
        mb.class_('CAutoGameSystem').rename('AutoGameSystem')
        mb.class_('CAutoGameSystemPerFrame').rename('AutoGameSystemPerFrame')
        
        cls = mb.class_('IGameSystem')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual' 
        #cls.mem_funs('IsPerFrame').virtuality = 'virtual' 
        #cls.mem_funs('SafeRemoveIfDesired').virtuality = 'virtual' 
        if self.isserver:
            cls.mem_funs('RunCommandPlayer').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            cls.mem_funs('RunCommandUserCmd').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            
        # Engine
        if self.isserver:
            cls = mb.class_('PyVEngineServer')
            cls.rename('VEngineServer')
            cls.include()
        else:
            cls = mb.class_('PyVEngineClient')
            cls.rename('VEngineClient')
            cls.include()
            cls.mem_funs('LoadModel').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.add_registration_code( "bp::scope().attr( \"engine\" ) = boost::ref(pyengine);" )   
        
        # Command line access
        cls = mb.class_('ICommandLine')
        cls.include()
        mb.free_function('CommandLine_Tier0').include()
        mb.free_function('CommandLine_Tier0').rename('CommandLine')
        mb.free_function('CommandLine_Tier0').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        
        # Accessing model info
        cls = mb.class_('PyVModelInfo')
        cls.rename('VModelInfo')
        cls.include()
        cls.mem_funs('GetModel').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        cls.mem_funs('FindOrLoadModel').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.add_registration_code( "bp::scope().attr( \"modelinfo\" ) = boost::ref(pymodelinfo);" )   
        
        if self.isserver:
            mb.free_function('PyMapEntity_ParseAllEntities').include()
            mb.free_function('PyMapEntity_ParseAllEntities').rename('MapEntity_ParseAllEntities')
            
            cls = mb.class_('IMapEntityFilter')
            cls.include()
            cls.mem_fun('CreateNextEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
            
            mb.class_('CMapEntityRef').include()
            mb.free_function('PyGetMapEntityRef').include()
            mb.free_function('PyGetMapEntityRef').rename('GetMapEntityRef')
            mb.free_function('PyGetMapEntityRefIteratorHead').include()
            mb.free_function('PyGetMapEntityRefIteratorHead').rename('GetMapEntityRefIteratorHead')
            mb.free_function('PyGetMapEntityRefIteratorNext').include()
            mb.free_function('PyGetMapEntityRefIteratorNext').rename('GetMapEntityRefIteratorNext')
            
            # Edicts
            cls = mb.class_('edict_t')
            cls.include()
            cls.mem_fun('GetCollideable').exclude()
            
        # model_t
        cls = mb.class_('wrap_model_t')
        cls.include()
        cls.rename('model_t')
        cls.calldefs('wrap_model_t').exclude()
        cls.no_init = True
        cls.vars('pModel').exclude()

        mb.add_registration_code( "ptr_model_t_to_wrap_model_t();" )
        mb.add_registration_code( "const_ptr_model_t_to_wrap_model_t();" )
        mb.add_registration_code( "wrap_model_t_to_model_t();" )
        
        # LUMPS
        mb.add_registration_code( "bp::scope().attr( \"LUMP_ENTITIES\" ) = (int)LUMP_ENTITIES;" )
        
        # Defines
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_NONE\" ) = (int)FCVAR_NONE;" )
        
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_UNREGISTERED\" ) = (int)FCVAR_UNREGISTERED;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_DEVELOPMENTONLY\" ) = (int)FCVAR_DEVELOPMENTONLY;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_GAMEDLL\" ) = (int)FCVAR_GAMEDLL;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_CLIENTDLL\" ) = (int)FCVAR_CLIENTDLL;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_HIDDEN\" ) = (int)FCVAR_HIDDEN;" )
        
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_PROTECTED\" ) = (int)FCVAR_PROTECTED;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_SPONLY\" ) = (int)FCVAR_SPONLY;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_ARCHIVE\" ) = (int)FCVAR_ARCHIVE;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_NOTIFY\" ) = (int)FCVAR_NOTIFY;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_USERINFO\" ) = (int)FCVAR_USERINFO;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_CHEAT\" ) = (int)FCVAR_CHEAT;" )
        
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_PRINTABLEONLY\" ) = (int)FCVAR_PRINTABLEONLY;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_UNLOGGED\" ) = (int)FCVAR_UNLOGGED;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_NEVER_AS_STRING\" ) = (int)FCVAR_NEVER_AS_STRING;" )
        
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_REPLICATED\" ) = (int)FCVAR_REPLICATED;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_DEMO\" ) = (int)FCVAR_DEMO;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_DONTRECORD\" ) = (int)FCVAR_DONTRECORD;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_NOT_CONNECTED\" ) = (int)FCVAR_NOT_CONNECTED;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_ARCHIVE_XBOX\" ) = (int)FCVAR_ARCHIVE_XBOX;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_SERVER_CAN_EXECUTE\" ) = (int)FCVAR_SERVER_CAN_EXECUTE;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_SERVER_CANNOT_QUERY\" ) = (int)FCVAR_SERVER_CANNOT_QUERY;" )
        mb.add_registration_code( "bp::scope().attr( \"FCVAR_CLIENTCMD_CAN_EXECUTE\" ) = (int)FCVAR_CLIENTCMD_CAN_EXECUTE;" )
        
        # Excludes
        #if self.isserver:
        #    mb.mem_funs(matchers.calldef_matcher_t(return_type=pointer_t(declarated_t(mb.class_('CTeam'))))).exclude()
            
    def AddAdditionalCode(self, mb):
        header = code_creators.include_t( 'srcpy_gameinterface_converters.h' )
        mb.code_creator.adopt_include(header)
        super(GameInterface, self).AddAdditionalCode(mb)
        