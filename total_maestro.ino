#include <LiquidCrystal.h>
#include <Wire.h>

LiquidCrystal lcd(11, 10, 9, 8, 7, 6);

const int botonControlPin = 13;
const int botonSeleccionPin = 12;
const int ledPin9 = 2;
const int ledPin8 = 3;
const int ledPin7 = 4;
const int ledPin6 = 5;
const int ledRojoPin = A2;
int potenciometroSegundosPin = A0;
int potenciometroMinutosPin = A1;
int sensorLuzPin = A3;

enum Estado {APAGADO, ENCENDIDO, MENU, LUZ_ALEATORIA, CAMBIAR_HORA, ALARMA, BLOQUEADO};
Estado estado = APAGADO;

enum OpcionMenu {OPCION_LUZ_ALEATORIA, OPCION_CAMBIAR_HORA};
OpcionMenu opcionMenu = OPCION_LUZ_ALEATORIA;

bool modoCambiarHoraActivo = false;
bool alarmaActivada = false;
int intentos = 0;
const int longitudClave = 4;
int claveCorrecta[longitudClave] = {1, 2, 3, 4};  // Clave correcta para desactivar la alarma
int claveIngresada[longitudClave] = {0, 0, 0, 0}; // Clave ingresada por el usuario
int indiceClave = 0;

int horaInicio = 0;
int minutoInicio = 0;
int segundoInicio = 0;
int pausa = 1000;

int horaAlarma = 0;
int minutoAlarma = 0;
int segundoAlarma = 0;

unsigned long ultimaPresionBotonControl = 0;
unsigned long ultimaPresionBotonSeleccion = 0;
unsigned long tiempoInicioPresionBotonControl = 0;
unsigned long tiempoInicioPresionBotonSeleccion = 0;
bool esperandoActivacionModoSeleccion = false;

unsigned long tiempoBloqueo = 0;
const unsigned long tiempoEspera = 10000; // 10 segundos

void setup() {
  pinMode(ledPin9, OUTPUT);
  pinMode(ledPin8, OUTPUT);
  pinMode(ledPin7, OUTPUT);
  pinMode(ledPin6, OUTPUT);
  pinMode(ledRojoPin, OUTPUT);

  lcd.begin(16, 2);

  pinMode(botonControlPin, INPUT_PULLUP);
  pinMode(botonSeleccionPin, INPUT_PULLUP);

  Serial.begin(9600);
  Wire.begin(); // Dirección I2C del maestro

  lcd.setCursor(0, 0);
  lcd.print("En espera....");
}

