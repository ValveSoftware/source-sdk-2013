from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies
from pyplusplus import function_transformers as FT
from pyplusplus import code_creators

class Particles(SemiSharedModuleGenerator):
    module_name = '_particles'
    
    clientfiles = [
        'tier1/utlvector.h',
        'tier1/utlsortvector.h',
        'tier1/utlobjectreference.h',

        'cbase.h',
        'particles_ez.h',
        'icliententityinternal.h',
        'convar.h',
        'fx.h',
    ]

    serverfiles = [
        'cbase.h',
        'gameinterface.h'
    ]
    
    files = [
        'particle_parse.h',
        'srcpy_particles.h',
    ]
    
    def ParseClient(self, mb):
        # Dynamic lights
        mb.class_('PyDLight').include()
        mb.class_('PyDLight').rename('DLight')
        mb.class_('PyELight').include()
        mb.class_('PyELight').rename('ELight')
        
        cls = mb.class_('PyDBaseLight')
        cls.include()
        cls.rename('DBaseLight')
        cls.mem_funs().exclude()
        cls.mem_funs('GetColor').call_policies = call_policies.return_internal_reference() 
        cls.mem_funs('GetDirection').call_policies = call_policies.return_internal_reference() 
        cls.mem_funs('GetOrigin').call_policies = call_policies.return_internal_reference() 
        cls.add_property( 'origin' , cls.member_function( 'GetOrigin' ), cls.member_function( 'SetOrigin' ) )
        cls.add_property( 'color' , cls.member_function( 'GetColor' ), cls.member_function( 'SetColor' ) )
        cls.add_property( 'die' , cls.member_function( 'GetDie' ), cls.member_function( 'SetDie' ) )
        cls.add_property( 'radius' , cls.member_function( 'GetRadius' ), cls.member_function( 'SetRadius' ) )
        cls.add_property( 'flags' , cls.member_function( 'GetFlags' ), cls.member_function( 'SetFlags' ) )
        cls.add_property( 'minlight' , cls.member_function( 'GetMinlight' ), cls.member_function( 'SetMinlight' ) )
        cls.add_property( 'decay' , cls.member_function( 'GetDecay' ), cls.member_function( 'SetDecay' ) )
        cls.add_property( 'style' , cls.member_function( 'GetStyle' ), cls.member_function( 'SetStyle' ) )
        cls.add_property( 'direction' , cls.member_function( 'GetDirection' ), cls.member_function( 'SetDirection' ) )
        cls.add_property( 'innerangle' , cls.member_function( 'GetInnerAngle' ), cls.member_function( 'SetInnerAngle' ) )
        cls.add_property( 'outerangle' , cls.member_function( 'GetOuterAngle' ), cls.member_function( 'SetOuterAngle' ) )
        
        mb.add_registration_code( "bp::scope().attr( \"DLIGHT_NO_WORLD_ILLUMINATION\" ) = (int)DLIGHT_NO_WORLD_ILLUMINATION;" )
        mb.add_registration_code( "bp::scope().attr( \"DLIGHT_NO_MODEL_ILLUMINATION\" ) = (int)DLIGHT_NO_MODEL_ILLUMINATION;" )
        mb.add_registration_code( "bp::scope().attr( \"DLIGHT_ADD_DISPLACEMENT_ALPHA\" ) = (int)DLIGHT_ADD_DISPLACEMENT_ALPHA;" )
        mb.add_registration_code( "bp::scope().attr( \"DLIGHT_SUBTRACT_DISPLACEMENT_ALPHA\" ) = (int)DLIGHT_SUBTRACT_DISPLACEMENT_ALPHA;" )
        mb.add_registration_code( "bp::scope().attr( \"DLIGHT_DISPLACEMENT_MASK\" ) = (int)DLIGHT_DISPLACEMENT_MASK;" )
        
        # Base particle
        cls = mb.class_('Particle')
        cls.include()
        cls.vars('m_Pos').rename('pos')
        cls.var('m_pNext').exclude()
        cls.var('m_pPrev').exclude()
        cls.var('m_pSubTexture').exclude()
        
        # Simple particle
        mb.class_('SimpleParticle').include()
        mb.class_('SimpleParticle').vars('m_flDieTime').rename('dietime')
        mb.class_('SimpleParticle').vars('m_flLifetime').rename('lifetime')
        mb.class_('SimpleParticle').vars('m_flRoll').rename('roll')
        mb.class_('SimpleParticle').vars('m_flRollDelta').rename('rolldelta')
        mb.class_('SimpleParticle').vars('m_iFlags').rename('flags')
        mb.class_('SimpleParticle').vars('m_uchColor').rename('color')
        mb.class_('SimpleParticle').vars('m_uchEndAlpha').rename('endalpha')
        mb.class_('SimpleParticle').vars('m_uchEndSize').rename('endsize')
        mb.class_('SimpleParticle').vars('m_uchStartAlpha').rename('startalpha')
        mb.class_('SimpleParticle').vars('m_uchStartSize').rename('startsize')
        mb.class_('SimpleParticle').vars('m_vecVelocity').rename('velocity')
        mb.enum('SimpleParticleFlag_t').include()
        
        # Textures
        cls = mb.class_('CParticleSubTexture')
        cls.include()
        cls.no_init = True
        cls.calldefs().exclude()
        cls.vars().exclude()
        
        cls = mb.class_('CParticleSubTextureGroup')
        cls.include()
        cls.no_init = True
        cls.calldefs().exclude()
        cls.vars().exclude()
        
        # CNewParticleEffect
        cls = mb.class_('CNewParticleEffect')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        
        cls.mem_funs('AddParticle').exclude()
        cls.mem_funs('GetPMaterial').call_policies = call_policies.return_internal_reference() 
        cls.mem_funs('ReplaceWith').exclude()
        cls.vars('m_pNext').exclude()
        cls.vars('m_pPrev').exclude()

        cls.mem_funs('GetOwner').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        
        cls.mem_funs('Create').call_policies = call_policies.return_internal_reference()
        cls.mem_funs('Create').exclude() # TODO
        #cls.mem_funs('CreateOrAggregate').call_policies = call_policies.return_internal_reference()
        #cls.mem_funs('CreateOrAggregatePrecached').call_policies = call_policies.return_internal_reference()

        #mb.class_('CNewParticleEffectHandle').include()
        #mb.class_('CNewParticleEffectHandle').mem_funs('GetParticleEffect').exclude()
        
        # ParticleManager
        cls =  mb.class_('CParticleMgr')
        cls.include()
        cls.calldefs('CParticleMgr').exclude()
        cls.no_init = True
        cls.mem_funs('GetPMaterial').call_policies = call_policies.return_internal_reference() 
        cls.mem_funs('AllocParticle').exclude()
        cls.mem_funs('CreateEffect').exclude()
        cls.mem_funs('GetModelView').exclude()
        cls.mem_funs('PMaterialToIMaterial').exclude()
        
        #cls.mem_funs('CreateNonDrawingEffect').call_policies = call_policies.return_internal_reference()
        #cls.mem_funs('FirstNewEffect').call_policies = call_policies.return_internal_reference()
        #cls.mem_funs('NextNewEffect').call_policies = call_policies.return_internal_reference()

        mb.free_function('ParticleMgr').include()
        mb.free_function('ParticleMgr').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )
        
        # Add effects
        mb.free_function('AddSimpleParticle').include()
        mb.free_function('AddEmberParticle').include()
        mb.free_function('AddFireSmokeParticle').include()
        mb.free_function('AddFireParticle').include()

        cls = mb.class_('CParticleProperty')
        cls.include()
        cls.mem_funs('Create').call_policies = call_policies.return_internal_reference() 
        cls.mem_funs('GetParticleEffectFromIdx').call_policies = call_policies.return_internal_reference()
        mb.mem_funs('GetOuter').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('GetBaseMap').exclude()
        mb.mem_funs('GetDataDescMap').exclude()
        mb.mem_funs('GetPredDescMap').exclude()
        #mb.mem_funs('CreatePrecached').exclude()
        
        # Frequently used materials with particles
        #mb.vars('g_Mat_Fleck_Wood').include()
        #mb.vars('g_Mat_Fleck_Cement').include()
        #mb.vars('g_Mat_Fleck_Antlion').include()
        #mb.vars('g_Mat_Fleck_Tile').include()
        #mb.vars('g_Mat_DustPuff').include()
        #mb.vars('g_Mat_BloodPuff').include()
        #mb.vars('g_Mat_Fleck_Glass').include()
        #mb.vars('g_Mat_SMG_Muzzleflash').include()
        #mb.vars('g_Mat_Combine_Muzzleflash').include()
        
        #mb.add_registration_code( "ptr_newparticleeffect_to_handle();" )
        #mb.add_registration_code( "handle_to_newparticleeffect();" )      
            
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        mb.enums('ParticleAttachment_t').include()
        
        mb.free_functions('GetAttachTypeFromString').include()
        mb.free_functions('PrecacheParticleSystem').include()
        mb.free_functions('DispatchParticleEffect').include()
        mb.free_functions('StopParticleEffects').include()
        #mb.free_functions('StopParticleEffect').include()
        
        mb.free_functions('PyShouldLoadSheets').include()
        mb.free_functions('PyShouldLoadSheets').rename('ShouldLoadSheets')
        mb.free_functions('PyReadParticleConfigFile').include()
        mb.free_functions('PyReadParticleConfigFile').rename('ReadParticleConfigFile')
        mb.free_functions('PyDecommitTempMemory').include()
        mb.free_functions('PyDecommitTempMemory').rename('DecommitTempMemory')
        if self.isclient:
            self.ParseClient(mb)

    #def AddAdditionalCode(self, mb):
        #super(Particles, self).AddAdditionalCode(mb)
        #if self.isClient:
            #header = code_creators.include_t( 'src_python_particles_converters.h' )
            #mb.code_creator.adopt_include(header)   
    