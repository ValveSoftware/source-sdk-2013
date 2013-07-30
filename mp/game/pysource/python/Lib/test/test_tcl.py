#!/usr/bin/env python3

import unittest
import sys
import os
from test import support

# Skip this test if the _tkinter module wasn't built.
_tkinter = support.import_module('_tkinter')

# Make sure tkinter._fix runs to set up the environment
support.import_fresh_module('tkinter')

from tkinter import Tcl
from _tkinter import TclError


class TkinterTest(unittest.TestCase):

    def testFlattenLen(self):
        # flatten(<object with no length>)
        self.assertRaises(TypeError, _tkinter._flatten, True)


class TclTest(unittest.TestCase):

    def setUp(self):
        self.interp = Tcl()

    def testEval(self):
        tcl = self.interp
        tcl.eval('set a 1')
        self.assertEqual(tcl.eval('set a'),'1')

    def testEvalException(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.eval,'set a')

    def testEvalException2(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.eval,'this is wrong')

    def testCall(self):
        tcl = self.interp
        tcl.call('set','a','1')
        self.assertEqual(tcl.call('set','a'),'1')

    def testCallException(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.call,'set','a')

    def testCallException2(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.call,'this','is','wrong')

    def testSetVar(self):
        tcl = self.interp
        tcl.setvar('a','1')
        self.assertEqual(tcl.eval('set a'),'1')

    def testSetVarArray(self):
        tcl = self.interp
        tcl.setvar('a(1)','1')
        self.assertEqual(tcl.eval('set a(1)'),'1')

    def testGetVar(self):
        tcl = self.interp
        tcl.eval('set a 1')
        self.assertEqual(tcl.getvar('a'),'1')

    def testGetVarArray(self):
        tcl = self.interp
        tcl.eval('set a(1) 1')
        self.assertEqual(tcl.getvar('a(1)'),'1')

    def testGetVarException(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.getvar,'a')

    def testGetVarArrayException(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.getvar,'a(1)')

    def testUnsetVar(self):
        tcl = self.interp
        tcl.setvar('a',1)
        self.assertEqual(tcl.eval('info exists a'),'1')
        tcl.unsetvar('a')
        self.assertEqual(tcl.eval('info exists a'),'0')

    def testUnsetVarArray(self):
        tcl = self.interp
        tcl.setvar('a(1)',1)
        tcl.setvar('a(2)',2)
        self.assertEqual(tcl.eval('info exists a(1)'),'1')
        self.assertEqual(tcl.eval('info exists a(2)'),'1')
        tcl.unsetvar('a(1)')
        self.assertEqual(tcl.eval('info exists a(1)'),'0')
        self.assertEqual(tcl.eval('info exists a(2)'),'1')

    def testUnsetVarException(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.unsetvar,'a')

    def testEvalFile(self):
        tcl = self.interp
        filename = "testEvalFile.tcl"
        fd = open(filename,'w')
        script = """set a 1
        set b 2
        set c [ expr $a + $b ]
        """
        fd.write(script)
        fd.close()
        tcl.evalfile(filename)
        os.remove(filename)
        self.assertEqual(tcl.eval('set a'),'1')
        self.assertEqual(tcl.eval('set b'),'2')
        self.assertEqual(tcl.eval('set c'),'3')

    def testEvalFileException(self):
        tcl = self.interp
        filename = "doesnotexists"
        try:
            os.remove(filename)
        except Exception as e:
            pass
        self.assertRaises(TclError,tcl.evalfile,filename)

    def testPackageRequireException(self):
        tcl = self.interp
        self.assertRaises(TclError,tcl.eval,'package require DNE')

    @unittest.skipUnless(sys.platform == 'win32', 'Requires Windows')
    def testLoadWithUNC(self):
        # Build a UNC path from the regular path.
        # Something like
        #   \\%COMPUTERNAME%\c$\python27\python.exe

        fullname = os.path.abspath(sys.executable)
        if fullname[1] != ':':
            raise unittest.SkipTest('Absolute path should have drive part')
        unc_name = r'\\%s\%s$\%s' % (os.environ['COMPUTERNAME'],
                                    fullname[0],
                                    fullname[3:])
        if not os.path.exists(unc_name):
            raise unittest.SkipTest('Cannot connect to UNC Path')

        with support.EnvironmentVarGuard() as env:
            env.unset("TCL_LIBRARY")
            f = os.popen('%s -c "import tkinter; print(tkinter)"' % (unc_name,))

        self.assertIn('tkinter', f.read())
        # exit code must be zero
        self.assertEqual(f.close(), None)

    def test_passing_values(self):
        def passValue(value):
            return self.interp.call('set', '_', value)

        self.assertEqual(passValue(True), True)
        self.assertEqual(passValue(False), False)
        self.assertEqual(passValue('string'), 'string')
        self.assertEqual(passValue('string\u20ac'), 'string\u20ac')
        for i in (0, 1, -1, 2**31-1, -2**31):
            self.assertEqual(passValue(i), i)
        for f in (0.0, 1.0, -1.0, 1/3,
                  sys.float_info.min, sys.float_info.max,
                  -sys.float_info.min, -sys.float_info.max):
            self.assertEqual(passValue(f), f)
        for f in float('nan'), float('inf'), -float('inf'):
            if f != f: # NaN
                self.assertNotEqual(passValue(f), f)
            else:
                self.assertEqual(passValue(f), f)
        self.assertEqual(passValue((1, '2', (3.4,))), (1, '2', (3.4,)))


def test_main():
    support.run_unittest(TclTest, TkinterTest)

if __name__ == "__main__":
    test_main()
