//Versión del software: 0.8.4

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------------------------LIBRERÍAS UTILIZADAS--------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

#include <EEPROM.h>
#include <DHT11.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <avr/wdt.h>
#include "RTClib.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------------------VALORES A MODIFICAR SEGUN CARACTERIÍSTICAS-------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

#define i2cdir 11 //direccion i2c del Arduino, es necesario para comunicarse con NodeMCU
char riego = hidroponia; //goteo;
#define descarga 30000 //Tiempo que mantiene la valvula de descarga encendida (asegura vaciado total) y si es riego por goteo, debe ser mucho menor (1000 por ejemplo)
#define xmax 2 //la cantidad maxima de plantas que tenemos cargadas -1 (si agregamos plantas, se debe incrementar este valor)
String planta[] = {"Personalizada", "Lechuga", "Tomate"}; //se pueden agregar más, aumentando el número entre [] y colocando las diferentes verduras entre ""
#define envio 1100 //Tiempo en milisegundos para enviar por puerto serie
#define rele 1000 //Tiempo en milisegundos para accionar los relé (para evitar quemar los aparatos)


//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------OTROS PARÁMETROS (NO TOCAR PARA EL CORRECTO FUNCIONAMIENTO)-----------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

//pines
#define lampara 9
#define ventilador 10
#define extractor 11
#define calefactor 14
#define aspersor 15
#define eve 16
#define evs 17
#define minano 2019

//parametros iniciales
byte menu = 1;
byte maxmenu;
byte orden = 1;
byte maxorden;
byte columna;
byte fila;
byte modo;
String parametro;
int param;
int maxparam;
byte dir = 0;
byte direccion;
int dato;
byte ti; //Temperatura en uso
byte tidia; //Temperatura ideal durante el dia
byte tinoche; //Temperatura ideal durante la noche
byte hi; //Humedad en uso
byte di; //cantidad ideal de dias a transcurrir
byte tn;
byte hn;
byte ln;
int x = 0;
byte y = 0;
byte z = 0;
byte nuevorecambio;
int recambio; //Cada cuantos día hara el recambio completo de agua (dias*horas*minutos*segundos*milisegundos)
byte restante;
int longitud;
int ano;
byte mes;
byte dia;
byte hora;
byte minuto;
byte segundo;
int anoinicio;
byte mesinicio;
byte diainicio;
byte horainicio;
byte minutoinicio;
float horaactual;
byte tipoano;
byte tipomes;
float horadia;
float horanoche;
byte horadiapers;
float mindiapers;
byte horanochepers;
float minnochepers;
DateTime fechaactual;
DateTime fechainicial;
int transcurrido;

//contadores y banderas para activar accionadores
unsigned long accionactual;
unsigned long accionanterior = 0;

//contadores y banderas para enviar por serie
unsigned long envioactual;
unsigned long envioanterior = 0;

//contadores y banderas para hacer parpadeo
#define intervalo 500
unsigned long tiempoactual;
unsigned long tiempoanterior = 0;

//contadores del proceso de recambio total de agua
unsigned long abre;
unsigned long cierra;

//NodeMCU
byte canal;

//Reloj
RTC_DS3231 rtc;

//sensor de humedad y temperatura (DHT11)
#define pindht 8
float hum, temp;
DHT11 dht11(pindht);

//sensor de luz
byte pinluz = A6;
float luz;

//flotante
#define pinflot 12
bool flot;

//pines digitales del display
#define rs 7
#define en 6
#define d4 5
#define d5 4
#define d6 3
#define d7 2
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------------------VALORES DE LAS DIFERENTES PLANTACIONES-----------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

//valores del usuario
byte tpersdia; //Temperatura escogida por el usuario
byte tpersnoche; //Temperatura escogida por el usuario
byte hpers; //Humedad escogida por el usuario
byte dpers; //Germinación: 3 días. Tiempo total de cultivo: 20-65 días
byte recpers;

//lechuga
byte tlechdia = 17; //Temperatura de germinación: 18-20ºC. crecimiento 14-18ºC por el día y 5-8ºC por la noche, la lechuga exige que haya diferencia de temperaturas.
byte tlechnoche = 7;
byte hlech = 65; //Humedad relativa: 60-80%.
byte dlech = 30; //Germinación: 3 días. Tiempo total de cultivo: 20-65 días
byte reclech = 14; //Días para el recambio total de agua

