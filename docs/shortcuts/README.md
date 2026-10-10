# Atajos de iOS y Siri para HomeLights

Integración con la app **Atajos** (Apple Shortcuts) y **Siri** usando la API HTTP que ya expone el ESP32 en la red Wi‑Fi local. No se añade firmware ni endpoints nuevos.

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

Endpoint común a todos los atajos:

```text
POST BASE_URL/api/command
Content-Type: application/json
```

Respuesta correcta: HTTP **200** y cuerpo `{"ok":true}`.

Errores habituales (cuerpo JSON con `"error"`):

| Código | Causa típica |
|--------|----------------|
| 400 | JSON inválido, zona/acción desconocida, nivel fuera de 0–100 |
| 503 | Sin Wi‑Fi en el ESP32 (p. ej. `/api/ota/check`; no aplica a `/api/command` en uso normal) |

Si la IP es incorrecta o el ESP32 está apagado, Atajos mostrará error de red (sin JSON).

## Tabla resumen de peticiones

Sustituye `BASE_URL` por tu URL (p. ej. `http://192.168.1.100`).

| Atajo (nombre sugerido) | Frase Siri (ejemplo) | Peticiones (orden) |
|-------------------------|----------------------|--------------------|
| [Cuna](01-cuna.md) | «Cuna» o «Enciende la cuna» | 1× `on` / `cuna` |
| [Luz nocturna](02-luz-nocturna.md) | «Luz nocturna» | 1× `off` / `setup`, luego 1× `level` / `cuna` / `20` |
| [Setup](03-setup.md) | «Setup» o «Enciende el setup» | 1× `on` / `setup` |
| [Apagar todo](04-apagar-todo.md) | «Apaga las luces» | 1× `off` / `both` |
| [Encender todo](05-encender-todo.md) | «Enciende las luces» | 1× `on` / `both` |

Cuerpos JSON exactos:

```json
{"action":"on","zone":"cuna"}
{"action":"off","zone":"setup"}
{"action":"level","zone":"cuna","value":20}
{"action":"on","zone":"setup"}
{"action":"off","zone":"both"}
{"action":"on","zone":"both"}
```

(Luz nocturna usa la segunda y la tercera línea, en ese orden.)

## Comportamiento respecto al firmware

Verificado en `HomeLights.ino` → `handleCommand` y `LightZone.cpp`:

- **`action":"on"`** hace fade al **`defaultOnLevel`** guardado en ajustes (NVS), sin cambiar ese ajuste.
- **`action":"level"`** fija el brillo al instante (sin fade por defecto en `setLightLevel`); **no** modifica `defaultOnLevel`.
- **`action":"off"`** apaga con fade según `fadeOffMs`.
- **`zone":"both"`** con `on`/`off` llama a `allLightsOn()` / `allLightsOff()`.
- Cada comando desactiva el modo alternado (`alternate.setEnabled(false, …)`); horarios, OTA y el panel web siguen igual.

## Configuración rápida en iPhone

1. Abre **Atajos** → **+** → nombre (p. ej. «Cuna»).
2. Añade **Texto** con tu URL base (ejemplo: `http://192.168.1.100`) — opcional pero recomendable para cambiar la IP en un solo sitio.
3. Añade **Obtener contenido de URL**:
   - URL: texto anterior + `/api/command` (p. ej. con acción **Combinar texto**: `[Texto]` + `/api/command`).
   - Método: **POST**.
   - Encabezados: `Content-Type` → `application/json`.
   - Cuerpo de solicitud: **JSON** según la tabla (ver guías por atajo).
4. (Opcional) **Obtener detalles de contenido de URL** → **Código de respuesta** para comprobar 200 al depurar.
5. **Ajustes del atajo** (icono ⚙ en la esquina): activa **Mostrar en Compartir** si quieres; para Siri, **Añadir a Siri** y graba la frase.

