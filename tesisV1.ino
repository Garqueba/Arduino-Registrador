#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

File dataFile; 
File logFile;
RTC_DS1307 rtc;
boolean microSD;
int RTDentrada;      // variable que almacena el valor (0 a 1023)
int RTDsalida;      // variable que almacena el valor (0 a 1023)
float Temperatura1,Temperatura2;        // variable que almacena la temperatura despues de la conversion
//Asignar direccion fisica
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// Fijar IP del arduino
IPAddress ip(192, 168, 1, 5); 
//Fijar DNS
IPAddress myDns(192,168,1,4);
// Inicializa la instancia client
EthernetClient client;
// Direccion del servidor
char server[] = "192.168.1.4";
// Variable de tiempo de la ultima conexion en milisegundos
unsigned long ultimaConexion1 = 0, ultimaConexion2 = 0;          
// Estado de la ultima conexion
boolean ultimoEstado = false;
// Intervalo en milisegundos entre conexiones
const unsigned long intervaloConexion1 = 1000, intervaloConexion2 = 300000, intervaloConexion3=300000;

void setup() {
  // Inicializa puerto serial
  Serial.begin(9600);
  Serial.println(F("Registrador"));
  // Espera 1 segundo para que se inicie la tarjeta Ethernet
  delay(1000);
  Ethernet.begin(mac, ip, myDns);
  if (!SD.begin(4)){
    Serial.println(F("Error al iniciar"));
    return;
  }
    if (!rtc.begin()) {
    Serial.println(F("Error RTC"));
    while (1);
  }
  // Si se ha perdido la corriente, fijar fecha y hora
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));    
  } 
}
// Loop principal
void loop() {
 RTDentrada = analogRead(0);        // realizar la lectura
 RTDsalida = analogRead(1);        // realizar la lectura
 Temperatura1 = Temp(RTDentrada);   // cambiar escala a 0 - 110
 Temperatura2 = Temp(RTDsalida);   // cambiar escala a 0.0 - 110

  // Si hay datos que llegan por la conexion los envia a la puerta serial
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  // Si no hay conexion de red y se conecto correctamente la ultima vez
  // detiene el cliente Ehternet
  if (!client.connected() && ultimoEstado) {
    Serial.println(F("Desconectando..."));
    client.stop();
  }
  dataFile=SD.open("datalog.txt");
  if(dataFile){
    conexionHTTP3();
  }
  dataFile.close();
  // Si no esta conectado y han pasado X segundos (intervaloConexion) 
  // despues de la ultima conexion envia datos al servidor
  if(!client.connected() && (millis() - ultimaConexion1 > intervaloConexion1)) {
    conexionHTTP1();
  }
   if(!client.connected() && (millis() - ultimaConexion2 > intervaloConexion2)) {
    conexionHTTP2();
  }

  if(!client.available() && (millis()-ultimaConexion1 > intervaloConexion3)){
    Serial.println(F("escribir SD"));
    DateTime now = rtc.now();
    logFile = SD.open("datalog.txt", FILE_WRITE);
    logValue(now, Temperatura1,Temperatura2);
    logFile.close();
  }
  
  // Actualiza la variable ultimoEstado 
  ultimoEstado = client.connected();
}
// Fin del loop principal
// Realiza la conexion http al servidor
void conexionHTTP1() {
  // Se conecta al servidor en el puerto 80 (web)
  if (client.connect(server, 80)) {
    client.print("GET /Principal/actual.php?valor1=");
    client.print(Temperatura1);
    client.print("&valor2=");
    client.print(Temperatura2);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("User-Agent: Arduino-Ethernet");
    client.println("Connection: close");
    client.println();
    // Actualiza el tiempo en milisegundos de la ultima conexion
    ultimaConexion1 = millis();
  } 
  else {
    // Si la conexion fallo se desconecta
    Serial.println(F("Error al conectarse al servidor"));
    client.stop();
  }
}
void conexionHTTP2() {
  // Se conecta al servidor en el puerto 80 (web)
  if (client.connect(server, 80)) {
    client.print("GET /Principal/arduino.php?valor1=");
    client.print(Temperatura1);
    client.print("&valor2=");
    client.print(Temperatura2);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("User-Agent: Arduino-Ethernet");
    client.println("Connection: close");
    client.println();
    // Actualiza el tiempo en milisegundos de la ultima conexion
    ultimaConexion2 = millis();
  }
  else {
    client.stop();
  }
}
void logValue(DateTime date, float value1, float value2){
  Serial.println("escribe");
  logFile.print("yy=");
  logFile.print(date.year(), DEC);
  logFile.print("&mm=");
  logFile.print(date.month(), DEC);
  logFile.print("&dd=");
  logFile.print(date.day(), DEC);
  logFile.print("&h=");
  logFile.print(date.hour(), DEC);
  logFile.print("&m=");
  logFile.print(date.minute(), DEC);
  logFile.print("&s=");
  logFile.print(date.second(), DEC);
  logFile.print("&pro=");
  logFile.print(value1);
  logFile.print("&ag=");
  logFile.print(value2);
  logFile.print('/');
  ultimaConexion1=millis();
}

  // if the file is available, write to it:
void conexionHTTP3(){

  // if the file is available, write to it:
   String var = "";
    while (dataFile.available()&&client.connect(server, 80)) {
          Serial.println("while ");
       if (!client.connected() && ultimoEstado) {
          Serial.println(F("Desconectando..."));
          client.stop();
        }
      char d=dataFile.read();

      if(d!='/'){ 
      var+=d;
      }

      if(d=='/'){
        Serial.println(var);
        client.print("GET /Principal/microsd.php?");
        client.print(var);
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(server);
        client.println("User-Agent: Arduino-Ethernet");
        client.println("Connection: close");
        client.println();
        var = "";
      }
      client.stop();
      ultimoEstado = client.connected();
      if (!dataFile.available()){
        dataFile.close();
        SD.remove("datalog.txt");
      }
    }
    
}
// cambio de escala entre floats
float Temp(float x){
  return (x) * (100) / (1023)+10;
}