void loop() {
  // Verificación del botón de control (pin 13)
  if (estado != BLOQUEADO) {
    if (digitalRead(botonControlPin) == LOW && millis() - ultimaPresionBotonControl > 200) {
      if (tiempoInicioPresionBotonControl == 0) {
        tiempoInicioPresionBotonControl = millis();
      }
      if (millis() - tiempoInicioPresionBotonControl > 2000) {  // Cambiado a 2 segundos
        activarOpcionControlApagar();
        tiempoInicioPresionBotonControl = 0;
        ultimaPresionBotonControl = millis();
      }
    } else if (digitalRead(botonControlPin) == HIGH && tiempoInicioPresionBotonControl != 0) {
      if (millis() - tiempoInicioPresionBotonControl < 2000) {  // Cambiado a 2 segundos
        manejarBotonControl();
        ultimaPresionBotonControl = millis();
      }
      tiempoInicioPresionBotonControl = 0;
    }
  }

  // Verificación del botón de selección (pin 12)
  if (digitalRead(botonSeleccionPin) == LOW && millis() - ultimaPresionBotonSeleccion > 200) {
    if (tiempoInicioPresionBotonSeleccion == 0) {
      tiempoInicioPresionBotonSeleccion = millis();
    }
    if (millis() - tiempoInicioPresionBotonSeleccion > 2000) {  // Cambiado a 2 segundos
      apagarModoLuzAleatoria();
      tiempoInicioPresionBotonSeleccion = 0;
      ultimaPresionBotonSeleccion = millis();
    }
  } else if (digitalRead(botonSeleccionPin) == HIGH && tiempoInicioPresionBotonSeleccion != 0) {
    if (millis() - tiempoInicioPresionBotonSeleccion < 2000) {  // Cambiado a 2 segundos
      manejarBotonSeleccion();
      ultimaPresionBotonSeleccion = millis();
    }
    tiempoInicioPresionBotonSeleccion = 0;
  }

  if (estado == ENCENDIDO || estado == LUZ_ALEATORIA) {
    mostrarHoraActual(horaInicio, minutoInicio, segundoInicio);
    actualizarTiempo();
    delay(1000); // Delay para sincronización con el segundo
  }

  if (estado == MENU && esperandoActivacionModoSeleccion && millis() - ultimaPresionBotonSeleccion >= 500) {  // Cambiado a 2 segundos
    activarModoSeleccionado();
  }

  if (modoCambiarHoraActivo) {
    cambiarHora();
  }

  if (estado == LUZ_ALEATORIA) {
    modoLuzAleatoria();
  }

  if (detectarLuz()) {
    if (estado != APAGADO) {
      horaAlarma = horaInicio;
      minutoAlarma = minutoInicio;
      segundoAlarma = segundoInicio;
      activarAlarma();
    }
  }

  if (alarmaActivada) {
    Wire.requestFrom(8, 1); // Solicitar una tecla del esclavo
    if (Wire.available()) {
      char tecla = Wire.read();
      manejarTeclado(tecla);
    }
  }

  if (estado == BLOQUEADO) {
    if (millis() - tiempoBloqueo > tiempoEspera) {
      estado = ALARMA; // Regresar al estado de alarma después del tiempo de bloqueo
      reiniciarIntentos(); // Resetear la clave ingresada y los intentos
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Ingrese clave:");
      lcd.setCursor(0, 1);
      lcd.print("Hora: " + String(minutoAlarma) + ":" + String(segundoAlarma));
    }
  }
}

void manejarBotonControl() {
  if (estado == APAGADO) {
    estado = ENCENDIDO;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HORA:");
  } else if (estado == ENCENDIDO) {
    estado = MENU;
    mostrarMenu();
  } else if (estado == MENU) {
    estado = ENCENDIDO;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HORA:");
  }
}

void manejarBotonSeleccion() {
  if (estado == MENU) {
    opcionMenu = (OpcionMenu)((opcionMenu + 1) % 2); // Alternar entre las opciones
    mostrarMenu();
    esperandoActivacionModoSeleccion = true; // Activar el modo de espera
  }
}

void activarOpcionControlApagar() {
  if (estado != APAGADO) {
    estado = APAGADO;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Apagando");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("En espera....");
  }
}

void activarModoSeleccionado() {
  esperandoActivacionModoSeleccion = false;
  switch (opcionMenu) {
    case OPCION_LUZ_ALEATORIA:
      estado = LUZ_ALEATORIA;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Luz Aleatoria");
      break;
    case OPCION_CAMBIAR_HORA:
      estado = CAMBIAR_HORA;
      modoCambiarHoraActivo = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cambiar Hora");
      cambiarHora();
      break;
  }
}

void mostrarMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Menu:");
  lcd.setCursor(0, 1);
  if (opcionMenu == OPCION_LUZ_ALEATORIA) {
    lcd.print("1: Luz Aleatoria");
  } else {
    lcd.print("2: Cambiar Hora");
  }
}

void mostrarHoraActual(int horaActual, int minutoActual, int segundoActual) {
  String hora = horaActual < 10 ? "0" + String(horaActual) : String(horaActual);
  String minuto = minutoActual < 10 ? "0" + String(minutoActual) : String(minutoActual);
  String segundo = segundoActual < 10 ? "0" + String(segundoActual) : String(segundoActual);

  String tiempo = minuto + ":" + segundo;
  
  lcd.setCursor(8 - tiempo.length() / 2, 1); // Centrando la hora en la pantalla
  lcd.print(tiempo);
}

