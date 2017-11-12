# Motherclock
ESP8266 project to emulate a "motherclock" as used in earlier times in plants and trainstations

The old clocks can not be simply powered using voltage - they require a impulse each minute with changing polarity. 

### Work here is heavilly inspired by:

* ["Mutteruhr"](http://www.elektronik-labor.de/AVR/Mutteruhr.html)<br>The author here shows to how generate positive and negative voltage out of a MAX232 to drive a clock.
* [RTCLib](https://github.com/NeiroNx/RTCLib)<br>Provided function to communicate with a PCF8583, was light adjusted to make it more useable for my purpose

### Dependencies:

* [NtpClient](https://github.com/gmag11/NtpClient)

### Used material:

* [ntp howto](https://diyprojects.io/esp8266-web-server-part-3-recover-time-time-server-ntp/)
* [Information for PCF8583](http://www.raspberry-pi-geek.de/Magazin/2015/02/Der-Uhrenbaustein-PCF8583-am-I-2-C-Bus-des-Raspberry-Pi)
* [nodemcu tutorial](http://www.mikrocontroller-elektronik.de/nodemcu-esp8266-tutorial-wlan-board-arduino-ide/)
* [Tipps for the ESP87266](http://esp8266-server.de/Tipps.html)
* 9b-esp8266-low_power_solutions_en.pdf

