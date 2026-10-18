# Atajos de iOS y Siri para HomeLights

Integración con la app **Atajos** (Apple Shortcuts) y **Siri** usando la API HTTP del ESP32 en la red Wi‑Fi local. El flujo recomendado es **leer estado → mostrar al usuario → ejecutar `toggle`** (el firmware invierte encendido/apagado como en el panel web).

## Requisitos

- iPhone/iPad en la **misma Wi‑Fi** que el ESP32.
- IP o nombre mDNS del dispositivo (ver [Dirección del ESP32](#dirección-del-esp32)).
- Atajos de Apple (app preinstalada).

## Dirección del ESP32

Sustituye `BASE_URL` en toda la documentación por la URL base **real** de tu placa, **sin** barra final:

| Origen | Ejemplo (solo ilustración) |
|--------|----------------------------|
| Monitor serial al arrancar | `http://192.168.1.100` |
| mDNS (`config.h` → `MDNS_HOSTNAME`) | `http://habitacion.local` |

> **Ejemplo de IP:** `192.168.1.100` es ficticio. Usa la IP que imprima el ESP32 o la reserva DHCP de tu router.

## Flujo en Atajos (toggle + estado)

Cada atajo de control debería hacer **dos** peticiones HTTP en este orden:

1. **GET** `BASE_URL/api/state` — leer cómo están las luces **antes** de actuar.
2. **Mostrar** al usuario el estado relevante (alerta, notificación o resultado del atajo).
3. **POST** `BASE_URL/api/command` con `Content-Type: application/json` y cuerpo `{"action":"toggle","zone":"…"}`.

El ESP32 no cambia la semántica de `toggle`: si la zona está encendida (`on: true`), pasa a apagada; si está apagada, enciende con fade hacia el **Nivel on** de ajustes (`defaultOnLevel`).

### Campos útiles en `GET /api/state`

Fragmento típico (cada zona tiene la misma forma):

```json
{
  "cuna": {
    "level": 45,
    "levelFine": 45.2,
    "on": true,
    "lit": true,
    "transitioning": false,
    "phase": "steady"
  },
  "setup": { "on": false, "level": 0, "...": "..." }
}
```

Para mostrar en Atajos (acción **Obtener valor del diccionario** sobre el JSON de la respuesta):

| Ruta | Uso en Siri / alerta |
|------|----------------------|
| `cuna.on` | ¿Cuna encendida? (booleano) |
| `cuna.levelFine` o `cuna.level` | Brillo actual (0–100) |
| `setup.on` | ¿Setup encendido? |
| `setup.levelFine` o `setup.level` | Brillo setup |

Opcional: `clock`, `routine.active`, `alternate.enabled` si quieres avisar de rutina o modo alternado (un `toggle` manual los cancela en el firmware).

### Estado directo de rutinas

`GET /api/state` también expone una clave booleana por cada rutina configurada, usando su `id` como nombre de clave. No necesitas recorrer `routines[]` en Atajos:

```json
{
  "baby": true,
  "habitacion": false
}
```

- `baby: true`: Baby es la rutina activa.
- `baby: false`: Baby no es la rutina activa (puede estar activa otra rutina, o ninguna).

Se mantiene `routines[]`, `routine` y `group` como antes, para no romper el panel web ni clientes existentes.

Respuesta correcta del **POST** toggle: HTTP **200** y `{"ok":true}`.

Errores habituales (cuerpo JSON con `"error"`):

| Código | Causa típica |
|--------|----------------|
| 400 | JSON inválido, zona desconocida |
| 503 | Sin Wi‑Fi en el ESP32 (p. ej. `/api/ota/check`; no aplica a `/api/command` en uso normal) |

Si la IP es incorrecta o el ESP32 está apagado, Atajos mostrará error de red (sin JSON).

## Atajos sugeridos (solo `toggle`)

Sustituye `BASE_URL` por tu URL (p. ej. `http://192.168.1.100`).

| Atajo (nombre sugerido) | Frase Siri (ejemplo) | GET | POST (después de mostrar estado) |
|-------------------------|----------------------|-----|----------------------------------|
| Cuna | «Cuna» | `/api/state` → mostrar `cuna` | `{"action":"toggle","zone":"cuna"}` |
| Setup | «Setup» | `/api/state` → mostrar `setup` | `{"action":"toggle","zone":"setup"}` |
| Luces habitación | «Luces habitación» | `/api/state` → mostrar ambas zonas | `{"action":"toggle","zone":"both"}` |
| Estado luces | «¿Cómo están las luces?» | `/api/state` → mostrar y **fin** (sin POST) | — |

**`zone":"both"`** con `toggle` aplica **toggle en cuna y en setup por separado** (no es un único interruptor “todo”). Si quieres control independiente, usa atajos «Cuna» y «Setup».

### Grupos / rutinas (opcional)

Si usas escenas **Habitación** o **Baby**, puedes mantener solo **`enabled":"toggle"`** (sin `true`/`false`):

```json
{"action":"group","group":"baby","enabled":"toggle"}
```

Mismo patrón: **GET** `/api/state` → leer directamente `baby` o `habitacion` (booleanos) → mostrar la acción prevista → **POST** el JSON anterior. Las claves directas reflejan qué rutina está activa en el controlador; se conserva `routine` / `routines[]` para compatibilidad.

## Comportamiento respecto al firmware

Verificado en `HomeLights.ino` → `handleCommand`:

- **`action":"toggle"`** invierte cada zona (`toggleLight`: apaga si hay luz o fade hacia encendido; si no, `on` con fade al nivel por defecto). Con **`both`**, aplica eso en cuna y setup por separado.
- Tras el toggle, **`on`** en el siguiente `GET /api/state` refleja el nuevo estado (puede haber fade: `transitioning: true` unos instantes).
- Cada comando desactiva el modo alternado y cancela la rutina activa, igual que un toque manual en el panel.

Las acciones **`on`** y **`off`** siguen existiendo en la API para el panel web y otras integraciones; **estos atajos no las usan**.

## Configuración paso a paso en iPhone

1. Atajos → **+** → nombre (p. ej. «Cuna»).
2. Acción **Texto** con tu URL base (ej. `http://192.168.1.100`).
3. **Obtener contenido de URL** — URL: `[Texto]` + `/api/state`, método **GET**.
4. **Obtener valor del diccionario** — clave `cuna`, subclave `on` / `levelFine` (repite o formatea texto para **Mostrar alerta** / **Mostrar notificación**).
5. Segunda **Obtener contenido de URL** — URL: `[Texto]` + `/api/command`, método **POST**, encabezado `Content-Type` → `application/json`, cuerpo JSON del toggle.
6. (Opcional) **Obtener detalles de contenido de URL** → **Código de respuesta** = 200 en el POST.
7. **Ajustes del atajo** → **Añadir a Siri**.

Duplica el atajo para Setup cambiando zona y textos del paso 4.

## Reservar IP (DHCP) en el router

Para que la IP no cambie y no tengas que editar todos los atajos:

1. En el monitor serial del ESP32 anota la **MAC** Wi‑Fi (o mírala en la lista de clientes del router).
2. Entra en la administración del router (suele ser `192.168.1.1` o similar).
3. Busca **DHCP**, **Reserva de direcciones**, **Address reservation** o **IP estática por MAC**.
4. Asocia la MAC del ESP32 a una IP libre (ejemplo: `192.168.1.100`).
5. Guarda y reinicia el ESP32; comprueba en serial que recibe esa IP.

Alternativa: usar `http://habitacion.local` si mDNS funciona bien en tu red iOS (a veces es menos fiable que IP reservada).

## Siri

- En cada atajo: **Ajustes del atajo** → **Añadir a Siri** → frase corta y distinta por escena.
- El atajo **Estado luces** encaja bien con preguntas («¿Cómo están las luces?») sin cambiar nada.
- Siri ejecuta el atajo en el iPhone; el teléfono debe estar en la **misma LAN**.

## Compartir con otro iPhone (p. ej. pareja)

1. En tu iPhone: abre el atajo → **Compartir** → **AirDrop** al otro dispositivo, o enlace iCloud si sincronizas Atajos.
2. En el otro iPhone: acepta el atajo; **edita** la acción Texto/URL y confirma `BASE_URL`.
3. El segundo usuario debe añadir **su** frase de Siri en **Añadir a Siri**.

## Seguridad

- La API usa **HTTP sin autenticación** en el puerto **80** (solo red local; CORS `*` para el panel web).
- Usa estos atajos **solo en Wi‑Fi doméstica de confianza**.
- **No** hagas port forwarding del puerto 80 del ESP32 a Internet.
- **No** pongas contraseñas Wi‑Fi ni `OTA_PASSWORD` en los atajos.

## Pruebas automatizadas (formato de peticiones)

En la raíz del repo:

```bash
python3 tools/test_shortcut_requests.py
```

Opcional, contra un ESP32 en la red:

```bash
HOMELIGHTS_BASE_URL=http://192.168.x.x python3 tools/test_shortcut_requests.py --live
```

`--live` comprueba **GET /api/state** (estructura de zonas) y que un **POST toggle** devuelve 200 (**cambia el estado** de la zona indicada; por defecto `cuna`).

## Limitaciones conocidas

- Sin acceso remoto fuera de la LAN.
- Durante fades, `on` y `level` pueden no coincidir con lo que ves aún en la habitación; puedes mostrar `transitioning` o esperar un segundo y volver a leer estado.
- mDNS puede fallar en algunas redes; preferir IP reservada.
- Escenas con brillo fijo (p. ej. «luz nocturna al 20 %») requieren `level` u otra acción; con **solo toggle** no se modelan en un único POST.

## Referencia API (Atajos)

| Método | Ruta | Uso en Atajos |
|--------|------|----------------|
| GET | `/api/state` | Leer estado antes de mostrar / decidir |
| POST | `/api/command` | `{"action":"toggle","zone":"cuna"\|"setup"\|"both"}` |

Otras acciones en `/api/command` (`on`, `off`, `level`, `fade`, `group`, …) y rutas (`/api/settings`, horarios, OTA) están pensadas para el panel web o automatización avanzada, no para los atajos mínimos de Siri descritos aquí.
