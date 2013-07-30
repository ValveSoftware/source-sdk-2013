from srcpy.module_generators import SemiSharedModuleGenerator

import pygccxml
from pyplusplus.module_builder import call_policies
from pyplusplus import function_transformers as FT
from pygccxml.declarations import reference_t, declarated_t, const_t
from srcpy.matchers import calldef_withtypes

class Utils(SemiSharedModuleGenerator):
    module_name = '_utils'
    
    clientfiles = [
        'cbase.h',

    ]
    
    serverfiles = [

    ]
    
    files = [
        'cbase.h',
        'srcpy_util.h',
        'util_shared.h',
        'gametrace.h',
        'engine\IEngineTrace.h',
        
        '#mathlib/vmatrix.h', 
        '#utlvector.h', 
        '#shareddefs.h',
        '#util.h',
        '#explode.h',
        
        '$cdll_util.h',
        '$iclientshadowmgr.h',
        '$viewrender.h',
        '$view.h',
    ]
    
    def ParseServer(self, mb):
        # Entity index functions
        mb.free_functions('INDEXENT').include()
        mb.free_functions('ENTINDEX').include()
        mb.free_functions('INDEXENT').call_policies = call_policies.return_internal_reference()
        
        # Don't care about the following functions
        mb.free_functions('UTIL_VarArgs').exclude()
        mb.free_functions('UTIL_LogPrintf').exclude()
        mb.free_functions('UTIL_FunctionToName').exclude()
        #mb.free_functions('UTIL_FunctionFromName').exclude()
        
        # Exclude for now
        mb.free_functions('UTIL_GetDebugColorForRelationship').exclude()
        mb.free_functions('UTIL_Beam').exclude()
        mb.free_functions('UTIL_StringFieldToInt').exclude()
        mb.free_functions('UTIL_EntitiesAlongRay').exclude()
        
        # Replace the following
        mb.free_functions('UTIL_SetSize').exclude()
        mb.free_functions('UTIL_PySetSize').rename('UTIL_SetSize')
        mb.free_functions('UTIL_SetModel').exclude()
        mb.free_functions('UTIL_PySetModel').rename('UTIL_SetModel')
        
        # Must use return_by_value. Then the converter will be used to wrap the vgui element in a safe handl
        mb.free_functions('UTIL_GetLocalPlayer').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.free_functions('UTIL_GetCommandClient').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.free_functions('UTIL_EntityByIndex').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.free_functions('UTIL_GetListenServerHost').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.free_functions('UTIL_PlayerByName').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.free_functions('UTIL_PlayerByUserId').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.free_functions('UTIL_FindClientInVisibilityPVS').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.free_functions('UTIL_EntitiesInPVS').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.free_functions('UTIL_FindClientInPVS').call_policies = call_policies.return_value_policy( call_policies.return_by_value )   
        
        #mb.free_functions('UTIL_FindClientInPVSGuts').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        #mb.free_functions('UTIL_GetLocalPlayerOrListenServerHost').call_policies = call_policies.return_value_policy( call_policies.return_by_value )   
        
        # Helper for message stuff
        cls = mb.class_('hudtextparms_s')
        cls.include()
        cls.rename('hudtextparms')
        
        # Tracefilters
        mb.class_('CTraceFilterMelee').include()
        mb.class_('CTraceFilterMelee').calldefs('CTraceFilterMelee').exclude()
        mb.class_('CTraceFilterMelee').add_wrapper_code(
                "CTraceFilterMelee_wrapper(::CBaseEntity const * passentity, int collisionGroup, ::CTakeDamageInfo * dmgInfo, float flForceScale, bool bDamageAnyNPC )\r\n" + \
                "    : CTraceFilterMelee( boost::python::ptr(passentity), collisionGroup, boost::python::ptr(dmgInfo), flForceScale, bDamageAnyNPC )\r\n" + \
                "    , bp::wrapper< CTraceFilterMelee >(){\r\n" + \
                "    // constructor\r\n" + \
                "}\r\n"
        )
        mb.class_('CTraceFilterMelee').add_registration_code(
            'def( bp::init< CBaseEntity const *, int, CTakeDamageInfo *, float, bool >(( bp::arg("passentity"), bp::arg("collisionGroup"), bp::arg("dmgInfo"), bp::arg("flForceScale"), bp::arg("bDamageAnyNPC") )) )\r\n'
        )
        
        # Include explode functions
        mb.free_functions('ExplosionCreate').include()
        
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_NODAMAGE\" ) = SF_ENVEXPLOSION_NODAMAGE;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_REPEATABLE\" ) = SF_ENVEXPLOSION_REPEATABLE;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_NOFIREBALL\" ) = SF_ENVEXPLOSION_NOFIREBALL;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_NOSMOKE\" ) = SF_ENVEXPLOSION_NOSMOKE;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_NODECAL\" ) = SF_ENVEXPLOSION_NODECAL;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_NOSPARKS\" ) = SF_ENVEXPLOSION_NOSPARKS;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_NOSOUND\" ) = SF_ENVEXPLOSION_NOSOUND;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_RND_ORIENT\" ) = SF_ENVEXPLOSION_RND_ORIENT;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_NOFIREBALLSMOKE\" ) = SF_ENVEXPLOSION_NOFIREBALLSMOKE;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_NOPARTICLES\" ) = SF_ENVEXPLOSION_NOPARTICLES;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_NODLIGHTS\" ) = SF_ENVEXPLOSION_NODLIGHTS;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_NOCLAMPMIN\" ) = SF_ENVEXPLOSION_NOCLAMPMIN;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_NOCLAMPMAX\" ) = SF_ENVEXPLOSION_NOCLAMPMAX;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_SURFACEONLY\" ) = SF_ENVEXPLOSION_SURFACEONLY;" )
        mb.add_registration_code( "bp::scope().attr( \"SF_ENVEXPLOSION_GENERIC_DAMAGE\" ) = SF_ENVEXPLOSION_GENERIC_DAMAGE;" )

    def ParseClient(self, mb):
        # Free functions that don't start with 'UTIL_'
        mb.free_functions('ScreenHeight').include()
        mb.free_functions('ScreenWidth').include()
        mb.free_functions('GetVectorInScreenSpace').include()
        mb.free_functions('GetTargetInScreenSpace').include()
        
        # Main view
        mb.free_function('MainViewOrigin').include()
        mb.free_function('MainViewAngles').include()
        mb.free_function('MainViewForward').include()
        mb.free_function('MainViewRight').include()
        mb.free_function('MainViewUp').include()
        mb.free_function('MainWorldToViewMatrix').include()
        
        # Call policies and excludes
        #mb.free_function('UTIL_GetLocalizedKeyString').exclude()
        #mb.free_function('UTIL_MessageText').exclude()
        #mb.free_functions('UTIL_EntityFromUserMessageEHandle').call_policies = call_policies.return_value_policy( call_policies.return_by_value )   
        mb.free_functions('UTIL_PlayerByUserId').call_policies = call_policies.return_value_policy( call_policies.return_by_value )   
            
        # Transformations
        mb.free_functions( 'GetVectorInScreenSpace' ).add_transformation( FT.output('iX'), FT.output('iY') )
        mb.free_functions( 'GetTargetInScreenSpace' ).add_transformation( FT.output('iX'), FT.output('iY') )

    def RecursiveFindDecl(self, cls, fnname):
        try:
            return cls.mem_fun('ShouldHitEntity')
        except pygccxml.declarations.matcher.declaration_not_found_t:
            for b in cls.bases:
                print(b.__dict__)
                ret = self.RecursiveFindDecl(b.related_class, fnname)
                if ret:
                    return ret
        return None
        
    def SetupTraceFilter(self, mb, clsname):
        cls = mb.class_(clsname)
        cls.include()
        cls.mem_funs(allow_empty=True).virtuality = 'not virtual'
        cls.mem_funs('ShouldHitEntity', allow_empty=True).virtuality = 'virtual'
            
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude() 
        
        # By default include all functions starting with "UTIL_". Rest we will do manually
        mb.free_functions( lambda decl: 'UTIL_' in decl.name ).include()

        # Exclude and replace
        mb.free_functions('UTIL_TraceLine').include()
        mb.free_functions('UTIL_TraceHull').include()
        mb.free_functions('UTIL_TraceEntity').include()
        mb.free_functions('UTIL_TraceRay').exclude()
        mb.free_functions('UTIL_PyTraceRay').rename('UTIL_TraceRay')     
        mb.free_functions('UTIL_PyEntitiesInSphere').rename('UTIL_EntitiesInSphere')
        mb.free_functions('UTIL_PyEntitiesInBox').rename('UTIL_EntitiesInBox')
        
        mb.free_functions('UTIL_EntitiesAlongRay').exclude()
        mb.free_functions('UTIL_PyEntitiesAlongRay').rename('UTIL_EntitiesAlongRay')

        # Enums
        mb.enum('ShakeCommand_t').include()
        
        # Call policies
        mb.free_functions('UTIL_PlayerByIndex').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        
        # Exclude
        # Don't care about the following functions
        mb.free_functions('UTIL_LoadFileForMe').exclude()
        mb.free_functions('UTIL_FreeFile').exclude()
        
        # Exclude for now
        mb.free_functions('UTIL_EntitiesInSphere').exclude()
        mb.free_functions('UTIL_EntitiesInBox').exclude()
        
        # Add
        mb.free_function('StandardFilterRules').include()
        mb.free_function('PassServerEntityFilter').include()
        
        # //--------------------------------------------------------------------------------------------------------------------------------
        # Tracing
        mb.class_('CBaseTrace').include()
        mb.class_('CGameTrace').include()
        mb.class_('CGameTrace').rename('trace_t')
        mb.vars('m_pEnt').rename('ent')
        #mb.global_ns.typedefs('trace_t').include()
        
        cls = mb.class_('PyRay_t')
        cls.include()
        cls.rename('Ray_t')
        cls.vars('m_Start').rename('start')
        cls.vars('m_Delta').rename('delta')
        cls.vars('m_StartOffset').rename('startoffset')
        cls.vars('m_Extents').rename('extents')
        cls.vars('m_IsRay').rename('isray')
        cls.vars('m_IsSwept').rename('isswept')
        
        # //--------------------------------------------------------------------------------------------------------------------------------
        # Trace Filters
        # By default, it's not possible to override TraceFilter methods
        cls = mb.class_('ITraceFilter')
        cls.include()
        cls.no_init = True
        cls.calldefs().exclude()
        cls.mem_fun('ShouldHitEntity').virtuality = 'not virtual'
        
        tracefilters = [
            'CTraceFilter',
            'CTraceFilterEntitiesOnly',
            'CTraceFilterWorldOnly',
            'CTraceFilterWorldAndPropsOnly',
            'CTraceFilterHitAll',
            'CTraceFilterSimple',
            'CTraceFilterSkipTwoEntities',
            'CTraceFilterSimpleList',
            'CTraceFilterOnlyNPCsAndPlayer',
            'CTraceFilterNoNPCsOrPlayer',
            'CTraceFilterLOS',
            'CTraceFilterSkipClassname',
            'CTraceFilterSkipTwoClassnames',
            'CTraceFilterSimpleClassnameList',
            'CTraceFilterChain',
            'CPyTraceFilterSimple',
        ]
        for clsname in tracefilters:
            self.SetupTraceFilter(mb, clsname)
            
        mb.mem_funs('ShouldHitEntity').exclude()

        mb.class_('CTraceFilterSimple').rename('CTraceFilterSimpleInternal')
        mb.class_('CPyTraceFilterSimple').rename('CTraceFilterSimple')
        
        mb.mem_funs('GetPassEntity').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        
        mb.class_('csurface_t').include()
        
        # //--------------------------------------------------------------------------------------------------------------------------------
        # Collision utils
        mb.free_functions('PyIntersectRayWithTriangle').include()
        mb.free_functions('PyIntersectRayWithTriangle').rename('IntersectRayWithTriangle')
        mb.free_functions('ComputeIntersectionBarycentricCoordinates').include()
        mb.free_functions('IntersectRayWithRay').include()
        mb.free_functions('IntersectRayWithSphere').include()
        mb.free_functions('IntersectInfiniteRayWithSphere').include()
        mb.free_functions('IsSphereIntersectingCone').include()
        mb.free_functions('IntersectRayWithPlane').include()
        mb.free_functions('IntersectRayWithAAPlane').include()
        mb.free_functions('IntersectRayWithBox').include()
        mb.class_('BoxTraceInfo_t').include()
        mb.free_functions('IntersectRayWithBox').include()
        mb.free_functions('IntersectRayWithOBB').include()
        mb.free_functions('IsSphereIntersectingSphere').include()
        mb.free_functions('IsBoxIntersectingSphere').include()
        mb.free_functions('IsBoxIntersectingSphereExtents').include()
        mb.free_functions('IsRayIntersectingSphere').include()
        mb.free_functions('IsCircleIntersectingRectangle').include()
        mb.free_functions('IsOBBIntersectingOBB').include()
        mb.free_functions('IsPointInCone').include()
        mb.free_functions('IntersectTriangleWithPlaneBarycentric').include()
        mb.enum('QuadBarycentricRetval_t').include()
        mb.free_functions('PointInQuadToBarycentric').include()
        mb.free_functions('PointInQuadFromBarycentric').include()
        mb.free_functions('TexCoordInQuadFromBarycentric').include()
        mb.free_functions('ComputePointFromBarycentric').include()
        mb.free_functions('IsRayIntersectingOBB').include()
        mb.free_functions('ComputeSeparatingPlane').include()
        mb.free_functions('IsBoxIntersectingTriangle').include()
        #mb.free_functions('CalcClosestPointOnTriangle').include()
        mb.free_functions('OBBHasFullyContainedIntersectionWithQuad').include()
        mb.free_functions('RayHasFullyContainedIntersectionWithQuad').include()
        
        vectorcls = mb.class_('Vector')
        excludetypes = [
            declarated_t(vectorcls),
            reference_t(declarated_t(vectorcls)),
            reference_t(const_t(declarated_t(vectorcls))),
        ]
        vectormatcher =  calldef_withtypes( excludetypes )
        mb.free_functions('IsBoxIntersectingBox', vectormatcher).include() 
        mb.free_functions('IsBoxIntersectingBoxExtents', vectormatcher).include()   
        mb.free_functions('IsBoxIntersectingRay', vectormatcher).include()
        mb.free_functions('IsPointInBox', vectormatcher).include()
        
        # Prediction functions
        mb.free_function('GetSuppressHost').include()
        mb.free_function('GetSuppressHost').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        
        if self.isserver:
            self.ParseServer(mb)
        else:
            self.ParseClient(mb)
      
        