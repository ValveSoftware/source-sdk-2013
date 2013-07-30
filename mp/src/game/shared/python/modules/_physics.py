from srcpy.module_generators import SemiSharedModuleGenerator

from pyplusplus.module_builder import call_policies
from pyplusplus import function_transformers as FT

class Physics(SemiSharedModuleGenerator):
    module_name = '_physics'
    
    files = [
        'cbase.h',
        'srcpy_physics.h',
        'physics.h',
        '#physics_impact_damage.h',
        'vphysics_interface.h',
        'physics_shared.h',
    ]
    
    def ParsePhysicObjects(self, mb):
        # Base Wrapper
        cls = mb.class_('PyPhysicsObjectBase')
        cls.rename('PhysicsObjectBase')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'   
        cls.mem_funs('Cmp').rename('__cmp__')
        cls.mem_funs('NonZero').rename('__nonzero__')
        
        cls.mem_fun('CheckValid').exclude()
        cls.mem_funs('GetPySelf').exclude()  
        cls.add_wrapper_code('virtual PyObject *GetPySelf() const { return boost::python::detail::wrapper_base_::get_owner(*this); }')
        
        # Bound to entity life time version
        #cls = mb.class_('PyEntPhysicsObject')
        #cls.include()
        #cls.calldefs().virtuality = 'not virtual'   
        #cls.add_wrapper_code('virtual PyObject *GetPySelf() const { return boost::python::detail::wrapper_base_::get_owner(*this); }')

        # General version
        cls = mb.class_('PyPhysicsObject')
        cls.rename('PhysicsObject') # NOTE: Name must match PyCreateEmptyPhysicsObject in src_python_physics.cpp!
        cls.include()
        cls.calldefs().virtuality = 'not virtual'  
        cls.mem_funs('SetEntity').exclude()
        cls.mem_funs('SetInvalid').exclude()
        cls.mem_funs('IsValid').exclude()
        cls.mem_funs('GetVPhysicsObject').exclude()
        cls.mem_funs('InitFromPhysicsObject').exclude()
        cls.add_wrapper_code('virtual PyObject *GetPySelf() const { return boost::python::detail::wrapper_base_::get_owner(*this); }')
        
        # Shadow controller
        cls = mb.class_('PyPhysicsShadowController')
        cls.rename('PhysicsShadowController')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'   
        cls.mem_fun('CheckValid').exclude()
        cls.mem_funs('Cmp').rename('__cmp__')
        cls.mem_funs('NonZero').rename('__nonzero__')
        
    def ParsePhysicCollision(self, mb):
        cls = mb.class_('PyPhysicsCollision')
        cls.rename('PhysicsCollision')
        cls.include()
        cls.no_init = True
        mb.add_registration_code( "bp::scope().attr( \"physcollision\" ) = boost::ref(pyphyscollision);" )
        
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        # Linux model_t fix ( correct? )
        mb.add_declaration_code( '#ifdef _LINUX\r\n' + \
                             'typedef struct model_t {};\r\n' + \
                             '#endif // _LINUX\r\n'
                           )
                           
        self.ParsePhysicObjects(mb)
        self.ParsePhysicCollision(mb)
        
        # Add useful functions
        mb.free_function('PyPhysModelCreateBox').include()
        mb.free_function('PyPhysModelCreateBox').rename('PhysModelCreateBox')
        mb.free_function('PyPhysModelCreateOBB').include()
        mb.free_function('PyPhysModelCreateOBB').rename('PhysModelCreateOBB')
        #mb.free_function('PyPhysModelCreateSphere').include()
        #mb.free_function('PyPhysModelCreateSphere').rename('PhysModelCreateSphere')
        mb.free_function('PyPhysDestroyObject').include()
        mb.free_function('PyPhysDestroyObject').rename('PhysDestroyObject')
        
        if self.isserver:
            mb.free_function('PyPhysCallbackImpulse').include()
            mb.free_function('PyPhysCallbackImpulse').rename('PhysCallbackImpulse')
            mb.free_function('PyPhysCallbackSetVelocity').include()
            mb.free_function('PyPhysCallbackSetVelocity').rename('PhysCallbackSetVelocity')
            mb.free_functions('PyPhysCallbackDamage').include()
            mb.free_functions('PyPhysCallbackDamage').rename('PhysCallbackDamage')
            mb.free_function('PyPhysCallbackRemove').include()
            mb.free_function('PyPhysCallbackRemove').rename('PhysCallbackRemove')
            mb.free_function('PhysIsInCallback').include()
        
        # CCollisionProperty
        cls = mb.class_('CCollisionProperty')
        cls.include()
        cls.no_init = True
        cls.calldefs().virtuality = 'not virtual' 
        cls.mem_funs('GetCollisionModel').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        cls.mem_funs('GetRootParentToWorldTransform').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        cls.mem_funs('GetOuter').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  

        mb.mem_funs('GetEntityHandle').exclude()
        mb.mem_funs('GetIClientUnknown').exclude()
        mb.mem_funs('GetBaseMap').exclude()
        mb.mem_funs('GetDataDescMap').exclude()
        #mb.mem_funs('Init').exclude()
        if self.isclient:
            mb.mem_funs('GetPredDescMap').exclude()
            mb.vars('m_PredMap').exclude()
            
        mb.enum('SurroundingBoundsType_t').include()
            
        # PyPhysicsSurfaceProps
        mb.class_('surfacedata_t').include()
        mb.class_('surfacephysicsparams_t').include()
        mb.class_('surfacephysicsparams_t').include()
        mb.class_('surfaceaudioparams_t').include()
        mb.class_('surfacesoundnames_t').include()
        mb.class_('surfacegameprops_t').include()
        mb.class_('surfacesoundhandles_t').include()
        
        cls = mb.class_('PyPhysicsSurfaceProps')
        cls.rename('PhysicsSurfaceProps')
        
        cls.include()
        cls.no_init = True
        cls.calldefs().virtuality = 'not virtual' 
        mb.add_registration_code( "bp::scope().attr( \"physprops\" ) = boost::ref(pyphysprops);" )
        
        if self.isserver:
            mb.free_function('PyCalculateDefaultPhysicsDamage').include()
            mb.free_function('PyCalculateDefaultPhysicsDamage').rename('CalculateDefaultPhysicsDamage')
            
        # Physics Hook
        #mb.free_function('PhysicsGameSystem').include()
        #mb.free_function('PhysicsGameSystem').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )
        
        mb.free_function('PyForcePhysicsSimulate').include()
        mb.free_function('PyForcePhysicsSimulate').rename('ForcePhysicsSimulate')
        
        if self.isserver:
            mb.free_function('Physics_RunThinkFunctions').include()
            