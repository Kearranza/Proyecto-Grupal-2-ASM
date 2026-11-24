// Banco de frenado: medicion de velocidad, calibracion del freno y pruebas a lazo abierto por serial
#include <Servo.h>

const byte PIN_SENSOR = 2, PIN_ENA = 5, PIN_IN1 = 7, PIN_IN2 = 8, PIN_SERVO = 9;
const int VENTANAS = 24;
const unsigned long DEB_US = 1500;
const int PWM_MAX = 220;
const int U_LIBRE = 544, U_MAX = 1200;
const int U_CAL_INICIO = 640, U_CAL_PASO = 2;

Servo freno;

volatile byte estado = LOW;
volatile unsigned long tCambio = 0, tUltimo = 0, sumaDt = 0;
volatile unsigned int nDt = 0;
float rpm = 0, rpmF = 0, alfa = 0.3;

unsigned long tsMs = 20, tSig = 0, tick = 0;
int pwm = 0, uServo = U_LIBRE;

enum Modo { REPOSO, CALIBRAR, ESCALON };
Modo modo = REPOSO;
unsigned long tickModo = 0;

int uContacto = 0;
float w0 = 0, sumaW = 0;
int nW = 0, faseCal = 0;

int uAntes = U_LIBRE, uDespues = U_LIBRE;
unsigned long ticksPre = 0, ticksPost = 0;

// acepta un cambio solo si paso DEB_US desde el anterior; acumula el periodo entre ventanas
void actualizar(byte nivel, unsigned long t) {
  if (nivel != estado && t - tCambio > DEB_US) {
    estado = nivel;
    tCambio = t;
    if (nivel == HIGH) {
      if (tUltimo != 0) { sumaDt += t - tUltimo; nDt++; }
      tUltimo = t;
    }
  }
}

void isrSensor() { actualizar(digitalRead(PIN_SENSOR), micros()); }

void resincronizar() {
  noInterrupts();
  actualizar(digitalRead(PIN_SENSOR), micros());
  interrupts();
}

// velocidad por periodo promedio desde la ultima muestra; 0 si no pasa una ventana en 300 ms
void medir() {
  noInterrupts();
  unsigned long s = sumaDt, tu = tUltimo;
  unsigned int n = nDt;
  sumaDt = 0;
  nDt = 0;
  bool parado = (n == 0 && micros() - tu > 300000UL);
  if (parado) tUltimo = 0;
  interrupts();
  if (n > 0) rpm = 60.0e6 * n / (VENTANAS * (float)s);
  else if (parado) rpm = 0;
  rpmF = alfa * rpm + (1 - alfa) * rpmF;
}

void ponerMotor(int p) {
  pwm = constrain(p, 0, PWM_MAX);
  analogWrite(PIN_ENA, pwm);
}

void ponerServo(int us) {
  uServo = constrain(us, U_LIBRE, U_MAX);
  freno.writeMicroseconds(uServo);
}

void detener() {
  ponerMotor(0);
  ponerServo(U_LIBRE);
  modo = REPOSO;
  Serial.println(F("# detenido"));
}

void registrar() {
  Serial.print((tick - tickModo) * tsMs / 1000.0, 3); Serial.print(',');
  Serial.print(uServo); Serial.print(',');
  Serial.print(rpm, 1); Serial.print(',');
  Serial.println(rpmF, 1);
}

// estabiliza 2 s, mide w0 y sube u hasta que la velocidad cae 3 % (contacto) y luego al 50 %
void pasoCalibrar() {
  unsigned long k = tick - tickModo;
  unsigned long kVentana = 300 / tsMs;
  if (faseCal == 0) {
    if (k >= 1000 / tsMs) { sumaW += rpm; nW++; }
    if (k >= 2000 / tsMs) {
      w0 = sumaW / nW;
      sumaW = 0; nW = 0; faseCal = 1; tickModo = tick;
      Serial.print(F("CAL_W0,")); Serial.println(w0, 1);
    }
    return;
  }
  if (k > kVentana / 3) { sumaW += rpm; nW++; }
  if (k >= kVentana) {
    float w = sumaW / nW;
    Serial.print(F("MAP,")); Serial.print(uServo); Serial.print(','); Serial.println(w, 1);
    if (uContacto == 0 && w < 0.97 * w0) uContacto = uServo;
    if (w < 0.5 * w0 || uServo >= U_MAX) {
      Serial.print(F("CAL_FIN,")); Serial.print(uContacto); Serial.print(','); Serial.println(w0, 1);
      ponerServo(U_LIBRE);
      modo = REPOSO;
      return;
    }
    ponerServo(uServo + U_CAL_PASO);
    sumaW = 0; nW = 0; tickModo = tick;
  }
}

void pasoEscalon() {
  unsigned long k = tick - tickModo;
  if (k == ticksPre) ponerServo(uDespues);
  registrar();
  if (k >= ticksPre + ticksPost) {
    ponerServo(uAntes);
    modo = REPOSO;
    Serial.println(F("FIN"));
  }
}

void ejecutar(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  cmd.replace(" ", "");
  if (cmd.length() == 0) return;
  char c = cmd[0];
  int v = cmd.substring(1).toInt();
  if (c == 'x') { detener(); return; }
  if (modo != REPOSO) { Serial.println(F("# ocupado, usar x para detener")); return; }
  if (c == 'm') ponerMotor(v);
  else if (c == 'u') ponerServo(v);
  else if (c == 't') tsMs = constrain(v, 5, 200);
  else if (c == 'f') alfa = constrain(v, 1, 100) / 100.0;
  else if (c == 'c') {
    if (pwm == 0) { Serial.println(F("# encender el motor antes de calibrar")); return; }
    uContacto = 0; faseCal = 0; sumaW = 0; nW = 0;
    ponerServo(U_CAL_INICIO);
    modo = CALIBRAR; tickModo = tick;
    Serial.println(F("CAL_INICIO"));
  }
  else if (c == 'e') {
    uAntes = uServo; uDespues = v;
    ticksPre = 1000 / tsMs; ticksPost = 6000 / tsMs;
    modo = ESCALON; tickModo = tick;
    Serial.println(F("INICIO"));
    Serial.println(F("t_s,u_us,rpm,rpm_f"));
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  ponerMotor(0);
  freno.attach(PIN_SERVO);
  ponerServo(U_LIBRE);
  pinMode(PIN_SENSOR, INPUT);
  estado = digitalRead(PIN_SENSOR);
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR), isrSensor, CHANGE);
  tSig = millis() + tsMs;
  Serial.println(F("# banco listo: m<pwm> u<us> c e<u> t<ms> f<alfa%> x"));
}

void loop() {
  if (Serial.available()) ejecutar(Serial.readStringUntil('\n'));
  resincronizar();

  if ((long)(millis() - tSig) < 0) return;
  tSig += tsMs;
  tick++;
  medir();

  if (modo == CALIBRAR) pasoCalibrar();
  else if (modo == ESCALON) pasoEscalon();
  else if (tick % (500 / tsMs) == 0) {
    Serial.print(F("# pwm=")); Serial.print(pwm);
    Serial.print(F(" u=")); Serial.print(uServo);
    Serial.print(F(" rpm=")); Serial.println(rpmF, 1);
  }
}
