import json
import time
import ast
import traceback
import paho.mqtt.client as mqtt
from esp32_mysql_data import MqttDados

# Defina as informações do servidor MQTT
mqtt_broker = "ipdamaquina"  # Ou nome de host
mqtt_port = 1883
mqtt_topic_read = "/teste" 
mqtt_topic_write = "/teste"
mqtt_username = "ESP32SOLAR"
mqtt_password = "senhadotopico"

#Connection success callback
def on_connect(client, userdata, flags, rc):
    print('Connected with result code '+str(rc))
    client.subscribe('/')

# Message receiving callback
def on_message(client, userdata, msg):
    data= msg.payload
    my_json = data.decode('utf8').replace("'", '"')
    print(my_json)
    data = json.loads(my_json)
    current_time = time.localtime()
    if current_time.tm_min % 10 == 0:
        MqttDados(data["DataHora"],data["Temperatura"],data["Corrente"],data["Watt"],data["tensaoPainel"],data["tensaoBatChumbo"],data["tensaoBatLition"],data["tensaoSistema"]).InsertDadaBase()
        

# Configurar o cliente MQTT
if __name__ == "__main__":    
    try:
        print("Rodando")    
        client = mqtt.Client()
        client.on_message = on_message
        # Configurar autenticação
        client.username_pw_set(mqtt_username, mqtt_password)
        # Conectar ao servidor MQTT
        client.connect(mqtt_broker, mqtt_port, 60)
        # Assinar o tópico
        client.subscribe(mqtt_topic_read)
        # Publish a message
        #for numero in range(1, 10):
            #client.publish(mqtt_topic_write,payload='Hello World',qos=0)
        client.loop_forever()
    except Exception as e:
        traceback.print_exc()
        print(f"An error occurred: {e}")



