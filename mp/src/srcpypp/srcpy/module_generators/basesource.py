import os

from . basegenerator import ModuleGenerator
from .. src_module_builder import src_module_builder_t
from pyplusplus import code_creators
from pyplusplus.module_builder import call_policies
from pygccxml.declarations import matchers, pointer_t, reference_t, declarated_t, void_t

class SourceModuleGenerator(ModuleGenerator):
    # Choices: client, server, semi_shared and pure_shared
    # semi_shared: the module is parsed two times, with different preprocessors.
    #              The output will be added in the same file between #ifdef CLIENT_DLL #else #endif
    # pure_shared: The generated bindings must match on the client and server.
    module_type = 'client'
    
    # Set by generation code
    isclient = False
    isserver = False
    
    def GetFiles(self):
        files = []
        for filename in self.files:
            if not filename:
                continue
            if filename.startswith('#'):
                if self.isserver:
                    files.append(filename[1:])
            elif filename.startswith('$'):
                if self.isclient:
                    files.append(filename[1:])
            else:
                files.append(filename)
        return files
        
    def PostCodeCreation(self, mb):
        ''' Allows modifying mb.code_creator just after the code creation. '''
        # Remove boost\python.hpp header. This is already included by srcpy.h
        # and directly including can break debug mode (because it redefines _DEBUG)
        # TODO: Maybe do this in a nicer way, but it's not too important.
        header = code_creators.include_t( r'boost\python.hpp' )
        found = False
        for creator in mb.code_creator.creators:
            try:
                if header.header == creator.header:
                    found = True
                    mb.code_creator.remove_creator(creator)
                    break
            except:
                pass
        
        if not found:
            raise Exception('Could not find boost/python.hpp header''')
 
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
        
        # Anything returning KeyValues should be returned by value so it calls the converter
        keyvalues = mb.class_('KeyValues')
        mb.calldefs(matchers.calldef_matcher_t(return_type=pointer_t(declarated_t(keyvalues))), allow_empty=True).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        
        # Anything returning a void pointer is excluded by default
        mb.calldefs(matchers.calldef_matcher_t(return_type=pointer_t(declarated_t(void_t()))), allow_empty=True).exclude()
        