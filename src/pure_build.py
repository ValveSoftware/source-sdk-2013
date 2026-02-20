#!/usr/bin/python
import argparse, tempfile, subprocess, os, sys, shutil, platform, typing

DEFAULT_ROOTDIR=".."
DEFAULT_CONTENTDIR="./content"

SYSTEM = platform.system()

def error(cause: str, code: int = 1):
  print(cause, file=sys.stderr, flush=True)
  exit(code)

if not (SYSTEM == "Windows" or SYSTEM == "Linux"):
  error(f"System \"{SYSTEM}\" is not supported.")

def _dont_print(*_args: typing.Any) -> None:
  pass
def _do_print(*args: typing.Any) -> None:
  print("[build.py LOG]", *args)
  
def get_log_function(quiet: bool):
  if quiet:
    return _dont_print
  else:
    return _do_print



def linux_build(args: argparse.Namespace):
  log = get_log_function(args.should_log)
  log("Running linux build: ./src/buildallprojects")
  linux_args = ["./src/buildallprojects", "release", "clean"]
  if args.is_playtest:
    linux_args.append("playtest")
  subprocess.run(linux_args, stdout=sys.stdout, stderr=sys.stderr, check=True)

def windows_build(args: argparse.Namespace):
  if args.is_playtest:
    raise NotImplementedError("Playtest build is not yet implemented on Windows")
  os.chdir("src")
  subprocess.run(["./createallprojects.bat"], stdout=sys.stdout, stderr=sys.stderr, check=True)
  raise NotImplementedError("Not yet ready to roll")

def main(args: argparse.Namespace):
  if shutil.which("git") is None:
    error("git is not installed. git is required to build PASS Fortress.\n\
      if you are on Windows, please download via https://git-scm.com/install/windows\n\
      if you are on Linux, please download via your package manager https://git-scm.com/install/linux",
      3
    )
  # print(args.is_playtest)

  log = get_log_function(not args.should_log)


  # cd to repository
  # python will automatically return to original cwd
  # after script exits =)
  os.chdir(os.path.dirname(os.path.realpath(__file__)))

  if not os.path.exists(args.rootdir):
    error(f"rootdir path invalid: {args.rootdir}\n\
      by default, the script expects to be placed in p4ss/p4ss-build/ContentBuilder\n\
      if this is not the case, manually specify rootdir via command line args", 
      2
    )
  if os.path.exists(args.contentdir):
    shutil.rmtree(args.contentdir)

  # clone the repository to the tempfile
  contentdir = os.path.realpath(args.contentdir)
  rootdir = os.path.realpath(args.rootdir)
  
  building_dir = tempfile.mkdtemp();
  log("building at: " + building_dir)

  try:
    log("Running git clone...")
    subprocess.run([
        "git", "clone", 
        # "--depth", "1", # to not excessively clone (ends up being unnecessary because its just symlinked)
        "--recurse-submodules", "--shallow-submodules", # in case we do end up having submodules
        rootdir, building_dir
      ], 
      stderr=sys.stderr, 
      check=True
    )
  
    os.chdir(building_dir)

    log("Running build")
    if SYSTEM == "Linux":
      linux_build(args)
    
    elif SYSTEM == "Windows":
      windows_build(args)

    if args.should_clear_temp:
      shutil.move(building_dir, contentdir)
    else:
      os.mkdir(args.contentdir)
      shutil.copytree(building_dir + "/game", contentdir)
    
  except (Exception, KeyboardInterrupt) as err:
    if args.should_clear_temp:
      log("Error occured. Clearing temporary building directory before excepting...\n")
      shutil.rmtree(building_dir)
    raise err




if __name__ == "__main__":
  parser = argparse.ArgumentParser(
    description="Builds PASS Fortress cleanly into the contentdir folder, \
      directly from a clone of the local repository. Supports cmd-style args and \
      linux-style args",
    prefix_chars='-/',
    add_help=False
  )

  # Arg order:
  # Short linux flag, short cmd flag, long linux flag, long cmd flag
  parser.add_argument("-h", "/?", "--help", action="help", help="show this help message and exit")
  parser.add_argument("-R", "/R", "--rootdir", default=DEFAULT_ROOTDIR,
    help="the root of the repository, containing src/ and game/ (e.g. `/home/lucy/code/p4ss`) \
      (default: %(default)s)"
  )
  parser.add_argument("-C", "/C", "--contentdir", default=DEFAULT_CONTENTDIR,
    help="the content directory to output built files to (default: %(default)s). \
      will clear all files inside this directory!"
  )
  parser.add_argument("-q", "/q", "--quiet", dest="should_log", action="store_false",
    help="if specified, doesn't log to stdout"
  )
  parser.add_argument("--playtest", "/playtest", dest="is_playtest", action="store_true",
    help="if specified, builds for the playtest (uses different AppID when compiling)"
  )
  parser.add_argument("--clear-temp", "/Ct", dest="should_clear_temp", action="store_true",
    help="if specified, removes the temp directory after building and copying to content/"
  )
  main(parser.parse_args())