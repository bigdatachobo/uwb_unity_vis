import time
import json
import paho.mqtt.client as mqtt
from awscrt import mqtt as awsiot_mqtt
from awsiot import mqtt_connection_builder

# AWS IoT Core Configuration
aws_endpoint = "end-point"
aws_port = 8883  # Change this to 443 if your network blocks MQTT.
aws_root_ca_path = "/home/pi/certs/AmazonRootCA1.pem"
aws_private_key_path = "/home/pi/certs/97eade49427ff64d7970eb2dbf272f30dc0dcbd37487280c94a6d16b4879ad5c-private.pem.key"
aws_certificate_path = "/home/pi/certs/97eade49427ff64d7970eb2dbf272f30dc0dcbd37487280c94a6d16b4879ad5c-certificate.pem.crt"
aws_client_id = "myid"

# Local MQTT Broker Configuration
local_mqtt_broker = "localhost"  # change to your local mqtt broker IP
local_mqtt_port = 1883
local_mqtt_topic = "bno"

# AWS MQTT Connection
aws_connection = mqtt_connection_builder.mtls_from_path(
    endpoint=aws_endpoint,
    port=aws_port,
    cert_filepath=aws_certificate_path,
    pri_key_filepath=aws_private_key_path,
    ca_filepath=aws_root_ca_path,
    client_id=aws_client_id,
    clean_session=False,
    keep_alive_secs=30
)

def on_aws_connection_interupted(connection, error, **kwargs):
    print(f"Connection interupted. error: {error}")

def on_aws_connection_resumed(connection, return_code, session_present, **kwargs):
    print(f"Connection resumed. return_code: {return_code} session_present: {session_present}")

aws_connection.on_connection_interrupted = on_aws_connection_interupted
aws_connection.on_connection_resumed = on_aws_connection_resumed

print("Connecting to AWS IoT Core...")
connect_future = aws_connection.connect()
connect_future.result()
print("Connected to AWS IoT Core!")

# Local MQTT Connection
def on_local_connect(client, userdata, flags, rc):
    print("Connected to local MQTT Broker with result code "+str(rc))
    client.subscribe(local_mqtt_topic)

def on_local_message(client, userdata, msg):
    print(f"Received message from local MQTT Broker: {msg.payload}")
    aws_connection.publish(
        topic=local_mqtt_topic,
        payload=msg.payload,
        qos=awsiot_mqtt.QoS.AT_LEAST_ONCE)
    print("Published message to AWS IoT Core")
    print(f"Your Topic is {local_mqtt_topic}")
    # print(f"{msg.payload} upload to AWS IoT Core Success!")

local_client = mqtt.Client()
local_client.on_connect = on_local_connect
local_client.on_message = on_local_message

local_client.connect(local_mqtt_broker, local_mqtt_port, 60)

try:
    local_client.loop_start()
    while True:
        time.sleep(1)  # Keep the main thread alive

except KeyboardInterrupt:
    print("Disconnecting...")
    local_client.loop_stop()
    disconnect_future = aws_connection.disconnect()
    disconnect_future.result()
    print("Disconnected from AWS IoT Core and Local MQTT Broker!")
