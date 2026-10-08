#!/usr/bin/env python3
"""Prepara los archivos de un GitHub Release para el OTA por descarga.

Uso:
  python tools/make_release.py RUTA/HomeLights.ino.bin [--version 1.0.1] [--out release]

Genera en --out:
  HomeLights.bin   (copia del binario compilado)
  version.json     ({"version", "url", "md5", "sha256", "size"})

Sube ambos archivos como assets de un Release con tag v<version> en GitHub.
El ESP32 lee .../releases/latest/download/version.json y, si la versión es
mayor que su FW_VERSION, descarga "url" y verifica md5 + sha256.
"""
import argparse
import hashlib
import json
import re
import shutil
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONFIG = ROOT / "HomeLights" / "config.h"
DEFAULT_REPO = "Cavazos32/Home"
ASSET_NAME = "HomeLights.bin"


def read_define(name: str) -> str | None:
    if not CONFIG.exists():
        return None
    m = re.search(rf'^\s*#define\s+{name}\s+"([^"]*)"', CONFIG.read_text(encoding="utf-8"), re.M)
    return m.group(1) if m else None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("bin", type=Path, help="binario compilado (HomeLights.ino.bin)")
    ap.add_argument("--version", help="versión (por defecto FW_VERSION de config.h)")
    ap.add_argument("--tag", help="tag del release (por defecto v<version>)")
    ap.add_argument("--repo", default=DEFAULT_REPO, help=f"usuario/repo de GitHub (por defecto {DEFAULT_REPO})")
    ap.add_argument("--out", type=Path, default=ROOT / "release", help="carpeta de salida")
    ap.add_argument("--allow-secrets", action="store_true",
                    help="no abortar si el .bin contiene las claves de config.h (NO recomendado en repo público)")
    args = ap.parse_args()

    data = args.bin.read_bytes()
    if len(data) < 1024 or data[0] != 0xE9:
        print(f"ERROR: {args.bin} no parece una imagen de app ESP32 (byte mágico 0xE9).", file=sys.stderr)
        return 1

    version = args.version or read_define("FW_VERSION")
    if not version:
        print("ERROR: no encontré FW_VERSION en config.h; usa --version.", file=sys.stderr)
        return 1
    version = version.lstrip("vV")
    if not re.fullmatch(r"\d+(\.\d+){0,3}", version):
        print(f"ERROR: versión inválida '{version}' (usa 1.2.3).", file=sys.stderr)
        return 1
    if version.encode() not in data:
        print(f"AVISO: el .bin no contiene el texto '{version}'. ¿Compilaste con FW_VERSION=\"{version}\"?", file=sys.stderr)

    # Protección: un release público expone el .bin y cualquiera puede leer sus textos.
    leaked = [n for n in ("WIFI_PASSWORD", "OTA_PASSWORD")
              if (v := read_define(n)) and len(v) >= 4 and v.encode() in data]
    if leaked and not args.allow_secrets:
        print("ERROR: el .bin contiene " + ", ".join(leaked) + " de config.h (no se muestran los valores).\n"
              "Si lo publicas en un release público, cualquiera puede extraer esas claves.\n"
              "Quita las claves del firmware (ver README) o usa --allow-secrets bajo tu riesgo.", file=sys.stderr)
        return 2

    tag = args.tag or f"v{version}"
    url = f"https://github.com/{args.repo}/releases/download/{tag}/{ASSET_NAME}"
    manifest = {
        "version": version,
        "url": url,
        "md5": hashlib.md5(data).hexdigest(),
        "sha256": hashlib.sha256(data).hexdigest(),
        "size": len(data),
        "built": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
    }

    args.out.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(args.bin, args.out / ASSET_NAME)
    (args.out / "version.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    print(f"Listo en {args.out}:")
    print(f"  {ASSET_NAME}  ({len(data)} bytes)")
    print("  version.json")
    print(json.dumps(manifest, indent=2))
    print(f"\nCrea el release con tag {tag} y sube ambos archivos como assets.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
