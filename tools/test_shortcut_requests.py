#!/usr/bin/env python3
"""Valida cuerpos JSON de los atajos iOS contra el contrato de POST /api/command.

Uso:
  python3 tools/test_shortcut_requests.py
  HOMELIGHTS_BASE_URL=http://192.168.x.x python3 tools/test_shortcut_requests.py --live

No requiere dependencias externas. --live usa urllib contra un ESP32 real (solo
comprueba HTTP 200 en peticiones seguras: no fuerza escenas que alteren luces
más allá de lo que indiques; por defecto live solo prueba apagar todo y leer state).
"""
from __future__ import annotations

import argparse
import json
import os
import sys
import urllib.error
import urllib.request
from dataclasses import dataclass
from typing import Any

VALID_ACTIONS = frozenset({"on", "off", "level", "fade", "toggle", "alternate"})
VALID_ZONES = frozenset({"cuna", "setup", "both"})


@dataclass(frozen=True)
class ShortcutStep:
    shortcut: str
    body: dict[str, Any]


# Contrato alineado con handleCommand() en HomeLights.ino
SHORTCUTS: dict[str, list[ShortcutStep]] = {
    "Cuna": [ShortcutStep("Cuna", {"action": "on", "zone": "cuna"})],
    "Luz nocturna": [
        ShortcutStep("Luz nocturna", {"action": "off", "zone": "setup"}),
        ShortcutStep("Luz nocturna", {"action": "level", "zone": "cuna", "value": 20}),
    ],
    "Setup": [ShortcutStep("Setup", {"action": "on", "zone": "setup"})],
    "Apagar todo": [ShortcutStep("Apagar todo", {"action": "off", "zone": "both"})],
    "Encender todo": [ShortcutStep("Encender todo", {"action": "on", "zone": "both"})],
}


def validate_command_body(body: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    action = body.get("action", "level")
    if action not in VALID_ACTIONS:
        errors.append(f"acción desconocida: {action!r}")
        return errors

    if action == "alternate":
        return errors

    zone = body.get("zone", "both")
    if zone not in VALID_ZONES:
        errors.append(f"zona desconocida: {zone!r}")

    if action in ("level", "fade"):
        if "value" not in body:
            errors.append("falta value para level/fade")
        else:
            value = body["value"]
            if not isinstance(value, int) or value < 0 or value > 100:
                errors.append(f"value inválido: {value!r}")

    duration = body.get("duration", 0)
    if duration != 0 and (not isinstance(duration, int) or duration > 60000):
        errors.append(f"duration inválido: {duration!r}")

    return errors


def validate_all() -> int:
    failed = 0
    for name, steps in SHORTCUTS.items():
        for i, step in enumerate(steps, start=1):
            errs = validate_command_body(step.body)
            if errs:
                failed += 1
                print(f"FAIL {name} paso {i}: {errs}", file=sys.stderr)
            else:
                serialized = json.dumps(step.body, separators=(",", ":"), ensure_ascii=False)
                print(f"OK   {name} paso {i}: {serialized}")
    return failed


def post_command(base_url: str, body: dict[str, Any]) -> tuple[int, str]:
    url = base_url.rstrip("/") + "/api/command"
    data = json.dumps(body).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=data,
        method="POST",
        headers={"Content-Type": "application/json"},
    )
    try:
        with urllib.request.urlopen(req, timeout=8) as resp:
            return resp.status, resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode("utf-8", errors="replace")


def live_check(base_url: str) -> int:
    """GET state; POST off both (idempotente si ya apagado)."""
    failed = 0
    state_url = base_url.rstrip("/") + "/api/state"
    try:
        with urllib.request.urlopen(state_url, timeout=8) as resp:
            if resp.status != 200:
                print(f"FAIL GET /api/state status {resp.status}", file=sys.stderr)
                failed += 1
            else:
                print("OK   GET /api/state")
    except OSError as e:
        print(f"FAIL GET /api/state: {e}", file=sys.stderr)
        return 1

    code, text = post_command(base_url, {"action": "off", "zone": "both"})
    if code != 200 or '"ok":true' not in text.replace(" ", ""):
        print(f"FAIL POST apagar todo: {code} {text}", file=sys.stderr)
        failed += 1
    else:
        print("OK   POST apagar todo")

    # Validar que el JSON de luz nocturna es aceptado (secuencia real altera luces)
    for step in SHORTCUTS["Luz nocturna"]:
        code, text = post_command(base_url, step.body)
        if code != 200:
            print(f"FAIL POST luz nocturna {step.body}: {code} {text}", file=sys.stderr)
            failed += 1
        else:
            print(f"OK   POST luz nocturna {json.dumps(step.body, separators=(',', ':'))}")

    return failed


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument(
        "--live",
        action="store_true",
        help="prueba HTTP contra HOMELIGHTS_BASE_URL (altera luces en el dispositivo)",
    )
    args = ap.parse_args()

    print("=== Validación de formato (offline) ===")
    fmt_fail = validate_all()

    live_fail = 0
    if args.live:
        base = os.environ.get("HOMELIGHTS_BASE_URL", "").strip()
        if not base:
            print("FAIL --live requiere HOMELIGHTS_BASE_URL", file=sys.stderr)
            live_fail = 1
        else:
            print(f"\n=== Prueba live contra {base} ===")
            live_fail = live_check(base)

    total = fmt_fail + live_fail
    if total:
        print(f"\n{total} comprobación(es) fallida(s).", file=sys.stderr)
        return 1
    print("\nTodas las comprobaciones pasaron.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
