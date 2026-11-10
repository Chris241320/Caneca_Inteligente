# Caneca Inteligente · Smart Trash Bin

Tecnología y Robótica · Grado 7.° · Ciclo 4 — **Modelo 1: caja ya construida**

La tapa se abre sola cuando una mano se acerca y se cierra cuando se aleja.
Si la caneca está llena o hay un error, se enciende una alerta.
*The lid opens when a hand gets close and closes when it moves away. If the bin is full or there is an error, an alert turns on.*

## Reglas lógicas / Logic rules

1. `IF hand_detected AND lid_closed THEN open_lid`
2. `IF NOT hand_detected THEN keep_lid_closed`
3. `IF bin_full OR system_error THEN show_alert`

## Conexiones / Wiring (Arduino UNO)

| Componente / Component | Pin |
|---|---|
| Sensor de mano HC-SR04 (TRIG / ECHO) | 9 / 10 |
| Servomotor SG90 (señal / signal) | 6 |
| LED rojo de alerta (+ resistencia 220 Ω) | 4 |
| Zumbador / Buzzer | 5 |
| Sensor de llenado opcional (TRIG / ECHO) | 11 / 12 |

## Simulación / Simulation

Sin segundo sensor, deja `USAR_SENSOR_LLENADO = false` y abre el Monitor Serie (9600 baudios):

- `L` → caneca llena / no llena (bin full)
- `E` → error / sin error (system error)

Funciona en Arduino real y en Tinkercad Circuits.
