Implementa el control de iluminación de mi habitación mediante un ESP32.

CONTEXTO:
- Tengo 2 zonas independientes de iluminación:
  1. ZONA_CUNA
  2. ZONA_SETUP
- Ambas utilizan tiras LED convencionales de 24 V DC.
- Cada zona será controlada mediante un módulo MOSFET.
- El ESP32 NO alimentará directamente las tiras LED.
- El ESP32 solamente generará la señal PWM hacia el MOSFET.
- Las dos zonas deben poder controlarse de forma independiente.
- El MOSFET trabaja como low-side switch.
- La fuente de las tiras es externa de 24 V.
- GND de la fuente de 24 V y GND del ESP32 serán comunes.

OBJETIVO:
Crear un sistema de iluminación con:
- ON/OFF independiente por zona.
- Control de intensidad de 0–100 % mediante PWM.
- Fade suave al encender y apagar.
- Función "breathing" opcional para la zona de la cuna.
- Preparar la arquitectura para posteriormente agregar sensores/eventos, por ejemplo:
  "si la bebé llora → encender luz de la cuna al 20 %".

REQUISITOS DE SOFTWARE:
- Usar PWM hardware del ESP32.
- No usar delay() bloqueante para los fades.
- Utilizar millis() o una máquina de estados para realizar transiciones suaves.
- Mantener el loop principal no bloqueante.
- Crear una abstracción/clase o estructura para cada zona de iluminación.
- La intensidad debe manejarse como porcentaje 0–100 % y convertirse internamente al duty cycle PWM.
- Evitar valores mágicos dispersos.
- Definir claramente:
    GPIO_LIGHT_CUNA
    GPIO_LIGHT_SETUP
    PWM_FREQUENCY
    PWM_RESOLUTION
    FADE_TIME
- Usar una frecuencia PWM apropiada para evitar parpadeo visible.
- Considerar que el ESP32 trabaja con lógica de 3.3 V y que el módulo MOSFET debe ser compatible con ese nivel lógico. No asumir compatibilidad eléctrica sin verificar el módulo utilizado.

FUNCIONES MÍNIMAS:

setLightLevel(zone, percent)
setLightOn(zone)
setLightOff(zone)
fadeLight(zone, targetPercent, durationMs)
toggleLight(zone)

Además:

setBothLights(level)
allLightsOff()
allLightsOn()

COMPORTAMIENTO:
- setLightLevel() debe permitir valores de 0 a 100 %.
- Limitar valores fuera del rango.
- setLightOn() debe restaurar el último nivel configurado.
- setLightOff() debe apagar la zona pero conservar el último nivel.
- fadeLight() debe realizar una transición suave y no bloqueante.
- Si se solicita un nuevo fade mientras otro está ejecutándose, debe cancelar/reemplazar correctamente el anterior.
- No utilizar delay().

EJEMPLO DE USO:

setLightLevel(ZONE_CUNA, 20);
setLightLevel(ZONE_SETUP, 70);

fadeLight(ZONE_CUNA, 0, 1500);

fadeLight(ZONE_SETUP, 100, 1000);

REQUISITO FUTURO:
Dejar la arquitectura preparada para poder agregar posteriormente eventos como:

if (babyCrying) {
    fadeLight(ZONE_CUNA, 20, 500);
}

No implementes todavía detección de llanto, micrófono ni sensores. Solo deja la interfaz preparada.

IMPORTANTE:
Antes de modificar código existente:
1. Revisa la arquitectura actual del proyecto.
2. Identifica cómo se manejan actualmente GPIO, timers, PWM y tareas.
3. Reutiliza las estructuras existentes cuando tenga sentido.
4. No dupliques lógica.
5. No cambies módulos no relacionados.
6. Si existe una implementación actual de iluminación, refactorízala en lugar de crear otra paralela.

Al terminar:
- Indica qué archivos modificaste.
- Explica brevemente la arquitectura implementada.
- Indica qué GPIO quedaron asignados.
- Indica frecuencia y resolución PWM utilizadas.
- Señala cualquier problema eléctrico que deba verificarse antes de conectar las tiras de 24 V.
- No inventes que el módulo MOSFET es compatible con 3.3 V: si no puedes determinarlo por el hardware/documentación disponible, indícalo como punto pendiente de validación.