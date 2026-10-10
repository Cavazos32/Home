# Resumen: Atajos de iOS y Siri para HomeLights

Integración con **Atajos** y **Siri** usando la API HTTP existente del ESP32 en la red Wi‑Fi local. **No se modificó el firmware**; solo documentación y validación de formatos JSON.

Guía detallada: [README.md](README.md)

---

## Análisis de la API (firmware actual)

- **Endpoint:** `POST /api/command`
- **Encabezado:** `Content-Type: application/json`
- **Acciones:** `on`, `off`, `level`, `fade`, `toggle`, `alternate`
- **Zonas:** `cuna`, `setup`, `both`
- **`on`:** fade al **Nivel on** (`defaultOnLevel` en NVS); no cambia ajustes guardados.
- **`level`:** brillo inmediato; no modifica `defaultOnLevel`.
- **`both` + `on`/`off`:** enciende o apaga ambas zonas en una petición.
- **Seguridad:** HTTP en puerto 80 **sin autenticación**; uso solo en LAN de confianza.

---

## Archivos del repositorio

| Acción | Ruta |
|--------|------|
| Creado | `docs/shortcuts/README.md` |
| Creado | `docs/shortcuts/01-cuna.md` … `05-encender-todo.md` |
| Creado | `docs/shortcuts/RESUMEN.md` (este archivo) |
| Creado | `tools/test_shortcut_requests.py` |
| Modificado | `README.md` (enlace a la guía) |

---

## Los cinco atajos y peticiones exactas

Sustituye `BASE_URL` por la URL real del ESP32 **sin** barra final.  
**Ejemplo ilustrativo:** `http://192.168.1.100` (no es la IP real del dispositivo).

Todas las peticiones van a:

```text
POST BASE_URL/api/command
Content-Type: application/json
```

Respuesta correcta: HTTP **200** y cuerpo `{"ok":true}`.

| Atajo | Petición(es) JSON |
|-------|-------------------|
| **Cuna** | `{"action":"on","zone":"cuna"}` |
| **Luz nocturna** | 1) `{"action":"off","zone":"setup"}` → 2) `{"action":"level","zone":"cuna","value":20}` |
| **Setup** | `{"action":"on","zone":"setup"}` |
| **Apagar todo** | `{"action":"off","zone":"both"}` |
| **Encender todo** | `{"action":"on","zone":"both"}` |

**Luz nocturna:** ejecutar en ese orden (primero apagar setup, luego cuna al 20 %) para no dejar setup encendido si falla un paso.

---

## Configuración en iPhone

1. Abre **Atajos** → **+** → asigna el nombre del escena.
2. Añade **Texto** con tu `BASE_URL`.
3. Añade **Obtener contenido de URL**:
   - URL: texto + `/api/command`
   - Método: **POST**
   - Encabezado: `Content-Type` = `application/json`
   - Cuerpo: JSON de la tabla anterior
4. Para **Luz nocturna**, repite **Obtener contenido de URL** dos veces (orden indicado).
5. (Opcional) Comprueba el código HTTP 200 al depurar.

Instrucciones por escena: `01-cuna.md` … `05-encender-todo.md`.

---

## Frases de Siri

En cada atajo: **Ajustes del atajo** → **Añadir a Siri**.

| Atajo | Ejemplo de frase |
|-------|------------------|
| Cuna | «Enciende la cuna» / «Cuna» |
| Luz nocturna | «Luz nocturna» |
| Setup | «Enciende el setup» / «Setup» |
| Apagar todo | «Apaga las luces» |
| Encender todo | «Enciende las luces» |

---

## Compartir con otro iPhone (p. ej. pareja)

1. **Compartir** el atajo → **AirDrop** o enlace iCloud (si Atajos lo genera en tu cuenta).
2. En el otro iPhone: aceptar y **revisar** que `BASE_URL` sea correcta.
3. Repetir para los cinco atajos.
4. Cada persona configura **su** frase en **Añadir a Siri**.
5. Ambos dispositivos deben estar en la **misma Wi‑Fi** de casa.

Este repositorio **no** incluye enlaces iCloud de importación validados.

---

## IP fija en el router (recomendado)

1. Anota la MAC Wi‑Fi del ESP32 (serial o lista de clientes del router).
2. En el router: reserva DHCP / IP estática por MAC.
3. Asigna una IP libre (ejemplo: `192.168.1.100`).
4. Actualiza el texto `BASE_URL` en los atajos una sola vez.

Alternativa: `http://habitacion.local` (mDNS; a veces menos fiable en iOS).

---

## Seguridad

- Atajos solo para **red doméstica de confianza**.
- **No** reenviar puerto 80 del ESP32 a Internet.
- **No** incluir contraseñas Wi‑Fi ni OTA en los atajos.

---

## Limitaciones conocidas

- Sin acceso remoto fuera de la LAN.
- Sin integración Alexa, Home Assistant ni Matter en esta etapa.
- Durante fades, el cambio visual no es instantáneo.
- Si falla la segunda petición de Luz nocturna, setup ya estará apagado pero cuna puede no quedar al 20 %.

---

## Pruebas

| Prueba | Resultado |
|--------|-----------|
| `python3 tools/test_shortcut_requests.py` | Validación offline de los 6 pasos JSON: **OK** |
| IP/puerto incorrecto | Error de conexión (p. ej. conexión rechazada) |
| ESP32 físico / Siri en iPhone | **No probado** desde el entorno de desarrollo del repo |

Prueba opcional contra tu placa ( **altera luces** ):

```bash
HOMELIGHTS_BASE_URL=http://TU_IP python3 tools/test_shortcut_requests.py --live
```

---

## Referencia rápida de errores HTTP

| Código | Causa típica |
|--------|----------------|
| 200 | `{"ok":true}` |
| 400 | JSON inválido, zona/acción desconocida, nivel fuera de 0–100 |
| (red) | IP incorrecta, ESP32 apagado o iPhone en otra Wi‑Fi |
