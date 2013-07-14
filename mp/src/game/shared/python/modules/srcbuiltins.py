from srcpy.module_generators import SharedModuleGenerator
from pyplusplus import code_creators
from pyplusplus import function_transformers as FT
from pyplusplus.module_builder import call_policies

class SrcBuiltins(SharedModuleGenerator):
    module_name = 'srcbuiltins'
    
    files = [
        #'cbase.h',
        #'tier0\dbg.h',
        
        'srcpy_srcbuiltins.h',
    ]
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()      
        
        # Include message functions and rename them
        mb.free_function('SrcPyMsg').include()
        mb.free_function('SrcPyWarning').include()
        mb.free_function('SrcPyDevMsg').include()
        
        mb.free_function('SrcPyMsg').rename('Msg')
        mb.free_function('SrcPyWarning').rename('PrintWarning')
        mb.free_function('SrcPyDevMsg').rename('DevMsg')

        # Include classes for redirecting output (replace stdout and stderr)
        mb.class_('SrcPyStdOut').include()
        mb.class_('SrcPyStdErr').include()