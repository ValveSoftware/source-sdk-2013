# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""Parser sub-package.
"""

from .config import config_t
from .config import gccxml_configuration_t
from .config import load_gccxml_configuration
from .config import gccxml_configuration_example

from .project_reader import COMPILATION_MODE
from .project_reader import project_reader_t
from .project_reader import file_configuration_t
from .project_reader import create_text_fc
from .project_reader import create_source_fc
from .project_reader import create_gccxml_fc
from .project_reader import create_cached_source_fc

from .source_reader import source_reader_t
from .source_reader import gccxml_runtime_error_t
from .declarations_cache import cache_base_t
from .declarations_cache import file_cache_t
from .declarations_cache import dummy_cache_t
from .directory_cache import directory_cache_t
#shortcut
CONTENT_TYPE = file_configuration_t.CONTENT_TYPE



def parse( files
           , config=None
           , compilation_mode=COMPILATION_MODE.FILE_BY_FILE
           , cache=None ):
    """
    Parse header files.

    :param files: The header files that should be parsed
    :type files: list of str
    :param config: Configuration object or None
    :type config: :class:`parser.config_t`
    :param compilation_mode: Determines whether the files are parsed individually or as one single chunk
    :type compilation_mode: :class:`parser.COMPILATION_MODE`
    :param cache: Declaration cache (None=no cache)
    :type cache: :class:`parser.cache_base_t` or str
    :rtype: list of :class:`declarations.declaration_t`
    """
    if not config:
        config = config_t()
    parser = project_reader_t( config=config, cache=cache )
    answer = parser.read_files(files, compilation_mode)
    return answer

def parse_string(content, config=None):
    if not config:
        config = config_t()
    parser = project_reader_t(config )
    return parser.read_string(content)

def parse_xml_file( content, config=None ):
    parser = source_reader_t( config )
    return parser.read_xml_file( content )
