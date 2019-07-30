//Version del programa 0.8.4

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------------------------LIBRERÍAS UTILIZADAS--------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

#include <Wire.h>
#include <CayenneMQTTESP8266.h>
#include <NTPClient.h>
#include <WiFiUDP.h>
#include "RTClib.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------PARÁMETROS ------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

#define CAYENNE_PRINT Serial
#define nanodir 11
#define longitud 9
#define intervalo 2000
#define seguro 5000
#define resetear 0
#define zona -3 //zona horaria del lugar donde vayamos a instalar la planta
#define actualizahora 15 //tiempo en segundos para actualizar con el servidor NTP

//Informacion de la red Wi-Fi
char ssid[] =  "NombreDeLaRed";
char contrasenawifi[] = "Password";

//Informacion de autentificacion con Cayenne (obtenidos al solicitar un nuevo dispositivo)
char usuario[] = "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX";
char contrasena[] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
char cliente[] = "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX";

//Informacion de cliente NTP para actualizar la hora
const unsigned long corrigezona = zona * 60 * 60;
WiFiUDP ntpudp;
NTPClient horario (ntpudp, "time.google.com", corrigezona, actualizahora);
String fechaactual;
int ano;
byte mes;
byte dia;
byte hora;
byte minuto;
byte segundo;


//Reloj
RTC_DS3231 rtc;

//contadores y banderas para activar accionadores
unsigned long actual;
unsigned long anterior = 0;
byte envioanterior = 0;
byte envioanterior2 = 0;
byte i;
byte j;
byte envio[longitud + 1];
byte fin;

// Variables de entrada
int canal;
int dato;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------------CONFIGURACIÓN DE PINES Y ESTADOS DE LA PLACA-------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

void setup() {
  //resetear el arduino en caso de quedar congelado
  pinMode(resetear, OUTPUT);
  digitalWrite(resetear, 1);

  // Inicia la conexion con el Arduino
  Wire.begin();

  // Inicia Serial
  Serial.begin(115200);

  //Inicia la conexion a internet y a la cola de mensajes de Cayenne
  Cayenne.begin(usuario, contrasena, cliente, ssid, contrasenawifi);

  // Inicia la conecion con el servidor NTP
  horario.begin();

  Serial.println("");
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------------------------------FUNCION PRINCIPAL--------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

void loop() {
  Cayenne.loop();
  horario.update();
  actual = millis();
  if (actual - anterior > intervalo) {
    anterior = actual;
    actualizarfecha ();
    recepcion();
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------------------------INTERRUPCION AL RECIBIR DATOS--------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

void recepcion () {  //desde aqui empieza a recibir el string:
  i = 0;
  Wire.requestFrom(nanodir, longitud); //luego de pedir datos, se queda esperando la respuesta
  while (Wire.available()) {
    Serial.print("Canal ");
    for (i = 0 ; i < longitud ; i++) {
      envio[i] = Wire.read();
      Serial.print(i);
      Serial.print(": ");
      Serial.print(envio[i]);
      if (i < longitud - 1) Serial.print("; Canal ");
    }
    fin = Wire.read();
    Serial.println("");
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------ACTUALIZA LA FECHA CON EL SERVIDOR NTP---------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

void actualizarfecha () {
  fechaactual = horario.getFormattedDate();
  ano = String(fechaactual.substring(0, 4)).toInt();
  mes = String(fechaactual.substring(6, 7)).toInt();
  dia = String(fechaactual.substring(8, 10)).toInt();
  hora = String(fechaactual.substring(11, 13)).toInt();
  minuto = String(fechaactual.substring(14, 16)).toInt();
  segundo = String(fechaactual.substring(17, 19)).toInt();
  rtc.adjust (DateTime(ano, mes, dia, hora, minuto, segundo));
  Serial.print (ano);
  Serial.print ("-");
  if (mes < 10) Serial.print ("0");
  Serial.print (mes);
  Serial.print ("-");
  if (dia < 10) Serial.print ("0");
  Serial.print (dia);
  Serial.print (" ");
  if (hora < 10) Serial.print ("0");
  Serial.print (hora);
  Serial.print (":");
  if (minuto < 10) Serial.print ("0");
  Serial.print (minuto);
  Serial.print (":");
  if (segundo < 10) Serial.print ("0");
  Serial.println (segundo);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------RECEPCION Y ENVIO DE DATOS A CAYENNE-----------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//

CAYENNE_OUT_DEFAULT() {
  envioanterior2 = envioanterior;
  envioanterior = envio[0];
  j = 1;
  for (j = 0 ; j < longitud ; j++) {
    Cayenne.virtualWrite(j, envio[j]);
  }
}

CAYENNE_IN_DEFAULT() {
  canal = request.channel;
  dato = getValue.asInt();
  Wire.beginTransmission(nanodir); //inicia el envio de parametros al Arduino
  Wire.write(canal);  //el primer valor enviado es el numero del canal
  Wire.write(dato); //el segundo valor enviado es el dato concreto
  Wire.endTransmission(); //termina el envio de parametros al Arduino
  Serial.print ("Recibido el dato del canal ");
  Serial.print (canal);
  Serial.print (": ");
  Serial.println (dato);
}
