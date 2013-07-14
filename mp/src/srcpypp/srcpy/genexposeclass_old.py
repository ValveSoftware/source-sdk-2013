import os
import time
import codecs

from . src_module_builder import src_module_builder_t
from pyplusplus import code_creators, file_writers

def unusedfile_f(file):
    print('Unused file: %s' % (file) )
    
class GenerateModule(object):
    settings = None # Contains ref to settings during Parse()
    
    # Settings
    module_name = 'the_name_of_this_module'
    split = False   # Can't be true if module_type == 'semi_shared'
    
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
            written_files = mb.split_module(os.path.join(self.path, self.module_name), on_unused_file_found=unusedfile_f)
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
                             
class GenerateModuleClient(GenerateModule):
    module_type = 'client'
    dll_name = 'Client'
    
    @property
    def path(self):
        returnself.settings.client_path
    
    # Create builder
    def CreateBuilder(self, files):
        return src_module_builder_t(files, is_client=True)    
    
class GenerateModuleServer(GenerateModule):
    module_type = 'server'
    dll_name = 'Server'
    
    @property
    def path(self):
        returnself.settings.server_path
    
class GenerateModulePureShared(GenerateModule):
    module_type = 'pure_shared'
    dll_name = 'Shared'
    
    @property
    def path(self):
        return self.settings.shared_path
    
 
class multiple_files_nowrite_t(file_writers.multiple_files_t): 
    def __init__(self, extmodule, directory_path, write_main=True, files_sum_repository=None, encoding='ascii'):
        super(multiple_files_nowrite_t, self).__init__(extmodule, directory_path, write_main, files_sum_repository, encoding)
        
        self.content = {}
        
    def write_file( self, fpath, content, files_sum_repository=None, encoding='ascii' ):
        writer_t = file_writers.writer.writer_t
        
        # From multiple_files_t
        if fpath in self.written_files:
            msg = ['`Py++` is going to write different content to the same file(%s).' % fpath]
            msg.append('The following is a short list of possible explanations for this behaviour:' )
            msg.append('* `Py++` bug, in this case, please report it' )
            msg.append('* module_builder_t contains two or more classes with the same alias')
            msg.append('* module_builder_t contains two or more classes with the same wrapper alias')
            msg.append('Please carefully review `Py++` warning messages. It should contain an additional information.')
            raise RuntimeError( os.linesep.join(msg) )

        self.written_files.append( fpath )
        
        # From writer_t
        fname = os.path.split( fpath )[1]
        writer_t.logger.debug( 'write code to file "%s" - started' % fpath )
        start_time = time.clock()
        fcontent_new = []
        if os.path.splitext( fpath )[1] == '.py':
            fcontent_new.append( '# This file has been generated by Py++.' )
        else:
            fcontent_new.append( '// This file has been generated by Py++.' )
        fcontent_new.append( os.linesep * 2 )
        fcontent_new.append( content )
        fcontent_new.append( os.linesep ) #keep gcc happy
        fcontent_new = ''.join( fcontent_new )
        if not isinstance( fcontent_new, str ):
            fcontent_new = str( fcontent_new, encoding )

        new_hash_value = None
        curr_hash_value = None
        if files_sum_repository:
            new_hash_value  = files_sum_repository.get_text_value( fcontent_new )
            curr_hash_value = files_sum_repository.get_file_value( fname )
            if new_hash_value == curr_hash_value:
                writer_t.logger.debug( 'file was not changed( hash ) - done( %f seconds )'
                                       % ( time.clock() - start_time ) )
                return

        if None is curr_hash_value and os.path.exists( fpath ):
            #It could be a first time the user uses files_sum_repository, don't force him
            #to recompile the code
            #small optimization to cut down compilation time
            f = codecs.open( fpath, 'rb', encoding )
            fcontent = f.read()
            f.close()
            if fcontent == fcontent_new:
                writer_t.logger.debug( 'file was not changed( content ) - done( %f seconds )'
                                       % ( time.clock() - start_time ) )
                return

        writer_t.logger.debug( 'file changed or it does not exist' )

        self.content[fpath] = fcontent_new
        
        if new_hash_value:
            files_sum_repository.update_value( fname, new_hash_value )
        writer_t.logger.info( 'file "%s" - updated( %f seconds )' % ( fname, time.clock() - start_time ) )
        
class class_multiple_files_nowrite_t(file_writers.class_multiple_files_t): 
    def __init__(self
                  , extmodule
                  , directory_path
                  , huge_classes
                  , num_of_functions_per_file=20
                  , files_sum_repository=None
                  , encoding='ascii'):
        super(class_multiple_files_nowrite_t, self).__init__(extmodule, directory_path,
                huge_classes, num_of_functions_per_file, files_sum_repository, encoding)
        
        self.content = {}
        
    #@staticmethod
    def write_file( self, fpath, content, files_sum_repository=None, encoding='ascii' ):
        """Write a source file.

        This method writes the string content into the specified file.
        An additional fixed header is written at the top of the file before
        content.

        :param fpath: File name
        :type fpath: str
        :param content: The content of the file
        :type content: str
        """
        fname = os.path.split( fpath )[1]
        file_writers.writer.writer_t.logger.debug( 'write code to file "%s" - started' % fpath )
        start_time = time.clock()
        fcontent_new = []
        if os.path.splitext( fpath )[1] == '.py':
            fcontent_new.append( '# This file has been generated by Py++.' )
        else:
            fcontent_new.append( '// This file has been generated by Py++.' )
        fcontent_new.append( os.linesep * 2 )
        fcontent_new.append( content )
        fcontent_new.append( os.linesep ) #keep gcc happy
        fcontent_new = ''.join( fcontent_new )
        if not isinstance( fcontent_new, unicode ):
            fcontent_new = unicode( fcontent_new, encoding )

        new_hash_value = None
        curr_hash_value = None
        if files_sum_repository:
            new_hash_value  = files_sum_repository.get_text_value( fcontent_new )
            curr_hash_value = files_sum_repository.get_file_value( fname )
            if new_hash_value == curr_hash_value:
                file_writers.writer.writer_t.logger.debug( 'file was not changed( hash ) - done( %f seconds )'
                                       % ( time.clock() - start_time ) )
                return
                
        self.content[fpath] = fcontent_new
        
        if new_hash_value:
            files_sum_repository.update_value( fname, new_hash_value )
        file_writers.writer.writer_t.logger.info( 'file "%s" - updated( %f seconds )' % ( fname, time.clock() - start_time ) )

        
        
