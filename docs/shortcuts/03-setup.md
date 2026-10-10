# Atajo: Setup

Enciende la zona **setup** con `on` y el **Nivel on** (`defaultOnLevel`) del ESP32.

## Nombre sugerido del atajo

`Setup`

## Frase de Siri (ejemplo)

«Enciende el setup» o «Setup»

## Petición

| Campo | Valor |
|-------|--------|
| URL | `BASE_URL/api/command` |
| Método | POST |
| Encabezado | `Content-Type: application/json` |

```json
{"action":"on","zone":"setup"}
```

## Resultado esperado

- HTTP **200**, `{"ok":true}`.
- Setup en fade hacia `defaultOnLevel`; cuna sin cambios.

## Pasos en Atajos

Igual que [Cuna](01-cuna.md), sustituyendo el cuerpo JSON por el de arriba.
