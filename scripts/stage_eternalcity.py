"""Create a new staging directory inside this worktree, never in Steam."""
import argparse
from pathlib import Path
import shutil
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--hl2-content", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    build = args.build.resolve()
    build.relative_to(root / "build")
    content = args.hl2_content.resolve()
    if '"' in str(content):
        raise ValueError("Content path must not contain quotation marks.")
    if not (content / "maps" / "d1_trainstation_01.bsp").is_file():
        raise FileNotFoundError("The installed HL2 content must contain the smoke-test map.")
    binaries = [build / "bin" / (name + ".dll") for name in ("client", "server")]
    for binary in binaries:
        if not binary.is_file():
            raise FileNotFoundError(binary)
    stage = root / "build" / ("ec-v145-stage-" + uuid.uuid4().hex[:8]) / "EternalCityPrototype"
    source = root / "game" / "mod_hl2"
    guide = (source / "gameinfo.txt").read_text()
    anchor = "\t\t\tplatform\t\t\t|all_source_engine_paths|platform\n"
    if guide.count(anchor) != 1:
        raise ValueError("Unexpected gameinfo search-path layout.")
    guide = guide.replace(anchor, f'\t\t\tgame\t\t\t\t"{content}"\n' + anchor)
    write_path = "game+game_write\t\thl2"
    if guide.count(write_path) != 1:
        raise ValueError("Unexpected gameinfo write-path layout.")
    guide = guide.replace(write_path, "game\t\t\t\thl2")
    shutil.copytree(source, stage)
    (stage / "gameinfo.txt").write_text(guide)
    (stage / "bin").mkdir(exist_ok=True)
    for binary in binaries:
        shutil.copy2(binary, stage / "bin" / binary.name)
    print("Staged:", stage)
    print("Installed HL2 content is mounted read-only; no VPKs were copied.")
    print("This is an unvalidated staging directory, not a Steam mod installation.")


if __name__ == "__main__":
    main()
