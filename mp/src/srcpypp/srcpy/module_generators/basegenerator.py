import os

from .. src_module_builder import src_module_builder_t
from pyplusplus import code_creators
from pyplusplus.module_builder import call_policies
from pygccxml.declarations import matchers, pointer_t, reference_t, declarated_t

class ModuleGenerator(object):
    settings = None # Contains ref to settings during Parse()
    
    # Settings
    module_name = None
    split = False

    files = []
    
    # Map some names
    dll_name = None
    path = 'specify a valid path'
    
    def GetFiles(self):
        return self.files
    
    # Main method
    def Run(self):
        mb = self.CreateBuilder(self.GetFiles())
        self.Parse(mb)
        self.FinalOutput(mb)
        
    # Parse method. Implement this.
    def Parse(self, mb):
        assert(0)
        
    # Create builder
    def CreateBuilder(self, files):
        return src_module_builder_t(files, is_client=False)
            
    def FinalOutput(self, mb):
        ''' Finalizes the output after generation of the bindings.
            Writes output to file.'''
        # Set pydocstring options
        mb.add_registration_code('bp::docstring_options doc_options( true, true, false );', tail=False)
    
        # Generate code
        mb.build_code_creator(module_name=self.module_name)
        self.PostCodeCreation(mb)
        
        # Add precompiled header + other general required stuff and write away
        self.AddAdditionalCode(mb)      
        if self.split:
            written_files = mb.split_module(os.path.join(self.path, self.module_name), on_unused_file_found=lambda file: print('Unused file: %s' % (file)))
        else:
            mb.write_module(os.path.join(os.path.abspath(self.path), self.module_name+'.cpp'))
            
    def PostCodeCreation(self, mb):
        ''' Allows modifying mb.code_creator just after the code creation. '''
        pass
        
    # Adds precompiled header + other default includes
    def AddAdditionalCode(self, mb):
        mb.code_creator.user_defined_directories.append( os.path.abspath('.') )
        header = code_creators.include_t( 'cbase.h' )
        mb.code_creator.adopt_creator( header, 0 )
        header = code_creators.include_t( 'srcpy.h' )
        mb.code_creator.adopt_include( header )
        header = code_creators.include_t( 'tier0/memdbgon.h' )
        mb.code_creator.adopt_include(header)
        
    def AddProperty(self, cls, propertyname, getter, setter=''):
        cls.mem_funs(getter).exclude()
        if setter: 
            cls.mem_funs(setter).exclude()
        if setter:
            cls.add_property(propertyname, cls.member_function( getter ), cls.member_function( setter ))
        else:
            cls.add_property(propertyname, cls.member_function( getter ))
            
    def IncludeEmptyClass(self, mb, clsname, no_init=True, removevirtual=True):
        c = mb.class_(clsname)
        c.include()
        c.no_init = no_init
        if removevirtual:
            c.mem_funs(allow_empty=True).virtuality = 'not virtual' 
        if c.classes(allow_empty=True):
            c.classes(allow_empty=True).exclude()
        if c.mem_funs(allow_empty=True):
            c.mem_funs(allow_empty=True).exclude()
        if c.vars(allow_empty=True):
            c.vars(allow_empty=True).exclude()
        if c.enums(allow_empty=True):
            c.enums(allow_empty=True).exclude()  
        
            