tmpl_content_semishared = '''#include "cbase.h"
#ifdef CLIENT_DLL
%(content_client)s
#else
%(content_server)s
#endif
'''
        
class GenerateModuleSemiShared(GenerateModule):
    module_type = 'semi_shared'
    dll_name = 'Shared'
    @property
    def path(self):
        if not self.split:
            return self.settings.shared_path # Into one file with #ifdefs around the different parts
        else:
            if self.isclient:
                return self.settings.client_path
            return self.settings.server_path
    
    client_huge_classes = None
    server_huge_classes = None
    
    clientfiles = []
    serverfiles = []
    
    def GetFiles(self):
        if self.isclient:
            return self.clientfiles + self.files
        return self.serverfiles + self.files 
    
    # Main method
    def Run(self):
        self.isclient = True
        self.isserver = False
        mb_client = self.CreateBuilder(self.GetFiles())
        self.Parse(mb_client)
        self.isclient = False
        self.isserver = True
        mb_server = self.CreateBuilder(self.GetFiles())
        self.Parse(mb_server)
        self.FinalOutput(mb_client, mb_server)
        
    # Create builder
    def CreateBuilder(self, files):
        if self.isclient:
            return src_module_builder_t(files, is_client=True)   
        return src_module_builder_t(files, is_client=False)   
        
    def GenerateContent(self, mb):
        return mb.get_module()
        
    # Default includes
    def AddAdditionalCode(self, mb):
        mb.code_creator.user_defined_directories.append( os.path.abspath('.') )
        header = code_creators.include_t( 'srcpy.h' )
        mb.code_creator.adopt_include( header )
        header = code_creators.include_t( 'tier0/memdbgon.h' )
        mb.code_creator.adopt_include(header)

    def FinalOutput(self, mb_client, mb_server):
        # Set pydocstring options
        mb_client.add_registration_code('bp::docstring_options doc_options( true, true, false );', tail=False)
        mb_server.add_registration_code('bp::docstring_options doc_options( true, true, false );', tail=False)
        
        # Generate code
        mb_client.build_code_creator( module_name=self.module_name )  
        mb_server.build_code_creator( module_name=self.module_name )     
        
        # Misc
        self.isclient = True
        self.isserver = False
        self.AddAdditionalCode(mb_client)   
        self.isclient = False
        self.isserver = True
        self.AddAdditionalCode(mb_server)   
        
        if not self.split:
            content_client = self.GenerateContent(mb_client)
            content_server = self.GenerateContent(mb_server)
            
            
            content = tmpl_content_semishared % {
                'content_client' : content_client,
                'content_server' : content_server,
            }
            
            # Write output
            target_dir = self.path
            file_writers.write_file(os.path.join(target_dir, self.module_name+'.cpp' ), content)
        else:    
            mb_client.merge_user_code()
            mb_server.merge_user_code()
            
            # Write client
            self.isclient = True
            self.isserver = False
            target_dir = os.path.join(self.path, self.module_name)
            if not os.path.isdir(target_dir):
                os.mkdir(target_dir)
                
            if not self.client_huge_classes:
                mfs_client = multiple_files_nowrite_t( mb_client.code_creator, target_dir, files_sum_repository=None, encoding='ascii' )
            else:
                mfs_client = class_multiple_files_nowrite_t( mb_client.code_creator, target_dir, self.client_huge_classes, files_sum_repository=None, encoding='ascii' )
            mfs_client.write()
            
            for key in mfs_client.content.keys():
                content = '#include "cbase.h"\r\n' + \
                          mfs_client.content[key]
                file_writers.write_file(key, content) 
            
            # Write server
            self.isclient = False
            self.isserver = True
            target_dir = os.path.join(self.path, self.module_name)
            if not os.path.isdir(target_dir):
                os.mkdir(target_dir)
                
            if not self.server_huge_classes:
                mfs_server = multiple_files_nowrite_t( mb_server.code_creator, target_dir, files_sum_repository=None, encoding='ascii' )
            else:
                mfs_server = class_multiple_files_nowrite_t( mb_server.code_creator, target_dir, self.server_huge_classes, files_sum_repository=None, encoding='ascii' )
            mfs_server.write()
            
            for key in mfs_server.content.keys():
                content = '#include "cbase.h"\r\n' + \
                          mfs_server.content[key]
                file_writers.write_file(key, content)
                