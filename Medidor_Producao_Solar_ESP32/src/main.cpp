#include <Arduino.h>
#include <WiFi.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "NTPClient.h" 
#include <string.h>

#define WORD_LENGTH 64


int LED_BUILTIN = 23;
int releVentoinha = 12;

int analogInputTensao = 35;
int analogInputTemperatura = 33; // Conexão do termistor
int analogInputTensaoPainel = 39;
int analogInputTensaoBateria = 36;
int analogInputTensaoBatLition = 34;

int analogInputCorrentePainel = 32;

const double bateriaGrandeFlut = 14.4;
const double batLitio = 14.4;

bool wificonecte = false;
bool arquivo_read = false;

double Calcula_corrente();
float Calcula_Tensao(int valor);
float Calcula_Temperatura();
float calcularCorrenteEsp32(int pino_sensor);
float Calcula_Temperatura(int pino_input_temperatura);

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
  pinMode(analogInputTensao,INPUT);
  pinMode(analogInputTemperatura,INPUT);
  pinMode(analogInputTensaoPainel,INPUT);
  pinMode(analogInputTensaoBateria,INPUT);
  pinMode(analogInputCorrentePainel,INPUT);
  pinMode(analogInputTensaoBatLition,INPUT);
  
  wificonecte = ConectarWifi();
  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    return;
  }else{
    Serial.println("Card Pronto para uso :)");
  }
    
}

void loop()
{  
    String datahoraDados;
    if(wificonecte)
    {
      ntp.begin(); //GMT em segundos // +1 = 3600 // +8 = 28800// -1 = -3600// -3 = -10800 (BRASIL)
      ntp.setTimeOffset(-10800);     
      if (ntp.update()) {
          //char * nome_arquivo = strtok((char *)ntp.getFormattedDate(),"T");
          //String nomeArquivo = "/dadossolar"+(String)[0]+".txt";
          if((ntp.getMinutes()%10) == 0)
          {
            datahoraDados = ntp.getFormattedDate();
            datahoraDados = datahoraDados +"-Temperatura="+(String)Calcula_Temperatura(analogInputTemperatura);
            datahoraDados = datahoraDados +"/Corrente="+(String)calcularCorrenteEsp32(analogInputCorrentePainel);
            datahoraDados = datahoraDados +"/Tensao01="+(String)Calcula_Tensao(analogRead(analogInputTensaoBateria));
            datahoraDados = datahoraDados +"/Tensao02="+(String)Calcula_Tensao(analogRead(analogInputTensaoPainel));
            datahoraDados = datahoraDados +"/Tensao03="+(String)Calcula_Tensao(analogRead(analogInputTensaoBatLition));
            datahoraDados = datahoraDados +"/Tensao04="+(String)Calcula_Tensao(analogRead(analogInputTensao))+"\n";
            if (SD.begin()) { 
              appendFile(SD, "/dados2728092023.txt", datahoraDados);  
              arquivo_read = false;             
            }
          } 
        }else{
          return;
        } 
    }
    if (Serial.available() > 0 && arquivo_read == false){
      if (Serial.read() == 116){
        readFile(SD, "/dados2627092023.txt");
        Serial.println("Acabou");
        arquivo_read = true;        
      }      
    }
    
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

float calcularCorrenteEsp32(int pino_sensor)
{
  int valor_lido;
  int menor_valor_acumulado = 0;
  int ZERO_SENSOR = 0;
  float corrente_pico;
  float corrente_eficaz;
  double maior_valor=0;
  double corrente_valor=0;
  int menor_valor = 4095;

  for(int i = 0; i < 10000 ; i++){
    valor_lido = analogRead(pino_sensor);
    if(valor_lido < menor_valor){
      menor_valor = valor_lido;    
    }
    delayMicroseconds(1);  
  }
  ZERO_SENSOR = menor_valor;
  
  delay(3000);

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
  return corrente_eficaz;
}
