"""Compile the actual event table against small registry/world doubles; no server build."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]

def main():
    source = (ROOT / "src/Ai/Dungeon/DungeonClear/Data/Events/BlackfathomEvents.cpp").read_text()
    source = "\n".join(line for line in source.splitlines() if not line.startswith("#include"))
    code = (ROOT / "tests/blackfathom_event_doubles.h").read_text() + "\n" + source
    code += (ROOT / "tests/blackfathom_event_scenarios.cpp").read_text()
    with tempfile.TemporaryDirectory() as temporary:
        path = Path(temporary)
        (path / "test.cpp").write_text(code)
        subprocess.run(["clang++", "-std=c++20", "-Wall", "-Wextra", "-Werror", str(path / "test.cpp"), "-o", str(path / "test")], check=True)
        subprocess.run([str(path / "test")], check=True)
    registry = (ROOT / "src/Ai/Dungeon/DungeonClear/Data/DungeonEventRegistry.cpp").read_text()
    assert "RegisterBlackfathomEvents(t)" in registry
    settings = (ROOT / "src/Ai/Dungeon/DungeonClear/Settings/DcSettingsRegistry.h").read_text()
    assert '"EnableBlackfathomFireEvent", DcType::Bool, 0, 0, 1, false' in settings
    assert "DungeonClear.EnableBlackfathomFireEvent = 0" in (ROOT / "conf/mod_dungeon_clear.conf.dist").read_text()
    doors = (ROOT / "src/Ai/Dungeon/DungeonClear/Data/DcEventDoorRegistry.h").read_text()
    assert "case 21117:" in doors.split("inline bool IsScriptOnly", 1)[1].split("return true", 1)[0]

if __name__ == "__main__":
    main()
