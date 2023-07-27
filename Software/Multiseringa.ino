/*
Arduino Mega
você 
Observações:
1. As seringas devem sempre começar completamente vazias
2. O HPLC só pode ser conectado á seringa A


00 Vibrador
01 HPLC
10 Aquecedor
11 Resfriador

*/

/*
Colunas:
1ª  - Sentido de giro do motor X (0 - Horário; 1 - Antihorário)
2ª  - N° de passos do motor X
3ª  - Sentido de giro do motor Z (0 - Horário; 1 - Antihorário)
4ª  - N° de passos do motor Z
5ª  - Sentido de giro do motor A (0 - Injetar; 1 - Extrair; 2 - Manter)
6ª  - Repetições do motor A
7ª  - N° de passos do motor A
8ª  - Sentido de giro do motor B (0 - Injetar; 1 - Extrair; 2 - Manter)
9ª  - Repetições do motor B
10ª - N° de passos do motor B
11ª - Sentido de giro do motor C (0 - Injetar; 1 - Extrair; 2 - Manter)
12ª - Repetições do motor C
13ª - N° de passos do motor C
14ª - Sentido de giro do motor D (0 - Injetar; 1 - Extrair; 2 - Manter)
15ª - Repetições do motor D
16ª - N° de passos do motor D
17ª - Sentido de giro do motor E (0 - Injetar; 1 - Extrair; 2 - Manter)
18ª - Repetições do motor E
19ª - N° de passos do motor E
20ª - Sentido de giro do motor F (0 - Injetar; 1 - Extrair; 2 - Manter)
21ª - Repetições do motor F
22ª - N° de passos do motor F
23ª - Intensidade do vibrador
24ª - Tempo de duração do vibrador
25ª - Tempo parado no HPLC
26ª - Temperatura controlada (0 - Resfriar; 1 - Esquentar; 2 - Manter)
27ª - Sentido de giro do motor Z (0 - Horário; 1 - Antihorário)
28ª - N° de passos do motor Z
*/

#include <SD.h>
#include <SPI.h>

const int chipSelect = 53;              
const unsigned int X_EN = 38;           
const unsigned int X_STEP = 54;       
const unsigned int X_DIR = 55;  
const unsigned int X_MIN = 3;
const unsigned int Z_EN = 62;          
const unsigned int Z_STEP = 46;         
const unsigned int Z_DIR = 48;          
const unsigned int Z_MAX = 19;
const unsigned int PORT_HPLC = 6;         
const unsigned int PORT_VIBRADOR = 8;    
const unsigned int PORT_AQUECEDOR = 9; 
const unsigned int PORT_RESFRIADOR = 10; 
const unsigned int PORT_VALVULA = 11;  

unsigned int VEL_MOTOR = 30;

int *buffer = NULL;
int size = 0;
int numlinhas = 0;

int saldo[6] = {0, 0, 0, 0, 0, 0};
char SER[6] = {'A', 'B', 'C', 'D', 'E', 'F'};
int S_EN[6] = {59, 24, 30, 0, 0, 0};    
int S_DIR[6] = {61, 28, 34, 0, 0, 0};   
int S_STEP[6] = {60, 26, 36, 0, 0, 0};  
int S_MAX[6] = {2, 18, 14, 15, 5, 4};

/*
 * COMPLETO COM PONTES H PARA OS MOTORES REMANECENTES
 * 0101
 * 0110
 * 1010
 * 1001
*/

int M1[3] = {16, 27, 35};
int M2[3] = {17, 29, 37};
int M3[3] = {23, 31, 39};
int M4[3] = {25, 33, 41};

bool F1[4] = {false, false, true, true};
bool F2[4] = {true, true, false, false};
bool F3[4] = {false, true, true, false};
bool F4[4] = {true, false, false, true};

/*
 * COMPLETO COM PONTES H PARA OS MOTORES REMANECENTES
*/

