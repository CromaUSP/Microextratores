/*
Arduino Uno

Observações:
1. As seringas devem sempre começar completamente vazias

*/

/*

D0  - AQUECEDOR
D1  - RESFRIADOR
D2  - X_STEP
D3  - Y_STEP
D4  - Z_STEP 
D5  - X_DIR     /   VALVULA
D6  - Y_DIR     /   VIBRADOR
D7  - Z_DIR     /
D8  - EN
D9  - HPLC
D10 - CS
D11 - MOSI
D12 - A_STEP    /   MISO
D13 - A_DIR     /   SCK
A0  - X_STOP
A1  - Y_STOP
A2  - Z_STOP
A3  - A_STOP
A4  - SDA
A5  - SCL

*/

/*
Colunas:
Colunas:
1ª  - Sentido de giro do motor X (0 - Horário; 1 - Antihorário)
2ª  - N° de passos do motor X
3ª  - Sentido de giro do motor Y (0 - Horário; 1 - Antihorário)
4ª  - N° de passos do motor Y
5ª  - Sentido de giro do motor Z (0 - Horário; 1 - Antihorário)
6ª  - N° de passos do motor Z
7ª  - Sentido de giro do motor A (0 - Injetar; 1 - Extrair; 2 - Manter)
8ª  - Repetições do motor A
9ª  - N° de passos do motor A
10ª - Intensidade do vibrador
11ª - Tempo de duração do vibrador
12ª - Tempo parado no HPLC
13ª - Temperatura controlada (0 - Resfriar; 1 - Esquentar; 2 - Manter)
14ª - Sentido de giro do motor Z (0 - Horário; 1 - Antihorário)
15ª - N° de passos do motor Z
*/

#include <SD.h>
#include <SPI.h>

const unsigned int AQUECEDOR = 0;
const unsigned int RESFRIADOR = 1;
const unsigned int X_STEP = 2;
const unsigned int Y_STEP = 3;
const unsigned int Z_STEP = 4;
const unsigned int X_DIR = 5;
const unsigned int Y_DIR = 6;
const unsigned int Z_DIR = 7;
const unsigned int VALVULA = 5;
const unsigned int VIBRADOR = 6;
const unsigned int EN = 8;
const unsigned int HPLC = 9;
const unsigned int chipSelect = 10;    
const unsigned int A_STEP = 12;
const unsigned int A_DIR = 13;
const unsigned int X_STOP = A0;
const unsigned int Y_STOP = A1;
const unsigned int Z_STOP = A2;
const unsigned int A_STOP = A3;

unsigned int VEL_MOTOR = 30;

int *buffer = NULL;
int size = 0;
int numlinhas = 0;

int saldo[6] = {0, 0, 0, 0, 0, 0};

