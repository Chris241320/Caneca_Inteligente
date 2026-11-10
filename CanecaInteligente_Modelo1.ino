/*
  ============================================================================
  CANECA INTELIGENTE - MODELO 1 (caja ya construida)
  SMART TRASH BIN   - MODEL 1 (existing box)
  Tecnología y Robótica · Grado 7.° · Ciclo 4
  ============================================================================

  ¿QUÉ HACE? / WHAT DOES IT DO?
  La tapa de la caja se abre sola cuando una mano se acerca y se cierra
  cuando la mano se aleja. Si la caneca está llena o hay un error, se
  enciende una alerta (LED + zumbador).
  The lid opens by itself when a hand gets close and closes when the hand
  moves away. If the bin is full or there is an error, an alert turns on.

  ENTRADA-PROCESO-COMUNICACIÓN-SALIDA / INPUT-PROCESS-COMMUNICATION-OUTPUT
    Entrada       / Input         : sensor ultrasónico HC-SR04 (detecta la mano)
    Comunicación  / Communication : señal del sensor (pin ECHO) -> Arduino
    Procesamiento / Processing    : Arduino comprueba las reglas AND, OR, NOT
    Salida        / Output        : servomotor mueve la tapa; LED y zumbador alertan

  REGLAS / RULES (sección 6 de la guía)
    1) IF hand_detected AND lid_closed  THEN open_lid
    2) IF NOT hand_detected             THEN keep_lid_closed
    3) IF bin_full OR system_error      THEN show_alert

  CONEXIONES / WIRING (Arduino UNO)
    Sensor de mano (HC-SR04)  TRIG -> pin 9    ECHO -> pin 10   VCC -> 5V  GND -> GND
    Servomotor (SG90)         Señal -> pin 6   Rojo -> 5V       Café -> GND
    LED rojo de alerta        pin 4 -> resistencia 220 ohm -> LED -> GND
    Zumbador (buzzer)         pin 5 -> buzzer (+) ; buzzer (-) -> GND
    OPCIONAL: sensor de llenado (2.º HC-SR04, dentro de la caja, mirando hacia abajo)
                              TRIG -> pin 11   ECHO -> pin 12

  SIN SEGUNDO SENSOR / WITHOUT A SECOND SENSOR
  Deja USAR_SENSOR_LLENADO en false. Así el estado "llena" es hipotético y
  se simula desde el Monitor Serie (9600 baudios):
    escribe  L  -> cambia caneca llena / no llena   (toggle bin full)
    escribe  E  -> cambia error / sin error         (toggle system error)

  Funciona en Arduino real y en Tinkercad Circuits (ciclo 5).
  Works on a real Arduino and in Tinkercad Circuits (cycle 5).
  ============================================================================
*/

#include <Servo.h>

// ---------------- CONFIGURACIÓN / SETTINGS ----------------
const bool USAR_SENSOR_LLENADO = false;   // true si instalas el 2.º sensor

// Pines / Pins
const int PIN_TRIG_MANO     = 9;
const int PIN_ECHO_MANO     = 10;
const int PIN_TRIG_LLENADO  = 11;
const int PIN_ECHO_LLENADO  = 12;
const int PIN_SERVO         = 6;
const int PIN_LED_ALERTA    = 4;
const int PIN_BUZZER        = 5;

// Ajustes de la maqueta / Model adjustments
const int DISTANCIA_MANO_CM    = 20;   // mano "cerca" si está a menos de 20 cm
const int DISTANCIA_LLENA_CM   = 8;    // basura a menos de 8 cm del sensor = llena
const int ANGULO_TAPA_CERRADA  = 0;    // grados del servo con la tapa cerrada
const int ANGULO_TAPA_ABIERTA  = 90;   // grados del servo con la tapa abierta
const unsigned long TIEMPO_ABIERTA_MS = 3000;  // espera 3 s antes de cerrar

// ---------------- ESTADOS / STATES ----------------
Servo servoTapa;

bool manoDetectada = false;   // hand_detected
bool tapaCerrada   = true;    // lid_closed
bool canecaLlena   = false;   // bin_full
bool errorSistema  = false;   // system_error

unsigned long ultimaVezMano = 0;  // último momento en que se vio la mano


// Mide distancia en cm con un HC-SR04. Devuelve -1 si no hay respuesta.
// Measures distance in cm. Returns -1 if the sensor does not answer.
long medirDistanciaCm(int pinTrig, int pinEcho) {
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);         // pulso de 10 microsegundos
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);

  long duracion = pulseIn(pinEcho, HIGH, 30000);  // máx. 30 ms de espera
  if (duracion == 0) {
    return -1;                         // sin eco = posible error
  }
  return duracion / 58;                // microsegundos -> centímetros
}