void setup() {
  pinMode(X_EN, OUTPUT);
  pinMode(X_STEP, OUTPUT);
  pinMode(X_DIR, OUTPUT); 
  pinMode(X_MIN, INPUT);
  pinMode(Z_EN, OUTPUT);
  pinMode(Z_STEP, OUTPUT);
  pinMode(Z_DIR, OUTPUT);
  pinMode(Z_MAX, INPUT);
  for (int a = 0; a < 3; a++) {
    pinMode(S_EN[a], OUTPUT);
    pinMode(S_STEP[a], OUTPUT);
    pinMode(S_DIR[a], OUTPUT);
    pinMode(S_MAX[a], INPUT);
    pinMode(M1[a], OUTPUT);
    pinMode(M2[a], OUTPUT);
    pinMode(M3[a], OUTPUT);
    pinMode(M4[a], OUTPUT);
  }
  pinMode(PORT_HPLC, OUTPUT);
  pinMode(PORT_VALVULA, OUTPUT);
  pinMode(PORT_AQUECEDOR, OUTPUT);
  pinMode(PORT_RESFRIADOR, OUTPUT);
  
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
  size--;
  numlinhas = size/28;
  size++;
  int dadosordenados[numlinhas][28] = {};
  int k = 0;
  for (int i = 0; i < numlinhas; i++) {
    for (int j = 0; j < 28; j++) {
      dadosordenados[i][j] = buffer[k];
      k++;
    }
  }
  
  arquivo.close();
  
  for (int l = 0; l < numlinhas; l++) {
    motor(X_EN, X_DIR, X_STEP, dadosordenados[l][2], intToBool(dadosordenados[l][1]), VEL_MOTOR);
    motor(Z_EN, Z_DIR, Z_STEP, dadosordenados[l][4], intToBool(dadosordenados[l][3]), VEL_MOTOR);
    for (int m = 0; m < 3; m++) {
      if(dadosordenados[l][5+(m*3)] != 2) {
        if (intToBool(dadosordenados[l][5+(m*3)])) {
          saldo[m] += dadosordenados[l][7+(m*3)];
          while (digitalRead(S_MAX[m]) != 1) motor(S_EN[m], S_DIR[m], S_STEP[m], dadosordenados[l][7+(m*3)], intToBool(dadosordenados[l][5+(m*3)]), VEL_MOTOR);
          for (int n = 0; n < dadosordenados[l][6+(m*3)]; n++) {
            while (digitalRead(S_MAX[m]) != 1) {
              motor(S_EN[m], S_DIR[m], S_STEP[m], dadosordenados[l][7+(m*3)], !intToBool(dadosordenados[l][5+(m*3)]), VEL_MOTOR);
              motor(S_EN[m], S_DIR[m], S_STEP[m], dadosordenados[l][7+(m*3)], intToBool(dadosordenados[l][5+(m*3)]), VEL_MOTOR);
            }
          }
        } else if (saldo[m] >= dadosordenados[l][7+(m*3)]) {
          saldo[m] -= dadosordenados[l][7+(m*3)];
          motor(S_EN[m], S_DIR[m], S_STEP[m], dadosordenados[l][7+(m*3)], intToBool(dadosordenados[l][5+(m*3)]), VEL_MOTOR);
          for (int n = 0; n < dadosordenados[l][6+(m*3)]; n++) {
            motor(S_EN[m], S_DIR[m], S_STEP[m], dadosordenados[l][7+(m*3)], !intToBool(dadosordenados[l][5+(m*3)]), VEL_MOTOR);
            motor(S_EN[m], S_DIR[m], S_STEP[m], dadosordenados[l][7+(m*3)], intToBool(dadosordenados[l][5+(m*3)]), VEL_MOTOR);
          }
        } else {
          Serial.print("Saldo de volume das seringas esta incorreto, a inserção da operação de numero ");
          Serial.print(l);
          Serial.print(" da seringa ");
          Serial.print(SER[m]); 
          Serial.print("não foi realizada");
          Serial.println("");
        }
      }
    }

    for (int m = 3; m < 6; m++) {
      if(dadosordenados[l][5+(m*3)] != 2) {
        if (intToBool(dadosordenados[l][5+(m*3)])) {
          saldo[m] += dadosordenados[l][7+(m*3)];
          while (digitalRead(S_MAX[m]) != 1) motorh(M1[m-3], M2[m-3], M3[m-3], M4[m-3], dadosordenados[l][7+(m*3)], intToBool(dadosordenados[l][5+(m*3)]), VEL_MOTOR);
          for (int n = 0; n < dadosordenados[l][6+(m*3)]; n++) {
            while (digitalRead(S_MAX[m]) != 1) {
              motorh(M1[m-3], M2[m-3], M3[m-3], M4[m-3], dadosordenados[l][7+(m*3)], !intToBool(dadosordenados[l][5+(m*3)]), VEL_MOTOR);
              motorh(M1[m-3], M2[m-3], M3[m-3], M4[m-3], dadosordenados[l][7+(m*3)], intToBool(dadosordenados[l][5+(m*3)]), VEL_MOTOR);
            }
          }
        } else if (saldo[m] >= dadosordenados[l][7+(m*3)]) {
          saldo[m] -= dadosordenados[l][7+(m*3)];
          motorh(M1[m-3], M2[m-3], M3[m-3], M4[m-3], dadosordenados[l][7+(m*3)], intToBool(dadosordenados[l][5+(m*3)]), VEL_MOTOR);
          for (int n = 0; n < dadosordenados[l][6+(m*3)]; n++) {
            motorh(M1[m-3], M2[m-3], M3[m-3], M4[m-3], dadosordenados[l][7+(m*3)], !intToBool(dadosordenados[l][5+(m*3)]), VEL_MOTOR);
            motorh(M1[m-3], M2[m-3], M3[m-3], M4[m-3], dadosordenados[l][7+(m*3)], intToBool(dadosordenados[l][5+(m*3)]), VEL_MOTOR);
          }
        } else {
          Serial.print("Saldo de volume das seringas esta incorreto, a inserção da operação de numero ");
          Serial.print(l);
          Serial.print(" da seringa ");
          Serial.print(SER[m]); 
          Serial.print("não foi realizada");
          Serial.println("");
        }
      }
    }

    VIBRADOR(dadosordenados[l][23], dadosordenados[l][24]);
    HPLC(dadosordenados[l][25]);
    RESFRIADOR(dadosordenados[l][26]);
    while (digitalRead(Z_MAX) != 1) motor(Z_EN, Z_DIR, Z_STEP, 1, intToBool(dadosordenados[l][27]), VEL_MOTOR);
  }
  while (digitalRead(X_MIN) != 1) motor(X_EN, X_DIR, X_STEP, 1, intToBool(buffer[size-1]), VEL_MOTOR);
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
    digitalWrite(PORT_VALVULA, HIGH);
    motor(S_EN[0], S_DIR[0], S_STEP[0], saldo[0], 0, VEL_MOTOR);
    digitalWrite(PORT_VALVULA, LOW);
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

void motorh (unsigned int FASEA, unsigned int FASEB, unsigned int FASEC, unsigned int FASED, unsigned int N_PASSOS, bool SENTIDO, unsigned int TEMPO) {
  for (unsigned int b = 0; b < N_PASSOS/4; b++) {
    for (unsigned int c = 0; c < 4; c++) {
      unsigned int d = c;
      if (SENTIDO) d = 3 - c;
      digitalWrite(FASEA, F1[d]);
      delayMicroseconds(TEMPO);
      digitalWrite(FASEB, F2[d]);
      delayMicroseconds(TEMPO);
      digitalWrite(FASEC, F3[d]);
      delayMicroseconds(TEMPO);
      digitalWrite(FASED, F4[d]);
      delayMicroseconds(TEMPO);
    }
  }
}