//tomate
byte ttomdia = 24; //Temperaturas de 24-25 ºC por el día y 15-18 ºC por la noche.
byte ttomnoche = 16;
byte htom = 70; //Humedad relativa: 70%
byte dtom = 90; //Germinación: 14-21 días. Tiempo total de cultivo: 90 días
byte rectom = 14; //Días para el recambio total de agua

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//-------------------------------------------BOTONERA (CAMBIAR SI SE DISPONE DE UNA DIFERENTE AL LCD KEYPAD SHIELD)-----------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

// definiciones de la botonera
#define pintecla A7
int tecla = 0;
#define DER  0
#define ARR  1
#define ABA  2
#define IZQ  3
#define SEL  4
#define RST  5

// funcion de lectura de botones
int leer_botones_LCD() {
  tecla = analogRead(pintecla);
  if (tecla > 1000) return RST;
  if (tecla < 50)   return DER;
  if (tecla < 250)  return ARR;
  if (tecla < 450)  return ABA;
  if (tecla < 650)  return IZQ;
  if (tecla < 850)  return SEL;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------PROGRAMA PRINCIPAL--------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------------CONFIGURACIÓN DE PINES Y ESTADOS DE LA PLACA-------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

void setup() {
  // seteamos el tipo de LCD (filas y columnas)
  wdt_disable();
  Serial.begin(115200);
  Wire.begin(i2cdir);
  lcd.begin(16, 2);
  pinMode(lampara, OUTPUT);
  pinMode(ventilador, OUTPUT);
  pinMode(extractor, OUTPUT);
  pinMode(calefactor, OUTPUT);
  pinMode(aspersor, OUTPUT);
  pinMode(pinflot, INPUT);
  EEPROM.get(dir, ti);
  EEPROM.get(dir + 1, tidia);
  EEPROM.get(dir + 2, tinoche);
  EEPROM.get(dir + 3, hi);
  EEPROM.get(dir + 7, horadia);
  EEPROM.get(dir + 11, horanoche);
  EEPROM.get(dir + 15, horadiapers);
  EEPROM.get(dir + 16, mindiapers);
  EEPROM.get(dir + 20, horanochepers);
  EEPROM.get(dir + 21, minnochepers);
  EEPROM.get(dir + 25, anoinicio);
  EEPROM.get(dir + 27, mesinicio);
  EEPROM.get(dir + 28, diainicio);
  EEPROM.get(dir + 29, horainicio);
  EEPROM.get(dir + 30, minutoinicio);
  EEPROM.get(dir + 31, nuevorecambio);
  EEPROM.get(dir + 32, recambio);
  if ((tidia == 0 && tinoche == 0 && hi == 0 && horadia == 0.0 && horanoche == 0.0 && nuevorecambio == 0)) {
    Serial.println("Partiendo desde memoria borrada ");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    fechainicial = DateTime(F(__DATE__), F(__TIME__));
    tidia = 20;
    tinoche = 15;
    hi = 65;
    horadiapers = 7;
    mindiapers = 0.0;
    horanochepers = 21;
    minnochepers = 0.0;
    horadia = horadiapers + mindiapers / 60.0;
    horanoche = horanochepers + minnochepers / 60.0;
    nuevorecambio = 14;
    recambio = 14;
    EEPROM.put(dir + 1, tidia);
    EEPROM.put(dir + 2, tinoche);
    EEPROM.put(dir + 3, hi);
    EEPROM.put(dir + 7, horadia); //flotante (4 bytes)
    EEPROM.put(dir + 11, horanoche); //flotante (4 bytes)
    EEPROM.put(dir + 15, horadiapers);
    EEPROM.put(dir + 16, mindiapers); //flotante (4 bytes)
    EEPROM.put(dir + 20, horanochepers);
    EEPROM.put(dir + 21, minnochepers); //flotante (4 bytes)
    EEPROM.put(dir + 31, nuevorecambio);
    EEPROM.put(dir + 32, recambio);
  }
  else fechainicial = DateTime(anoinicio, mesinicio, diainicio, horainicio, minutoinicio, 0);
  fechaactual = rtc.now();
  anoinicio = fechainicial.year();
  mesinicio = fechainicial.month();
  diainicio = fechainicial.day();
  horainicio = fechainicial.hour();
  minutoinicio = fechainicial.minute();
  EEPROM.put(dir + 25, anoinicio); //entero (2 bytes)
  EEPROM.put(dir + 27, mesinicio);
  EEPROM.put(dir + 28, diainicio);
  EEPROM.put(dir + 29, horainicio);
  EEPROM.put(dir + 30, minutoinicio);
  Wire.onReceive(recibe); //evento al recibir un dato desde el NodeMCU
  Wire.onRequest(envia); // evento de envio de datos al NodeMCU
  wdt_enable (WDTO_500MS);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------------------------------FUNCION PRINCIPAL--------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

void loop() {
  luz = map(analogRead(pinluz), 0, 1024, 99, 0);
  ln = luz;
  flot = digitalRead(pinflot);
  dht11.read(hum, temp);
  tn = temp;
  hn = hum;
  dia = rtc.now().day();
  mes = rtc.now().month();
  ano = rtc.now().year();
  hora = rtc.now().hour();
  minuto = rtc.now().minute();
  segundo = rtc.now().second();
  horaactual = hora + minuto / 60.0;
  transcurrido = ((ano - anoinicio) * (365.242189)) + ((dia - diainicio) + ((mes - mesinicio) * (365.242189 / 12)));
  restante = recambio - transcurrido;
  if (horaactual > horadia && horaactual < horanoche) ti = tidia;
  else ti = tinoche;

  //Pantalla
  if (leer_botones_LCD() == SEL) {
    modo++;
    if (modo > 1) modo = 0;
  }
  switch (modo) {
    case 0:
      pantalla();
      break;
    case 1:
      setear();
      break;
  }
  menus();
  serie();
  accion();
  wdt_reset();
  delay (175);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------OTRAS FUNCIONES---------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------MENUS MOSTRADOS EN PANTALLA---------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

void menus() {
  maxmenu = 8;
  switch (menu) {
    case 1: //Datos del reloj
      lcd.setCursor(0, 0);
      lcd.print("Fecha:");
      if (modo == 1 && orden == 1) parpadeo();
      else {
        if (dia < 10) lcd.print("0");
        lcd.print(dia);
      }
      lcd.print("/");
      if (modo == 1 && orden == 2) parpadeo();
      else {
        if (mes < 10) lcd.print("0");
        lcd.print(mes);
      }
      lcd.print("/");
      if (modo == 1 && orden == 3) parpadeo();
      else lcd.print(ano);
      lcd.setCursor(0, 1);
      lcd.print("Hora: ");
      if (modo == 1 && orden == 4) parpadeo();
      else {
        if (hora < 10) lcd.print("0");
        lcd.print(hora);
      }
      lcd.print(":");
      if (modo == 1 && orden == 5) parpadeo();
      else {
        if (minuto < 10) lcd.print("0");
        lcd.print(minuto);
      }
      lcd.print("     ");
      break;
    case 2: //Temperatura
      lcd.setCursor(0, 0);
      lcd.print("Temperatura: ");
      lcd.print(temp, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print("Tdia:");
      if (modo == 1 && orden == 1) parpadeo();
      else {
        if (tidia < 10) lcd.print("0");
        lcd.print(tidia);
      }
      lcd.print(" Tnoch:");
      if (modo == 1 && orden == 2) parpadeo();
      else {
        if (tinoche < 10) lcd.print("0");
        lcd.print(tinoche);
      }
      break;
    case 3: //Humedad
      lcd.setCursor(0, 0);
      lcd.print("Humedad: ");
      lcd.print(hum, 0);
      lcd.print("     ");
      lcd.setCursor(0, 1);
      lcd.print("Hum. Ideal: ");
      if (modo == 0) {
        if (hi < 10) lcd.print("0");
        lcd.print(hi);
      }
      else parpadeo();
      lcd.print("  ");
      break;
    case 4: //Iluminacion
      lcd.setCursor(0, 0);
      lcd.print("Nivel de Luz: ");
      lcd.print(luz, 0);
      lcd.print("  ");
      lcd.setCursor(0, 1);
      lcd.print("Lampara: ");
      if (horaactual > horadia && horaactual < horanoche && luz < 75) lcd.print("Encend.");
      else lcd.print("Apagad.");
      break;
    case 5: //Amanecer y anochecer
      lcd.setCursor(0, 0);
      lcd.print("Amanecer:  ");
      if (modo == 1 && orden == 1) parpadeo();
      else {
        if (horadiapers < 10) lcd.print("0");
        lcd.print(horadiapers);
      }
      lcd.print(":");
      if (modo == 1 && orden == 2) parpadeo();
      else {
        if (mindiapers < 10) lcd.print("0");
        lcd.print(mindiapers);
      }
      lcd.print("   ");
      lcd.setCursor(0, 1);
      lcd.print("Anochecer: ");
      if (modo == 1 && orden == 3) parpadeo();
      else {
        if (horanochepers < 10) lcd.print("0");
        lcd.print(horanochepers);
      }
      lcd.print(":");
      if (modo == 1 && orden == 4) parpadeo();
      else {
        if (minnochepers < 10) lcd.print("0");
        lcd.print(minnochepers);
      }
      lcd.print(" ");
      break;
    case 6:
      lcd.setCursor(0, 0);
      lcd.print("Recambio agua:");
      if (modo == 0) {
        if (nuevorecambio < 10) lcd.print("0");
        lcd.print(nuevorecambio);
      }
      else parpadeo();
      lcd.setCursor(0, 1);
      lcd.print("Restantes: ");
      if (restante < 10) lcd.print("0");
      lcd.print(restante);
      lcd.print(" D.");
      break;
    case 7: //tiempo transcurrido
      lcd.setCursor(0, 0);
      lcd.print("Ini: ");
      if (diainicio < 10) lcd.print("0");
      lcd.print(diainicio);
      lcd.print("/");
      if (mesinicio < 10) lcd.print("0");
      lcd.print(mesinicio);
      lcd.print("/");
      lcd.print(anoinicio);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print("Duracion: ");
      if (modo == 1) parpadeo();
      else {
        if (transcurrido < 100) lcd.print(" ");
        if (transcurrido < 10) lcd.print(" ");
        lcd.print((int)transcurrido);
      }
      lcd.print(" D.");
      break;
    case 8: //Seleccíon de temperaturas y humedades segun plantación
      lcd.setCursor(0, 0);
      lcd.print("Plantacion:      ");
      lcd.setCursor(0, 1);
      if (modo == 0) lcd.print(planta[x]);
      else parpadeo();
      lcd.print("                 ");
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------------------------SETEAR DIFERENTES VALORES---------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

void setear() {
  switch (menu) {
    case 1: //fecha y hora
      maxorden = 5;
      tipomes = mes;
      tipoano = ano;
      lateral();
      switch (orden) {
        case 1:
          fila = 0;
          columna = 6;
          param = dia;
          maxparam = diasmes();
          subebaja();
          dia = param;
          break;
        case 2:
          fila = 0;
          columna = 9;
          param = mes;
          maxparam = 12;
          subebaja();
          mes = param;
          break;
        case 3:
          fila = 0;
          columna = 12;
          if (ano < minano) ano = minano;
          param = ano;
          maxparam = 2050;
          subebaja();
          ano = param;
          break;
        case 4:
          fila = 1;
          columna = 6;
          param = hora;
          maxparam = 23;
          subebaja();
          hora = param;
          break;
        case 5:
          fila = 1;
          columna = 9;
          param = minuto;
          maxparam = 59;
          subebaja();
          minuto = param;
          break;
      }
      rtc.adjust(DateTime(ano, mes, dia, hora, minuto, 0));
      fechaactual = DateTime(ano, mes, dia, hora, minuto, 0);
      break;
    case 2: //temperatura
      maxorden = 2;
      fila = 1;
      lateral();
      switch (orden) {
        case 1:
          columna = 5;
          param = tidia;
          maxparam = 50;
          subebaja();
          tidia = param;
          break;
        case 2:
          columna = 14;
          param = tinoche;
          maxparam = 50;
          subebaja();
          tinoche = param;
          break;
      }
      break;
    case 3: //humedad
      fila = 1;
      columna = 12;
      param = hi;
      maxparam = 99;
      subebaja();
      hi = param;
      break;
    case 5: //amanecer y anochecer
      maxorden = 4;
      lateral();
      switch (orden) {
        case 1:
          fila = 0;
          columna = 11;
          param = horadiapers;
          maxparam = 23;
          subebaja();
          horadiapers = param;
          break;
        case 2:
          fila = 0;
          columna = 14;
          param = mindiapers;
          maxparam = 59;
          subebaja();
          mindiapers = param;
          break;
        case 3:
          fila = 1;
          columna = 11;
          param = horanochepers;
          maxparam = 23;
          subebaja();
          horanochepers = param;
          break;
        case 4:
          fila = 1;
          columna = 14;
          param = minnochepers;
          maxparam = 59;
          subebaja();
          minnochepers = param;
          break;
      }
      break;
    case 6:
      fila = 0;
      columna = 14;
      param = nuevorecambio;
      maxparam = 99;
      switch (leer_botones_LCD ()) {
        case ARR:
          recambio = recambio - param;
          param++;
          if (param > maxparam) param = 1;
          recambio = recambio + param;
          break;
        case ABA:
          recambio = recambio - param;
          param--;
          if (param <= 0) param = maxparam;
          recambio = recambio + param;
          break;
      }
      nuevorecambio = param;
      break;
    case 7: //tiempo transcurrido
      fila = 1;
      columna = 10;
      param = transcurrido;
      switch (leer_botones_LCD ()) {
        case ARR:
          fechainicial = DateTime(ano, mes, dia, hora, minuto, 0);
          break;
        case ABA:
          fechainicial = DateTime(ano, mes, dia, hora, minuto, 0);
          break;
      }
      recambio = nuevorecambio;
      anoinicio = ano;
      mesinicio = mes;
      diainicio = dia;
      horainicio = hora;
      minutoinicio = minuto;
      break;
    case 8: //seleccion de plantacion predeterminada
      fila = 1;
      columna = 0;
      parametro = planta[x];
      switch (leer_botones_LCD ()) {
        case ARR:
          x++;
          if (x > xmax) x = 0;
          cualplanta();
          break;
        case ABA:
          x--;
          if (x < 0) x = xmax;
          cualplanta();
          break;
      }
      break;
  }

  tpersdia = tidia;
  tpersnoche = tinoche;
  hpers = hi;
  horadia = horadiapers + mindiapers / 60.0;
  horanoche = horanochepers + minnochepers / 60.0;
  recpers = recambio;

  EEPROM.put(dir, ti);
  EEPROM.put(dir + 1, tidia);
  EEPROM.put(dir + 2, tinoche);
  EEPROM.put(dir + 3, hi);
  EEPROM.put(dir + 4, tpersdia);
  EEPROM.put(dir + 5, tpersnoche);
  EEPROM.put(dir + 6, hpers);
  EEPROM.put(dir + 7, horadia); //flotante (4 bytes)
  EEPROM.put(dir + 11, horanoche); //flotante (4 bytes)
  EEPROM.put(dir + 15, horadiapers);
  EEPROM.put(dir + 16, mindiapers); //flotante (4 bytes)
  EEPROM.put(dir + 20, horanochepers);
  EEPROM.put(dir + 21, minnochepers); //flotante (4 bytes)
  EEPROM.put(dir + 25, anoinicio); //entero (2 bytes)
  EEPROM.put(dir + 27, mesinicio);
  EEPROM.put(dir + 28, diainicio);
  EEPROM.put(dir + 29, horainicio);
  EEPROM.put(dir + 30, minutoinicio);
  EEPROM.put(dir + 31, nuevorecambio);
  EEPROM.put(dir + 32, recambio);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------------------------ACTIVACIÓN DE ACTUADORES----------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

void accion() {
  //Accion de las electrovalvulas
  switch (riego) {
    case "hidroponia":
      if (flot == 0 && digitalRead(evs) == 1) digitalWrite(eve, 0);
      else digitalWrite(eve, 1);

      if (transcurrido >= recambio) {
        digitalWrite(evs, 0);
        if (y = 0) {
          y++;
          abre = millis();
        }
        cierra = millis();
        if (cierra - abre > descarga) {
          digitalWrite(evs, 1);
          y = 0;
          recambio = recambio + nuevorecambio;
        }
      }
     break;
     case "goteo":
      if (transcurrido >= recambio) {
        digitalWrite(eve, 0);
        if (y = 0) {
          y++;
          abre = millis();
        }
        cierra = millis();
        if (cierra - abre > descarga) {
          digitalWrite(eve, 1);
          y = 0;
          recambio = recambio + nuevorecambio;
        }
      }      
  }

  //Accion del sensor de luz segun el horario
  if (horaactual > horadia && horaactual < horanoche && luz < 75) digitalWrite(lampara, 0);
  else digitalWrite(lampara, 1);

  accionactual = millis();
  if (accionactual - accionanterior > rele) {
    accionanterior = accionactual;

    //Acción del sensor de temperatura y humedad
    if (temp > ti + 1 && hum > hi + 3) { //temperatura y humedad altas
      digitalWrite(ventilador, 0);
      digitalWrite(extractor, 0);
      digitalWrite(calefactor, 1);
      digitalWrite(aspersor, 1);
    }
    if (temp > ti + 1 && hi - 3 <= hum && hum <= hi + 3) { //temperatura alta y humedad normal
      digitalWrite(ventilador, 0);
      digitalWrite(extractor, 1);
      digitalWrite(calefactor, 1);
      digitalWrite(aspersor, 1);
    }
    if (temp > ti + 1 && hi - 3 > hum) { //temperatura alta y humedad baja
      digitalWrite(ventilador, 1);
      digitalWrite(extractor, 1);
      digitalWrite(calefactor, 1);
      digitalWrite(aspersor, 0);
    }
    if (ti - 1 <= temp && temp < ti + 1 && hum > hi + 3) { //temperatura normal y humedad alta
      digitalWrite(ventilador, 1);
      digitalWrite(extractor, 0);
      digitalWrite(calefactor, 1);
      digitalWrite(aspersor, 1);
    }
    if (ti - 1 <= temp && temp <= ti + 1 && hi - 3 <= hum && hum <= hi + 3) { //temperatura y humedad normal
      digitalWrite(ventilador, 1);
      digitalWrite(extractor, 1);
      digitalWrite(calefactor, 1);
      digitalWrite(aspersor, 1);
    }
    if (ti - 1 <= temp && temp <= ti + 1 && hi - 3 > hum) { //temperatura normal y humedad baja
      digitalWrite(ventilador, 1);
      digitalWrite(extractor, 1);
      digitalWrite(calefactor, 1);
      digitalWrite(aspersor, 0);
    }
    if (ti - 1 > temp && hum > hi + 3) { //temperatura baja y humedad alta
      digitalWrite(ventilador, 1);
      digitalWrite(extractor, 1);
      digitalWrite(calefactor, 0);
      digitalWrite(aspersor, 1);
    }
    if (ti - 1 > temp && hi - 3 <= hum && hum <= hi + 3) { //temperatura baja y humedad normal
      digitalWrite(ventilador, 1);
      digitalWrite(extractor, 1);
      digitalWrite(calefactor, 0);
      digitalWrite(aspersor, 1);
    }
    if (ti - 1 > temp && hi - 3 > hum) { //temperatura y humedad baja
      digitalWrite(ventilador, 1);
      digitalWrite(extractor, 1);
      digitalWrite(calefactor, 0);
      digitalWrite(aspersor, 0);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------------------------PARAMETROS DEL EXTERIOR----------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

//Enviar valores por el puerto serie
void serie() {
  envioactual = millis();
  if (envioactual - envioanterior > envio) {
    envioanterior = envioactual;

    if (dia < 10) Serial.print("0");
    Serial.print(dia);
    Serial.print("/");
    if (mes < 10) Serial.print("0");
    Serial.print(mes);
    Serial.print("/");
    Serial.print(ano);
    Serial.print(" ");
    if (hora < 10) Serial.print("0");
    Serial.print(hora);
    Serial.print(":");
    if (minuto < 10) Serial.print("0");
    Serial.print(minuto);
    Serial.print(" - Transcurrido: ");
    Serial.print(transcurrido);
    Serial.print(", Recambio: ");
    Serial.print(recambio);
    Serial.print(" - Amanecer:");
    if (horadiapers < 10) Serial.print("0");
    Serial.print(horadiapers);
    Serial.print(":");
    if (mindiapers < 10) Serial.print("0");
    Serial.print(mindiapers, 0);
    Serial.print(", Anochecer:");
    if (horanochepers < 10) Serial.print("0");
    Serial.print(horanochepers);
    Serial.print(":");
    if (minnochepers < 10) Serial.print("0");
    Serial.println(minnochepers, 0);

    Serial.print("                   Temperatura: ");
    Serial.print(tn);
    Serial.print(", Temp.ideal: ");
    if (horaactual > horadia && horaactual < horanoche) Serial.print(tidia);
    else Serial.print (tinoche);
    Serial.print(" - Humedad: ");
    Serial.print(hn);
    Serial.print(", Hum.ideal: ");
    Serial.print(hi);
    Serial.print(" - Luz: ");
    if (luz < 10) lcd.print("0");
    Serial.print(luz, 0);
    Serial.print("% - Flotante: ");
    Serial.println(flot);

    Serial.print("                   Lampara: ");
    Serial.print(digitalRead(lampara));
    Serial.print(", Ventilador: ");
    Serial.print(digitalRead(ventilador));
    Serial.print(", Extractor:");
    Serial.print(digitalRead(extractor));
    Serial.print(", Calefactor: ");
    Serial.print(digitalRead(calefactor));
    Serial.print(", Aspersor: ");
    Serial.print(digitalRead(aspersor));
    Serial.print(", EV entrada: ");
    Serial.print(digitalRead(eve));
    Serial.print(", EV salida: ");
    Serial.println(digitalRead(evs));
  }
}

//envio de datos al NodeMCU
void envia() {
  Wire.write(segundo); //byte 1
  Wire.write(tn); //byte 1
  Wire.write(ti); //byte 1
  Wire.write(hn); //byte 1
  Wire.write(hi); //byte 1
  Wire.write(ln); //byte 1
  Wire.write(flot); //byte 1
  Wire.write(transcurrido); //byte 1
  Wire.write(recambio); //byte 1
}

//recepcion de datos del NodeMCU
void recibe (int cuantos) {
  while (Wire.available()) {
    canal = Wire.read();
    switch (canal) {
      case 9: //temperatura ideal diurna
        tidia = Wire.read();
        tpersdia = tidia;
        EEPROM.put(dir + 1, tidia);
        EEPROM.put(dir + 4, tpersdia);
        Serial.print ("Temperatura ideal diurna modificada: ");
        Serial.print (tidia);
        Serial.println ("°C");
        break;
      case 10: //temperatura ideal nocturna
        tinoche = Wire.read();
        tpersnoche = tinoche;
        EEPROM.put(dir + 2, tinoche);
        EEPROM.put(dir + 5, tpersnoche);
        Serial.print ("Temperatura ideal nocturna modificada: ");
        Serial.print (tinoche);
        Serial.println ("°C");
        break;
      case 11: //humedad ideal
        hi = Wire.read();
        Serial.print ("Humedad ideal modificada: ");
        Serial.print (hi);
        Serial.println ("%");
        EEPROM.put(dir + 3, hi);
        break;
      case 12: //Intervalo de recambio
        nuevorecambio = Wire.read();
        recambio = nuevorecambio;
        Serial.print ("Intervalo de recambio de agua modificado ");
        Serial.print (nuevorecambio);
        Serial.println (" dias");
        EEPROM.put(dir + 31, nuevorecambio);
        EEPROM.put(dir + 32, recambio);
        break;
      case 13: //Hora del amanecer
        horadiapers = Wire.read();
        Serial.print ("Hora del amanecer modificada ");
        Serial.print (horadiapers);
        Serial.println ("hs");
        horadia = horadiapers + mindiapers / 60.0;
        EEPROM.put(dir + 7, horadia); //flotante (4 bytes)
        EEPROM.put(dir + 15, horadiapers);
        break;
      case 14: //Minutos del amanecer
        mindiapers = Wire.read();
        Serial.print ("Minutos del amanecer modificados ");
        Serial.print (mindiapers);
        Serial.println ("min.");
        horadia = horadiapers + mindiapers / 60.0;
        EEPROM.put(dir + 7, horadia); //flotante (4 bytes)
        EEPROM.put(dir + 16, mindiapers); //flotante (4 bytes)
        break;
      case 15: //hora del anochecer
        horanochepers = Wire.read();
        Serial.print ("Hora del anochecer modificada ");
        Serial.print (horanochepers);
        Serial.println ("hs");
        horanoche = horanochepers + minnochepers / 60.0;
        EEPROM.put(dir + 11, horanoche); //flotante (4 bytes)
        EEPROM.put(dir + 20, horanochepers);
        break;
      case 16: //minutos del anochecer
        minnochepers = Wire.read();
        Serial.print ("Minutos del anochecer modificados ");
        Serial.print (minnochepers);
        Serial.println ("min.");
        horanoche = horanochepers + minnochepers / 60.0;
        EEPROM.put(dir + 11, horanoche); //flotante (4 bytes)
        EEPROM.put(dir + 21, minnochepers); //flotante (4 bytes)
        break;
    }

  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------------------MOVERSE ENTRE DIFERENTES PANTALLAS------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

void pantalla() {
  switch (leer_botones_LCD ()) {
    case ABA:
      menu++;
      if (menu > maxmenu) menu = 1;
      break;
    case ARR:
      menu--;
      if (menu < 1) menu = maxmenu;
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------SELECCIONAR ITEMS DENTRO DE UNA PANTALLA--------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

int subebaja() {
  switch (leer_botones_LCD()) {
    case ARR:
      param++;
      if (param > maxparam) {
        if (maxparam == 59 || maxparam == 23) param = 0;
        else param = 1;
      }
      break;
    case ABA:
      param--;
      if (maxparam == 59 || maxparam == 23) {
        if (param < 0) param = maxparam;
      }
      else {
        if (param < 1) param = maxparam;
      }
      break;
  }
  return param;
}

void lateral() {
  switch (leer_botones_LCD()) {
    case DER:
      orden++;
      if (orden > maxorden) orden = 1;
      break;
    case IZQ:
      orden--;
      if (orden < 1) orden = maxorden;
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------------------------CONOCER LOS DIAS QUE TRAE UN MES-----------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

byte diasmes() {
  if (tipomes == 1 || tipomes == 3 || tipomes == 5 || tipomes == 7 || tipomes == 8 || tipomes == 10 || tipomes == 12 ) return 31;
  if (tipomes == 4 || tipomes == 6 || tipomes == 9 || tipomes == 11) return 30;
  else {
    if (tipoano / 4 == 0 && (tipoano / 100 != 0 || tipoano / 400 == 0)) return 29;
    else return 28;
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------------------SELECCION DE VALORES SEGUN PLANTACION---------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

void cualplanta() {
  switch (x) {
    case 0:
      tidia = tpersdia;
      tinoche = tpersnoche;
      hi = hpers;
      nuevorecambio = rectom;
      horadiapers = 6;
      mindiapers = 30;
      horanochepers = 22;
      minnochepers = 30;
      break;
    case 1:
      tidia = tlechdia;
      tinoche = tlechnoche;
      hi = hlech;
      nuevorecambio = rectom;
      horadiapers = 6;
      mindiapers = 30;
      horanochepers = 22;
      minnochepers = 30;

      break;
    case 2:
      tidia = ttomdia;
      tinoche = ttomnoche;
      hi = htom;
      nuevorecambio = rectom;
      horadiapers = 6;
      mindiapers = 30;
      horanochepers = 22;
      minnochepers = 30;
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------------------------PARPADEO DE PANTALLA AL MODIFICAR----------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

void parpadeo() {
  tiempoactual = millis();
  if (tiempoactual - tiempoanterior > intervalo) {
    tiempoanterior = tiempoactual;
    lcd.setCursor(columna, fila);
    if (menu == 1 && orden == 3) lcd.print("    ");
    if (menu == 7) lcd.print(" ");
    if (menu == 8) lcd.print("                ");
    else lcd.print("  ");
  }
  else {
    lcd.setCursor(columna, fila);
    if (menu == 7) {
      if (param < 10) {
        lcd.print("  ");
        lcd.print(param);
      }
      else {
        if (param < 100 && param > 9) {
          lcd.print(" ");
          lcd.print(param);
        }
        else lcd.print(param);
      }
    }
    else {
      if (menu == 8) lcd.print(planta[x]);
      else {
        if (param < 10) {
          lcd.print("0");
          lcd.print(param);
        }
        else lcd.print(param);
      }
    }
  }
}