void abrirTapa() {
  servoTapa.write(ANGULO_TAPA_ABIERTA);
  tapaCerrada = false;
  Serial.println(">> Tapa ABIERTA / Lid OPEN");
}

void cerrarTapa() {
  servoTapa.write(ANGULO_TAPA_CERRADA);
  tapaCerrada = true;
  Serial.println(">> Tapa CERRADA / Lid CLOSED");
}

void mostrarAlerta(bool activa) {
  if (activa) {
    digitalWrite(PIN_LED_ALERTA, HIGH);
    tone(PIN_BUZZER, 1000, 100);       // pitido corto
  } else {
    digitalWrite(PIN_LED_ALERTA, LOW);
    noTone(PIN_BUZZER);
  }
}


// Lee órdenes del Monitor Serie para simular estados hipotéticos.
// Reads Serial Monitor commands to simulate hypothetical states.
bool errorSimulado = false;

void leerSimulacion() {
  while (Serial.available() > 0) {
    char letra = Serial.read();
    if (letra == 'L' || letra == 'l') {
      canecaLlena = !canecaLlena;
      Serial.print("Simulación: caneca llena = ");
      Serial.println(canecaLlena ? "SÍ" : "NO");
    }
    if (letra == 'E' || letra == 'e') {
      errorSimulado = !errorSimulado;
      Serial.print("Simulación: error = ");
      Serial.println(errorSimulado ? "SÍ" : "NO");
    }
  }
}


void setup() {
  Serial.begin(9600);

  pinMode(PIN_TRIG_MANO, OUTPUT);
  pinMode(PIN_ECHO_MANO, INPUT);
  pinMode(PIN_TRIG_LLENADO, OUTPUT);
  pinMode(PIN_ECHO_LLENADO, INPUT);
  pinMode(PIN_LED_ALERTA, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  servoTapa.attach(PIN_SERVO);
  cerrarTapa();                        // la caneca empieza cerrada

  Serial.println("Caneca Inteligente - Modelo 1 lista / ready");
  Serial.println("Escribe L (llena) o E (error) para simular.");
}


void loop() {
  // ---------- 1. ENTRADA: leer sensores / INPUT: read sensors ----------
  leerSimulacion();

  long distanciaMano = medirDistanciaCm(PIN_TRIG_MANO, PIN_ECHO_MANO);
  bool errorSensorMano = (distanciaMano == -1);
  manoDetectada = (!errorSensorMano) && (distanciaMano < DISTANCIA_MANO_CM);

  bool errorSensorLlenado = false;
  if (USAR_SENSOR_LLENADO) {
    long distanciaBasura = medirDistanciaCm(PIN_TRIG_LLENADO, PIN_ECHO_LLENADO);
    errorSensorLlenado = (distanciaBasura == -1);
    // Solo mide el llenado con la tapa cerrada (el sensor va en la tapa/arriba)
    if (tapaCerrada && !errorSensorLlenado) {
      canecaLlena = (distanciaBasura < DISTANCIA_LLENA_CM);
    }
  }

  // Nota: el HC-SR04 no responde si no hay nada al frente en 5 m; por eso
  // solo el sensor de llenado (que siempre ve el fondo) cuenta como error real.
  errorSistema = errorSensorLlenado || errorSimulado;

  if (manoDetectada) {
    ultimaVezMano = millis();
  }

  // ---------- 2. PROCESAMIENTO: reglas lógicas / PROCESSING: logic rules ----------

  // REGLA 1: IF hand_detected AND lid_closed THEN open_lid
  if (manoDetectada && tapaCerrada) {
    abrirTapa();
  }

  // REGLA 2: IF NOT hand_detected THEN keep_lid_closed
  // (espera unos segundos para que alcances a botar la basura)
  if (!manoDetectada && !tapaCerrada) {
    if (millis() - ultimaVezMano > TIEMPO_ABIERTA_MS) {
      cerrarTapa();
    }
  }

  // REGLA 3: IF bin_full OR system_error THEN show_alert
  if (canecaLlena || errorSistema) {
    mostrarAlerta(true);
  } else {
    mostrarAlerta(false);
  }

  // ---------- 3. COMUNICACIÓN: reporte en el Monitor Serie ----------
  Serial.print("Distancia mano: ");
  Serial.print(distanciaMano);
  Serial.print(" cm | mano=");
  Serial.print(manoDetectada ? "SÍ" : "NO");
  Serial.print(" | tapa=");
  Serial.print(tapaCerrada ? "cerrada" : "abierta");
  Serial.print(" | llena=");
  Serial.print(canecaLlena ? "SÍ" : "NO");
  Serial.print(" | error=");
  Serial.println(errorSistema ? "SÍ" : "NO");

  delay(200);   // repite 5 veces por segundo
}
