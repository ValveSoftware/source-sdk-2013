"""Do a minimal test of all the modules that aren't otherwise tested."""

from test import support
import sys
import unittest

class TestUntestedModules(unittest.TestCase):
    def test_at_least_import_untested_modules(self):
        with support.check_warnings(quiet=True):
            import bdb
            import cgitb

            import distutils.bcppcompiler
            import distutils.ccompiler
            import distutils.cygwinccompiler
            import distutils.emxccompiler
            import distutils.filelist
            if sys.platform.startswith('win'):
                import distutils.msvccompiler
            import distutils.text_file
            import distutils.unixccompiler

            import distutils.command.bdist_dumb
            if sys.platform.startswith('win'):
                import distutils.command.bdist_msi
            import distutils.command.bdist
            import distutils.command.bdist_rpm
            import distutils.command.bdist_wininst
            import distutils.command.build_clib
            import distutils.command.build_ext
            import distutils.command.build
            import distutils.command.clean
            import distutils.command.config
            import distutils.command.install_data
            import distutils.command.install_egg_info
            import distutils.command.install_headers
            import distutils.command.install_lib
            import distutils.command.register
            import distutils.command.sdist
            import distutils.command.upload

            import encodings
            import formatter
            import getpass
            import html.entities
            import imghdr
            import keyword
            import macurl2path
            import mailcap
            import nturl2path
            import os2emxpath
            import pstats
            import py_compile
            import sndhdr
            import tabnanny
            try:
                import tty     # not available on Windows
            except ImportError:
                if support.verbose:
                    print("skipping tty")


def test_main():
    support.run_unittest(TestUntestedModules)

if __name__ == "__main__":
    test_main()
