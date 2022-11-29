# Connect the pico with a serial interface to the nextion display
# Get sliders and switch values, and send via mqtt to jarvis-cam
#
# 
#
# Dem Jevy seng Bastelbuud 11/2022



import network
import time

from simple import MQTTClient
from machine import UART, Pin, PWM
import secrets


nextion_uart = UART(1,baudrate=9600)

LEDGPIO = "LED"
prefix = "DUMSHOME/JARVIS/jarvisCam/"
PING_INTERVAL = 60
mqtt_client_id = "jarviscamcontroller"

panvalue=5
tiltvalue=5
lightvalue=0

led = Pin(LEDGPIO,Pin.OUT)
end_cmd=b'\xFF\xFF\xFF'

# the following functions are only to ensure the mqtt connection is still alice

mqtt_con_flag = False #mqtt connection flag
pingresp_rcv_flag = True #indicator that we received PINGRESP
lock = True #to lock callback function from recursion when many commands are received look mqqt.py line 138

next_ping_time = 0    

def ping_reset():
 global next_ping_time
 next_ping_time = time.time() + PING_INTERVAL #we use time.time() for interval measuring interval
 print("Next MQTT ping at", next_ping_time)

def ping():
 client.ping()
 ping_reset()


def check():
 global next_ping_time
 global mqtt_con_flag
 global pingresp_rcv_flag
 if (time.time() >= next_ping_time): #we use time.time() for interval measuring interval
   if not pingresp_rcv_flag :
    mqtt_con_flag = False #we have not received an PINGRESP so we are disconnected
    print("We have not received PINGRESP so broker disconnected")
   else :
    print("MQTT ping at", time.time())
    ping()
    pingresp_rcv_flag = False
 res = client.check_msg()
 if(res == b"PINGRESP") :
  pingresp_rcv_flag = True
  print("PINGRESP")

def mqtt_connect():
 global next_ping_time 
 global pingresp_rcv_flag
 global mqtt_con_flag
 global lock
 while not mqtt_con_flag:
  
  try:
   client.connect()
   mqtt_con_flag=True
   pingresp_rcv_flag = True
   next_ping_time = time.time() + PING_INTERVAL
   lock = False # we allow callbacks only after everything is set
  except Exception as e:
   print("Error in mqtt connect: [Exception] %s: %s" % (type(e).__name__, e))
  time.sleep(0.5) # to brake the loop

    
def flashLed(times,duration=0.5):
    for i in range(0,times,1):
        led.value(1)            #Set led turn on
        time.sleep(duration)
        led.value(0)            #Set led turn off
        time.sleep(duration)
    led.value(0)

def heartbeat(first):
    global lastping
    if first:
        client.ping()
        lastping = time.ticks_ms()
    if time.ticks_diff(time.ticks_ms(), lastping) >= 300000:
        client.ping()
        lastping = time.ticks_ms()
    return


# we initiate the netowrk with the credentials from the secrets.py file


wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect(secrets.SSID, secrets.PASSWORD)
while not wlan.isconnected() and wlan.status() >= 0:
	print("Waiting to connect:")
	time.sleep(1)
flashLed(6)

client = MQTTClient(mqtt_client_id, secrets.MQTTSERVER,user=secrets.MQTTUSER, password=secrets.MQTTPWD, keepalive=300, ssl=False, ssl_params={})
client.connect()
flashLed(4)
heartbeat(True)

client.publish(prefix+'nextion mesg', "connected")

startT = time.ticks_ms()
startC = time.ticks_ms()
while True:
    # check wlan connection
    if not wlan.isconnected():
        wlan.connect(secrets.SSID, secrets.PASSWORD)
        while not wlan.isconnected():
            print(". ")
            pass
    #check mqtt connection
        
    mqtt_connect()#ensure connection to broker
    try:
     check()
    except Exception as e:
     print("Error in Mqtt check message: [Exception] %s: %s" % (type(e).__name__, e))
     print("MQTT disconnected due to network problem")
     lock = True # reset the flags for restart of connection
     mqtt_con_flag = False 
    time.sleep(0.5)
       
    data = nextion_uart.read(7)
    if data is not None:
        print("header received : ") # tilt id = x05, pan id = x04, light id = x0b 
        print(data)
        time.sleep_ms(10)
        value = nextion_uart.read(3)
        if value is not None:
            # analyse id
            # format : b'e\x00\x05\x00\xff\xff\xff'
            # id is the 3 caracter
            # tilt id = x05, pan id = x04, light id = x0b
            stepper = False
            if data[2] == 0x05:
                print ("tilt id received")
                tiltvalue = value.decode('UTF-8')
                print(" with value "+str(tiltvalue))
                stepper = True
            if data[2] == 0x04:
                print ("pan id received")
                panvalue = value.decode('UTF-8')
                print(" with value "+str(panvalue))
                stepper = True
            if stepper == True:
                msg = "{'pan':"+str(panvalue)+",'tilt':"+str(tiltvalue)+"}"
                print(msg)
                client.publish(prefix+'direct', msg)
                print("publishing "+msg)
                stepper = False
            if data[2] == 0x0b:
                print ("light id received ")
                lightvalue = value.decode('UTF-8')
                print(" with value " +str(lightvalue))
                if str(lightvalue) == "0":
                    lightstr = "OFF"
                if str(lightvalue) == "1":
                    lightstr = "ON"
                client.publish(prefix+'light', lightstr)
                print("publishing "+lightstr)
            flashLed(4)
    
    
     # quick blink 2 times every 10 seconds
    if time.ticks_diff(time.ticks_ms(),startC) > 10000:
        flashLed(2,0.125)
        print("heartbeat")
        startC = time.ticks_ms()