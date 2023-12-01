# Zoe ELM2MQTT
Translation tool from ELM bluetooth dongle connected to OBD2 port in the car to MQTT server for HomeAssistant

# Initial steps
Clone the repository to your local folder.
Open the file elm2mqtt.ino using Arduino IDE.
Edit the file secrets.h - add your wifi SSID, wifi password, mqtt server, mqtt username/password and most importantly, the ELM327 OBD dongle name and bluetooth pin.
Once modified, save the file.
Set the board type in Arduino to WEMOS LOLIN32.
Change the partition scheme to "Minimal SPIFFS (Large APPS with OTA)". Otherwise the code will not fit into memory.
Make sure you install necessery libraries:
- ArduinoOTA
- NTPClient
- PubSubClient
Compile the code and upload to your ESP32 board.

Get the ESP32 somewhere near the car, plug the ELM dongle to the car OBD2 port and turn the ESP32 on.
The ESP will connect to your local network and will start to try to connect to the dongle.
The ELM will only work while the car is charging or doors are open, otherwise the battery data will not be available.

# Monitoring
There is a possibility to monitor the ESP32 communication using in-built websocket server.
Connect to the IP address of the ESP32 and you'll be present with basic interface.
In the bottom input, type "h" for help and it will show the list of available commands.
It will also continously print debug messages from the dongle - attempting to measure the battery statistics every 60 seconds (configurable by changing variable `rInt`).

# HomeAssistant part
The tool reports data to MQTT server.
HomeAssistant can use this feature to create virtual entities based on MQTT.
In configuration file, add following section:
```
mqtt:
  sensor:
    - name: "Zoe - State of Charge"
      state_topic: "elm327report/result/SoC"
      unique_id: "zoe.SoC"
      unit_of_measurement: "%"
      icon: |-
        {% if states("zoe.SoC") | float > 80 %}
        mdi:battery-80
        {% else %}
        mdi:battery-10
        {% end if %}
      
    - name: "Zoe - Available range"
      state_topic: "elm327report/result/AvailableRange"
      icon: mdi:gauge-low
      unit_of_measurement: "km"
      unique_id: "zoe.Range"
      
    - name: "Zoe - Available Energy"
      state_topic: "elm327report/result/AvailableEnergy"
      icon: mdi:lightning-bolt
      unit_of_measurement: "kWh"
      unique_id: "zoe.Energy"
      
    - name: "Zoe - Dongle status"
      state_topic: "elm327report/status"
      unique_id: "zoe.DongleState"
      
    - name: "Zoe - Debug - message sent"
      state_topic: "elm327report/debug/elmSend"
      unique_id: "zoe.ElmSend"
      
    - name: "Zoe - Debug - message received"
      state_topic: "elm327report/debug/elmResponse"
      
    - name: "Zoe - Debug - ODB connection status"
      state_topic: "elm327report/heartbeat"
      unique_id: "zoe.ElmHeartbeat"

```

This will create entities in HomeAssistant with the corresponding MQTT message topics and start to track the data.
