@echo off
rem Run Tests.  Run the regression test suite.
rem Usage:  rt [-d] [-O] [-q] [-x64] regrtest_args
rem -d   Run Debug build (python_d.exe).  Else release build.
rem -O   Run python.exe or python_d.exe (see -d) with -O.
rem -q   "quick" -- normally the tests are run twice, the first time
rem      after deleting all the .py[co] files reachable from Lib/.
rem      -q runs the tests just once, and without deleting .py[co] files.
rem -x64 Run the 64-bit build of python (or python_d if -d was specified)
rem      from the 'amd64' dir instead of the 32-bit build in this dir.
rem All leading instances of these switches are shifted off, and
rem whatever remains is passed to regrtest.py.  For example,
rem     rt -O -d -x test_thread
rem runs
rem     python_d -O ../lib/test/regrtest.py -x test_thread
rem twice, and
rem     rt -q -g test_binascii
rem runs
rem     python_d ../lib/test/regrtest.py -g test_binascii
rem to generate the expected-output file for binascii quickly.
rem
rem Confusing:  if you want to pass a comma-separated list, like
rem     -u network,largefile
rem then you have to quote it on the rt line, like
rem     rt -u "network,largefile"

setlocal

set prefix=.\
set suffix=
set qmode=
set dashO=
set tcltk=tcltk

:CheckOpts
if "%1"=="-O" (set dashO=-O)     & shift & goto CheckOpts
if "%1"=="-q" (set qmode=yes)    & shift & goto CheckOpts
if "%1"=="-d" (set suffix=_d)    & shift & goto CheckOpts
if "%1"=="-x64" (set prefix=amd64) & (set tcltk=tcltk64) & shift & goto CheckOpts

PATH %PATH%;%~dp0..\..\%tcltk%\bin
set exe=%prefix%\python%suffix%
set cmd=%exe% %dashO% -Wd -E -bb ../lib/test/regrtest.py %1 %2 %3 %4 %5 %6 %7 %8 %9
if defined qmode goto Qmode

echo Deleting .pyc/.pyo files ...
%exe% rmpyc.py

echo on
%cmd%
@echo off

echo About to run again without deleting .pyc/.pyo first:
pause

:Qmode
echo on
%cmd%
