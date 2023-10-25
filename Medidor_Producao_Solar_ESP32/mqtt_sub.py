import paho.mqtt.client as mqtt

#Connection success callback
def on_connect(client, userdata, flags, rc):
    print('Connected with result code '+str(rc))
    client.subscribe('/')

# Message receiving callback
def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))

client = mqtt.Client()

# Specify callback function
client.on_connect = on_connect
client.on_message = on_message

# Establish a connection
client.connect('127.0.0.1', 1883, 60)
# Publish a message
client.publish('test',payload='Hello World',qos=0)

client.loop_forever()
#1701  mosquitto_pub -m "Mensagem" -t "test"
#1702  mosquitto_pub -m "WilliansLindo" -t "test"
#pid_file /run/mosquitto/mosquitto.pid
#persistence_location /var/lib/mosquitto/
#log_dest file /var/log/mosquitto/mosquitto.log
#include_dir /etc/mosquitto/conf.d
