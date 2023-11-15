#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "NTPClient.h" 
#include <string.h>

#define WORD_LENGTH 64
WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP);

const char *ssid = "nomewifi";
const char *password = "senhawifi";
const char* mqtt_server = "ipmaquina";
const int mqtt_port = 1883; 
const char* mqtt_user = "ESP32SOLAR";
const char* mqtt_password = "senhatopico";

String datahoraDados= "";
String nomeArquivo = "";

int LED_BUILTIN = 23;
int releVentoinha = 12;

int analogPinTensao = 36;
int analogPinTensaoPainel = 39;
int analogPinTensaoBateria = 35;
int analogPinTensaoBatLition = 34;

//Sensor de temperatura
int analogPinTemperatura = 32; 
int analogPinCorrentePainel = 33;

const double bateriaGrandeFlut = 14.4;
const double batLitio = 14.4;

bool wifi_status = false;
bool sd_status = false;
bool mqtt_status = false;
bool date_atualizado = false;
bool arquivo_read = false;

int mVperAmp = 100;           // this the 5A version of the ACS712 -use 100 for 20A Module and 66 for 30A Module
int Watt = 0;
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;
double tensaoPainel = 0;

double Calcula_corrente();
float Calcula_Tensao(int valor);
float Calcula_Temperatura();
float calcularCorrenteEsp32(int pino_sensor);
float Calcula_Temperatura(int pino_input_temperatura);
float getVPP(int sensorIn);
float Calcula_corrente(int pinos);
bool ConectarWifi();
void reconnect();
void appendFile(fs::FS &fs, String path, String message);


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.print(topic);
  Serial.print(". Conteúdo: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

bool setDataTimeESP32(){
  ntp.begin();
  ntp.setTimeOffset(-10800);     
  if (ntp.update()) {
      String str = ntp.getFormattedDate();
      for(int i = 0; i<10;i++){nomeArquivo +=str[i];}
      nomeArquivo = "";
      nomeArquivo = "/dadossolar"+nomeArquivo+".txt";
      return true;
  }
  return false;
}

void realizarLeiturasSistema(){
  AmpsRMS = Calcula_corrente(analogPinCorrentePainel)/100;
  tensaoPainel = Calcula_Tensao(analogRead(analogPinTensaoPainel));
  if(AmpsRMS>0 && AmpsRMS > 0.06 && wifi_status && date_atualizado){ // 
    Watt = (AmpsRMS*tensaoPainel/1.2);    
    if((ntp.getMinutes()%5) == 0)
    {
      datahoraDados= "";
      datahoraDados = ntp.getFormattedDate();
      datahoraDados = "{\'DataHora\':\'"+(String)datahoraDados +"\',\'Temperatura\':"+(String)Calcula_Temperatura(analogPinTemperatura);
      datahoraDados = datahoraDados +",\'Corrente\':"+(String)AmpsRMS;
      datahoraDados = datahoraDados +",\'Watt\':"+(String)Watt;
      datahoraDados = datahoraDados +",\'tensaoPainel\':"+(String)Calcula_Tensao(analogRead(analogPinTensaoPainel));
      datahoraDados = datahoraDados +",\'tensaoBatChumbo\':"+(String)Calcula_Tensao(analogRead(analogPinTensaoBateria));
      datahoraDados = datahoraDados +",\'tensaoBatLition\':"+(String)Calcula_Tensao(analogRead(analogPinTensaoBatLition));
      datahoraDados = datahoraDados +",\'tensaoSistema\':"+(String)Calcula_Tensao(analogRead(analogPinTensao))+"}\n";
      if (!client.connected()) {
        reconnect();
        mqtt_status =true;
      }
      if(mqtt_status) client.publish("/teste", datahoraDados.c_str());
      
      if (sd_status) { 
        appendFile(SD, nomeArquivo, datahoraDados);  
        arquivo_read = false;             
      }
    }
  }else{
    wifi_status = ConectarWifi();
    sd_status = SD.begin(5);
    date_atualizado = setDataTimeESP32();
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT); 
  pinMode(analogPinTensao,INPUT);
  pinMode(analogPinTemperatura,INPUT);
  pinMode(analogPinTensaoPainel,INPUT);
  pinMode(analogPinTensaoBateria,INPUT);
  pinMode(analogPinCorrentePainel,INPUT);
  pinMode(analogPinTensaoBatLition,INPUT);
  
  wifi_status = ConectarWifi();
  sd_status = SD.begin(5);
  date_atualizado = setDataTimeESP32();
    
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  mqtt_status = client.connect("ESP32Client", mqtt_user, mqtt_password);
  if(!client.connect("ESP32Client", mqtt_user, mqtt_password)){
    Serial.println("Erro ao conectar ao mqtt");
    //ESP.restart();
  }
  
  if(wifi_status)Serial.println("Wifi ok!!");
  if(mqtt_status)Serial.println("mqtt ok!!");
  if(date_atualizado)Serial.println("DataHora ok!!");

  
}

void loop() {
  //client.setCallback(callback);
  realizarLeiturasSistema();    
  delay(5000); 
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao servidor MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("Conectado!");
      client.subscribe("/teste");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

bool ConectarWifi(){
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
        delay(100);
    }
    return true;
}

void appendFile(fs::FS &fs, String path, String message)
{
  File file = fs.open(path, FILE_APPEND);
  if(!file)
  {
    file.close();
    return;
  }
  if(file.print(message)){
    file.close();
    return;
  } 
  
}

void readFile(fs::FS &fs, String path)
{
  File file = fs.open(path);
  if(!file){
    return;
  }
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}


float Calcula_Tensao(int leituraAnalogica)
{
    const float vcc = 3.30; // Tensão de referência da ESP32 em Volts
    const int resolucaoADC = 4096; // Resolução do ADC (12 bits)
    const float R1 = 100000.0; // Resistência R1 em ohms (100K)
    const float R2 = 10000.0; // Resistência R2 em ohms (10K)
    float vout = ((leituraAnalogica+117) / (float)resolucaoADC) * vcc;
    float valorTensao = vout / (R2 / (R1 + R2));
    valorTensao += 0.42;
    
    if (leituraAnalogica == 0) {
      valorTensao = 0.0;
    }
    return valorTensao;
}

//Testa
float Calcula_Temperatura(int pino_input_temperatura)
{    
    const double vcc = 3.30;
    const int resolucaoADC = 4096;
    const double R = 10000.0;
    const double beta = 3600.0;
    const double r0 = 10000.0;
    const double t0 = 273.0 + 25.0;
    const int nAmostras = 5.2;
    const double rx = r0 * exp(-beta / t0);
    int  soma = 0;
    for (int i = 0; i < nAmostras; i++)
    {
        soma += analogRead(pino_input_temperatura);
        delay(10);
    }
    // Determina a resistência do termistor
    double v = (vcc * soma) / (nAmostras * resolucaoADC);
    double rt = (vcc * R) / v - R;

    // Calcula a temperatura
    double t = beta / log(rt / rx);
    double ReturnT = t - 273.0;
    return ReturnT;
}


//unico que chegou perto da medida do multimetro
float Calcula_corrente(int pinos)
{
    int mVperAmp = 100; 
    int RawValue = 0;
    int ACSoffset =0;
    double Voltage = 0;
    double Amps = 0;

    RawValue = analogRead(pinos);
    //Voltage = (RawValue / 4096)*3300; // Gets you mV
    Amps = (RawValue-1890); //Amps = ((Voltage - ACSoffset) / mVperAmp);
    return Amps;
}
