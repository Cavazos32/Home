# Atajo: Cuna

Enciende la zona **cuna** con la acción `on`, usando el **Nivel on** (`defaultOnLevel`) configurado en el ESP32 (panel web → Ajustes). No modifica ajustes en flash.

## Nombre sugerido del atajo

`Cuna`

## Frase de Siri (ejemplo)

«Enciende la cuna» o «Cuna»

## Petición

| Campo | Valor |
|-------|--------|
| URL | `BASE_URL/api/command` |
| Método | POST |
| Encabezado | `Content-Type: application/json` |
| Cuerpo | ver abajo |

`BASE_URL` = tu URL base, **sin** `/` final (ejemplo ilustrativo: `http://192.168.1.100`).

```json
{"action":"on","zone":"cuna"}
```

## Resultado esperado

- HTTP **200**, cuerpo: `{"ok":true}`.
- La cuna hace fade hacia `defaultOnLevel` (ver `GET /api/state` → `settings.defaultOnLevel`).
- Setup no cambia.

## Pasos en la app Atajos

1. **Texto** → `http://TU_IP` (sustituye; no uses `192.168.1.100` a ciegas).
2. **Obtener contenido de URL** → URL: **Combinar texto** `[Texto]` + `/api/command`.
3. Método **POST**, encabezado `Content-Type` = `application/json`, cuerpo **JSON** de arriba.
4. **Añadir a Siri** con la frase elegida.

## Errores

- IP incorrecta: error de conexión en Atajos.
- JSON mal escrito: respuesta **400** con `{"error":"..."}`.
