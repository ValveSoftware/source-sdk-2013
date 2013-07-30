# Common utility functions used by various script execution tests
#  e.g. test_cmd_line, test_cmd_line_script and test_runpy

import importlib
import sys
import os
import os.path
import tempfile
import subprocess
import py_compile
import contextlib
import shutil
import zipfile

from imp import source_from_cache
from test.support import make_legacy_pyc, strip_python_stderr

# Executing the interpreter in a subprocess
def _assert_python(expected_success, *args, **env_vars):
    cmd_line = [sys.executable]
    if not env_vars:
        cmd_line.append('-E')
    # Need to preserve the original environment, for in-place testing of
    # shared library builds.
    env = os.environ.copy()
    # But a special flag that can be set to override -- in this case, the
    # caller is responsible to pass the full environment.
    if env_vars.pop('__cleanenv', None):
        env = {}
    env.update(env_vars)
    cmd_line.extend(args)
    p = subprocess.Popen(cmd_line, stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                         env=env)
    try:
        out, err = p.communicate()
    finally:
        subprocess._cleanup()
        p.stdout.close()
        p.stderr.close()
    rc = p.returncode
    err =  strip_python_stderr(err)
    if (rc and expected_success) or (not rc and not expected_success):
        raise AssertionError(
            "Process return code is %d, "
            "stderr follows:\n%s" % (rc, err.decode('ascii', 'ignore')))
    return rc, out, err

def assert_python_ok(*args, **env_vars):
    """
    Assert that running the interpreter with `args` and optional environment
    variables `env_vars` is ok and return a (return code, stdout, stderr) tuple.
    """
    return _assert_python(True, *args, **env_vars)

def assert_python_failure(*args, **env_vars):
    """
    Assert that running the interpreter with `args` and optional environment
    variables `env_vars` fails and return a (return code, stdout, stderr) tuple.
    """
    return _assert_python(False, *args, **env_vars)

def spawn_python(*args, **kw):
    cmd_line = [sys.executable, '-E']
    cmd_line.extend(args)
    return subprocess.Popen(cmd_line, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            **kw)

def kill_python(p):
    p.stdin.close()
    data = p.stdout.read()
    p.stdout.close()
    # try to cleanup the child so we don't appear to leak when running
    # with regrtest -R.
    p.wait()
    subprocess._cleanup()
    return data

# Script creation utilities
@contextlib.contextmanager
def temp_dir():
    dirname = tempfile.mkdtemp()
    dirname = os.path.realpath(dirname)
    try:
        yield dirname
    finally:
        shutil.rmtree(dirname)

def make_script(script_dir, script_basename, source):
    script_filename = script_basename+os.extsep+'py'
    script_name = os.path.join(script_dir, script_filename)
    # The script should be encoded to UTF-8, the default string encoding
    script_file = open(script_name, 'w', encoding='utf-8')
    script_file.write(source)
    script_file.close()
    importlib.invalidate_caches()
    return script_name

def make_zip_script(zip_dir, zip_basename, script_name, name_in_zip=None):
    zip_filename = zip_basename+os.extsep+'zip'
    zip_name = os.path.join(zip_dir, zip_filename)
    zip_file = zipfile.ZipFile(zip_name, 'w')
    if name_in_zip is None:
        parts = script_name.split(os.sep)
        if len(parts) >= 2 and parts[-2] == '__pycache__':
            legacy_pyc = make_legacy_pyc(source_from_cache(script_name))
            name_in_zip = os.path.basename(legacy_pyc)
            script_name = legacy_pyc
        else:
            name_in_zip = os.path.basename(script_name)
    zip_file.write(script_name, name_in_zip)
    zip_file.close()
    #if test.support.verbose:
    #    zip_file = zipfile.ZipFile(zip_name, 'r')
    #    print 'Contents of %r:' % zip_name
    #    zip_file.printdir()
    #    zip_file.close()
    return zip_name, os.path.join(zip_name, name_in_zip)

def make_pkg(pkg_dir, init_source=''):
    os.mkdir(pkg_dir)
    make_script(pkg_dir, '__init__', init_source)

def make_zip_pkg(zip_dir, zip_basename, pkg_name, script_basename,
                 source, depth=1, compiled=False):
    unlink = []
    init_name = make_script(zip_dir, '__init__', '')
    unlink.append(init_name)
    init_basename = os.path.basename(init_name)
    script_name = make_script(zip_dir, script_basename, source)
    unlink.append(script_name)
    if compiled:
        init_name = py_compile(init_name, doraise=True)
        script_name = py_compile(script_name, doraise=True)
        unlink.extend((init_name, script_name))
    pkg_names = [os.sep.join([pkg_name]*i) for i in range(1, depth+1)]
    script_name_in_zip = os.path.join(pkg_names[-1], os.path.basename(script_name))
    zip_filename = zip_basename+os.extsep+'zip'
    zip_name = os.path.join(zip_dir, zip_filename)
    zip_file = zipfile.ZipFile(zip_name, 'w')
    for name in pkg_names:
        init_name_in_zip = os.path.join(name, init_basename)
        zip_file.write(init_name, init_name_in_zip)
    zip_file.write(script_name, script_name_in_zip)
    zip_file.close()
    for name in unlink:
        os.unlink(name)
    #if test.support.verbose:
    #    zip_file = zipfile.ZipFile(zip_name, 'r')
    #    print 'Contents of %r:' % zip_name
    #    zip_file.printdir()
    #    zip_file.close()
    return zip_name, os.path.join(zip_name, script_name_in_zip)
