from . basesource import SourceModuleGenerator

class SharedModuleGenerator(SourceModuleGenerator):
    ''' This module generates code for server and client, 
        but only parses once using the server project settings.
        It assumes the output code is the same on server and client.
    '''
    module_type = 'pure_shared'
    dll_name = 'Shared'
    
    @property
    def path(self):
        return self.settings.shared_path