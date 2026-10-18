#!/usr/bin/env python3
"""Valida el flujo Atajos iOS: GET /api/state + POST /api/command con toggle.

Uso:
  python3 tools/test_shortcut_requests.py
  HOMELIGHTS_BASE_URL=http://192.168.x.x python3 tools/test_shortcut_requests.py --live

No requiere dependencias externas. --live altera luces (POST toggle en cuna por defecto).
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

VALID_ACTIONS = frozenset({"on", "off", "level", "fade", "toggle", "alternate", "group"})
VALID_ZONES = frozenset({"cuna", "setup", "both"})
VALID_GROUPS = frozenset({"habitacion", "baby"})

ZONE_STATE_KEYS = frozenset(
    {"level", "levelFine", "targetLevel", "on", "lit", "transitioning", "phase"}
)


@dataclass(frozen=True)
class ShortcutStep:
    shortcut: str
    body: dict[str, Any]


# POST usado tras GET /api/state en cada atajo (sin on/off).
SHORTCUTS: dict[str, list[ShortcutStep]] = {
    "Cuna": [ShortcutStep("Cuna", {"action": "toggle", "zone": "cuna"})],
    "Setup": [ShortcutStep("Setup", {"action": "toggle", "zone": "setup"})],
    "Luces habitación": [
        ShortcutStep("Luces habitación", {"action": "toggle", "zone": "both"}),
    ],
}

# Atajos que solo leen estado (sin POST en la guía).
SHORTCUTS_GET_ONLY = frozenset({"Estado luces"})


def validate_command_body(body: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    action = body.get("action", "level")
    if action not in VALID_ACTIONS:
        errors.append(f"acción desconocida: {action!r}")
        return errors

    if action == "alternate":
        return errors

    if action == "group":
        group = body.get("group", "habitacion")
        if group not in VALID_GROUPS:
            errors.append(f"grupo desconocido: {group!r}")
        enabled = body.get("enabled")
        if not isinstance(enabled, bool) and enabled != "toggle":
            errors.append(f"enabled inválido (true, false o \"toggle\"): {enabled!r}")
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


def validate_zone_state(obj: Any, zone_name: str) -> list[str]:
    errors: list[str] = []
    if not isinstance(obj, dict):
        return [f"{zone_name}: no es objeto"]
    missing = ZONE_STATE_KEYS - obj.keys()
    if missing:
        errors.append(f"{zone_name}: faltan claves {sorted(missing)}")
    if "on" in obj and not isinstance(obj["on"], bool):
        errors.append(f"{zone_name}.on debe ser bool")
    return errors


def validate_state_payload(data: Any) -> list[str]:
    errors: list[str] = []
    if not isinstance(data, dict):
        return ["state: raíz no es objeto"]
    for zone in ("cuna", "setup"):
        if zone not in data:
            errors.append(f"state: falta zona {zone!r}")
        else:
            errors.extend(validate_zone_state(data[zone], zone))

    # Estados directos para rutinas; Atajos puede leer baby sin recorrer routines[].
    for routine_id in ("habitacion", "baby"):
        if routine_id not in data:
            errors.append(f"state: falta estado directo de rutina {routine_id!r}")
        elif not isinstance(data[routine_id], bool):
            errors.append(f"state.{routine_id} debe ser bool")
    return errors


# Contrato de action "group" (Habitación / Baby): (cuerpo, ¿válido?).
GROUP_CASES: list[tuple[dict[str, Any], bool]] = [
    ({"action": "group", "group": "habitacion", "enabled": "toggle"}, True),
    ({"action": "group", "group": "baby", "enabled": "toggle"}, True),
    ({"action": "group", "enabled": "toggle"}, True),
    ({"action": "group", "group": "baby"}, False),
    ({"action": "group", "group": "baby", "enabled": "true"}, False),
    ({"action": "group", "group": "cocina", "enabled": "toggle"}, False),
]


def validate_group_cases() -> int:
    failed = 0
    for body, expected in GROUP_CASES:
        ok = not validate_command_body(body)
        serialized = json.dumps(body, separators=(",", ":"), ensure_ascii=False)
        if ok != expected:
            failed += 1
            print(f"FAIL group {'aceptado' if ok else 'rechazado'}: {serialized}", file=sys.stderr)
        else:
            print(f"OK   group {'válido' if expected else 'inválido'}: {serialized}")
    return failed


def validate_all() -> int:
    failed = validate_group_cases()
    for name, steps in SHORTCUTS.items():
        for i, step in enumerate(steps, start=1):
            errs = validate_command_body(step.body)
            if errs:
                failed += 1
                print(f"FAIL {name} paso {i}: {errs}", file=sys.stderr)
            else:
                serialized = json.dumps(step.body, separators=(",", ":"), ensure_ascii=False)
                print(f"OK   {name} paso {i}: {serialized}")
    for name in sorted(SHORTCUTS_GET_ONLY):
        print(f"OK   {name}: solo GET /api/state (sin POST)")
    sample = {
        "cuna": {
            "level": 0,
            "levelFine": 0.0,
            "targetLevel": 0,
            "on": False,
            "lit": False,
            "transitioning": False,
            "phase": "steady",
        },
        "setup": {
            "level": 50,
            "levelFine": 50.0,
            "targetLevel": 50,
            "on": True,
            "lit": True,
            "transitioning": False,
            "phase": "steady",
        },
        "habitacion": False,
        "baby": True,
        "routine": {"active": "baby", "state": "running", "step": "cuna"},
        "routines": [
            {"id": "habitacion", "active": False},
            {"id": "baby", "active": True},
        ],
    }
    state_errs = validate_state_payload(sample)
    if state_errs:
        failed += 1
        print(f"FAIL muestra state: {state_errs}", file=sys.stderr)
    else:
        print("OK   contrato GET /api/state (zonas cuna/setup)")
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


def get_state(base_url: str) -> tuple[int, Any]:
    state_url = base_url.rstrip("/") + "/api/state"
    with urllib.request.urlopen(state_url, timeout=8) as resp:
        raw = resp.read().decode("utf-8", errors="replace")
        return resp.status, json.loads(raw)


def live_check(base_url: str, toggle_zone: str) -> int:
    """GET state (validar JSON); POST toggle (altera luces)."""
    failed = 0
    try:
        status, data = get_state(base_url)
        if status != 200:
            print(f"FAIL GET /api/state status {status}", file=sys.stderr)
            failed += 1
        else:
            errs = validate_state_payload(data)
            if errs:
                failed += 1
                print(f"FAIL GET /api/state estructura: {errs}", file=sys.stderr)
            else:
                cuna_on = data["cuna"]["on"]
                setup_on = data["setup"]["on"]
                print(f"OK   GET /api/state (cuna.on={cuna_on}, setup.on={setup_on})")
    except (OSError, json.JSONDecodeError) as e:
        print(f"FAIL GET /api/state: {e}", file=sys.stderr)
        return 1

    body = {"action": "toggle", "zone": toggle_zone}
    if validate_command_body(body):
        print(f"FAIL zona toggle inválida: {toggle_zone!r}", file=sys.stderr)
        return failed + 1

    code, text = post_command(base_url, body)
    if code != 200 or '"ok":true' not in text.replace(" ", ""):
        print(f"FAIL POST toggle {toggle_zone}: {code} {text}", file=sys.stderr)
        failed += 1
    else:
        print(f"OK   POST toggle zone={toggle_zone}")

    return failed


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument(
        "--live",
        action="store_true",
        help="GET state + POST toggle contra HOMELIGHTS_BASE_URL (altera luces)",
    )
    ap.add_argument(
        "--live-toggle-zone",
        default="cuna",
        choices=sorted(VALID_ZONES),
        help="zona usada en --live (default: cuna)",
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
            live_fail = live_check(base, args.live_toggle_zone)

    total = fmt_fail + live_fail
    if total:
        print(f"\n{total} comprobación(es) fallida(s).", file=sys.stderr)
        return 1
    print("\nTodas las comprobaciones pasaron.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
