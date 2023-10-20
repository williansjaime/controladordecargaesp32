#include <Arduino.h>
#include <WiFi.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "NTPClient.h" 
#include <string.h>

#define WORD_LENGTH 64

const char *ssid = "nome";
const char *password = "senha";


int LED_BUILTIN = 23;
int releVentoinha = 12;

int analogPinTensao = 35;
int analogPinTensaoPainel = 39;
int analogPinTensaoBateria = 36;
int analogPinTensaoBatLition = 34;


int analogPinTemperatura = 32; 
int analogPinCorrentePainel = 33;

const double bateriaGrandeFlut = 14.4;
const double batLitio = 14.4;

bool wificonecte = false;
bool arquivo_read = false;

int mVperAmp = 100;           // this the 5A version of the ACS712 -use 100 for 20A Module and 66 for 30A Module
int Watt = 0;
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;

double Calcula_corrente();
float Calcula_Tensao(int valor);
float Calcula_Temperatura();
float calcularCorrenteEsp32(int pino_sensor);
float Calcula_Temperatura(int pino_input_temperatura);
float getVPP(int sensorIn);
float Calcula_corrente(int pinos);

WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP);


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
    return;
  }
  if(file.print(message)){
    Serial.println("mensagem escrita com sucesso");
    return;
  } 
  file.close();
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


void setup(){
  Serial.begin(9600);  
  pinMode(LED_BUILTIN, OUTPUT); 
  pinMode(analogPinTensao,INPUT);
  pinMode(analogPinTemperatura,INPUT);
  pinMode(analogPinTensaoPainel,INPUT);
  pinMode(analogPinTensaoBateria,INPUT);
  pinMode(analogPinCorrentePainel,INPUT);
  pinMode(analogPinTensaoBatLition,INPUT);
  
  wificonecte = ConectarWifi();
  if(wificonecte){Serial.println("Wifi ok");}
  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    return;
  }else{
    Serial.println("Card Pronto para uso :)");
  }
    
}

void loop()
{  
 
  AmpsRMS = Calcula_corrente(analogPinCorrentePainel)/100;
  if(AmpsRMS>0 && AmpsRMS > 0.06){
    Watt = (AmpsRMS*Calcula_Tensao(analogRead(analogPinTensao))/1.2);
    String datahoraDados;
    String nomeArquivo = "";
    if(wificonecte)
    {
      ntp.begin(); //GMT em segundos // +1 = 3600 // +8 = 28800// -1 = -3600// -3 = -10800 (BRASIL)
      ntp.setTimeOffset(-10800);     
      if (ntp.update()) {
          String str = ntp.getFormattedDate();
          for(int i = 0; i<10;i++){nomeArquivo +=str[i];}
          nomeArquivo = "/dadossolar"+nomeArquivo+".txt";
          if((ntp.getMinutes()%10) == 0)
          {
            datahoraDados = ntp.getFormattedDate();
            datahoraDados = datahoraDados +"-Temperatura="+(String)Calcula_Temperatura(analogPinTemperatura);
            datahoraDados = datahoraDados +"/Corrente="+(String)AmpsRMS;
            datahoraDados = datahoraDados +"/Watt="+(String)Watt;
            datahoraDados = datahoraDados +"/Tensao01="+(String)Calcula_Tensao(analogRead(analogPinTensaoBateria));
            datahoraDados = datahoraDados +"/Tensao02="+(String)Calcula_Tensao(analogRead(analogPinTensaoPainel));
            datahoraDados = datahoraDados +"/Tensao03="+(String)Calcula_Tensao(analogRead(analogPinTensaoBatLition));
            datahoraDados = datahoraDados +"/Tensao04="+(String)Calcula_Tensao(analogRead(analogPinTensao))+"\n";
            Serial.println(datahoraDados);
            if (SD.begin()) { 
              //appendFile(SD, nomeArquivo, datahoraDados);  
              arquivo_read = false;             
            }
          }
        }else{
          Serial.println("erro");
        } 
    }else{
          Serial.println("erro no wifi");
        }
    /*if (Serial.available() > 0 && arquivo_read == false){
      if (Serial.read() == 116){
        readFile(SD, nomeArquivo);
        Serial.println("Acabou");
        arquivo_read = true;        
      }      
    }*/
  }
  delay(1000);
    
}
 
//Testado e calibrado
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

