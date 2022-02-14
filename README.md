# ESP8266 Alexa Skills Thermostat

Alexa skill to control HomeVision Thermostat via ESP8266 without opening a router's port/firewall.

<b>You must have a HomeVision Controller running HomeVisionXL software with the NetIO plug-in and an RCS TX-15B thermostat for this to work.</b>

This project is based on the work of Nassir Malik. See his Youtube tutorials at https://www.youtube.com/channel/UCn8v7OzXk7IqRdKZdf14yjA , specifically https://www.youtube.com/watch?v=OrhkuJiahDs, and code at https://github.com/nassir-malik.

TCP code used to talk to the HomeVision system is a modified version from Big Dan the Blogging Man at https://bigdanzblog.wordpress.com/2016/02/09/esp8266arduino-ide-communicating-with-tcp/

In this project you will be able to control a HomeVision Thermostat via ESP8266 with Alexa skills without opening a firewall port or setting up a reverse proxy.

1. Download this project and unzip.

2. Deploy this project to Heroku by clicking this button
    [![Deploy](https://www.herokucdn.com/deploy/button.svg)](https://heroku.com/deploy)

3. Copy following folders to "C:\Program Files (x86)\Arduino\libraries"

    arduinoWebSockets

    ArduinoJson

4. Update and flash your ESP8266 with "ESP8266/ESP8266_WebSocketClient/ESP8266_WebSocketClient.ino"

5. Create an Alexa skills using the info in alexa utterences.txt and add Heroku app URL as an end-point

6. Test and enjoy.


See also this Project's [Wiki page](https://github.com/rebel7580/ESP8266-Alexa_Skills_Thermostat/wiki) for more details.
