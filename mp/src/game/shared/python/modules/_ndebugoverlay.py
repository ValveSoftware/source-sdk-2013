from srcpy.module_generators import SemiSharedModuleGenerator
from pygccxml.declarations import matchers
from pyplusplus import messages

class NDebugOverlay(SemiSharedModuleGenerator):
    module_name = '_ndebugoverlay'
    
    files = [
        'cbase.h',
        'debugoverlay_shared.h'
    ]
    
    def Parse(self, mb):
        # Exclude everything, then add what we need
        # Otherwise we get very big source code and dlls
        mb.decls().exclude()  
        
        mb.add_declaration_code( 'class CBaseEntity;\r\n')
        
        mb.namespace('NDebugOverlay').include()
        mb.free_functions('DrawOverlayLines').exclude()
        
        # Remove any protected function 
        #mb.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
        
        # Silent warnings of generating class wrappers
        mb.classes().disable_warnings( messages.W1027 )
    