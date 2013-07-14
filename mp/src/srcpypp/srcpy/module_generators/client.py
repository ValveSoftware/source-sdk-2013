from . basegenerator import ModuleGenerator

class ClientModuleGenerator(ModuleGenerator):
    module_type = 'client'
    dll_name = 'Client'
    
    @property
    def path(self):
        return self.settings.client_path
    
    # Create builder
    def CreateBuilder(self, files):
        return src_module_builder_t(files, is_client=True)    
    

    
