#pragma once

// Contenido embebido de index.html (servido en / e /index.html)
#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es" data-theme="dark">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
  <title>Home</title>
  <script>
    try {
      if (localStorage.getItem('homeTheme') === 'light') {
        document.documentElement.dataset.theme = 'light';
      }
    } catch (e) {}
  </script>
  <style>
    :root {
      --text: #f6f1e8;
      --muted: rgba(246, 241, 232, 0.58);
      --bg: #141210;
      --card: #2a2723;
      --card-border: rgba(255, 255, 255, 0.06);
      --cuna-on: #e4b15a;
      --setup-on: #efe6d4;
      --group-on: #f0c56e;
      --on-text: #2a2114;
      --nav: rgba(22, 20, 18, 0.92);
      --field: rgba(0, 0, 0, 0.28);
      --line: rgba(255, 255, 255, 0.08);
      --accent: #e4b15a;
      color-scheme: dark;
    }

    html[data-theme="light"] {
      --text: #1e1a16;
      --muted: rgba(30, 26, 22, 0.58);
      --bg: #f3eee7;
      --card: #fffdf9;
      --card-border: rgba(40, 30, 16, 0.08);
      --cuna-on: #e7b45c;
      --setup-on: #d9e6f5;
      --group-on: #f2d48a;
      --on-text: #2a2114;
      --nav: rgba(255, 252, 247, 0.94);
      --field: #fff;
      --line: rgba(40, 30, 16, 0.1);
      --accent: #c8892d;
      color-scheme: light;
    }

    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }

    body {
      margin: 0;
      min-height: 100dvh;
      font-family: -apple-system, "Segoe UI", system-ui, sans-serif;
      color: var(--text);
      background: var(--bg);
    }

    .app {
      width: min(440px, 100%);
      margin: 0 auto;
      padding:
        max(0.85rem, calc(env(safe-area-inset-top) + 0.35rem))
        max(1rem, env(safe-area-inset-right))
        calc(6.2rem + env(safe-area-inset-bottom))
        max(1rem, env(safe-area-inset-left));
    }

    .top {
      display: flex;
      align-items: flex-start;
      justify-content: space-between;
      gap: 0.75rem;
      margin-bottom: 0.35rem;
    }

    h1 {
      margin: 0;
      font-size: 2rem;
      font-weight: 650;
      letter-spacing: -0.03em;
    }

    .header-meta {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 0.45rem 0.6rem;
      margin-top: 0.35rem;
    }

    #datetime, .status {
      margin: 0;
      color: var(--muted);
      font-size: 0.82rem;
      line-height: 1.35;
    }

    .status:empty { display: none; }

    .weather-pill {
      display: inline-flex;
      align-items: center;
      gap: 0.3rem;
      min-height: 28px;
      padding: 0.15rem 0.55rem;
      border-radius: 999px;
      background: var(--card);
      font-size: 0.78rem;
    }

    .weather-pill.is-hidden { display: none; }
    .weather-icon svg { width: 16px; height: 16px; display: block; }

    .icon-btn, .tab, .switch, .back, .choice, .chip, .text-btn, .primary {
      font: inherit;
      color: inherit;
      cursor: pointer;
      touch-action: manipulation;
    }

    .icon-btn {
      width: 44px;
      height: 44px;
      border: 0;
      border-radius: 14px;
      background: var(--card);
      display: grid;
      place-items: center;
      flex: 0 0 auto;
    }

    .icon-btn svg, .tab svg, .tile-icon svg { width: 22px; height: 22px; display: block; }
    .icon-btn.is-on { background: var(--accent); color: var(--on-text); }

    .banner {
      margin: 0.8rem 0 0;
      padding: 0.7rem 0.85rem;
      border-radius: 14px;
      background: var(--accent);
      color: var(--on-text);
      font-size: 0.86rem;
    }

    .banner[hidden] { display: none; }

    .section-label {
      margin: 1.25rem 0.15rem 0.5rem;
      font-size: 0.72rem;
      font-weight: 700;
      letter-spacing: 0.14em;
      text-transform: uppercase;
      color: var(--muted);
    }

    .tile {
      background: var(--card);
      border: 1px solid var(--card-border);
      border-radius: 22px;
      margin-bottom: 0.7rem;
      overflow: hidden;
    }

    .tile.cuna.is-on { background: var(--cuna-on); color: var(--on-text); border-color: transparent; }
    .tile.setup.is-on { background: var(--setup-on); color: var(--on-text); border-color: transparent; }
    .tile.group.is-on { background: var(--group-on); color: var(--on-text); border-color: transparent; }

    .tile-row { display: flex; align-items: center; padding-right: 0.85rem; }

    .tile-main {
      flex: 1;
      min-width: 0;
      min-height: 78px;
      display: flex;
      align-items: center;
      gap: 0.8rem;
      padding: 0.8rem 0.35rem 0.8rem 0.9rem;
      border: 0;
      background: transparent;
      color: inherit;
      text-align: left;
    }

    .tile-icon {
      width: 42px;
      height: 42px;
      border-radius: 14px;
      display: grid;
      place-items: center;
      background: rgba(255, 255, 255, 0.08);
      flex: 0 0 auto;
    }

    .tile.is-on .tile-icon { background: rgba(255, 255, 255, 0.28); }

    .tile-copy { min-width: 0; }
    .tile-name, .tile-sub { display: block; }
    .tile-name { font-size: 1.05rem; font-weight: 650; }
    .tile-sub { margin-top: 0.15rem; font-size: 0.8rem; opacity: 0.75; }

    .switch {
      width: 52px;
      height: 32px;
      border: 0;
      border-radius: 999px;
      background: #4a4642;
      position: relative;
      flex: 0 0 auto;
    }

    html[data-theme="light"] .switch { background: #d7d1c8; }

    .switch::after {
      content: "";
      position: absolute;
      width: 26px;
      height: 26px;
      border-radius: 50%;
      background: #fff;
      top: 3px;
      left: 3px;
      transition: transform 0.18s ease;
      box-shadow: 0 1px 3px rgba(0, 0, 0, 0.35);
    }

    .switch[aria-checked="true"] { background: rgba(255, 255, 255, 0.92); }
    .switch[aria-checked="true"]::after { transform: translateX(20px); }

    .tile-extra { padding: 0 1rem 0.95rem; }
    .tile-extra[hidden] { display: none; }
    .tile-extra .field-label { margin: 0 0 0.35rem; }
    .hint { margin: 0; font-size: 0.82rem; opacity: 0.8; }

    input[type="range"] { width: 100%; height: 32px; accent-color: currentColor; }

    .menu-row, .back {
      width: 100%;
      min-height: 58px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 0.75rem;
      padding: 0.85rem 1rem;
      border: 0;
      border-bottom: 1px solid var(--line);
      background: transparent;
      color: inherit;
      text-align: left;
      font-size: 1rem;
    }

    .panel {
      background: var(--card);
      border: 1px solid var(--card-border);
      border-radius: 22px;
      overflow: hidden;
      margin-bottom: 0.85rem;
    }

    .panel h2, .subhead {
      margin: 0;
      padding: 1rem 1rem 0.2rem;
      font-size: 1.15rem;
    }

    .help {
      margin: 0.25rem 1rem 0.8rem;
      color: var(--muted);
      font-size: 0.82rem;
      line-height: 1.4;
    }

    label.field-label {
      display: block;
      margin: 0.7rem 1rem 0.3rem;
      color: var(--muted);
      font-size: 0.75rem;
      font-weight: 650;
      letter-spacing: 0.04em;
      text-transform: uppercase;
    }

    .field {
      width: calc(100% - 2rem);
      min-height: 44px;
      margin: 0 1rem 0.35rem;
      padding: 0.65rem 0.8rem;
      border-radius: 12px;
      border: 1px solid var(--line);
      background: var(--field);
      color: var(--text);
      font: inherit;
      font-size: 1rem;
    }

    .choices, .chips, .row { display: flex; gap: 0.5rem; }
    .choices, .chips { padding: 0.35rem 1rem 1rem; }
    .row { padding: 0.4rem 1rem 1rem; }

    .choice, .chip, .text-btn, .primary {
      min-height: 44px;
      border-radius: 14px;
      border: 1px solid var(--line);
      background: transparent;
    }

    .choice, .chip { flex: 1; padding: 0.55rem 0.4rem; }
    .choice[aria-pressed="true"], .chip[aria-pressed="true"], .primary {
      background: var(--text);
      color: var(--bg);
      border-color: transparent;
      font-weight: 700;
    }

    .text-btn, .primary { flex: 1; padding: 0.7rem 0.8rem; }
    .text-btn.danger { color: #ffb4a8; }

    .routine {
      display: flex;
      align-items: center;
      gap: 0.75rem;
      padding: 0.9rem 1rem;
      border-top: 1px solid var(--line);
    }

    .routine-time { font-size: 1.25rem; font-weight: 700; letter-spacing: -0.03em; }
    .routine-detail { color: var(--muted); font-size: 0.82rem; margin-top: 0.15rem; }
    .routine button {
      margin-left: auto;
      min-width: 44px;
      min-height: 44px;
      border: 0;
      border-radius: 12px;
      background: transparent;
      color: var(--muted);
      font: inherit;
      font-size: 1.3rem;
    }

    .empty { margin: 0; padding: 1rem; color: var(--muted); }

    .dirty-bar {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 0.75rem;
      margin-bottom: 0.8rem;
      padding: 0.7rem 0.8rem;
      border-radius: 16px;
      background: var(--accent);
      color: var(--on-text);
    }

    .dirty-bar[hidden] { display: none; }
    .dirty-bar button {
      min-height: 40px;
      border: 0;
      border-radius: 12px;
      background: var(--on-text);
      color: var(--accent);
      font: inherit;
      font-weight: 700;
      padding: 0.4rem 0.75rem;
    }

    .tabbar {
      position: fixed;
      left: 0;
      right: 0;
      bottom: 0;
      z-index: 5;
      padding: 0.35rem 1rem calc(0.35rem + env(safe-area-inset-bottom));
      background: var(--nav);
      backdrop-filter: blur(16px);
      border-top: 1px solid var(--line);
    }

    .tabbar-inner {
      width: min(440px, 100%);
      margin: 0 auto;
      display: grid;
      grid-template-columns: repeat(3, 1fr);
    }

    .tab {
      min-height: 52px;
      border: 0;
      background: transparent;
      color: var(--muted);
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 0.15rem;
      font-size: 0.68rem;
      font-weight: 650;
    }

    .tab[aria-current="page"] { color: var(--text); }

    button:focus-visible, .field:focus-visible, input:focus-visible {
      outline: 2px solid var(--accent);
      outline-offset: 2px;
    }

    .view[hidden], .settings-page[hidden] { display: none; }
  </style>
</head>
<body>
  <div class="app">
    <header class="top">
      <div>
        <h1>Home</h1>
        <div class="header-meta">
          <p id="datetime">Conectando…</p>
          <div class="weather-pill is-hidden" id="weather-pill" aria-hidden="true">
            <span class="weather-icon" id="weather-icon"></span>
            <span class="weather-temp" id="weather-temp"></span>
          </div>
        </div>
      </div>
      <button class="icon-btn" id="alt-toggle" type="button" aria-pressed="false" aria-label="Activar modo alternado">
        <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M7 7h11l-2.5-2.5M17 17H6l2.5 2.5" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/></svg>
      </button>
    </header>
    <p id="status" class="status" role="status"></p>
    <p id="alt-banner" class="banner" hidden>Modo alternado activo: Cunero y Setup se encienden por turnos.</p>

    <section class="view" id="view-home">
      <h2 class="section-label">Zonas</h2>
      <article class="tile cuna" id="tile-cuna">
        <div class="tile-row">
          <button class="tile-main" id="open-cuna" type="button" aria-expanded="false" aria-controls="extra-cuna">
            <span class="tile-icon" aria-hidden="true"><svg viewBox="0 0 24 24"><path d="M5 10V7.5A1.5 1.5 0 0 1 6.5 6H8M19 10V7.5A1.5 1.5 0 0 0 17.5 6H16M4 10h16M7 10v7M17 10v7M4 17h16" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round"/></svg></span>
            <span class="tile-copy"><span class="tile-name">Cunero</span><span class="tile-sub" id="cuna-sub">Apagado</span></span>
          </button>
          <button class="switch" id="switch-cuna" type="button" role="switch" aria-checked="false" aria-label="Cunero"></button>
        </div>
        <div class="tile-extra" id="extra-cuna" hidden>
          <p class="hint" id="hint-cuna">Enciende la zona para ajustar el brillo.</p>
          <label class="field-label" for="level-cuna">Brillo</label>
          <input id="level-cuna" type="range" min="1" max="100" value="50" aria-valuemin="1" aria-valuemax="100" hidden>
        </div>
      </article>
      <article class="tile setup" id="tile-setup">
        <div class="tile-row">
          <button class="tile-main" id="open-setup" type="button" aria-expanded="false" aria-controls="extra-setup">
            <span class="tile-icon" aria-hidden="true"><svg viewBox="0 0 24 24"><path d="M4 14h16M6 14V8h12v6M8 18v-4M16 18v-4" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"/></svg></span>
            <span class="tile-copy"><span class="tile-name">Setup</span><span class="tile-sub" id="setup-sub">Apagado</span></span>
          </button>
          <button class="switch" id="switch-setup" type="button" role="switch" aria-checked="false" aria-label="Setup"></button>
        </div>
        <div class="tile-extra" id="extra-setup" hidden>
          <p class="hint" id="hint-setup">Enciende la zona para ajustar el brillo.</p>
          <label class="field-label" for="level-setup">Brillo</label>
          <input id="level-setup" type="range" min="1" max="100" value="50" aria-valuemin="1" aria-valuemax="100" hidden>
        </div>
      </article>

      <h2 class="section-label">Grupos</h2>
      <article class="tile group" id="tile-group">
        <div class="tile-row">
          <div class="tile-main" style="cursor:default">
            <span class="tile-icon" aria-hidden="true"><svg viewBox="0 0 24 24"><path d="M12 4l8 4-8 4-8-4 8-4zM4 12l8 4 8-4M4 16l8 4 8-4" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linejoin="round"/></svg></span>
            <span class="tile-copy"><span class="tile-name">Habitación</span><span class="tile-sub" id="group-sub">Primero Cunero al 35%, luego Setup al 100%</span></span>
          </div>
          <button class="switch" id="switch-group" type="button" role="switch" aria-checked="false" aria-label="Grupo Habitación"></button>
        </div>
      </article>
    </section>

    <section class="view" id="view-routines" hidden>
      <h2 class="section-label">Rutinas</h2>
      <div class="dirty-bar" id="routine-dirty" hidden>
        <span>Cambios sin guardar</span>
        <button type="button" id="save-routines">Guardar</button>
      </div>
      <div class="panel">
        <h2>Nueva rutina</h2>
        <p class="help">Elige cuándo y qué debe ocurrir. Encender usa el nivel configurado en Ajustes. El grupo Habitación, en Inicio, hace su propia secuencia.</p>
        <label class="field-label" for="sch-time">Hora</label>
        <input class="field" id="sch-time" type="time" value="22:00" required>
        <p class="field-label">Dónde</p>
        <div class="chips" id="zone-chips" role="radiogroup" aria-label="Dónde">
          <button class="chip" type="button" data-value="cuna" aria-pressed="true">Cunero</button>
          <button class="chip" type="button" data-value="setup" aria-pressed="false">Setup</button>
          <button class="chip" type="button" data-value="both" aria-pressed="false">Ambas</button>
        </div>
        <p class="field-label">Qué hacer</p>
        <div class="chips" id="action-chips" role="radiogroup" aria-label="Qué hacer">
          <button class="chip" type="button" data-value="off" aria-pressed="true">Apagar</button>
          <button class="chip" type="button" data-value="on" aria-pressed="false">Encender</button>
          <button class="chip" type="button" data-value="level" aria-pressed="false">Nivel</button>
        </div>
        <div id="level-box">
          <label class="field-label" for="sch-level">Nivel</label>
          <input class="field" id="sch-level" type="number" min="0" max="100" value="30">
        </div>
        <div class="row"><button class="primary" id="add-routine" type="button">Agregar</button></div>
      </div>
      <div class="panel" id="routine-list"></div>
    </section>

    <section class="view" id="view-settings" hidden>
      <div class="settings-page" id="settings-root">
        <h2 class="section-label">Ajustes</h2>
        <div class="panel">
          <h2>Apariencia</h2>
          <p class="help">Elige el modo que se vea mejor en la habitación.</p>
          <div class="choices" role="radiogroup" aria-label="Apariencia">
            <button class="choice" id="theme-dark" type="button" aria-pressed="true">Oscuro</button>
            <button class="choice" id="theme-light" type="button" aria-pressed="false">Claro</button>
          </div>
        </div>
        <div class="panel">
          <button class="menu-row" type="button" data-page="settings-light">Encendido <span aria-hidden="true">›</span></button>
          <button class="menu-row" type="button" data-page="settings-alternate">Modo alternado <span aria-hidden="true">›</span></button>
          <button class="menu-row" type="button" data-page="settings-firmware">Firmware <span aria-hidden="true">›</span></button>
          <button class="menu-row" id="open-link" type="button" data-page="settings-link" hidden>Conexión <span aria-hidden="true">›</span></button>
        </div>
      </div>

      <div class="settings-page" id="settings-light" hidden>
        <div class="panel">
          <button class="back" type="button">‹ Ajustes</button>
          <h2 class="subhead">Encendido</h2>
          <p class="help">Estos valores cambian la suavidad de todas las zonas y del grupo.</p>
          <label class="field-label" for="cfg-fade-on">Encendido suave (ms)</label>
          <input class="field" id="cfg-fade-on" type="number" min="200" max="15000" step="50" value="1100">
          <label class="field-label" for="cfg-fade-off">Apagado suave (ms)</label>
          <input class="field" id="cfg-fade-off" type="number" min="200" max="15000" step="50" value="1400">
          <label class="field-label" for="cfg-gamma">Curva de brillo</label>
          <input class="field" id="cfg-gamma" type="number" min="1" max="4" step="0.1" value="2.2">
          <label class="field-label" for="cfg-default-on">Nivel al encender (%)</label>
          <input class="field" id="cfg-default-on" type="number" min="1" max="100" value="50">
          <div class="row"><button class="primary" id="save-settings" type="button">Guardar</button></div>
        </div>
      </div>

      <div class="settings-page" id="settings-alternate" hidden>
        <div class="panel">
          <button class="back" type="button">‹ Ajustes</button>
          <h2 class="subhead">Modo alternado</h2>
          <p class="help">También puedes activarlo con el icono de la pantalla principal.</p>
          <div class="menu-row"><span>Activado</span><button class="switch" id="alt-switch" type="button" role="switch" aria-checked="false" aria-label="Modo alternado"></button></div>
          <label class="field-label" for="alt-level">Intensidad (%)</label>
          <input class="field" id="alt-level" type="number" min="1" max="100" value="80">
          <label class="field-label" for="alt-period">Tiempo de cada turno (ms)</label>
          <input class="field" id="alt-period" type="number" min="200" max="5000" step="100" value="700">
          <div class="row"><button class="primary" id="save-alternate" type="button">Guardar</button></div>
        </div>
      </div>

      <div class="settings-page" id="settings-firmware" hidden>
        <div class="panel">
          <button class="back" type="button">‹ Ajustes</button>
          <h2 class="subhead">Firmware</h2>
          <p class="field-label">Versión instalada</p>
          <p class="help" id="ota-version">–</p>
          <p class="field-label">Estado</p>
          <p class="help" id="ota-msg">–</p>
          <div class="row"><button class="primary" id="ota-btn" type="button">Buscar actualización</button></div>
        </div>
      </div>

      <div class="settings-page" id="settings-link" hidden>
        <div class="panel">
          <button class="back" type="button">‹ Ajustes</button>
          <h2 class="subhead">Conexión</h2>
          <label class="field-label" for="esp-base">Dirección del controlador</label>
          <input class="field" id="esp-base" type="url" placeholder="http://192.168.0.24" required>
          <div class="row"><button class="primary" id="save-link" type="button">Conectar</button></div>
        </div>
      </div>
    </section>
  </div>

  <nav class="tabbar" aria-label="Secciones">
    <div class="tabbar-inner">
      <button class="tab" type="button" data-view="home" aria-current="page"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M4 11.5 12 5l8 6.5V20a1 1 0 0 1-1 1h-5v-6H10v6H5a1 1 0 0 1-1-1v-8.5z" fill="currentColor"/></svg>Inicio</button>
      <button class="tab" type="button" data-view="routines" aria-current="false"><svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="12" cy="12" r="8" fill="none" stroke="currentColor" stroke-width="1.8"/><path d="M12 8v5l3 2" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/></svg>Rutinas</button>
      <button class="tab" type="button" data-view="settings" aria-current="false"><svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="12" cy="12" r="3" fill="none" stroke="currentColor" stroke-width="1.8"/><path d="M12 3.5v2.2M12 18.3v2.2M3.5 12h2.2M18.3 12h2.2M6 6l1.6 1.6M16.4 16.4 18 18M18 6l-1.6 1.6M7.6 16.4 6 18" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/></svg>Ajustes</button>
    </div>
  </nav>

<script>
const POLL_MS = 350;
const ZONE_LABEL = { cuna: 'Cunero', setup: 'Setup', both: 'Ambas' };
const ACTION_LABEL = { off: 'Apagar', on: 'Encender', level: 'Poner al' };
let schedules = [];
let schedulesDirty = false;
let alternateOn = false;
let expanded = null;
let groupRunning = false;
let groupStep = 'idle';
const groupLevels = { cuna: 35, setup: 100 };
const phase = { cuna: 'off', setup: 'off' };
const levelShown = { cuna: 0, setup: 0 };
const expectLevel = { cuna: null, setup: null };
const pending = { cuna: null, setup: null, group: null };
const dragging = new Set();
const dirtyFields = new Set();
const editingFields = new Set();
let settingsQuietUntil = 0;
let settingsSaveToken = 0;
let schedulesQuietUntil = 0;
let alternateQuietUntil = 0;
let pollRunning = false;
let pollQueued = false;
const sendTimers = {};
const routineChoice = { zone: 'cuna', action: 'off' };

function setStatus(message) {
  document.getElementById('status').textContent = message || '';
}

function setTheme(theme) {
  const next = theme === 'light' ? 'light' : 'dark';
  document.documentElement.dataset.theme = next;
  localStorage.setItem('homeTheme', next);
  document.getElementById('theme-dark').setAttribute('aria-pressed', String(next === 'dark'));
  document.getElementById('theme-light').setAttribute('aria-pressed', String(next === 'light'));
}

function showView(name) {
  document.querySelectorAll('.view').forEach((view) => {
    view.hidden = view.id !== 'view-' + name;
  });
  document.querySelectorAll('.tab').forEach((tab) => {
    tab.setAttribute('aria-current', tab.dataset.view === name ? 'page' : 'false');
  });
  if (name === 'settings') showSettingsPage('settings-root');
}

function showSettingsPage(id) {
  document.querySelectorAll('.settings-page').forEach((page) => {
    page.hidden = page.id !== id;
  });
  if (id === 'settings-firmware') refreshOta();
}

function trackFieldEdits() {
  document.querySelectorAll('.field').forEach((el) => {
    if (!el.id) return;
    el.addEventListener('focus', () => editingFields.add(el.id));
    el.addEventListener('input', () => dirtyFields.add(el.id));
    el.addEventListener('change', () => dirtyFields.add(el.id));
    el.addEventListener('blur', () => {
      setTimeout(() => {
        if (document.activeElement !== el) editingFields.delete(el.id);
      }, 300);
    });
  });
}

function setFieldValue(id, value, force) {
  if (!force && (editingFields.has(id) || dirtyFields.has(id))) return;
  const el = document.getElementById(id);
  if (el && String(el.value) !== String(value)) el.value = value;
}

function validateFields(ids) {
  for (const id of ids) {
    const el = document.getElementById(id);
    if (el && !el.reportValidity()) return false;
  }
  return true;
}

function isFileMode() { return location.protocol === 'file:'; }

function apiBase() {
  if (!isFileMode()) return '';
  const el = document.getElementById('esp-base');
  return ((el && el.value) || localStorage.getItem('espApiBase') || '').replace(/\/$/, '');
}

async function api(path, body) {
  const base = apiBase();
  if (isFileMode() && !base) throw new Error('Falta la dirección del controlador');
  const r = await fetch(base + path, {
    method: body ? 'POST' : 'GET',
    headers: body ? { 'Content-Type': 'application/json' } : {},
    body: body ? JSON.stringify(body) : undefined
  });
  const text = await r.text();
  let data = {};
  try { data = text ? JSON.parse(text) : {}; }
  catch (e) { throw new Error('Respuesta no válida'); }
  if (!r.ok) throw new Error(data.error || ('HTTP ' + r.status));
  return data;
}

function phaseFromServer(z) {
  if (!z) return 'off';
  if (z.phase) return z.phase;
  if (z.transitioning) return (+z.targetLevel <= 0 ? 'turning_off' : 'turning_on');
  return (z.on || +z.level > 0.5) ? 'on' : 'off';
}

function zoneText(zone) {
  const pct = Math.round(levelShown[zone]);
  if (phase[zone] === 'turning_on') return 'Encendiendo · ' + pct + '%';
  if (phase[zone] === 'turning_off') return 'Apagando · ' + pct + '%';
  if (phase[zone] === 'off') return 'Apagado';
  return pct + '%';
}

function paint() {
  ['cuna', 'setup'].forEach((zone) => {
    const on = phase[zone] !== 'off';
    document.getElementById('tile-' + zone).classList.toggle('is-on', on);
    document.getElementById(zone + '-sub').textContent = zoneText(zone);
    document.getElementById('switch-' + zone).setAttribute('aria-checked', String(on));
    const open = expanded === zone;
    document.getElementById('extra-' + zone).hidden = !open;
    document.getElementById('open-' + zone).setAttribute('aria-expanded', String(open));
    const slider = document.getElementById('level-' + zone);
    const hint = document.getElementById('hint-' + zone);
    slider.hidden = !on;
    slider.previousElementSibling.hidden = !on;
    hint.hidden = on;
    if (on && !dragging.has(zone)) {
      const pct = Math.max(1, Math.min(100, Math.round(levelShown[zone]) || 1));
      slider.value = String(pct);
      slider.setAttribute('aria-valuenow', String(pct));
    }
  });

  const anyOn = phase.cuna !== 'off' || phase.setup !== 'off';
  const bothOn = phase.cuna !== 'off' && phase.setup !== 'off';
  const groupOn = groupRunning || bothOn || pending.group === 'on';
  document.getElementById('tile-group').classList.toggle('is-on', groupOn);
  document.getElementById('switch-group').setAttribute('aria-checked', String(groupOn));
  let detail = 'Primero Cunero al ' + groupLevels.cuna + '%, luego Setup al ' + groupLevels.setup + '%';
  if (groupStep === 'cuna' || (pending.group === 'on' && !bothOn)) detail = 'Encendiendo Cunero…';
  else if (groupStep === 'setup') detail = 'Encendiendo Setup…';
  else if (!groupOn && anyOn) detail = 'Solo una zona está encendida';
  document.getElementById('group-sub').textContent = detail;

  document.getElementById('alt-toggle').setAttribute('aria-pressed', String(alternateOn));
  document.getElementById('alt-toggle').classList.toggle('is-on', alternateOn);
  document.getElementById('alt-toggle').setAttribute('aria-label', alternateOn ? 'Detener modo alternado' : 'Activar modo alternado');
  document.getElementById('alt-switch').setAttribute('aria-checked', String(alternateOn));
  document.getElementById('alt-banner').hidden = !alternateOn;
}

function acceptZone(zone, server) {
  const nextPhase = phaseFromServer(server);
  const serverLevel = server.levelFine != null ? +server.levelFine : +server.level;
  if (pending[zone] === 'on' && nextPhase === 'off') return;
  if (pending[zone] === 'off' && nextPhase !== 'off' && nextPhase !== 'turning_off') return;
  if (pending[zone] === 'on' && nextPhase !== 'off') pending[zone] = null;
  if (pending[zone] === 'off' && nextPhase === 'off') pending[zone] = null;
  phase[zone] = nextPhase;
  if (dragging.has(zone)) return;
  if (expectLevel[zone] != null) {
    if (Math.abs(serverLevel - expectLevel[zone]) <= 2 || nextPhase === 'off') expectLevel[zone] = null;
    else {
      levelShown[zone] = expectLevel[zone];
      return;
    }
  }
  levelShown[zone] = nextPhase === 'off' ? 0 : serverLevel;
}

function applyGroup(s) {
  const g = s.group || {};
  if (Number.isFinite(+g.cunaLevel)) groupLevels.cuna = Math.round(+g.cunaLevel);
  if (Number.isFinite(+g.setupLevel)) groupLevels.setup = Math.round(+g.setupLevel);
  const running = !!g.running;
  const bothOn = phase.cuna !== 'off' && phase.setup !== 'off';
  const bothOff = phase.cuna === 'off' && phase.setup === 'off';
  if (pending.group === 'on' && !running && !bothOn) {
    groupRunning = true;
    groupStep = 'cuna';
    return;
  }
  if (pending.group === 'off' && !bothOff) {
    groupRunning = false;
    groupStep = 'idle';
    return;
  }
  pending.group = null;
  groupRunning = running;
  groupStep = g.step || 'idle';
}

function sendLevel(zone, value) {
  clearTimeout(sendTimers[zone]);
  sendTimers[zone] = setTimeout(async () => {
    if (expectLevel[zone] !== value || phase[zone] === 'off') return;
    try {
      await api('/api/command', { zone, action: 'level', value });
    } catch (e) {
      expectLevel[zone] = null;
      setStatus(e.message);
    }
  }, 40);
}

async function toggleZone(zone) {
  const turnOn = phase[zone] === 'off';
  pending[zone] = turnOn ? 'on' : 'off';
  phase[zone] = turnOn ? 'turning_on' : 'turning_off';
  if (!turnOn) levelShown[zone] = 0;
  paint();
  try {
    await api('/api/command', { zone, action: turnOn ? 'on' : 'off' });
    poll();
  } catch (e) {
    pending[zone] = null;
    setStatus(e.message);
    poll();
  }
}

async function toggleGroup() {
  const on = document.getElementById('switch-group').getAttribute('aria-checked') === 'true';
  pending.group = on ? 'off' : 'on';
  if (pending.group === 'on') {
    groupRunning = true;
    groupStep = 'cuna';
  } else {
    groupRunning = false;
    groupStep = 'idle';
    pending.cuna = 'off';
    pending.setup = 'off';
    phase.cuna = 'turning_off';
    phase.setup = 'turning_off';
  }
  paint();
  try {
    await api('/api/command', { action: 'group', group: 'habitacion', enabled: !on });
    poll();
  } catch (e) {
    pending.group = null;
    setStatus(e.message);
    poll();
  }
}

async function toggleAlternate() {
  if (!validateFields(['alt-level', 'alt-period'])) {
    showView('settings');
    showSettingsPage('settings-alternate');
    return;
  }
  const next = !alternateOn;
  alternateOn = next;
  paint();
  try {
    const res = await api('/api/alternate', {
      period: +document.getElementById('alt-period').value,
      level: +document.getElementById('alt-level').value,
      enabled: next
    });
    ['alt-period', 'alt-level'].forEach((id) => dirtyFields.delete(id));
    applyAlternateFromServer(res.alternate, true);
    alternateQuietUntil = Date.now() + 8000;
    poll();
  } catch (e) {
    alternateOn = !next;
    paint();
    setStatus(e.message);
  }
}

function bindChips(id, key) {
  const root = document.getElementById(id);
  root.querySelectorAll('.chip').forEach((chip) => {
    chip.addEventListener('click', () => {
      root.querySelectorAll('.chip').forEach((item) => item.setAttribute('aria-pressed', 'false'));
      chip.setAttribute('aria-pressed', 'true');
      routineChoice[key] = chip.dataset.value;
      if (key === 'action') document.getElementById('level-box').hidden = chip.dataset.value !== 'level';
    });
  });
}

function formatClock(hour, minute) {
  const pm = hour >= 12;
  let h = hour % 12;
  if (h === 0) h = 12;
  return h + ':' + String(minute).padStart(2, '0') + (pm ? ' p.m.' : ' a.m.');
}

function renderSchedules() {
  document.getElementById('routine-dirty').hidden = !schedulesDirty;
  const el = document.getElementById('routine-list');
  if (!schedules.length) {
    el.innerHTML = '<p class="empty">Todavía no hay rutinas.</p>';
    return;
  }
  el.innerHTML = schedules.map((item, i) => {
    const where = ZONE_LABEL[item.zone] || item.zone;
    const what = item.action === 'level'
      ? 'Poner ' + where + ' al ' + item.level + '%'
      : ACTION_LABEL[item.action] + ' ' + where;
    return '<article class="routine"><div><div class="routine-time">' + formatClock(+item.hour, +item.minute) +
      '</div><div class="routine-detail">' + what + (item.enabled ? '' : ' · pausada') +
      '</div></div><button type="button" aria-label="Eliminar rutina ' + (i + 1) + '" data-remove="' + i + '">×</button></article>';
  }).join('');
}

function addSchedule() {
  if (!validateFields(['sch-time', 'sch-level'])) return;
  if (schedules.length >= 10) {
    setStatus('Puedes guardar hasta 10 rutinas');
    return;
  }
  const parts = document.getElementById('sch-time').value.split(':');
  const hour = +parts[0];
  const minute = +parts[1];
  if (!Number.isInteger(hour) || !Number.isInteger(minute)) return;
  schedulesDirty = true;
  schedules.push({
    enabled: true,
    hour: hour,
    minute: minute,
    zone: routineChoice.zone,
    action: routineChoice.action,
    level: +document.getElementById('sch-level').value
  });
  renderSchedules();
}

async function saveSchedules() {
  schedulesQuietUntil = Date.now() + 8000;
  try {
    const res = await api('/api/schedule', { schedules });
    if (res.schedules) schedules = res.schedules;
    schedulesDirty = false;
    schedulesQuietUntil = Date.now() + 8000;
    renderSchedules();
    setStatus('Rutinas guardadas');
  } catch (e) {
    schedulesQuietUntil = 0;
    setStatus(e.message);
  }
}

function readSettingsForm() {
  return {
    fadeOnMs: +document.getElementById('cfg-fade-on').value,
    fadeOffMs: +document.getElementById('cfg-fade-off').value,
    fadeGamma: +document.getElementById('cfg-gamma').value,
    defaultOnLevel: +document.getElementById('cfg-default-on').value
  };
}

function applySettingsFromServer(st, force, token) {
  if (!st) return;
  if (!force && (Date.now() < settingsQuietUntil || (token != null && token !== settingsSaveToken))) return;
  [['cfg-fade-on', st.fadeOnMs], ['cfg-fade-off', st.fadeOffMs], ['cfg-gamma', st.fadeGamma], ['cfg-default-on', st.defaultOnLevel]]
    .forEach(([id, value]) => {
      if (Number.isFinite(+value)) setFieldValue(id, value, force);
    });
}

async function saveSettings() {
  if (!validateFields(['cfg-fade-on', 'cfg-fade-off', 'cfg-gamma', 'cfg-default-on'])) return;
  const payload = readSettingsForm();
  if (Object.values(payload).some((value) => !Number.isFinite(value))) {
    setStatus('Revisa los valores');
    return;
  }
  settingsSaveToken++;
  settingsQuietUntil = Date.now() + 10000;
  try {
    const res = await api('/api/settings', { settings: payload });
    ['cfg-fade-on', 'cfg-fade-off', 'cfg-gamma', 'cfg-default-on'].forEach((id) => dirtyFields.delete(id));
    applySettingsFromServer(res.settings, true);
    settingsQuietUntil = Date.now() + 3000;
    setStatus('Encendido guardado');
    poll();
  } catch (e) {
    settingsSaveToken--;
    settingsQuietUntil = 0;
    setStatus(e.message);
  }
}

function applyAlternateFromServer(alt, force) {
  if (!alt) return;
  if (!force && Date.now() < alternateQuietUntil) return;
  setFieldValue('alt-period', alt.period, force);
  setFieldValue('alt-level', alt.level, force);
  alternateOn = !!alt.enabled;
  paint();
}

async function saveAlternate() {
  if (!validateFields(['alt-level', 'alt-period'])) return;
  alternateQuietUntil = Date.now() + 8000;
  try {
    const res = await api('/api/alternate', {
      period: +document.getElementById('alt-period').value,
      level: +document.getElementById('alt-level').value
    });
    ['alt-period', 'alt-level'].forEach((id) => dirtyFields.delete(id));
    applyAlternateFromServer(res.alternate, true);
    alternateQuietUntil = Date.now() + 8000;
    setStatus('Modo alternado guardado');
  } catch (e) {
    alternateQuietUntil = 0;
    setStatus(e.message);
  }
}

const WEATHER_CACHE_KEY = 'homeWeatherV1';
const WEATHER_FETCH_MS = 30 * 60 * 1000;
const WEATHER_CACHE_MAX_MS = 2 * 60 * 60 * 1000;
let weatherLat = null;
let weatherLon = null;
let weatherTimerStarted = false;
let lastDateTimeMinuteKey = '';
const DIAS = ['Domingo', 'Lunes', 'Martes', 'Miércoles', 'Jueves', 'Viernes', 'Sábado'];
const MESES = ['enero', 'febrero', 'marzo', 'abril', 'mayo', 'junio', 'julio', 'agosto', 'septiembre', 'octubre', 'noviembre', 'diciembre'];

function clockToDate(clock) {
  const match = String(clock || '').match(/^(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})$/);
  if (!match) return new Date();
  return new Date(+match[1], +match[2] - 1, +match[3], +match[4], +match[5], +match[6]);
}

function updateDateTimeFromClock(clock) {
  const date = clockToDate(clock);
  const key = [date.getFullYear(), date.getMonth(), date.getDate(), date.getHours(), date.getMinutes()].join('-');
  if (key === lastDateTimeMinuteKey) return;
  lastDateTimeMinuteKey = key;
  let hour = date.getHours();
  const pm = hour >= 12;
  if (hour === 0) hour = 12;
  else if (hour > 12) hour -= 12;
  document.getElementById('datetime').textContent =
    DIAS[date.getDay()] + ' ' + date.getDate() + ' de ' + MESES[date.getMonth()] +
    ' · ' + hour + ':' + String(date.getMinutes()).padStart(2, '0') + (pm ? ' p.m.' : ' a.m.');
}

function weatherIcon(kind, isDay) {
  if (kind === 'clear' && isDay) return '<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="4" fill="currentColor"/></svg>';
  if (kind === 'clear') return '<svg viewBox="0 0 24 24"><path d="M15 5a6 6 0 1 0 6 8 5 5 0 0 1-8 6H8a5 5 0 0 1 0-10 6 6 0 0 1 7-4z" fill="currentColor"/></svg>';
  if (kind === 'rain') return '<svg viewBox="0 0 24 24"><path d="M7 15a5 5 0 0 1 0-10 6 6 0 0 1 11 2 4 4 0 0 1 0 8z" fill="none" stroke="currentColor" stroke-width="1.6"/><path d="M9 18l-1 2M13 18l-1 2" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/></svg>';
  return '<svg viewBox="0 0 24 24"><path d="M7 16a5 5 0 0 1 0-10 6 6 0 0 1 11 2 4 4 0 0 1 0 8z" fill="currentColor"/></svg>';
}

function renderWeather(data) {
  const pill = document.getElementById('weather-pill');
  if (!data || data.temp == null) {
    pill.classList.add('is-hidden');
    pill.setAttribute('aria-hidden', 'true');
    return;
  }
  const isDay = data.isDay != null ? !!data.isDay : (new Date().getHours() >= 7 && new Date().getHours() < 19);
  const code = +data.code || 0;
  const kind = code <= 1 ? 'clear' : ((code >= 51 && code <= 67) || code >= 80 ? 'rain' : 'cloud');
  document.getElementById('weather-icon').innerHTML = weatherIcon(kind, isDay);
  document.getElementById('weather-temp').textContent = Math.round(+data.temp) + '°';
  pill.classList.remove('is-hidden');
  pill.setAttribute('aria-hidden', 'false');
}

function readWeatherCache() {
  try { return JSON.parse(localStorage.getItem(WEATHER_CACHE_KEY) || 'null'); }
  catch (e) { return null; }
}

async function fetchWeatherFromNetwork() {
  if (weatherLat == null || weatherLon == null) return;
  try {
    const response = await fetch('https://api.open-meteo.com/v1/forecast?latitude=' + weatherLat + '&longitude=' + weatherLon + '&current=temperature_2m,weather_code,is_day');
    if (!response.ok) throw new Error('clima');
    const json = await response.json();
    const current = json.current || {};
    const data = { temp: current.temperature_2m, code: current.weather_code, isDay: current.is_day === 1 };
    localStorage.setItem(WEATHER_CACHE_KEY, JSON.stringify({ ts: Date.now(), data }));
    renderWeather(data);
  } catch (e) {
    const cached = readWeatherCache();
    if (cached && Date.now() - cached.ts < WEATHER_CACHE_MAX_MS) renderWeather(cached.data);
  }
}

function syncWeatherCoords(s) {
  if (!Number.isFinite(+s.weatherLat) || !Number.isFinite(+s.weatherLon)) return;
  const changed = weatherLat !== +s.weatherLat || weatherLon !== +s.weatherLon;
  weatherLat = +s.weatherLat;
  weatherLon = +s.weatherLon;
  if (!changed && weatherTimerStarted) return;
  weatherTimerStarted = true;
  const cached = readWeatherCache();
  if (cached && Date.now() - cached.ts < WEATHER_CACHE_MAX_MS) renderWeather(cached.data);
  fetchWeatherFromNetwork();
  setInterval(fetchWeatherFromNetwork, WEATHER_FETCH_MS);
}

function applyStateFromPoll(s, token) {
  if (!s.cuna || !s.setup) throw new Error('Estado inválido');
  updateDateTimeFromClock(s.clock);
  syncWeatherCoords(s);
  ['cuna', 'setup'].forEach((zone) => acceptZone(zone, s[zone]));
  applyGroup(s);
  applyAlternateFromServer(s.alternate, false);
  applySettingsFromServer(s.settings, false, token);
  if (!schedulesDirty && Date.now() >= schedulesQuietUntil) {
    schedules = s.schedules || [];
    renderSchedules();
  }
  paint();
}

let liveError = '';

async function poll() {
  if (pollRunning) {
    pollQueued = true;
    return;
  }
  pollRunning = true;
  const token = settingsSaveToken;
  try {
    applyStateFromPoll(await api('/api/state'), token);
    if (liveError) {
      liveError = '';
      setStatus('');
    }
  } catch (e) {
    if (e.name !== 'AbortError') {
      liveError = e.message;
      setStatus(e.message);
    }
  } finally {
    pollRunning = false;
    if (pollQueued) {
      pollQueued = false;
      poll();
    }
  }
}

function applyOtaStatus(o) {
  if (!o) return;
  document.getElementById('ota-version').textContent = 'v' + o.version + (o.latest ? ' · publicada v' + o.latest : '');
  document.getElementById('ota-msg').textContent = (o.message || o.state || '–') + (o.state === 'downloading' ? ' ' + o.progress + '%' : '');
  document.getElementById('ota-btn').disabled = !!o.busy;
}

async function refreshOta() {
  try { applyOtaStatus(await api('/api/ota/status')); }
  catch (e) { document.getElementById('ota-msg').textContent = e.message; }
}

async function checkOta() {
  document.getElementById('ota-btn').disabled = true;
  try {
    await api('/api/ota/check', {});
    document.getElementById('ota-msg').textContent = 'Buscando versión nueva…';
  } catch (e) {
    document.getElementById('ota-msg').textContent = e.message;
  }
  setTimeout(refreshOta, 1500);
}

document.querySelectorAll('.tab').forEach((tab) => {
  tab.addEventListener('click', () => showView(tab.dataset.view));
});
document.querySelectorAll('[data-page]').forEach((button) => {
  button.addEventListener('click', () => showSettingsPage(button.dataset.page));
});
document.querySelectorAll('.back').forEach((button) => {
  button.addEventListener('click', () => showSettingsPage('settings-root'));
});
document.getElementById('theme-dark').addEventListener('click', () => setTheme('dark'));
document.getElementById('theme-light').addEventListener('click', () => setTheme('light'));
document.getElementById('alt-toggle').addEventListener('click', toggleAlternate);
document.getElementById('alt-switch').addEventListener('click', toggleAlternate);
document.getElementById('switch-group').addEventListener('click', toggleGroup);
document.getElementById('save-settings').addEventListener('click', saveSettings);
document.getElementById('save-alternate').addEventListener('click', saveAlternate);
document.getElementById('ota-btn').addEventListener('click', checkOta);
document.getElementById('add-routine').addEventListener('click', addSchedule);
document.getElementById('save-routines').addEventListener('click', saveSchedules);
document.getElementById('routine-list').addEventListener('click', (event) => {
  const button = event.target.closest('[data-remove]');
  if (!button) return;
  schedulesDirty = true;
  schedules.splice(+button.dataset.remove, 1);
  renderSchedules();
});
document.getElementById('save-link').addEventListener('click', () => {
  const input = document.getElementById('esp-base');
  if (!input.reportValidity()) return;
  localStorage.setItem('espApiBase', input.value.trim().replace(/\/$/, ''));
  dirtyFields.delete('esp-base');
  poll();
});

['cuna', 'setup'].forEach((zone) => {
  document.getElementById('open-' + zone).addEventListener('click', () => {
    expanded = expanded === zone ? null : zone;
    paint();
  });
  document.getElementById('switch-' + zone).addEventListener('click', () => toggleZone(zone));
  const slider = document.getElementById('level-' + zone);
  slider.addEventListener('pointerdown', () => dragging.add(zone));
  const release = () => dragging.delete(zone);
  slider.addEventListener('pointerup', release);
  slider.addEventListener('pointercancel', release);
  slider.addEventListener('input', () => {
    const value = Math.round(+slider.value);
    expectLevel[zone] = value;
    levelShown[zone] = value;
    phase[zone] = 'on';
    slider.setAttribute('aria-valuenow', String(value));
    document.getElementById(zone + '-sub').textContent = value + '%';
    sendLevel(zone, value);
  });
});

bindChips('zone-chips', 'zone');
bindChips('action-chips', 'action');
document.getElementById('level-box').hidden = true;
setTheme(document.documentElement.dataset.theme || 'dark');
trackFieldEdits();
renderSchedules();
paint();

if (isFileMode()) {
  document.getElementById('open-link').hidden = false;
  const saved = localStorage.getItem('espApiBase');
  if (saved) document.getElementById('esp-base').value = saved;
}

poll();
setInterval(poll, POLL_MS);
setInterval(() => {
  if (!document.getElementById('settings-firmware').hidden) refreshOta();
}, 2000);
</script>
</body>
</html>

)rawliteral";
