"""Regenera index.h desde index.html (UTF-8)."""
from pathlib import Path

root = Path(__file__).resolve().parent
html = (root / "index.html").read_text(encoding="utf-8")
header = (
    "#pragma once\n\n"
    "// Contenido embebido de index.html (servido en / e /index.html)\n"
    "#include <pgmspace.h>\n\n"
    'const char INDEX_HTML[] PROGMEM = R"rawliteral(\n'
)
footer = "\n)rawliteral\";\n"
(root / "index.h").write_text(header + html + footer, encoding="utf-8", newline="\n")
print("index.h actualizado")