Detalle paso a paso por escena: archivos `01-cuna.md` … `05-encender-todo.md`.

## Reservar IP (DHCP) en el router

Para que la IP no cambie y no tengas que editar todos los atajos:

1. En el monitor serial del ESP32 anota la **MAC** Wi‑Fi (o mírala en la lista de clientes del router).
2. Entra en la administración del router (suele ser `192.168.1.1` o similar).
3. Busca **DHCP**, **Reserva de direcciones**, **Address reservation** o **IP estática por MAC**.
4. Asocia la MAC del ESP32 a una IP libre (ejemplo: `192.168.1.100`).
5. Guarda y reinicia el ESP32; comprueba en serial que recibe esa IP.

Alternativa: usar `http://habitacion.local` si mDNS funciona bien en tu red iOS (a veces es menos fiable que IP reservada).

## Siri

- En cada atajo: **Ajustes del atajo** → **Añadir a Siri** → elige una frase corta y distinta por escena.
- Evita frases ambiguas entre atajos («luces» vs «luz nocturna»).
- Siri ejecuta el atajo en el iPhone; el teléfono debe estar en la **misma LAN**.

## Compartir con otro iPhone (p. ej. pareja)

1. En tu iPhone: abre el atajo → **Compartir** (icono compartir) → **AirDrop** al otro dispositivo, o **Copiar enlace** si Atajos ofrece enlace iCloud (solo válido si iCloud sincroniza Atajos en tu cuenta).
2. En el otro iPhone: acepta el atajo; **edita** la acción Texto/URL y confirma que `BASE_URL` es correcta (misma red de casa).
3. Repite para los cinco atajos o duplica una plantilla ya configurada.
4. El segundo usuario debe añadir **su** frase de Siri en **Añadir a Siri** (no se copia la voz de Siri entre usuarios).

No incluimos enlaces iCloud de importación generados aquí: no se han validado en dispositivos reales.

## Seguridad

- La API usa **HTTP sin autenticación** en el puerto **80** (solo red local; CORS `*` para el panel web).
- Usa estos atajos **solo en Wi‑Fi doméstica de confianza**.
- **No** hagas port forwarding del puerto 80 del ESP32 a Internet.
- **No** pongas contraseñas Wi‑Fi ni `OTA_PASSWORD` en los atajos.

## Importación automática de atajos

Apple permite compartir atajos como archivos `.shortcut` o enlaces iCloud creados manualmente desde la app. Este repositorio **no** incluye binarios importables: la configuración manual con **Obtener contenido de URL** es la vía soportada y reproducible.

## Pruebas automatizadas (formato de peticiones)

En la raíz del repo:

```bash
python3 tools/test_shortcut_requests.py
```

Opcional, contra un ESP32 en la red:

```bash
HOMELIGHTS_BASE_URL=http://192.168.x.x python3 tools/test_shortcut_requests.py --live
```

(`--live` solo comprueba conectividad y que `POST /api/command` devuelve 200; **no** sustituye prueba física de luces ni Siri.)

## Limitaciones conocidas

- Sin acceso remoto fuera de la LAN.
- Si el atajo **Luz nocturna** falla tras apagar setup pero antes de nivel cuna, la cuna queda como estaba; si falla después de encender cuna al 20 %, el setup ya estaría apagado (orden documentado: apagar setup **primero**).
- Durante fades largos, el estado intermedio puede no ser instantáneo.
- mDNS puede fallar en algunas redes; preferir IP reservada.

## Referencia API

| Método | Ruta | Descripción |
|--------|------|-------------|
| GET | `/api/state` | Estado completo (zonas, ajustes, horarios) |
| POST | `/api/command` | Control de luces (usado por atajos) |
| POST | `/api/settings` | Cambia ajustes persistentes (no usado por estos atajos) |

Acciones en `/api/command`: `on`, `off`, `level`, `fade`, `toggle`, `alternate`. Zonas: `cuna`, `setup`, `both`.