void setup() {
  pinMode(AQUECEDOR, OUTPUT);
  pinMode(RESFRIADOR, OUTPUT);
  pinMode(X_STEP, OUTPUT);
  pinMode(Y_STEP, OUTPUT);
  pinMode(Z_STEP, OUTPUT);
  pinMode(X_DIR, OUTPUT);
  pinMode(Y_DIR, OUTPUT);
  pinMode(Z_DIR, OUTPUT);
  pinMode(VALVULA, OUTPUT);
  pinMode(VIBRADOR, OUTPUT);
  pinMode(EN, OUTPUT);
  pinMode(HPLC, OUTPUT);
  pinMode(chipSelect, OUTPUT);
  pinMode(X_STOP, INPUT);
  pinMode(Y_STOP, INPUT);
  pinMode(Z_STOP, INPUT);
  pinMode(A_STOP, INPUT);
  
  Serial.begin(9600);
  
  if (!SD.begin(chipSelect)) {
    Serial.println("Falha ao inicializar o cartão SD");
    return;
  }
  
  File arquivo = SD.open("dados.csv");
  if (!arquivo) {
    Serial.println("Arquivo não encontrado");
    return;
  }
  
  String linha;
  while (arquivo.available()) {
    char c = arquivo.read();
    if (c == 'X') {
      break;
    }
    if (c == '\n') {
      //int numero = linha.toInt();
      addElement(linha.toInt());
      //Serial.println(numero);
      linha = "";
    } else {
      linha += c;
    }
  }
  size -= 3;
  numlinhas = size/15;
  size++;
  int dadosordenados[numlinhas][15] = {};
  int k = 0;
  for (int i = 0; i < numlinhas; i++) {
    for (int j = 0; j < 15; j++) {
      dadosordenados[i][j] = buffer[k];
      k++;
    }
  }
  
  arquivo.close();
  
  pinMode(A_STEP, OUTPUT);
  pinMode(A_DIR, OUTPUT);

  for (int l = 0; l < numlinhas; l++) {

    motor(EN, X_DIR, X_STEP, dadosordenados[l][2], intToBool(dadosordenados[l][1]), VEL_MOTOR);
    motor(EN, Y_DIR, Y_STEP, dadosordenados[l][2], intToBool(dadosordenados[l][1]), VEL_MOTOR);
    motor(EN, Z_DIR, Z_STEP, dadosordenados[l][6], intToBool(dadosordenados[l][5]), VEL_MOTOR);
    if(dadosordenados[l][7] != 2) {
      if (intToBool(dadosordenados[l][7])) {
        saldo[m] += dadosordenados[l][9];
        while (digitalRead(A_STOP) != 1) motor(EN, A_DIR, A_STEP, dadosordenados[l][9], intToBool(dadosordenados[l][7]), VEL_MOTOR);
        for (int n = 0; n < dadosordenados[l][8]; n++) {
          while (digitalRead(A_STOP) != 1) {
            motor(EN, A_DIR, A_STEP, dadosordenados[l][9], !intToBool(dadosordenados[l][7]), VEL_MOTOR);
            motor(EN, A_DIR, A_STEP, dadosordenados[l][9], intToBool(dadosordenados[l][7]), VEL_MOTOR);
          }
        }
      } else if (saldo[m] >= dadosordenados[l][9]) {
        saldo[m] -= dadosordenados[l][9];
        motor(EN, A_DIR, A_STEP, dadosordenados[l][9], intToBool(dadosordenados[l][7]), VEL_MOTOR);
        for (int n = 0; n < dadosordenados[l][8]; n++) {
          motor(EN, A_DIR, A_STEP, dadosordenados[l][9], !intToBool(dadosordenados[l][7]), VEL_MOTOR);
          motor(EN, A_DIR, A_STEP, dadosordenados[l][9], intToBool(dadosordenados[l][7]), VEL_MOTOR);
        }
      } else {
        Serial.print("Saldo de volume das seringas esta incorreto, a inserção da operação de numero ");
        Serial.print(l);
        Serial.print("não foi realizada");
        Serial.println("");
      }
    }

    digitalWrite(EN, HIGH);
    VIBRADOR(dadosordenados[l][10], dadosordenados[l][11]);
    digitalWrite(EN, LOW);
    HPLC(dadosordenados[l][12]);
    RESFRIADOR(dadosordenados[l][13]);
    while (digitalRead(Z_STOP) != 1) motor(EN, Z_DIR, Z_STEP, 1, intToBool(dadosordenados[l][14]), VEL_MOTOR);
  
  }
  while (digitalRead(X_STOP) != 1) motor(EN, X_DIR, X_STEP, 1, intToBool(buffer[size-3]), VEL_MOTOR);
  while (digitalRead(Y_STOP) != 1) motor(EN, Y_DIR, Y_STEP, 1, intToBool(buffer[size-1]), VEL_MOTOR);
}

void loop() {
}

void addElement(int value) {
  int *temp = (int *)realloc(buffer, (size + 1) * sizeof(int));
  if (temp != NULL) {
    buffer = temp;
    buffer[size] = value;
    size++;
  }
}

bool intToBool(int x) {
  return x != 0;
}

void motor(unsigned int PORT_ENABLE, unsigned int PORT_DIR, unsigned int PORT_STEP, unsigned int N_PASSOS, bool SENTIDO, unsigned int TEMPO) {
  digitalWrite(PORT_ENABLE, LOW);
  digitalWrite(PORT_DIR, SENTIDO);
  Serial.print("Motor ");
  Serial.println(PORT_ENABLE);
  for(unsigned int i = 0; i < N_PASSOS; i++){
    digitalWrite(PORT_STEP, LOW);
    delayMicroseconds(TEMPO);
    digitalWrite(PORT_STEP, HIGH);
    delayMicroseconds(TEMPO);
    Serial.print("Passo n° ");
    Serial.println(N_PASSOS-i);
  }
}

void VIBRADOR(unsigned int INTENSIDADE, unsigned int TEMPO) {
  analogWrite(PORT_VIBRADOR, (INTENSIDADE*255)/100);
  delay(TEMPO*1000);
}

void HPLC(unsigned int TEMPO) {
  if (TEMPO != 0) {
    digitalWrite(PORT_HPLC, HIGH);
    digitalWrite(EN, HIGH);
    digitalWrite(PORT_VALVULA, HIGH);
    motor(S_EN[0], S_DIR[0], S_STEP[0], saldo[0], 0, VEL_MOTOR);
    digitalWrite(PORT_VALVULA, LOW);
    digitalWrite(EN, LOW);
  }
  delay(TEMPO*1000);
}

void RESFRIADOR(unsigned int COMANDO) {
  switch (COMANDO) {
    case 2:
      digitalWrite(PORT_AQUECEDOR, LOW);
      digitalWrite(PORT_RESFRIADOR, LOW);
      break;
    case 1:
      digitalWrite(PORT_AQUECEDOR, HIGH);
      digitalWrite(PORT_RESFRIADOR, LOW);
      break;  
    case 0:
      digitalWrite(PORT_AQUECEDOR, LOW);
      digitalWrite(PORT_RESFRIADOR, HIGH);
      break;
  }
}
