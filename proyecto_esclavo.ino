#include <Wire.h>
#include <Keypad.h>

const byte FILAS = 4; // Cuatro filas
const byte COLUMNAS = 4; // Cuatro columnas

char teclas[FILAS][COLUMNAS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pinesFilas[FILAS] = {13, 12, 11, 10}; // Pines de las filas
byte pinesColumnas[COLUMNAS] = {9, 8, 7, 6}; // Pines de las columnas

Keypad teclado = Keypad(makeKeymap(teclas), pinesFilas, pinesColumnas, FILAS, COLUMNAS);

void setup() {
  Wire.begin(8); // Dirección I2C del esclavo
  Wire.onRequest(enviarTecla);

  Serial.begin(9600);
}

char teclaPresionada = NO_KEY;

void loop() {
  char tecla = teclado.getKey();
  if (tecla) {
    teclaPresionada = tecla;
    Serial.println(teclaPresionada);
  }
}

void enviarTecla() {
  Wire.write(teclaPresionada);
  teclaPresionada = NO_KEY;
}


