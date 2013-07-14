from . basegenerator import ModuleGenerator

class ServerModuleGenerator(ModuleGenerator):
    module_type = 'server'
    dll_name = 'Server'
    
    @property
    def path(self):
        return self.settings.server_path