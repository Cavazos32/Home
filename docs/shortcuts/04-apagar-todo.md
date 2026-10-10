# Atajo: Apagar todo

Apaga **cuna** y **setup** en una sola petición (`zone: both`).

## Nombre sugerido del atajo

`Apagar todo`

## Frase de Siri (ejemplo)

«Apaga las luces» o «Apagar todo»

## Petición

```json
{"action":"off","zone":"both"}
```

`POST BASE_URL/api/command`, `Content-Type: application/json`.

## Resultado esperado

- HTTP **200**, `{"ok":true}`.
- Ambas zonas en fade a apagado (`allLightsOff()` en firmware).

## Pasos en Atajos

Una sola acción **Obtener contenido de URL** con el JSON anterior (misma plantilla de URL base que el resto).
