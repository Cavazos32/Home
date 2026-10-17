#!/usr/bin/env python3
"""Compila y ejecuta las pruebas de host de las rutinas del firmware.

Usa los .cpp reales de HomeLights/ con stubs mínimos de Arduino/Preferences.

Uso:
  python tools/firmware_tests/run.py

Compilador: variable CXX (p. ej. "g++" o "clang++"); si no existe, prueba g++,
clang++ y "python -m ziglang c++" (pip install ziglang).
ArduinoJson: variable ARDUINOJSON_SRC o ~/Documents/Arduino/libraries/ArduinoJson/src.
"""
from __future__ import annotations

import importlib.util
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
FIRMWARE = ROOT / "HomeLights"
SOURCES = ["LightController.cpp"]


def find_compiler() -> list[str] | None:
    env = os.environ.get("CXX")
    if env:
        return shlex.split(env)
    for name in ("g++", "clang++"):
        if shutil.which(name):
            return [name]
    if importlib.util.find_spec("ziglang"):
        return [sys.executable, "-m", "ziglang", "c++"]
    return None


def find_arduinojson() -> Path | None:
    env = os.environ.get("ARDUINOJSON_SRC")
    candidates = [Path(env)] if env else []
    candidates.append(Path.home() / "Documents" / "Arduino" / "libraries" / "ArduinoJson" / "src")
    candidates.append(Path.home() / "Arduino" / "libraries" / "ArduinoJson" / "src")
    for c in candidates:
        if (c / "ArduinoJson.h").exists():
            return c
    return None


def main() -> int:
    cxx = find_compiler()
    if not cxx:
        print("ERROR: no hay compilador C++ (define CXX o instala g++/clang++/ziglang).", file=sys.stderr)
        return 2
    json_src = find_arduinojson()
    if not json_src:
        print("ERROR: no encontré ArduinoJson (define ARDUINOJSON_SRC).", file=sys.stderr)
        return 2
    if not (FIRMWARE / "config.h").exists():
        print("ERROR: falta HomeLights/config.h (crea el archivo; ver README).", file=sys.stderr)
        return 2

    out_dir = Path(tempfile.mkdtemp(prefix="hl-tests-"))
    exe = out_dir / ("test_routines.exe" if os.name == "nt" else "test_routines")
    cmd = cxx + [
        "-std=c++17", "-O1", "-Wall", "-Wextra",
        "-I", str(HERE / "stubs"),
        "-I", str(FIRMWARE),
        "-I", str(json_src),
        str(HERE / "test_routines.cpp"),
        *[str(FIRMWARE / s) for s in SOURCES],
        "-o", str(exe),
    ]
    print("Compilando:", " ".join(cmd[:4]), "...")
    build = subprocess.run(cmd)
    if build.returncode != 0:
        return build.returncode
    return subprocess.run([str(exe)]).returncode


if __name__ == "__main__":
    sys.exit(main())
