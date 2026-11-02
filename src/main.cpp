#include <Arduino.h>
#include <Servo.h>

// =================================================================
// CONFIGURAÇÕES DE PORTAS (PINOS)
// =================================================================

// Pinos dos Analógicos (2 Joysticks = 4 eixos)
const int PINO_JOY1_X = A0;   //esquerdo horizontal
const int PINO_JOY1_Y = A1;   //esquerdo vertical
const int PINO_JOY2_X = A2;   //direito horizontal
const int PINO_JOY2_Y = A3;   //direito vertical

// Pinos dos Botões (Opcional)
const int PINO_BTN1 = 2;
const int PINO_BTN2 = 3;

// Pinos dos Servos (Devem ser portas PWM)
const int PINO_SERVO1 = 11;   //base
const int PINO_SERVO2 = 7;    //junta 1
const int PINO_SERVO3 = 10;   //junta 2
const int PINO_SERVO4 = 9;    //garra

// =================================================================
// CONFIGURAÇÕES DE INVERSÃO DE CONTROLE
// =================================================================
// Se um servo estiver girando para o lado contrário do desejado, 
// altere de "false" para "true".

const bool INVERTE_JOY1_X = true;
const bool INVERTE_JOY1_Y = false;
const bool INVERTE_JOY2_X = true;
const bool INVERTE_JOY2_Y = false;

// =================================================================
// CONFIGURAÇÕES DE LIMITES DOS SERVOS (SOFT LIMITS)
// =================================================================
// Defina aqui até onde cada servo pode ir, protegendo sua mecânica.
// O valor mínimo normal é 0 e o máximo é 180.

const int MIN_SERVO1 = 50;
const int MAX_SERVO1 = 160;

const int MIN_SERVO2 = 0;
const int MAX_SERVO2 = 180;

const int MIN_SERVO3 = 50;
const int MAX_SERVO3 = 160;

const int MIN_SERVO4 = 54;
const int MAX_SERVO4 = 100;

// =================================================================
// CONFIGURAÇÕES DE COMPORTAMENTO
// =================================================================

const int ZONA_MORTA = 30; // Ignora variações no centro do joystick
const float VELOCIDADE_MAXIMA = 2.0; // Graus máximos adicionados por ciclo de atualização

const int INTERVALO_SERVO = 20; // Tempo de atualização do servo (em ms)
const int TEMPO_DEBUG = 250;    // Tempo de atualização do debug (em ms)

// =================================================================
// VARIÁVEIS DO SISTEMA
// =================================================================

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

// Variáveis de posição atual dos servos
float angulo_s1;
float angulo_s2;
float angulo_s3;
float angulo_s4;

unsigned long tempo_atual = 0;
unsigned long tempo_anterior_debug = 0;
unsigned long tempo_anterior_servo = 0;

// Função que calcula o incremento considerando a zona morta e a inversão
float calculaIncremento(int pinoAnalogico, bool invertido) {
  int valor_bruto = analogRead(pinoAnalogico);
  
  // Se estiver dentro da zona morta, o incremento é 0
  if (abs(valor_bruto - 512) <= ZONA_MORTA) {
    return 0.0;
  }
  
  // Transforma (0 a 1023) em uma taxa (-1.0 a +1.0)
  float taxa = (valor_bruto - 512.0) / 512.0; 
  float incremento = taxa * VELOCIDADE_MAXIMA;
  
  // Inverte o sinal caso configurado
  if (invertido) {
    incremento = -incremento;
  }
  
  return incremento;
}

void setup() {
  Serial.begin(115200); 

  pinMode(PINO_JOY1_X, INPUT);
  pinMode(PINO_JOY1_Y, INPUT);
  pinMode(PINO_JOY2_X, INPUT);
  pinMode(PINO_JOY2_Y, INPUT);
  
  pinMode(PINO_BTN1, INPUT_PULLUP);
  pinMode(PINO_BTN2, INPUT_PULLUP);

  servo1.attach(PINO_SERVO1);
  servo2.attach(PINO_SERVO2);
  servo3.attach(PINO_SERVO3);
  servo4.attach(PINO_SERVO4);
  
  // Calcula o ponto central baseado nos limites configurados para cada servo
  angulo_s1 = (MIN_SERVO1 + MAX_SERVO1) / 2.0;
  angulo_s2 = (MIN_SERVO2 + MAX_SERVO2) / 2.0;
  angulo_s3 = (MIN_SERVO3 + MAX_SERVO3) / 2.0;
  angulo_s4 = (MIN_SERVO4 + MAX_SERVO4) / 2.0;

  // Envia os servos para a posição inicial de descanso (centro do soft limit)
  servo1.write(angulo_s1);
  servo2.write(angulo_s2);
  servo3.write(angulo_s3);
  servo4.write(angulo_s4);
}

void loop() {
  tempo_atual = millis();

  // =================================================================
  // ROTINA DOS SERVOS
  // =================================================================
  if (tempo_atual - tempo_anterior_servo >= INTERVALO_SERVO) {
    tempo_anterior_servo = tempo_atual;

    // Calcula os incrementos passando a configuração de inversão
    angulo_s1 += calculaIncremento(PINO_JOY1_X, INVERTE_JOY1_X);
    angulo_s2 += calculaIncremento(PINO_JOY1_Y, INVERTE_JOY1_Y);
    angulo_s3 += calculaIncremento(PINO_JOY2_X, INVERTE_JOY2_X);
    angulo_s4 += calculaIncremento(PINO_JOY2_Y, INVERTE_JOY2_Y);

    // Aplica os Soft Limits (Impede os servos de passarem do Mínimo e Máximo configurados)
    angulo_s1 = constrain(angulo_s1, MIN_SERVO1, MAX_SERVO1);
    angulo_s2 = constrain(angulo_s2, MIN_SERVO2, MAX_SERVO2);
    angulo_s3 = constrain(angulo_s3, MIN_SERVO3, MAX_SERVO3);
    angulo_s4 = constrain(angulo_s4, MIN_SERVO4, MAX_SERVO4);

    // Atualiza os motores
    servo1.write(angulo_s1);
    servo2.write(angulo_s2);
    servo3.write(angulo_s3);
    servo4.write(angulo_s4);
  }

  // =================================================================
  // ROTINA DE DEBUG
  // =================================================================
  if (tempo_atual - tempo_anterior_debug >= TEMPO_DEBUG) {
    tempo_anterior_debug = tempo_atual;

    bool btn1_pressionado = !digitalRead(PINO_BTN1);
    bool btn2_pressionado = !digitalRead(PINO_BTN2);

    Serial.println("=================================================");
    Serial.println("LEITURAS DE DEBUG (Angulo Acumulado e Limitado):");
    
    Serial.print("Joy 1 -> X (Servo1): ");
    Serial.print((int)angulo_s1);
    Serial.print("º | Y (Servo2): ");
    Serial.print((int)angulo_s2);
    Serial.print("º | Btn1: ");
    Serial.println(btn1_pressionado);

    Serial.print("Joy 2 -> X (Servo3): ");
    Serial.print((int)angulo_s3);
    Serial.print("º | Y (Servo4): ");
    Serial.print((int)angulo_s4);
    Serial.print("º | Btn2: ");
    Serial.println(btn2_pressionado);
    
    Serial.println("=================================================");
  }
}