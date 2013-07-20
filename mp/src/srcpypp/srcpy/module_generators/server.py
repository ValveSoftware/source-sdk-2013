from . basesource import SourceModuleGenerator

class ServerModuleGenerator(SourceModuleGenerator):
    module_type = 'server'
    dll_name = 'Server'
    
    @property
    def path(self):
        return self.settings.server_path