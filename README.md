# Luces habitación (ESP32 + Arduino IDE)

Control web de dos zonas (dimmer PWM, encender/apagar, horarios NTP, modo alternado broma).  
Sketch en carpeta **`HomeLights/`** (ábrela con Arduino IDE).

## Requisitos

1. [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. Paquete de placas **esp32** (Espressif): *Gestor de tarjetas* → URL  
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`  
   → instalar **esp32** (versión 3.x recomendada).
3. Librería **ArduinoJson** (*Gestor de librerías* → buscar `ArduinoJson` by Benoit Blanchon).

## Placa y primera carga (USB)

- **Herramientas → Placa:** ESP32 Dev Module (o tu modelo).
- **Puerto:** COM del cable USB.
- Abre `HomeLights/HomeLights.ino` y pulsa **Subir**.

En **Monitor serial (115200)** verás la IP, por ejemplo:

`Panel web: http://192.168.x.x`

Abre esa URL en el navegador (misma WiFi), o `http://<IP>/index.html`.

### Interfaz en Chrome (archivo local)

Edita y abre `HomeLights/index.html` en Chrome. Indica la URL de la ESP32 (ej. `http://192.168.x.x`); la API usa CORS.  
Tras cambiar `index.html`, regenera el firmware con:

`python HomeLights/pack_index.py`

Credenciales WiFi y contraseña OTA: `HomeLights/config.h`.

## Subir código por WiFi (OTA)

Después de la **primera** carga por cable:

1. El ESP32 debe estar en la misma red WiFi.
2. Reinicia la IDE o espera unos segundos: en **Herramientas → Puerto** debe aparecer algo como  
   `habitacion en 192.168.x.x` (mDNS / OTA).
3. Selecciona ese puerto de red y pulsa **Subir** otra vez.
4. Te pedirá la contraseña OTA (`OTA_PASSWORD` en `config.h`).

Si no sale el puerto de red: comprueba firewall, misma subred, y que el monitor serial muestre `OTA listo`.

## Funciones

| Función | Descripción |
|--------|-------------|
| Dimmer | 0–100 % por zona (cuna / setup) |
| Encender / apagar | Restaura el último nivel al encender |
| Fade | Transición suave (API / botones web) |
| Horarios | Hasta 10 eventos; hora por NTP; guardado en flash |
| Modo alternado | Cuna ↔ setup rítmico (broma) |
| OTA | Actualizar firmware sin cable |

## GPIO

| Zona | GPIO |
|------|------|
| Cuna | 25 |
| Setup | 26 |

PWM: **5 kHz**, **10 bits**. MOSFET low-side, GND común con fuente 24 V.

## Zona horaria

Por defecto **UTC−6** (`config.h`). Ajusta `GMT_OFFSET_SEC` si tu ubicación usa otro huso.
