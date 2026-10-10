# Atajo: Luz nocturna

Escena: **cuna al 20 %**, **setup apagado**. Usa `level` (no cambia el Nivel on guardado) y `off`. **Dos** peticiones POST en serie.

## Nombre sugerido del atajo

`Luz nocturna`

## Frase de Siri (ejemplo)

«Luz nocturna»

## Orden de peticiones (importante)

Ejecuta **de arriba abajo** en Atajos:

1. Apagar setup (evita dejar setup encendido si la segunda petición falla).
2. Poner cuna al 20 %.

| Paso | Cuerpo JSON |
|------|-------------|
| 1 | `{"action":"off","zone":"setup"}` |
| 2 | `{"action":"level","zone":"cuna","value":20}` |

Misma URL y encabezados que el resto: `POST BASE_URL/api/command`, `Content-Type: application/json`.

## Resultado esperado

- Ambas respuestas **200** y `{"ok":true}`.
- Setup apagado (fade según `fadeOffMs`).
- Cuna al 20 % de brillo lineal (sin alterar `defaultOnLevel` en NVS).

## Pasos en la app Atajos

1. **Texto** con `BASE_URL` (ejemplo: `http://192.168.1.100`).
2. **Obtener contenido de URL** (petición 1) → URL combinada + `/api/command`, POST, JSON paso 1.
3. **Obtener contenido de URL** (petición 2) → misma URL, POST, JSON paso 2.
4. Opcional: entre medias, **Si** el código de respuesta del paso 1 ≠ 200, **Mostrar alerta** y detener atajo.
5. **Añadir a Siri**.

## Errores

- Si solo falla el paso 2: setup ya apagado, cuna sin cambiar al 20 %.
- `value` fuera de 0–100 → **400** `invalid level`.
