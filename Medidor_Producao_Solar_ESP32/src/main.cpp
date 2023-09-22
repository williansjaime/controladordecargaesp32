#include <Arduino.h>
#include <WiFi.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <NTPClient.h> 
#include <stdio.h>

#define WORD_LENGTH 64

const char *ssid = "nome_da_red";
const char *password = "Senha_da_rede";

int LED_BUILTIN = 23;
int releVentoinha = 12;

int analogInputTensao = 35;
int analogInputTemperatura = 33; // Conexão do termistor
int analogInputTensaoPainel = 39;
int analogInputTensaoBateria = 36;
int analogInputCorrentePainel = 32;
int analogInputTensaoBatLition = 34;



const double bateriaGrandeFlut = 14.4;
const double batLitio = 14.4;

char valorEntrada; 

//File myFile;

double Calcula_corrente();
float Calcula_Tensao(int valor);
float Calcula_Temperatura();
float calcularCorrenteEsp32(int pino_sensor);

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

void setup(){
  Serial.begin(9600);  
  pinMode(LED_BUILTIN, OUTPUT); 
  pinMode(analogInputTensao,INPUT);
  pinMode(analogInputTemperatura,INPUT);
  pinMode(analogInputTensaoPainel,INPUT);
  pinMode(analogInputTensaoBateria,INPUT);
  pinMode(analogInputCorrentePainel,INPUT);
  pinMode(analogInputTensaoBatLition,INPUT);
  
  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    return;
  }
  
}

void loop(){
  /*
    if(ConectarWifi()){
        ntp.begin();
        //GMT em segundos
        // +1 = 3600
        // +8 = 28800
        // -1 = -3600
        // -3 = -10800 (BRASIL)
        ntp.setTimeOffset(-10800);
     
        if (ntp.update()) {
            Serial.print("DATA/HORA: ");
            //Serial.println(ntp.getFormattedDate());

            Serial.print("HORARIO: ");
            Serial.println(ntp.getFormattedTime());

            Serial.print("HORA: ");
            Serial.println(ntp.getHours());

            Serial.print("MINUTOS: ");
            Serial.println(ntp.getMinutes());

            Serial.print("SEGUNDOS: ");
            Serial.println(ntp.getSeconds());

            Serial.print("DIA DA SEMANA (0=domingo): ");
            Serial.println(ntp.getDay());

            Serial.println();

        } else {
            Serial.println("!Erro ao atualizar NTP!\n");
        }
        delay(10000);
    }
     **/
    char InputCorrentePainel[WORD_LENGTH];
    sprintf(InputCorrentePainel, "%.2f", calcularCorrenteEsp32(analogInputCorrentePainel));
    Serial.println(InputCorrentePainel);
    char InputTensaoBateria[WORD_LENGTH];
    sprintf(InputTensaoBateria, "%.2f", Calcula_Tensao(analogRead(analogInputTensaoBateria)));
    Serial.println(InputTensaoBateria);
    char TensaoPainel[WORD_LENGTH];
    sprintf(TensaoPainel, "%.2f", Calcula_Tensao(analogRead(analogInputTensaoPainel)));
    Serial.println(TensaoPainel);
    char TensaoBatLition[WORD_LENGTH];    
    sprintf(TensaoBatLition, "%.2f", Calcula_Tensao(analogRead(analogInputTensaoBatLition)));
    Serial.println(TensaoBatLition);    
    delay(1000);
    
    /*Serial.println(InputCorrentePainel);
    Serial.print("InputTensaoBateria::%s",InputTensaoBateria);
    Serial.print("TensaoPainel::%s",TensaoPainel);*/

    /*
    calcularCorrenteEsp32(analogInputCorrentePainel);
    Serial.println("Tensao::");
    Serial.println(Calcula_Tensao(analogRead(analogInputTensao)));
    Serial.println("Temperatura::");
    Serial.println(Calcula_Temperatura(analogInputTemperatura));
    */

    /*
    if (SD.begin()) { 
        myFile = SD.open("dadosSolar.txt", FILE_WRITE); // Cria / Abre arquivo .txt
        if (myFile) { // Se o Arquivo abrir imprime:
            Serial.println("/TensaoBateria: "+a+"/TensaoPainel: "+b+"/TensaoBatLition: "+c+"/Calcula_corrente: "+d+"/Calcula_Temperatura: "+e+"/");
            myFile.println("/TensaoBateria: "+a+"/TensaoPainel: "+b+"/TensaoBatLition: "+c+"/Calcula_corrente: "+d+"/Calcula_Temperatura: "+e+"/"); 
            myFile.close(); 
        }else {     
            Serial.println("Erro cartao");
        } 
        myFile.close(); 
    }*/ 
}

//Testado
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
    // Parâmetros do termistor    
    const double vcc = 3.3;
    const int resolucaoADC = 4096;
    // Parâmetros do circuito
    const double R = 10000.0;
    const double beta = 3600.0;
    const double r0 = 10000.0;
    const double t0 = 273.0 + 25.0;
    const int nAmostras = 5.2;
    const double rx = r0 * exp(-beta / t0);
    int  soma = 0;
    for (int i = 0; i < nAmostras; i++)
    {
        soma += analogRead(analogInputTemperatura);
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

float calcularCorrenteEsp32(int pino_sensor){
  int menor_valor;
  int valor_lido;
  int menor_valor_acumulado = 0;
  int ZERO_SENSOR = 0;
  float corrente_pico;
  float corrente_eficaz;
  double maior_valor=0;
  double corrente_valor=0;
  
  menor_valor = 4095;
  
  for(int i = 0; i < 10000 ; i++){
    valor_lido = analogRead(pino_sensor);
    if(valor_lido < menor_valor){
      menor_valor = valor_lido;    
    }
    delayMicroseconds(1);  
  }
  ZERO_SENSOR = menor_valor;
  
  delay(3000);

  //Zerar valores
  menor_valor = 4095;
 
  for(int i = 0; i < 1600 ; i++){
    valor_lido = analogRead(pino_sensor);
    if(valor_lido < menor_valor){
      menor_valor = valor_lido;    
    }
    delayMicroseconds(10);  
  }

  //Transformar o maior valor em corrente de pico
  corrente_pico = ZERO_SENSOR - menor_valor; // Como o ZERO do sensor é 2,5 V, é preciso remover este OFFSET. Na leitura Analógica do ESp32 com este sensor, vale 2800 (igual a 2,5 V).
  corrente_pico = corrente_pico*0.805; // A resolução mínima de leitura para o ESp32 é de 0.8 mV por divisão. Isso transforma a leitura analógica em valor de tensão em [mV}
  corrente_pico = corrente_pico/185;   // COnverter o valor de tensão para corrente de acordo com o modelo do sensor. No meu caso, esta sensibilidade vale 185mV/A
                                      // O modelo dele é ACS712-05B. Logo, precisamos dividir o valor encontrado por 185 para realizar esta conversão                                       
 
  //Converter para corrente eficaz  
  corrente_eficaz = corrente_pico/1.4;
  /*Serial.print("Corrente Eficaz:");
  Serial.print(corrente_eficaz);
  Serial.print(corrente_eficaz*1000);
  Serial.println(" mA");*/
  return corrente_eficaz;
}
