"""Experimental VS2026 Win32 build; never publishes or stages the mod."""
import argparse
import os
from pathlib import Path
import subprocess
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--visual-studio-path", type=Path)
    parser.add_argument("--allow-legacy-warnings", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / (
        r"Microsoft Visual Studio\Installer\vswhere.exe"
    )
    vs = args.visual_studio_path
    if vs is None:
        result = subprocess.run(
            [str(vswhere), "-latest", "-products", "*", "-requires",
             "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
             "-property", "installationPath"],
            check=True, capture_output=True, text=True,
        )
        if not result.stdout.strip():
            raise RuntimeError("No installed Visual C++ toolchain was found.")
        vs = Path(result.stdout.strip())
    vs = vs.resolve()
    vcvars = vs / r"VC\Auxiliary\Build\vcvarsall.bat"
    msbuild = vs / r"MSBuild\Current\Bin\MSBuild.exe"
    toolset = vs / r"MSBuild\Microsoft\VC\v180\Platforms\Win32\PlatformToolsets\v145"
    if not (vcvars.is_file() and msbuild.is_file() and toolset.is_dir()):
        raise RuntimeError("This experimental build requires VS2026 v145 Win32.")
    if any('"' in str(path) for path in (root, vs)):
        raise ValueError("Build paths must not contain quotation marks.")

    build_root = root / "build" / ("ec-v145-" + uuid.uuid4().hex)
    build_root.mkdir(parents=True)
    print("Experimental build output:", build_root, flush=True)
    print("No staging or post-build publication will be performed.", flush=True)
    env = os.environ.copy()
    env["PATH"] = str(vswhere.parent) + os.pathsep + env["PATH"]
    env["_CL_"] = (
        "/source-charset:windows-1252 /Zc:sizedDealloc- /Zc:threadSafeInit-"
    )
    if args.allow_legacy_warnings:
        print("Legacy warnings remain in logs but are not errors.", flush=True)
        env["_CL_"] += " /WX-"
    env["LINK"] = "legacy_stdio_definitions.lib /NODEFAULTLIB:libucrt.lib ucrt.lib"
    for project in ("client", "server"):
        project_dir = root / "src" / "game" / project
        vpc = root / "src" / "devtools" / "bin" / "vpc.exe"
        subprocess.run(
            [str(vpc), "/checkfiles", "+" + project, "/hl2"],
            cwd=project_dir, check=True,
        )
        subprocess.run(
            [str(vpc), "-crc2", project + "_hl2.vcxproj"],
            cwd=project_dir, check=True,
        )
        out_dir = build_root / "bin"
        int_dir = build_root / project / "obj"
        log_path = build_root / (project + ".log")
        command = (
            f'call "{vcvars}" x64_x86 >nul && "{msbuild}" {project}_hl2.vcxproj'
            ' /t:Build /p:Configuration=Release /p:Platform=Win32'
            ' /p:PlatformToolset=v145 /p:WindowsTargetPlatformVersion=10.0'
            f' /p:OutDir="{out_dir}" /p:IntDir="{int_dir}"'
            ' /p:PostBuildEventUseInBuild=false /nologo /verbosity:minimal'
            ' /maxcpucount:2'
        )
        with log_path.open("wb") as log:
            result = subprocess.run(
                command, shell=True, cwd=project_dir, env=env,
                stdout=log, stderr=subprocess.STDOUT,
            )
        if result.returncode:
            raise RuntimeError(
                f"{project} build failed ({result.returncode}). Log: {log_path}"
            )
        dll = out_dir / (project + ".dll")
        if not dll.is_file():
            raise RuntimeError(f"Build reported success without {dll}.")
        print("Built", dll, flush=True)
    print("Both DLLs built. Runtime ABI, allocator, and gameplay NOT validated.")


if __name__ == "__main__":
    main()
