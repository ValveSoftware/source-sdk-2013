from srcpy.module_generators import SemiSharedModuleGenerator

from pyplusplus import function_transformers as FT
from pyplusplus.module_builder import call_policies
from pyplusplus import messages
from pygccxml.declarations import matchers, pointer_t, reference_t, declarated_t
from pyplusplus import code_creators

# Templates for client and server class
tmpl_clientclass = '''virtual ClientClass* GetClientClass() {
    if( GetCurrentThreadId() != g_hPythonThreadID )
        return %(clsname)s::GetClientClass();
    ClientClass *pClientClass = SrcPySystem()->Get<ClientClass *>( "pyClientClass", GetPyInstance(), NULL, true );
    if( pClientClass )
        return pClientClass;
    return %(clsname)s::GetClientClass();
}
'''

tmpl_serverclass = '''virtual ServerClass* GetServerClass() {
    #if defined(_WIN32)
    #if defined(_DEBUG)
    Assert( GetCurrentThreadId() == g_hPythonThreadID );
    #elif defined(PY_CHECKTHREADID)
    if( GetCurrentThreadId() != g_hPythonThreadID )
        Error( "GetServerClass: Client? %d. Thread ID is not the same as in which the python interpreter is initialized! %%d != %%d. Tell a developer.\\n", CBaseEntity::IsClient(), g_hPythonThreadID, GetCurrentThreadId() );
    #endif // _DEBUG/PY_CHECKTHREADID
    #endif // _WIN32
    #if defined(_DEBUG) || defined(PY_CHECK_LOG_OVERRIDES)
    if( py_log_overrides.GetBool() )
        Msg("Calling GetServerClass(  ) of Class: %(clsname)s\\n");
    #endif // _DEBUG/PY_CHECK_LOG_OVERRIDES
    ServerClass *pServerClass = SrcPySystem()->Get<ServerClass *>( "pyServerClass", GetPyInstance(), NULL, true );
    if( pServerClass )
        return pServerClass;
    return %(clsname)s::GetServerClass();
}
'''

# Templates for entities handles and converters
tmpl_enthandle = '''{ //::%(handlename)s
        typedef bp::class_< %(handlename)s, bp::bases< CBaseHandle > > %(handlename)s_exposer_t;
        %(handlename)s_exposer_t %(handlename)s_exposer = %(handlename)s_exposer_t( "%(handlename)s", bp::init< >() );
        %(handlename)s_exposer.def( bp::init< %(clsname)s * >(( bp::arg("pVal") )) );
        %(handlename)s_exposer.def( bp::init< int, int >(( bp::arg("iEntry"), bp::arg("iSerialNumber") )) );
        { //::%(handlename)s::GetAttr
        
            typedef bp::object ( ::%(handlename)s::*GetAttr_function_type )( const char * ) const;
            
            %(handlename)s_exposer.def( 
                "__getattr__"
                , GetAttr_function_type( &::%(handlename)s::GetAttr )
            );
        
        }
        { //::%(handlename)s::Cmp
        
            typedef bool ( ::%(handlename)s::*Cmp_function_type )( bp::object ) const;
            
            %(handlename)s_exposer.def( 
                "__cmp__"
                , Cmp_function_type( &::%(handlename)s::Cmp )
            );
        
        }
        { //::%(handlename)s::NonZero
        
            typedef bool ( ::%(handlename)s::*NonZero_function_type )( ) const;
            
            %(handlename)s_exposer.def( 
                "__nonzero__"
                , NonZero_function_type( &::%(handlename)s::NonZero )
            );
        
        }
        { //::%(handlename)s::Set
        
            typedef void ( ::%(handlename)s::*Set_function_type )( %(clsname)s * ) const;
            
            %(handlename)s_exposer.def( 
                "Set"
                , Set_function_type( &::%(handlename)s::Set )
            );
        
        }
        { //::%(handlename)s::GetSerialNumber
        
            typedef int ( ::%(handlename)s::*GetSerialNumber_function_type )( ) const;
            
            %(handlename)s_exposer.def( 
                "GetSerialNumber"
                , GetSerialNumber_function_type( &::%(handlename)s::GetSerialNumber )
            );
        
        }
        { //::%(handlename)s::GetEntryIndex
        
            typedef int ( ::%(handlename)s::*GetEntryIndex_function_type )(  ) const;
            
            %(handlename)s_exposer.def( 
                "GetEntryIndex"
                , GetEntryIndex_function_type( &::%(handlename)s::GetEntryIndex )
            );
        
        }
        %(handlename)s_exposer.def( bp::self != bp::self );
        %(handlename)s_exposer.def( bp::self == bp::self );
    }
'''

