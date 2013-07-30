from importlib import machinery
from . import util

import collections
import imp
import sys
import unittest


class PathHookTests(unittest.TestCase):

    """Test the path hook for extension modules."""
    # XXX Should it only succeed for pre-existing directories?
    # XXX Should it only work for directories containing an extension module?

    def hook(self, entry):
        return machinery.FileFinder.path_hook((machinery.ExtensionFileLoader,
            machinery.EXTENSION_SUFFIXES))(entry)

    def test_success(self):
        # Path hook should handle a directory where a known extension module
        # exists.
        self.assertTrue(hasattr(self.hook(util.PATH), 'find_module'))


def test_main():
    from test.support import run_unittest
    run_unittest(PathHookTests)


if __name__ == '__main__':
    test_main()
