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
    
    # Choices: client, server, semi_shared and pure_shared
    # semi_shared: the module is parsed two times, with different preprocessors.
    #              The output will be added in the same file between #ifdef CLIENT_DLL #else #endif
    # pure_shared: The generated bindings must match on the client and server.
    module_type = 'client'  
    
    files = []
    
    # Map some names
    dll_name = None
    path = 'specify a valid path'
    
    isclient = False
    isserver = False

    # Main method
    def Run(self):
        mb = self.CreateBuilder(self.files)
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

        # Add precompiled header + other general required stuff and write away
        self.AddAdditionalCode(mb)      
        if self.split:
            written_files = mb.split_module(os.path.join(self.path, self.module_name), on_unused_file_found=lambda file: print('Unused file: %s' % (file)))
        else:
            mb.write_module(os.path.join(os.path.abspath(self.path), self.module_name+'.cpp'))
        
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
        if setter: cls.mem_funs(setter).exclude()
        if setter:
            cls.add_property(propertyname, cls.member_function( getter ), cls.member_function( setter ))
        else:
            cls.add_property(propertyname, cls.member_function( getter ))
            
    def IncludeEmptyClass(self, mb, clsname):
        c = mb.class_(clsname)
        c.include()
        if c.classes(allow_empty=True):
            c.classes(allow_empty=True).exclude()
        if c.mem_funs(allow_empty=True):
            c.mem_funs(allow_empty=True).exclude()
        if c.vars(allow_empty=True):
            c.vars(allow_empty=True).exclude()
        if c.enums(allow_empty=True):
            c.enums(allow_empty=True).exclude()  
            
    # Applies common rules to code
    def ApplyCommonRules(self, mb):
        # Common function added for getting the "PyObject" of an entity
        mb.mem_funs('GetPySelf').exclude()
        
        # All return values derived from IHandleEntity entity will be returned by value, 
        # so the converter is called
        ihandlecls = mb.class_('IHandleEntity')
        def testInherits(memfun):
            try:
                othercls = memfun.return_type.base.declaration
                for testcls in othercls.recursive_bases:
                    if ihandlecls == testcls.related_class:
                        return True
            except AttributeError:
                pass
            return False
        mb.calldefs(matchers.custom_matcher_t(testInherits)).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        
            