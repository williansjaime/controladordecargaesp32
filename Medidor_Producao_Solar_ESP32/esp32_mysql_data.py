  
import csv
import sys
import time
import serial
import traceback
import mysql.connector
from datetime import datetime 

class MqttDados():

    def __init__(self,DataHora, Temperatura, Corrente, Watt,tensaoPainel,tensaoBatChumbo,tensaoBatLition,tensaoSistema):
        self.Temperatura = Temperatura
        self.Corrente = Corrente
        self.DataHora = DataHora.replace("Z","")
        self.Watt = Watt
        self.tensaoPainel = tensaoPainel
        self.tensaoBatChumbo = tensaoBatChumbo
        self.tensaoBatLition = tensaoBatLition
        self.tensaoSistema = tensaoSistema
    
    def InsertDadaBase(self):
        try:
            SQLServe = DB_Mysql()
            sqlserver = SQLServe.cursor()
            insert_dados_solar  = ("""
                INSERT INTO DadosSistemaSolar.tbDadosEsp32Solar(
                    datahora, 
                    temperatura, 
                    corrente, 
                    wattHora, 
                    tensaoPainel, 
                    tensaoBatChumbo, 
                    tensaoBatLition, 
                    tensaoSistema 
                    )
                VALUES
                    (
                    '{}'
                    ,{}
                    ,{}
                    ,{}
                    ,{}
                    ,{}
                    ,{}
                    ,{}) 
                """).format(
                    self.DataHora,
                    self.Temperatura,
                    self.Corrente,
                    self.Watt,
                    self.tensaoPainel,
                    self.tensaoBatChumbo,
                    self.tensaoBatLition,
                    self.tensaoSistema                                                                                    
                )
            sqlserver.execute(insert_dados_solar)
            SQLServe.commit()            
            SQLServe.close()    
            print("Salvo com sucesso!!!\n")
        except Exception as err:
            traceback.print_exc()



def DB_Mysql():
    Mysql = None
    try:
        hosts = "localhost"
        username= "root"
        password= "12345"
        database= "DadosSistemaSolar"
        Mysql = mysql.connector.connect(
            host=hosts,
            user=username,
            password=password,
            database=database)
        return Mysql        
    except Exception as err:
        LogFile.logFile(err)

   

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
        minuto = datetime.today()
        while True:
            VALUE_SERIAL = comport.readline()
            if not VALUE_SERIAL:
                break  # Exit the loop when no more data is received
            print('\nRetorno da serial: %s' % (VALUE_SERIAL))
            with open('DadosColetroSolar'+str(minuto)+'.csv', 'a') as f_object:
                w = csv.writer(f_object)
                w.writerow([str(VALUE_SERIAL.strip())])
            if "Acabou" in str(VALUE_SERIAL):
                print("Entrou no if")
                break
        print("Saiu do while")
        comport.close()
        

        #with open('DadosColetroSolar29092023.csv', 'a') as f_object:
            #w = csv.writer(f_object)
            #w.writerow([minuto, str(VALUE_SERIAL.strip())])  # Write time and serial data to CSV

    except Exception as e:
        print(f"An error occurred: {e}")


def USBDataRead(self):    
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
