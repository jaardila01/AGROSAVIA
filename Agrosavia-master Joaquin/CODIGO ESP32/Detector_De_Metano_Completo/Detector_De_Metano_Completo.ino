#include <WiFi.h>
#include <WiFiUdp.h>
#include "max6675.h"
#include <Wire.h>
#include <MPUandes.h>


#define LED0 2
#define LED1 15
#define RXD2 16
#define TXD2 17
#define CONFIG_TCSCK_PIN      18
#define CONFIG_TCCS_PIN       13
#define CONFIG_TCDO_PIN       19

#define PUMP 5
#define Flow_Pin 34
#define Methane_Pin 35

double Flow = 0.0;
double Methane = 0.0;


const char* ssid = "Agrosavia2.4G";
const char* password = "Agrosavia";

MAX6675 thermocouple(CONFIG_TCSCK_PIN, CONFIG_TCCS_PIN, CONFIG_TCDO_PIN);

int Id_client = 1; //Identificación del cliente  Id_client=[1,2,3,4,5]
int p = 0; //Indicador de cada dato (paquete) enviado
int contconexion = 0;
double Temp = 0.0;
WiFiUDP     Udp;


const int scl = 22;
const int sda = 21;

int AccelScaleFactor = 2048;
int GyroScaleFactor = 131;

MPUandes acelerometro = MPUandes(sda, scl);



void setup()
{
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  digitalWrite(LED0, 1);
  Serial.begin(115200);
  Wire.begin(sda, scl);
  delay(100);
  acelerometro.MPU6050_Init();
  delay(2000);
  // Setting The Mode Of Pins

  pinMode(Flow_Pin, INPUT);
  pinMode(Methane_Pin, INPUT);
  pinMode(PUMP, OUTPUT);

  WiFi.mode(WIFI_STA); //para que no inicie el SoftAP en el modo normal
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED and contconexion < 50) //Cuenta hasta 50 si no se puede conectar lo cancela
  {
    ++contconexion;
    delay(250);
    Serial.print(".");
  }
  if (contconexion < 50)
  {
    //Se está usando el código con IP dinámica por problemas con la ip fija. (funciona igualmente bien con IP dinámica)
    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.gatewayIP());
  }
  else
  {
    Serial.println("");
    Serial.println("Error de conexion");
  }
  Serial2.begin(38400, SERIAL_8N2, RXD2, TXD2);
  delay(1000);
  Serial2.setTimeout(10);
  Serial2.print("[A]");
  Serial.println("Modo normal activado");
  delay(1000);
  digitalWrite(PUMP, 1);
  digitalWrite(LED0, 0);
}


int Ax1, Ay1, Az1, Gx1, Gy1, Gz1;
double Ax, Ay, Az, Gx, Gy, Gz;
String msg = "";
String ppm;
String ppm_CH4 = "00000000";
long t = 0.0;
long number = 0;


void loop()
{
  if (Serial2.available() >= 0)
  {
    if (Serial2.readStringUntil('\n') == "0000005b")
    {
      ppm = Serial2.readStringUntil('\n');
      ppm_CH4 = ppm;
      number = (int) strtol(&ppm_CH4[1], NULL, 16);
      Serial.println(number);
    }
    else
    {
      acel();
      transmitir();
    }
  }
}


void acel()
{
  Ax1 = (int)acelerometro.Read_Ax();
  Ay1 = (int)acelerometro.Read_Ay();
  Az1 = (int)acelerometro.Read_Az();
  Gx1 = (int)acelerometro.Read_Gx();
  Gy1 = (int)acelerometro.Read_Gy();
  Gz1 = (int)acelerometro.Read_Gz();
  //divide each with their sensitivity scale factor
  Ax = (double)Ax1 / AccelScaleFactor;
  Ay = (double)Ay1 / AccelScaleFactor;
  Az = (double)Az1 / AccelScaleFactor;
  Gx = (double)Gx1 / GyroScaleFactor;
  Gy = (double)Gy1 / GyroScaleFactor;
  Gz = (double)Gz1 / GyroScaleFactor;
}


void transmitir()
{
  digitalWrite(LED0, 1);
  Udp.beginPacket("192.168.0.115", 9001);  //// Esta ip es la ip del computador servidor y el puerto debe coincidir

  //digitalWrite(LED1, HIGH);
  msg = String(Ax) + "#" + String(Ay) + "#" + String(Az) + "#" + String(Gx) + "#" + String(Gy) + "#" + String(Gz) + "#" + String(Id_client) + "#" + String(p) + "#" + String(number); //El mensaje completo contiene el id del cliente y el numero de paquete enviado

  for (int i = 0; i < msg.length(); i++)
  {
    int old = micros();
    Udp.write(msg[i]);
    while (micros() - old < 87);
  }

  Udp.endPacket();
  t = millis();
  p = p + 1;
  Serial.println(msg);
  digitalWrite(LED0, 0);
}
