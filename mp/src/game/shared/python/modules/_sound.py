from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies
from pyplusplus import function_transformers as FT
from pyplusplus import code_creators
from pygccxml.declarations import matchers

class Sound(SemiSharedModuleGenerator):
    module_name = '_sound'
    
    files = [
        'wchar.h'
        , 'string_t.h'
        , 'cbase.h'
        , 'shareddefs.h'
        , 'srcpy_sound.h'
        , 'soundflags.h'
    ]

    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        # EngineSound access
        mb.class_('PyEngineSound').include()
        mb.class_('PyEngineSound').rename('EngineSound')
        
        # soundengine should follow after PyEngineSound!
        mb.class_('PyEngineSound').add_registration_code( "}bp::scope().attr( \"soundengine\" ) = boost::ref(pysoundengine);{", False )   
        
        # EmitSound_t
        cls = mb.class_('EmitSound_t')
        cls.rename('EmitSound')
        cls.include()
        cls.var('m_UtlVecSoundOrigin').exclude()
        
        cls.var('m_nChannel').rename('channel')
        cls.var('m_pSoundName').rename('soundname')
        cls.var('m_flVolume').rename('volume')
        cls.var('m_SoundLevel').rename('soundlevel')
        cls.var('m_nFlags').rename('flags')
        cls.var('m_nPitch').rename('pitch')
        cls.var('m_pOrigin').rename('origin')
        cls.var('m_flSoundTime').rename('soundtime')
        cls.var('m_pflSoundDuration').rename('soundduration')
        cls.var('m_bEmitCloseCaption').rename('emitclosecaption')
        cls.var('m_bWarnOnMissingCloseCaption').rename('warnonmissingclosecaption')
        cls.var('m_bWarnOnDirectWaveReference').rename('warnondirectwavereference')
        cls.var('m_nSpeakerEntity').rename('speakerentity')
        cls.var('m_UtlVecSoundOrigin').rename('utlvecsoundorigin')
        cls.var('m_hSoundScriptHandle').rename('soundscripthandle')
    
        # SoundEnvelope
        mb.class_('CSoundEnvelopeControllerHandle').include()
        mb.class_('CSoundEnvelopeControllerHandle').calldefs('CSoundEnvelopeControllerHandle').exclude()
        mb.class_('CSoundEnvelopeControllerHandle').rename('CSoundEnvelopeController')
        mb.class_('CSoundPatchHandle').include()
        mb.class_('CSoundPatchHandle').rename('CSoundPatch')
        mb.mem_funs('SoundCreate').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
        mb.vars('m_pSoundPatch').exclude()
        mb.class_('CSoundPatchHandle').calldefs('CSoundPatchHandle').exclude()
        
        # CSoundParameters
        mb.class_('CSoundParameters').include()
        
        # Enums
        mb.enums('soundcommands_t').include()
        mb.enums('soundcommands_t').rename('soundcommands')
        mb.enums('soundlevel_t').include()
        mb.enums('soundlevel_t').rename('soundlevel')
        
        # //--------------------------------------------------------------------------------------------------------------------------------
        # Remove any protected function 
        mb.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
        
        
    def AddAdditionalCode(self, mb):
        super(Sound, self).AddAdditionalCode(mb)
        
        # Some vars
        mb.add_registration_code( "bp::scope().attr( \"PITCH_NORM\" ) = PITCH_NORM;" )
        mb.add_registration_code( "bp::scope().attr( \"PITCH_LOW\" ) = PITCH_LOW;" )
        mb.add_registration_code( "bp::scope().attr( \"PITCH_HIGH\" ) = PITCH_HIGH;" )        
        
        #mb.add_registration_code( "bp::scope().attr( \"soundengine\" ) = boost::ref(pysoundengine);" )   


    