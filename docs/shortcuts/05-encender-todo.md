# Atajo: Encender todo

Enciende **cuna** y **setup** con el Nivel on de cada zona en una petición (`zone: both`, `action: on`).

## Nombre sugerido del atajo

`Encender todo`

## Frase de Siri (ejemplo)

«Enciende las luces» o «Encender todo»

## Petición

```json
{"action":"on","zone":"both"}
```

`POST BASE_URL/api/command`, `Content-Type: application/json`.

## Resultado esperado

- HTTP **200**, `{"ok":true}`.
- Ambas zonas hacen fade a `defaultOnLevel` (`allLightsOn()`).

## Pasos en Atajos

Una sola acción **Obtener contenido de URL** con el JSON anterior.
