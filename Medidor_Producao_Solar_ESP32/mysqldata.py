import sys
import serial
#import mysql.connector
import time
from datetime import datetime   
import csv

#https://blog.eletrogate.com/como-conectar-o-arduino-com-o-python/


def USBDataConnect():
    while True:
        try:
            arduino = serial.Serial('/dev/ttyUSB0', 9600)
            print('Arduino conectado')
            break
        except:
            pass
def ArduinoRead():
    try:
        # Iniciando conexao serial
        comport = serial.Serial('/dev/ttyUSB0', 9600)
        #comport = serial.Serial('/dev/ttyUSB0', 9600, timeout=1) # Setando timeout 1s para a conexao
        
        PARAM_CARACTER = 't'
        PARAM_ASCII = str(chr(116))  # Equivalente 116 = t
        
        # Time entre a conexao serial e o tempo para escrever (enviar algo)
        time.sleep(1.8)  # Entre 1.5s a 2s
        
        # comport.write(PARAM_CARACTER)
        comport.write(b't')
        while True:
            VALUE_SERIAL = comport.readline()
            if not VALUE_SERIAL:
                break  # Exit the loop when no more data is received
            print('\nRetorno da serial: %s' % (VALUE_SERIAL))
            if "Acabou" in str(VALUE_SERIAL):
                print("Entrou no if")
                break
        print("Saiu do while")
        comport.close()
        minuto = datetime.today()

        with open('DadosColetroSolar.csv', 'a') as f_object:
            w = csv.writer(f_object)
            w.writerow([minuto, str(VALUE_SERIAL.strip())])  # Write time and serial data to CSV

    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    minuto = datetime.today().minute
    if(minuto % 1) == 0:
        ArduinoRead()


def USBDataRead():
    
    arduino = serial.Serial('/dev/ttyUSB0', 9600)
    arduino.write(b't')
    #arduino.write('l'.encode())
    #arduino.flush()
    time.sleep(1.8)

    print('Arduino conectado')
    msg = str(arduino.readline()) #Lê os dados em formato de string
    msg = msg[2:-5] #Fatia a string
    splitvariavel = msg.split("/")
    while(len(splitvariavel)< 7):
        msg = str(arduino.readline()) #Lê os dados em formato de string
        msg = msg[2:-5] #Fatia a string
        splitvariavel = msg.split("/")
    print(splitvariavel)  
    arduino.flush() #Limpa a comunicação
    arduino.close() 
    # 1. cria o arquivo
    #f = open('DadosColetroSolar.csv', 'w', newline='', encoding='utf-8')
    minuto = datetime.today()
    splitvariavel.append(minuto)    
    with open('DadosColetroSolar.csv', 'a') as f_object:     
        w = csv.writer(f_object)
        w.writerow(splitvariavel)    
    #w.close() 

          
    #TensaoPainel = "0.00"
    #TensaoBateriaChub = "0.00"
    #TensaoBateriaLit = "0.00"
    #CorrentePainel = "0.00"
    #DateTimeNow = datetime.datetime.now()
    #TemperaturaBat = "00.0"
    #variaveis = (TensaoPainel,TensaoBateriaChub,TensaoBateriaLit,CorrentePainel,DateTimeNow,TemperaturaBat)
    #return variaveis"""
  
def main():
    USBDataRead()
    #InsertDataBase(USBDataRead())
    #serial.Serial('/dev/ttyUSB0')
    #texto = str(serial.readline())
    #print(texto[2:-5])

#while True:# __name__ == "__main__":
    #minuto = datetime.today().minute
    #print("Rodando while")
    #if(minuto%1)==0:ArduinoRead()
    #ArduinoRead()
    #USBDataRead() 
    #if(minuto==6):USBDataRead() 
    
    
     
    