tmpl_ent_converters = '''
struct %(ptr_convert_to_py_name)s : bp::to_python_converter<%(clsname)s *, ptr_%(clsname)s_to_handle>
{
    static PyObject* convert(%(clsname)s *s)
    {
        return s ? bp::incref(s->GetPyHandle().ptr()) : bp::incref(Py_None);
    }
};

struct %(convert_to_py_name)s : bp::to_python_converter<%(clsname)s, %(clsname)s_to_handle>
{
    static PyObject* convert(const %(clsname)s &s)
    {
        return bp::incref(s.GetPyHandle().ptr());
    }
};

struct %(convert_from_py_name)s
{
    handle_to_%(clsname)s()
    {
        bp::converter::registry::insert(
            &extract_%(clsname)s, 
            bp::type_id<%(clsname)s>()
            );
    }

    static void* extract_%(clsname)s(PyObject* op){
       CBaseHandle h = bp::extract<CBaseHandle>(op);
       if( h.Get() == NULL )
           return Py_None;
       return h.Get();
    }
};
'''

class Entities(SemiSharedModuleGenerator):
    module_name = '_entities'
    split = True
    
    # Includes
    clientfiles = [
        'cbase.h',
        'takedamageinfo.h',
        'c_ai_basenpc.h',
    ]

    serverfiles = [
        'cbase.h',
        'SkyCamera.h',
        'ai_basenpc.h',
    ]
    
    files = [
        'srcpy_entities.h',
    ]
    
    # List of entity classes want to have exposed
    cliententities = [ 
        'C_BaseEntity', 
    ]
    
    serverentities = [ 
        'CBaseEntity', 
    ]
    
    # Handle expose code
    def AddEntityConverter(self, mb, clsname):
        cls = mb.class_(clsname)
        
        handlename = '%sHANDLE' % (clsname)
        
        ptr_convert_to_py_name = 'ptr_%s_to_handle' % (clsname)
        convert_to_py_name = '%s_to_handle' % (clsname)
        convert_from_py_name = 'handle_to_%s' % (clsname)
        
        # Add handle typedef
        mb.add_declaration_code( 'typedef CEPyHandle< %s > %s;\r\n'% (clsname, handlename) )
        
        # Expose handle code
        mb.add_registration_code( tmpl_enthandle % {'clsname' : clsname, 'handlename' : handlename}, True )
        
        # Add declaration code for converters
        mb.add_declaration_code( tmpl_ent_converters % {
            'clsname' : clsname,
            'ptr_convert_to_py_name' : ptr_convert_to_py_name,
            'convert_to_py_name' : convert_to_py_name,
            'convert_from_py_name' : convert_from_py_name,
        })
        
        # Add registration code
        mb.add_registration_code( ptr_convert_to_py_name+"();" )
        mb.add_registration_code( convert_to_py_name+"();" )
        mb.add_registration_code( convert_from_py_name+"();" )
        
    # Parse methods
    def SetupEntityClass(self, mb, clsname):
        ''' This function is called for both exposed client and server entities and 
            applies shared functionality. '''
        cls = mb.class_(clsname)
        
        cls.include()
        cls.calldefs(matchers.access_type_matcher_t( 'protected' ), allow_empty=True).exclude()
        cls.no_init = True
        
        # Be selective about we need to override
        # DO NOT REMOVE. Some functions are not thread safe, which will cause runtime errors because we did not setup python threadsafe (slower)
        cls.mem_funs(allow_empty=True).virtuality = 'not virtual' 

        # Add converters + a python handle class
        self.AddEntityConverter(mb, clsname)  
        
        # Use by converters to check if a Python Object is attached
        cls.add_wrapper_code(
            'virtual PyObject *GetPySelf() const { return bp::detail::wrapper_base_::get_owner(*this); }'
        )
        
        # Apply common rules to the entity class
        
        # Don't care about the following:
        cls.vars(lambda decl: 'NetworkVar' in decl.name, allow_empty=True).exclude()        # Don't care or needs a better look
        cls.classes(lambda decl: 'NetworkVar' in decl.name, allow_empty=True).exclude()        # Don't care or needs a better look
        cls.mem_funs(lambda decl: 'YouForgotToImplement' in decl.name, allow_empty=True).exclude()   # Don't care
        cls.mem_funs('GetPyInstance').exclude()          # Not needed, used when converting entities to python
        #cls.mem_funs('ClearPyInstance').exclude()        # Not needed, used for cleaning up python entities
        cls.mem_funs('PyAllocate').exclude()             # Python Intern only
        cls.mem_funs('PyDeallocate').exclude()             # Python Intern only
        cls.mem_funs('GetBaseMap').exclude()             # Not needed
        cls.mem_funs('GetDataDescMap').exclude()         # Not needed
        cls.mem_funs('GetDataDescMap').exclude()         # Not needed
        cls.mem_funs('GetRefEHandle').exclude()          # We already got an auto conversion to a safe handle
        cls.mem_funs('GetBeamTraceFilter').exclude()     # Don't care
        cls.mem_funs('GetCollideable').exclude()         # Don't care for now
        cls.mem_funs('GetModel').exclude()               # Do we want this? 
        cls.mem_funs('GetTeam').exclude()     # Don't care
        
        cls.mem_funs('VPhysicsGetObject').exclude()         # Don't care for now
        cls.mem_funs('VPhysicsInitNormal').exclude()         # Don't care for now
        cls.mem_funs('VPhysicsInitShadow').exclude()         # Don't care for now
        cls.mem_funs('VPhysicsInitStatic').exclude()         # Don't care for now
        
        cls.mem_funs('AddEntityToGroundList').exclude()
        
        # Certain python methods should be excluded
        cls.mem_funs('CreatePyHandle').exclude() # Use GetHandle instead.
        
        # 
        #cls.mem_funs(matchers.calldef_matcher_t(return_type='::%s *' % (clsname))).exclude()
        #cls.calldefs(matchers.calldef_matcher_t(return_type=pointer_t(declarated_t(cls)))).call_policies = call_policies.return_value_policy(call_policies.return_by_value)

        matrix3x4 = mb.class_('matrix3x4_t')
        cls.calldefs(matchers.calldef_matcher_t(return_type=reference_t(declarated_t(matrix3x4)))).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        
        cls.vars().exclude()
        
        return cls
            
    def ParseClientEntities(self, mb):
        # Made not virtual so no wrapper code is generated in IClientUnknown and IClientEntity
        mb.class_('IClientRenderable').mem_funs().virtuality = 'not virtual' 
        mb.class_('IClientNetworkable').mem_funs().virtuality = 'not virtual' 
        mb.class_('IClientThinkable').mem_funs().virtuality = 'not virtual' 
        
        self.IncludeEmptyClass(mb, 'IClientUnknown')
        self.IncludeEmptyClass(mb, 'IClientEntity')
        
        for clsname in self.cliententities:
            cls = self.SetupEntityClass(mb, clsname)

            # Check if the python class is networkable. Add code for getting the "ClientClass" if that's the case.
            decl = cls.mem_funs('GetPyNetworkType', allow_empty=True)
            if decl:
                decl.Include()
                cls.add_wrapper_code( tmpl_clientclass % {'clsname' : clsname})
                
            # Excludes
            cls.mem_funs('FirstShadowChild').exclude()
            cls.mem_funs('GetClientClass').exclude()
            cls.mem_funs('GetClientNetworkable').exclude()
            cls.mem_funs('GetClientRenderable').exclude()
            cls.mem_funs('GetClientThinkable').exclude()
            cls.mem_funs('GetClientVehicle').exclude()
            cls.mem_funs('GetMouth').exclude()
            cls.mem_funs('GetOriginInterpolator').exclude()
            cls.mem_funs('GetPVSNotifyInterface').exclude()
            cls.mem_funs('GetPredDescMap').exclude()
            cls.mem_funs('GetRenderClipPlane').exclude() # Pointer to 4 floats, requires manual conversion...
            cls.mem_funs('GetRotationInterpolator').exclude()
            cls.mem_funs('GetShadowParent').exclude()
            cls.mem_funs('GetThinkHandle').exclude()
            cls.mem_funs('GetVarMapping').exclude()
            cls.mem_funs('GetDataTableBasePtr').exclude()
            cls.mem_funs('GetOriginalNetworkDataObject').exclude()
            cls.mem_funs('GetPredictedFrame').exclude()
            cls.mem_funs('NextShadowPeer').exclude()
            cls.mem_funs('OnNewModel').exclude() # Don't care for now
            cls.mem_funs('PhysicsMarkEntityAsTouched').exclude() # Don't care for now
            cls.mem_funs('RenderHandle').exclude()
            mb.mem_funs('PhysicsAddHalfGravity').exclude()  # No definition on the client!
        
    def ParseServerEntities(self, mb):
        self.IncludeEmptyClass(mb, 'IServerUnknown')
        self.IncludeEmptyClass(mb, 'IServerEntity')
        
        for clsname in self.serverentities:
            cls = self.SetupEntityClass(mb, clsname)
            
            # Check if the python class is networkable. Add code for getting the "ServerClass" if that's the case.
            decl = cls.mem_funs('GetPyNetworkType', allow_empty=True)
            if decl:
                decl.Include()
                cls.add_wrapper_code(tmpl_serverclass % {'clsname' : clsname})
            
            # Excludes
            cls.mem_funs('edict').exclude()          # Not needed
            cls.mem_funs('GetNetworkable').exclude()         # Don't care for now
            cls.mem_funs('GetServerClass').exclude()         # Don't care about this one
            cls.mem_funs('MyNextBotPointer').exclude()     # Don't care
            cls.mem_funs('GetServerVehicle').exclude()     # Don't care
            cls.mem_funs('NetworkProp').exclude()            # Don't care
            cls.mem_funs('GetResponseSystem').exclude()         # Don't care for now
            
            cls.vars('m_pTimedOverlay').exclude()
        
        # Creation and spawning
        mb.free_functions('CreateEntityByName').include()
        mb.free_functions('DispatchSpawn').include()
        
    def ParseBaseEntityHandles(self, mb):
        # Dead entity
        cls = mb.class_('DeadEntity')
        cls.include()
        cls.mem_fun('NonZero').rename('__nonzero__')
        
        # Entity Handles
        cls = mb.class_('CBaseHandle')
        cls.include()
        cls.mem_funs().exclude()
        cls.mem_funs('GetEntryIndex').include()
        cls.mem_funs('GetSerialNumber').include()
        cls.mem_funs('ToInt').include()
        cls.mem_funs('IsValid').include()
        
        cls = mb.class_('PyHandle')
        cls.include()
        cls.mem_fun('Get').exclude()
        cls.mem_fun('PyGet').rename('Get')
        cls.mem_fun('GetAttr').rename('__getattr__')
        cls.mem_fun('GetAttribute').rename('__getattribute__')
        cls.mem_fun('SetAttr').rename('__setattr__')
        cls.mem_fun('Cmp').rename('__cmp__')
        cls.mem_fun('NonZero').rename('__nonzero__')
        cls.mem_fun('Str').rename('__str__')
        cls.mem_funs('GetPySelf').exclude()
        
        cls.add_wrapper_code(
            'virtual PyObject *GetPySelf() { return boost::python::detail::wrapper_base_::get_owner(*this); }'
        )
        
    def ParseBaseEntity(self, mb):
        # Exclude operators
        mb.global_ns.mem_opers('new').exclude()
        mb.global_ns.mem_opers('delete').exclude()
    
        cls = mb.class_('C_BaseEntity') if self.isclient else mb.class_('CBaseEntity')
    
        # Call policies
        cls.mem_funs('CollisionProp').call_policies = call_policies.return_internal_reference() 
        
        # Excludes
        mb.mem_funs('GetDataObject').exclude() # Don't care
        mb.mem_funs('CreateDataObject').exclude() # Don't care
        mb.mem_funs('DestroyDataObject').exclude() # Don't care
        mb.mem_funs('DestroyAllDataObjects').exclude() # Don't care
        mb.mem_funs('AddDataObjectType').exclude() # Don't care
        mb.mem_funs('RemoveDataObjectType').exclude() # Don't care
        mb.mem_funs('HasDataObjectType').exclude() # Don't care
        
        if self.isclient:
            cls.mem_funs('ParticleProp').call_policies = call_policies.return_internal_reference() 
    
    def ParseEntities(self, mb):
        self.ParseBaseEntityHandles(mb)
        
        self.IncludeEmptyClass(mb, 'IHandleEntity')
        
        if self.isclient:
            self.ParseClientEntities(mb)
        else:
            self.ParseServerEntities(mb)
            
        self.ParseBaseEntity(mb)
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        self.ParseEntities(mb)
        
        # Finally apply common rules to all includes functions and classes, etc
        self.ApplyCommonRules(mb)
        