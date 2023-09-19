#include <Arduino.h>
#include <WiFi.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <NTPClient.h> 

const char *ssid = "nome_da_red";
const char *password = "Senha_da_rede";

int LED_BUILTIN = 23;

int analogInputTensao = 35;
int analogInputTemperatura = 33; // Conexão do termistor
int analogInputTensaoPainel = 39;
int analogInputTensaoBateria = 36;
int analogInputCorrentePainel = 32;
int analogInputTensaoBatLition = 34;

int releVentoinha = 12;



// Parâmetros do circuito
const double vcc = 4.6;
const double R = 10000.0;

const double bateriaGrandeFlut = 14.4;
const double batLitio = 14.4;

char valorEntrada; 
// Numero de amostras na leitura
const int nAmostras = 5.2;

//File myFile;

double Calcula_corrente();
float Calcula_Tensao(int valor);
float Calcula_Temperatura();
void calcularCorrenteEsp32(int pino_sensor);

WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP);

void ListaDiretorios(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.println("  DIR : ");
      Serial.println(file.name());
      if(levels){
        ListaDiretorios(fs, file.name(), levels -1);
      }
    } else {
      Serial.println("  FILE: ");
      Serial.println(file.name());
      Serial.println("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char * path){
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path)){
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
      Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char * path){
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if(file){
    len = file.size();
    size_t flen = len;
    start = millis();
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for(i=0; i<2048; i++){
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}


void CardTipo(uint8_t cardType)
{
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
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

  uint8_t cardType = SD.cardType();
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
 
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  ListaDiretorios(SD, "/", 0);
  createDir(SD, "/mydir");
  ListaDiretorios(SD, "/", 0);
  removeDir(SD, "/mydir");
  ListaDiretorios(SD, "/", 2);
  writeFile(SD, "/hello.txt", "Hello ");
  appendFile(SD, "/hello.txt", "World!\n");
  readFile(SD, "/hello.txt");
  deleteFile(SD, "/foo.txt");
  renameFile(SD, "/hello.txt", "/foo.txt");
  readFile(SD, "/foo.txt");
  testFileIO(SD, "/test.txt");
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
 
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
    Serial.print("Corrente::");
    //Serial.println(Calcula_corrente());
    calcularCorrenteEsp32(analogInputCorrentePainel);
    Serial.println("Tensao::");
    Serial.println(Calcula_Tensao(analogRead(analogInputTensao)));
    Serial.println(Calcula_Tensao(analogRead(analogInputTensaoBateria)));
    Serial.println(Calcula_Tensao(analogRead(analogInputTensaoPainel)));
    Serial.println(Calcula_Tensao(analogRead(analogInputTensaoBatLition)));
    delay(10000); 
    /*char *a = (char *)Calcula_Tensao(analogRead(analogInputTensaoBateria));
    char *b = (char *)Calcula_Tensao(analogRead(analogInputTensaoPainel));
    char *c = (char *)Calcula_Tensao(analogRead(analogInputTensaoBatLition));
    char *d = (char *)Calcula_corrente(); 
    char *e = (char *)Calcula_Temperatura();        
    //Serial.println("/TensaoBateria: "+a+"/TensaoPainel: "+b+"/TensaoBatLition: "+c+"/Calcula_corrente: "+d+"/Calcula_Temperatura: "+e+"/");   

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
    }    */
}


double Calcula_corrente()
{
    int mVperAmp = 100; 
    int RawValue = 0;
    int ACSoffset = 2500;
    double Voltage = 0;
    double Amps = 0;

    RawValue = analogRead(analogInputCorrentePainel);
    Voltage = (RawValue / 1024.0) * 4600; // Gets you mV
    Amps = ((Voltage - ACSoffset) / mVperAmp);
    return Amps;
}

float Calcula_Tensao(int valor)
{
    float vout = 0.0;
    float vin = 0.0;
    float R1 = 100000.0; // resistência de R1 (100K)
    float R2 = 10000.0; // resistência de R2 (10K) 
    float ValorRetorno = 0;  
    vout = (valor * vcc) / 1024.0; // see text
    ValorRetorno = vout / (R2/(R1+R2)); 
    if (ValorRetorno<0.09) {
        ValorRetorno=0.0;// declaração para anular a leitura indesejada!
    }
    return ValorRetorno;
}

float Calcula_Temperatura()
{
    // Parâmetros do termistor
    const double beta = 3600.0;
    const double r0 = 10000.0;
    const double t0 = 273.0 + 25.0;
    const double rx = r0 * exp(-beta / t0);
    int  soma = 0;
    for (int i = 0; i < nAmostras; i++)
    {
        soma += analogRead(analogInputTemperatura);
        delay(10);
    }
    // Determina a resistência do termistor
    double v = (vcc * soma) / (nAmostras * 1024.0);
    double rt = (vcc * R) / v - R;

    // Calcula a temperatura
    double t = beta / log(rt / rx);
    double ReturnT = t - 273.0;
    return ReturnT;
}

void calcularCorrenteEsp32(int pino_sensor){
int menor_valor;
int valor_lido;
int menor_valor_acumulado = 0;
int ZERO_SENSOR = 0;
float corrente_pico;
float corrente_eficaz;
double maior_valor=0;
double corrente_valor=0;
 /*
 ZERO_SENSOR = analogRead(pino_sensor); 
 for(int i = 0; i < 10000 ; i++){
 valor_lido = analogRead(pino_sensor); 
 ZERO_SENSOR = (ZERO_SENSOR +  valor_lido)/2; 
 delayMicroseconds(1);  
 }
 Serial.print("Zero do Sensor:");
 Serial.println(ZERO_SENSOR);
 delay(3000);

 */
menor_valor = 4095;
 
  for(int i = 0; i < 10000 ; i++){
  valor_lido = analogRead(pino_sensor);
  if(valor_lido < menor_valor){
  menor_valor = valor_lido;    
  }
  delayMicroseconds(1);  
  }
  ZERO_SENSOR = menor_valor;
  Serial.print("Zero do Sensor:");
  Serial.println(ZERO_SENSOR);
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

  
  Serial.print("Menor Valor:");
  Serial.println(menor_valor);

  //Transformar o maior valor em corrente de pico
  corrente_pico = ZERO_SENSOR - menor_valor; // Como o ZERO do sensor é 2,5 V, é preciso remover este OFFSET. Na leitura Analógica do ESp32 com este sensor, vale 2800 (igual a 2,5 V).
  corrente_pico = corrente_pico*0.805; // A resolução mínima de leitura para o ESp32 é de 0.8 mV por divisão. Isso transforma a leitura analógica em valor de tensão em [mV}
  corrente_pico = corrente_pico/185;   // COnverter o valor de tensão para corrente de acordo com o modelo do sensor. No meu caso, esta sensibilidade vale 185mV/A
                                      // O modelo dele é ACS712-05B. Logo, precisamos dividir o valor encontrado por 185 para realizar esta conversão                                       
  
  Serial.print("Corrente de Pico:");
  Serial.print(corrente_pico);
  Serial.print(" A");
  Serial.print(" --- ");
  Serial.print(corrente_pico*1000);
  Serial.println(" mA");
  
 
  //Converter para corrente eficaz  
  corrente_eficaz = corrente_pico/1.4;
  Serial.print("Corrente Eficaz:");
  Serial.print(corrente_eficaz);
  Serial.print(" A");
  Serial.print(" --- ");
  Serial.print(corrente_eficaz*1000);
  Serial.println(" mA");
 
 delay(5000);
}