void actualizarTiempo() {
  segundoInicio++;
  if (segundoInicio > 59) {
    segundoInicio = 0;
    minutoInicio++;
    if (minutoInicio > 59) {
      minutoInicio = 0;
      horaInicio++;
      if (horaInicio > 23) {
        horaInicio = 0;
      }
    }
  }
}

void cambiarHora() {
  int nuevoMinuto = 0;
  int nuevoSegundo = 0;
  
  while (modoCambiarHoraActivo) {
    int valorSegundos = analogRead(potenciometroSegundosPin);
    nuevoSegundo = map(valorSegundos, 0, 1023, 0, 59);

    int valorMinutos = analogRead(potenciometroMinutosPin);
    nuevoMinuto = map(valorMinutos, 0, 1023, 0, 59);

    lcd.setCursor(5, 1);
    lcd.print(String("") + nuevoMinuto + ":" + nuevoSegundo);

    if (digitalRead(botonSeleccionPin) == LOW) {
      delay(200); // Debounce
      minutoInicio = nuevoMinuto;
      segundoInicio = nuevoSegundo;
      modoCambiarHoraActivo = false;
      estado = ENCENDIDO;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("HORA:");
      break;
    }
  }
}

void modoLuzAleatoria() {
  int leds[] = {ledPin9, ledPin8, ledPin7, ledPin6};
  int led = random(0, 4);

  digitalWrite(leds[led], HIGH);
  delay(pausa);
  digitalWrite(leds[led], LOW);
  
  if (minutoInicio == 0 && segundoInicio == 20) {
    apagarModoLuzAleatoria();
  }
}

bool detectarLuz() {
  int valorLuz = analogRead(sensorLuzPin);
  if (valorLuz > 500) { // Umbral para detección de luz
    return true;
  }
  return false;
}

void activarAlarma() {
  if (!alarmaActivada) {
    alarmaActivada = true;
    estado = ALARMA;
    digitalWrite(ledRojoPin, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Alarma Activada");
    lcd.setCursor(0, 1);
    lcd.print("Hora: " + String(minutoAlarma) + ":" + String(segundoAlarma));
    Serial.print("Alarma Activada a las ");
    Serial.print(minutoAlarma);
    Serial.print(":");
    Serial.println(segundoAlarma);
  }
}

void desactivarAlarma() {
  alarmaActivada = false;
  digitalWrite(ledRojoPin, LOW);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Alarma desactivada");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hora Actual:");
  intentos = 0;
  reiniciarIntentos(); // Reiniciar la clave ingresada
  estado = ENCENDIDO;
}

void manejarTeclado(char tecla) {
  if (tecla == '*') { // Validar clave
    if (indiceClave == longitudClave) {
      if (validarClave()) {
        desactivarAlarma();
      } else {
        intentos++;
        if (intentos >= 2) {
          estado = BLOQUEADO;
          tiempoBloqueo = millis(); // Guardar el tiempo de bloqueo
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Bloqueado");
          lcd.setCursor(0, 1);
          lcd.print("Espere.....");
        } else {
          reiniciarIntentos(); // Resetear la clave ingresada
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Clave incorrecta");
          delay(2000);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Ingrese clave:");
          lcd.setCursor(0, 1);
          lcd.print("Hora: " + String(minutoAlarma) + ":" + String(segundoAlarma));
        }
      }
    }
  } else if (tecla >= '0' && tecla <= '9') {
    if (indiceClave < longitudClave) {
      claveIngresada[indiceClave] = tecla - '0';
      lcd.setCursor(11 + indiceClave, 1); // Mostrar clave ingresada en las últimas 5 casillas
      lcd.print(tecla);
      indiceClave++;
    }
  }
}

bool validarClave() {
  for (int i = 0; i < longitudClave; i++) {
    if (claveIngresada[i] != claveCorrecta[i]) {
      return false;
    }
  }
  return true;
}

void reiniciarIntentos() {
  for (int i = 0; i < longitudClave; i++) {
    claveIngresada[i] = 0;
  }
  indiceClave = 0;
  lcd.setCursor(11, 1);
  lcd.print("     "); // Limpiar clave ingresada en el LCD
}

void apagarModoLuzAleatoria() {
  estado = ENCENDIDO;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hora Actual:");
}


